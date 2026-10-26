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
    U64 summary_ms,summary_drops,summary_failures;
    unsigned summary_sent,finalized;
    EventLogOverlapped overlap;
} event_logger;
#ifdef GF2VR_LOGGER_EXIT_OBSERVE
#include "event_logger_exit.h"
#else
#define event_log_exit_marker(stage) ((void)(stage))
#endif
/* Sequence-tagged bounded ring: a stalled producer cannot publish a later
   record before its reserved predecessor. Full queues drop the new line. */
static int event_log_enqueue_owned(const char *bytes,unsigned length,U64 *ticket,unsigned final) {
    if(!__atomic_load_n(&event_logger.ready,__ATOMIC_ACQUIRE)||
       (!final && __atomic_load_n(&event_logger.closing,__ATOMIC_ACQUIRE)))return 0;
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
static int event_log_enqueue(const char *bytes,unsigned length,U64 *ticket) {
    return event_log_enqueue_owned(bytes,length,ticket,0);
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
        if(!ok&&GetLastError()!=997) {
            /* No pending operation owns this batch. Do not query an invalid
               OVERLAPPED on the next flush; release this failed queue batch. */
            event_logger.overlap.internal=0xc0000001u;
            __atomic_fetch_add(&event_logger.io_failures,1,__ATOMIC_RELAXED);
            __atomic_fetch_add(&event_logger.drops,event_logger.batch_end-event_logger.tail,__ATOMIC_RELAXED);
            event_log_commit_batch();return 0;
        }
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
static void event_log_summary_due(unsigned final) {
    U64 now=GetTickCount64(),drops=__atomic_load_n(&event_logger.drops,__ATOMIC_RELAXED),
        failures=__atomic_load_n(&event_logger.io_failures,__ATOMIC_RELAXED);
    U64 elapsed=now>=event_logger.summary_ms?now-event_logger.summary_ms:10000;
    /* Counter changes coalesce to at most 1Hz; unchanged counters still get a
       ten-second heartbeat. Process termination may omit DLL detach entirely,
       so only the periodic path is a product contract, not a final-pair promise. */
    if(!final && ((drops==event_logger.summary_drops && failures==event_logger.summary_failures && elapsed<10000) ||
       (event_logger.summary_sent && elapsed<1000)))return;
    event_logger.summary_ms=now;event_logger.summary_drops=drops;
    event_logger.summary_failures=failures;event_logger.summary_sent=1;
    char line[512];unsigned n=event_line(line,"event_log_drop_count",
        drops,GetCurrentThreadId(),GetCurrentProcessId(),
        GetTickCount64(),__atomic_load_n(&first_tid,__ATOMIC_ACQUIRE),(U64)unity);
    (void)event_log_enqueue_owned(line,n,0,final);
    n=event_line(line,"event_log_io_failures",failures,
        GetCurrentThreadId(),GetCurrentProcessId(),GetTickCount64(),__atomic_load_n(&first_tid,__ATOMIC_ACQUIRE),(U64)unity);
    (void)event_log_enqueue_owned(line,n,0,final);
}
static void event_log_summary(void) {event_log_summary_due(0);}
/* Sole worker on orderly closing, or detach AFTER all other threads exited.
   Final summaries bypass only the closing enqueue gate, never IO ownership. */
static void event_log_finish(void) {
    event_log_exit_marker("finish_before");
    if(event_logger.finalized){event_log_exit_marker("finish_after");return;}
    event_log_flush();
    /* Only closing owns this recovery: process detach has no live producer.
       A thread terminated after reserving head cannot publish its slot now.
       Count/discard that incomplete line, preserving all complete successors.
       Never discard a batch that still owns an outstanding IO receipt. */
    while(!event_logger.batch_bytes && event_logger.tail<event_logger.head) {
        U64 pos=event_logger.tail;EventLogSlot *s=&event_logger.storage->slots[pos&(EVENT_LOG_CAPACITY-1)];
        if(__atomic_load_n(&s->sequence,__ATOMIC_ACQUIRE)!=pos+1) {
            __atomic_fetch_add(&event_logger.drops,1,__ATOMIC_RELAXED);
            __atomic_store_n(&s->sequence,pos+EVENT_LOG_CAPACITY,__ATOMIC_RELEASE);
            __atomic_store_n(&event_logger.tail,pos+1,__ATOMIC_RELEASE);
        } else {event_log_flush();if(event_logger.tail==pos)break;}
    }
    event_log_summary_due(1);event_log_flush();event_logger.finalized=1;
    event_log_exit_marker("finish_after");
}
static void WINAPI event_log_worker(void *instance,void *context) {
    (void)instance;(void)context;
    while(!__atomic_load_n(&event_logger.closing,__ATOMIC_ACQUIRE)) {
        Sleep(200);event_log_flush();event_log_summary();event_log_flush();
    }
    event_log_finish();
}
static void event_log_initialize(void) {
    if(!GF2VR_LOGGING_ALLOWED())return;
    DWORD saved=GetLastError();
    event_logger.storage=VirtualAlloc(0,sizeof(EventLogStorage),0x3000u,4u);
    if(!event_logger.storage){SetLastError(saved);return;}
    for(U64 i=0;i<EVENT_LOG_CAPACITY;++i)event_logger.storage->slots[i].sequence=i;
    event_logger.overlap.internal=0xc0000001u;
    event_logger.summary_ms=GetTickCount64();
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
    __atomic_store_n(&event_logger.closing,1,__ATOMIC_RELEASE);
    event_log_finish();
    CloseHandle(event_logger.file);event_logger.ready=0;
    VirtualFree(event_logger.storage,0,0x8000u);event_logger.storage=0;SetLastError(saved);
}
#endif
