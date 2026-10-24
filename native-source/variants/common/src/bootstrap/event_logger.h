#ifndef GF2VR_EVENT_LOGGER_H
#define GF2VR_EVENT_LOGGER_H
/* One bounded MPSC queue, one append handle, one process-lifetime work item.
   Producers only format/copy/CAS; they never allocate, call IO or wait.
   The pinned module cannot dynamically unload. Process detach runs after the
   other threads have exited, and can finish the outstanding OVERLAPPED IO
   itself (IO completion only, never a worker join or lock held by a dead thread).
   Queue/batch storage is allocated once, only when logging is ON. */
#ifndef EVENT_LOG_CAPACITY
#define EVENT_LOG_CAPACITY 1024u
#endif
#define EVENT_LOG_LINE_BYTES 512u
#define EVENT_LOG_BATCH_BYTES 262144u
_Static_assert((EVENT_LOG_CAPACITY&(EVENT_LOG_CAPACITY-1))==0,"power of two queue");
typedef struct {U64 sequence;unsigned length;char bytes[EVENT_LOG_LINE_BYTES];} EventLogSlot;
typedef struct {U64 internal,internal_high;DWORD offset,offset_high;HANDLE event;} EventLogOverlapped;
_Static_assert(sizeof(EventLogOverlapped)==32,"Win64 OVERLAPPED");
typedef void (WINAPI *EventLogCallback)(void *,void *);
#ifndef EVENT_LOG_HOST_TEST
__declspec(dllimport) BOOL WINAPI TrySubmitThreadpoolCallback(EventLogCallback,void *,void *);
__declspec(dllimport) BOOL WINAPI GetOverlappedResult(HANDLE,void *,DWORD *,BOOL);
__declspec(dllimport) void *WINAPI VirtualAlloc(void *,U64,DWORD,DWORD);
__declspec(dllimport) BOOL WINAPI VirtualFree(void *,U64,DWORD);
#endif
typedef struct {EventLogSlot slots[EVENT_LOG_CAPACITY];char batch[EVENT_LOG_BATCH_BYTES];} EventLogStorage;
static struct {
    EventLogStorage *storage;
    U64 head,tail,drops,io_failures;
    unsigned ready,closing;
    HANDLE file;
    unsigned batch_bytes,batch_written,operation_start;
    U64 batch_end;
    EventLogOverlapped overlap;
} event_logger;
/* Sequence-tagged bounded ring: a stalled producer cannot publish a later
   record before its reserved predecessor. Full queues drop the new line. */
static int event_log_enqueue(const char *bytes,unsigned length,U64 *ticket) {
    if(!__atomic_load_n(&event_logger.ready,__ATOMIC_ACQUIRE)||
       __atomic_load_n(&event_logger.closing,__ATOMIC_ACQUIRE))return 0;
    if(!length||length>EVENT_LOG_LINE_BYTES){__atomic_fetch_add(&event_logger.drops,1,__ATOMIC_RELAXED);return 0;}
    U64 pos=__atomic_load_n(&event_logger.head,__ATOMIC_RELAXED);EventLogSlot *s;
    for(;;) {
        s=&event_logger.storage->slots[pos&(EVENT_LOG_CAPACITY-1)];
        U64 seq=__atomic_load_n(&s->sequence,__ATOMIC_ACQUIRE);
        long long difference=(long long)(seq-pos);
        if(!difference) {
            if(__atomic_compare_exchange_n(&event_logger.head,&pos,pos+1,0,__ATOMIC_RELAXED,__ATOMIC_RELAXED))break;
        } else if(difference<0){__atomic_fetch_add(&event_logger.drops,1,__ATOMIC_RELAXED);return 0;}
        else pos=__atomic_load_n(&event_logger.head,__ATOMIC_RELAXED);
    }
    for(unsigned i=0;i<length;++i)s->bytes[i]=bytes[i];s->length=length;
    if(ticket)*ticket=pos;
    __atomic_store_n(&s->sequence,pos+1,__ATOMIC_RELEASE);return 1;
}
static void event_log_commit_batch(void) {
    U64 pos=event_logger.tail;
    while(pos<event_logger.batch_end) {
        __atomic_store_n(&event_logger.storage->slots[pos&(EVENT_LOG_CAPACITY-1)].sequence,pos+EVENT_LOG_CAPACITY,__ATOMIC_RELEASE);
        __atomic_store_n(&event_logger.tail,++pos,__ATOMIC_RELEASE);
    }
    event_logger.batch_bytes=event_logger.batch_written=0;
}
static int event_log_complete_io(void) {
    /* STATUS_UNSUCCESSFUL marks a prepared but not yet submitted operation.
       WriteFile sets Internal before submitting. At process detach no worker
       can race this recovery, including termination between syscall/return. */
    if(event_logger.overlap.internal==(U64)0xc0000001u)return 1;
    DWORD bytes=0;
    if(!GetOverlappedResult(event_logger.file,&event_logger.overlap,&bytes,TRUE)) {
        __atomic_fetch_add(&event_logger.io_failures,1,__ATOMIC_RELAXED);return 0;
    }
    if(bytes>event_logger.batch_bytes-event_logger.operation_start)return 0;
    event_logger.batch_written=event_logger.operation_start+bytes;
    event_logger.overlap.internal=0xc0000001u;
    return bytes!=0;
}
static int event_log_write_batch(void) {
    if(!event_log_complete_io())return 0;
    while(event_logger.batch_written<event_logger.batch_bytes) {
        event_logger.overlap=(EventLogOverlapped){0xc0000001u,0,0xffffffffu,0xffffffffu,0};
        event_logger.operation_start=event_logger.batch_written;
        BOOL ok=WriteFile(event_logger.file,event_logger.storage->batch+event_logger.batch_written,
            event_logger.batch_bytes-event_logger.batch_written,0,&event_logger.overlap);
        if(!ok&&GetLastError()!=997) {__atomic_fetch_add(&event_logger.io_failures,1,__ATOMIC_RELAXED);return 0;}
        if(!event_log_complete_io())return 0;
    }
    event_log_commit_batch();return 1;
}
/* Only the worker, or process detach after all other threads have exited. */
static void event_log_flush(void) {
    if(!event_logger.ready)return;
    if(event_logger.batch_bytes&&!event_log_write_batch())return;
    for(;;) {
        U64 pos=event_logger.tail;unsigned bytes=0;
        while(pos<__atomic_load_n(&event_logger.head,__ATOMIC_ACQUIRE)) {
            EventLogSlot *s=&event_logger.storage->slots[pos&(EVENT_LOG_CAPACITY-1)];
            if(__atomic_load_n(&s->sequence,__ATOMIC_ACQUIRE)!=pos+1||bytes+s->length>EVENT_LOG_BATCH_BYTES)break;
            for(unsigned i=0;i<s->length;++i)event_logger.storage->batch[bytes+i]=s->bytes[i];bytes+=s->length;++pos;
        }
        if(!bytes)return;
        event_logger.batch_end=pos;event_logger.batch_written=0;
        event_logger.overlap=(EventLogOverlapped){0xc0000001u,0,0xffffffffu,0xffffffffu,0};
        /* Publish last: interrupted batch construction leaves the queue intact. */
        event_logger.batch_bytes=bytes;
        if(!event_log_write_batch())return;
    }
}
static void event_log_summary(void) {
    char line[512];unsigned n=event_line(line,"event_log_drop_count",
        __atomic_load_n(&event_logger.drops,__ATOMIC_RELAXED),GetCurrentThreadId(),GetCurrentProcessId(),
        GetTickCount64(),__atomic_load_n(&first_tid,__ATOMIC_ACQUIRE),(U64)unity);
    (void)event_log_enqueue(line,n,0);
    n=event_line(line,"event_log_io_failures",__atomic_load_n(&event_logger.io_failures,__ATOMIC_RELAXED),
        GetCurrentThreadId(),GetCurrentProcessId(),GetTickCount64(),__atomic_load_n(&first_tid,__ATOMIC_ACQUIRE),(U64)unity);
    (void)event_log_enqueue(line,n,0);
}
static void WINAPI event_log_worker(void *instance,void *context) {
    (void)instance;(void)context;
    while(!__atomic_load_n(&event_logger.closing,__ATOMIC_ACQUIRE)) {
        Sleep(200);event_log_flush();event_log_summary();event_log_flush();
    }
}
static void event_log_initialize(void) {
    if(!GF2VR_LOGGING_ALLOWED())return;
    DWORD saved=GetLastError();
    event_logger.storage=VirtualAlloc(0,sizeof(EventLogStorage),0x3000u,4u);
    if(!event_logger.storage){SetLastError(saved);return;}
    for(U64 i=0;i<EVENT_LOG_CAPACITY;++i)event_logger.storage->slots[i].sequence=i;
    event_logger.overlap.internal=0xc0000001u;
    event_logger.file=CreateFileW(GF2VR_PROBE_LOG,FILE_APPEND_DATA,7,0,OPEN_ALWAYS,128|0x40000000u,0);
    if(event_logger.file!=INVALID_HANDLE_VALUE) {
        if(TrySubmitThreadpoolCallback(event_log_worker,0,0))__atomic_store_n(&event_logger.ready,1,__ATOMIC_RELEASE);
        else CloseHandle(event_logger.file);
    }
    if(!event_logger.ready){VirtualFree(event_logger.storage,0,0x8000u);event_logger.storage=0;}
    SetLastError(saved);
}
static void event_log_close_process(void) {
    if(!event_logger.ready)return;
    DWORD saved=GetLastError();
    /* This is process termination, not an ordinary source/XR retirement. The
       module is pinned, so no dynamic detach with a live worker is supported. */
    event_log_flush();event_log_summary();event_log_flush();
    __atomic_store_n(&event_logger.closing,1,__ATOMIC_RELEASE);
    CloseHandle(event_logger.file);event_logger.ready=0;
    VirtualFree(event_logger.storage,0,0x8000u);event_logger.storage=0;SetLastError(saved);
}
#endif
