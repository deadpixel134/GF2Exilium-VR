#ifndef GF2_COPY_GPU_OBSERVE_H
#define GF2_COPY_GPU_OBSERVE_H
#include "eye_gpu_copy.h"
#define CGO_CAPACITY 32u
enum { CGO_VALID=1,CGO_DISJOINT,CGO_ZERO_FREQUENCY,CGO_REVERSED,CGO_CREATE_ERROR,CGO_READ_ERROR };
typedef struct {
    EyeGpuTicket ticket;
    unsigned long long gpu_begin,gpu_end,gpu_frequency,cpu_begin,cpu_end,cpu_read,device,context;
    unsigned thread,width,height,format,copy_mask,upright,status,result,pending_reads;
} CopyGpuObservation;
void copy_gpu_observe_arm(void);
void copy_gpu_observe_begin(void *,void *,EyeGpuTicket,EyeGpuDescription,int allowed);
void copy_gpu_observe_end(void *,EyeGpuTicket,unsigned copies,unsigned upright);
int copy_gpu_observe_take(unsigned *,CopyGpuObservation *);
#ifdef GF2_COPY_GPU_IMPLEMENTATION
#ifndef CGO_TICK
#define CGO_TICK() GetTickCount64()
#endif
/* One original copy/render producer, one numeric main consumer. Never completion authority.
   Armed by first actual fresh XR output, warmed2s, serial gap4/time gap100ms, max32.
   No GPU/COM operations in arm. GetData only at next authorized native-copy submission.
   Three queries retain only their device/context, never Unity/XR textures.
   Unfinished queries on source close remain bounded process-lived diagnostics. */
static struct {
    ID3D11Device *device;ID3D11DeviceContext *context;ID3D11Query *queries[3];
    CopyGpuObservation current;
    unsigned thread,active,pending,terminal,claimed;
    unsigned long long last_serial,last_ms;
    struct {unsigned ready;CopyGpuObservation row;} rows[CGO_CAPACITY];
} cgo;
static unsigned long long cgo_fresh_ms;
void copy_gpu_observe_arm(void) {
    DWORD saved=GetLastError();
    if(!__atomic_load_n(&cgo_fresh_ms,__ATOMIC_ACQUIRE)) {
        unsigned long long unset=0,now=CGO_TICK();
        if(now)__atomic_compare_exchange_n(&cgo_fresh_ms,&unset,now,0,__ATOMIC_RELEASE,__ATOMIC_RELAXED);
    }
    SetLastError(saved);
}
static unsigned long long cgo_qpc(void) {
    LARGE_INTEGER q={0};QueryPerformanceCounter(&q);return (unsigned long long)q.QuadPart;
}
static int cgo_same(EyeGpuTicket a,EyeGpuTicket b) {
    return a.serial && a.serial==b.serial && a.generation==b.generation && a.epoch==b.epoch;
}
static void cgo_publish(unsigned status,HRESULT result) {
    if(cgo.claimed>=CGO_CAPACITY)return;
    unsigned n=cgo.claimed++;
    cgo.current.status=status;cgo.current.result=(unsigned)result;
    cgo.rows[n].row=cgo.current;
    __atomic_store_n(&cgo.rows[n].ready,1,__ATOMIC_RELEASE);
}
static void cgo_release_completed(void) {
    for(unsigned i=0;i<3;++i)if(cgo.queries[i]) {ID3D11Query_Release(cgo.queries[i]);cgo.queries[i]=0;}
    if(cgo.context){ID3D11DeviceContext_Release(cgo.context);cgo.context=0;}
    if(cgo.device){ID3D11Device_Release(cgo.device);cgo.device=0;}
    cgo.terminal=1;
}
static void cgo_collect(void) {
    if(!cgo.pending)return;
    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT frequency={0};UINT64 begin=0,end=0;
    void *outputs[]={&frequency,&begin,&end};unsigned sizes[]={sizeof(frequency),sizeof(begin),sizeof(end)};
    for(unsigned i=0;i<3;++i) {
        HRESULT hr=ID3D11DeviceContext_GetData(cgo.context,(ID3D11Asynchronous *)cgo.queries[i],
            outputs[i],sizes[i],D3D11_ASYNC_GETDATA_DONOTFLUSH);
        if(hr==S_FALSE){++cgo.current.pending_reads;return;} /* no loop/Flush/wait */
        if(hr!=S_OK){cgo.current.cpu_read=cgo_qpc();cgo_publish(CGO_READ_ERROR,hr);cgo.terminal=1;return;}
    }
    cgo.current.gpu_begin=begin;cgo.current.gpu_end=end;cgo.current.gpu_frequency=frequency.Frequency;
    cgo.current.cpu_read=cgo_qpc();
    unsigned status=frequency.Disjoint?CGO_DISJOINT:!frequency.Frequency?CGO_ZERO_FREQUENCY:
        end<begin?CGO_REVERSED:CGO_VALID;
    cgo_publish(status,S_OK);cgo.pending=0;
    if(cgo.claimed>=CGO_CAPACITY)cgo_release_completed();
}
void copy_gpu_observe_begin(void *device,void *context,EyeGpuTicket ticket,EyeGpuDescription desc,int allowed) {
    DWORD saved=GetLastError();unsigned thread=GetCurrentThreadId();
    if(!allowed || cgo.terminal || !device || !context || !ticket.serial || !ticket.generation || !ticket.epoch ||
       (cgo.thread && cgo.thread!=thread) || (cgo.context && cgo.context!=context) ||
       (cgo.device && cgo.device!=device))goto done;
    unsigned long long fresh=__atomic_load_n(&cgo_fresh_ms,__ATOMIC_ACQUIRE),now=CGO_TICK();
    if(!fresh || now<fresh || now-fresh<2000)goto done;
    cgo_collect();
    if(cgo.terminal || cgo.active || cgo.pending || cgo.claimed>=CGO_CAPACITY ||
       (ticket.serial>=cgo.last_serial && ticket.serial-cgo.last_serial<4 && cgo.last_serial))goto done;
    if(cgo.last_ms && (now<cgo.last_ms || now-cgo.last_ms<100))goto done;
    cgo.current=(CopyGpuObservation){0};cgo.current.ticket=ticket;cgo.current.thread=thread;
    cgo.current.width=desc.width;cgo.current.height=desc.height;cgo.current.format=desc.format;
    cgo.current.device=(unsigned long long)device;cgo.current.context=(unsigned long long)context;
    if(!cgo.device) {
        cgo.device=device;cgo.context=context;cgo.thread=thread;
        ID3D11Device_AddRef(cgo.device);ID3D11DeviceContext_AddRef(cgo.context);
        for(unsigned i=0;i<3;++i) {
            D3D11_QUERY_DESC q={i?D3D11_QUERY_TIMESTAMP:D3D11_QUERY_TIMESTAMP_DISJOINT,0};
            HRESULT hr=ID3D11Device_CreateQuery(cgo.device,&q,&cgo.queries[i]);
            if(FAILED(hr) || !cgo.queries[i]) {cgo_publish(CGO_CREATE_ERROR,hr);cgo_release_completed();goto done;}
        }
    }
    ID3D11DeviceContext_Begin(cgo.context,(ID3D11Asynchronous *)cgo.queries[0]);
    ID3D11DeviceContext_End(cgo.context,(ID3D11Asynchronous *)cgo.queries[1]);
    cgo.current.cpu_begin=cgo_qpc();cgo.active=1;cgo.last_serial=ticket.serial;cgo.last_ms=now;
done:
    SetLastError(saved);
}
void copy_gpu_observe_end(void *context,EyeGpuTicket ticket,unsigned copies,unsigned upright) {
    DWORD saved=GetLastError();
    if(!cgo.terminal && cgo.active && cgo.thread==GetCurrentThreadId() && cgo.context==context &&
       cgo_same(cgo.current.ticket,ticket)) {
        ID3D11DeviceContext_End(cgo.context,(ID3D11Asynchronous *)cgo.queries[2]);
        ID3D11DeviceContext_End(cgo.context,(ID3D11Asynchronous *)cgo.queries[0]);
        cgo.current.cpu_end=cgo_qpc();cgo.current.copy_mask=copies;cgo.current.upright=upright;
        cgo.active=0;cgo.pending=1;
    }
    SetLastError(saved);
}
int copy_gpu_observe_take(unsigned *cursor,CopyGpuObservation *out) {
    if(!cursor || !out || *cursor>=CGO_CAPACITY ||
       !__atomic_load_n(&cgo.rows[*cursor].ready,__ATOMIC_ACQUIRE))return 0;
    *out=cgo.rows[*cursor].row; ++*cursor;return 1;
}
#endif
#endif
