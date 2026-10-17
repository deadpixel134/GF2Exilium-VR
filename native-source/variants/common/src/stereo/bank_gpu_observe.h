#ifndef GF2_BANK_GPU_OBSERVE_H
#define GF2_BANK_GPU_OBSERVE_H
#include "eye_gpu_copy.h"
#define BGO_CAPACITY 32u
enum { BGO_VALID=1,BGO_DISJOINT,BGO_ZERO_FREQUENCY,BGO_REVERSED,BGO_CREATE_ERROR,BGO_READ_ERROR };
typedef struct {
    EyeGpuTicket ticket;
    unsigned long long gpu_begin,gpu_end,gpu_frequency,cpu_begin,cpu_end,cpu_read,device,context;
    unsigned thread,width,height,format,draw_mask,clear_mask,status,result,pending_reads;
} BankGpuObservation;
void bank_gpu_observe_begin(void *,void *,EyeGpuTicket,EyeGpuDescription,int allowed);
void bank_gpu_observe_end(void *,EyeGpuTicket,unsigned draw,unsigned clear);
int bank_gpu_observe_take(unsigned *,BankGpuObservation *);
#ifdef GF2_BANK_GPU_IMPLEMENTATION
#ifndef BGO_TICK
#define BGO_TICK() GetTickCount64()
#endif
/* One render producer, one numeric main consumer. Never completion authority.
   Three queries retain only their device/context, never Unity/XR textures.
   Unfinished queries on source close remain bounded process-lived diagnostics. */
static struct {
    ID3D11Device *device;ID3D11DeviceContext *context;ID3D11Query *queries[3];
    BankGpuObservation current;
    unsigned thread,active,pending,terminal,claimed;
    unsigned long long last_serial,last_ms;
    struct {unsigned ready;BankGpuObservation row;} rows[BGO_CAPACITY];
} bgo;
static unsigned long long bgo_qpc(void) {
    LARGE_INTEGER q={0};QueryPerformanceCounter(&q);return (unsigned long long)q.QuadPart;
}
static int bgo_same(EyeGpuTicket a,EyeGpuTicket b) {
    return a.serial && a.serial==b.serial && a.generation==b.generation && a.epoch==b.epoch;
}
static void bgo_publish(unsigned status,HRESULT result) {
    if(bgo.claimed>=BGO_CAPACITY)return;
    unsigned n=bgo.claimed++;
    bgo.current.status=status;bgo.current.result=(unsigned)result;
    bgo.rows[n].row=bgo.current;
    __atomic_store_n(&bgo.rows[n].ready,1,__ATOMIC_RELEASE);
}
static void bgo_release_completed(void) {
    for(unsigned i=0;i<3;++i)if(bgo.queries[i]) {ID3D11Query_Release(bgo.queries[i]);bgo.queries[i]=0;}
    if(bgo.context){ID3D11DeviceContext_Release(bgo.context);bgo.context=0;}
    if(bgo.device){ID3D11Device_Release(bgo.device);bgo.device=0;}
    bgo.terminal=1;
}
static void bgo_collect(void) {
    if(!bgo.pending)return;
    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT frequency={0};UINT64 begin=0,end=0;
    void *outputs[]={&frequency,&begin,&end};unsigned sizes[]={sizeof(frequency),sizeof(begin),sizeof(end)};
    for(unsigned i=0;i<3;++i) {
        HRESULT hr=ID3D11DeviceContext_GetData(bgo.context,(ID3D11Asynchronous *)bgo.queries[i],
            outputs[i],sizes[i],D3D11_ASYNC_GETDATA_DONOTFLUSH);
        if(hr==S_FALSE){++bgo.current.pending_reads;return;} /* no loop/Flush/wait */
        if(hr!=S_OK){bgo.current.cpu_read=bgo_qpc();bgo_publish(BGO_READ_ERROR,hr);bgo.terminal=1;return;}
    }
    bgo.current.gpu_begin=begin;bgo.current.gpu_end=end;bgo.current.gpu_frequency=frequency.Frequency;
    bgo.current.cpu_read=bgo_qpc();
    unsigned status=frequency.Disjoint?BGO_DISJOINT:!frequency.Frequency?BGO_ZERO_FREQUENCY:
        end<begin?BGO_REVERSED:BGO_VALID;
    bgo_publish(status,S_OK);bgo.pending=0;
    if(bgo.claimed>=BGO_CAPACITY)bgo_release_completed();
}
void bank_gpu_observe_begin(void *device,void *context,EyeGpuTicket ticket,EyeGpuDescription desc,int allowed) {
    DWORD saved=GetLastError();unsigned thread=GetCurrentThreadId();
    if(!allowed || bgo.terminal || !device || !context || !ticket.serial || !ticket.generation || !ticket.epoch ||
       (bgo.thread && bgo.thread!=thread) || (bgo.context && bgo.context!=context) ||
       (bgo.device && bgo.device!=device))goto done;
    bgo_collect();
    if(bgo.terminal || bgo.active || bgo.pending || bgo.claimed>=BGO_CAPACITY ||
       (ticket.serial>=bgo.last_serial && ticket.serial-bgo.last_serial<4 && bgo.last_serial))goto done;
    unsigned long long now=BGO_TICK();
    if(bgo.last_ms && (now<bgo.last_ms || now-bgo.last_ms<100))goto done;
    bgo.current=(BankGpuObservation){0};bgo.current.ticket=ticket;bgo.current.thread=thread;
    bgo.current.width=desc.width;bgo.current.height=desc.height;bgo.current.format=desc.format;
    bgo.current.device=(unsigned long long)device;bgo.current.context=(unsigned long long)context;
    if(!bgo.device) {
        bgo.device=device;bgo.context=context;bgo.thread=thread;
        ID3D11Device_AddRef(bgo.device);ID3D11DeviceContext_AddRef(bgo.context);
        for(unsigned i=0;i<3;++i) {
            D3D11_QUERY_DESC q={i?D3D11_QUERY_TIMESTAMP:D3D11_QUERY_TIMESTAMP_DISJOINT,0};
            HRESULT hr=ID3D11Device_CreateQuery(bgo.device,&q,&bgo.queries[i]);
            if(FAILED(hr) || !bgo.queries[i]) {bgo_publish(BGO_CREATE_ERROR,hr);bgo_release_completed();goto done;}
        }
    }
    ID3D11DeviceContext_Begin(bgo.context,(ID3D11Asynchronous *)bgo.queries[0]);
    ID3D11DeviceContext_End(bgo.context,(ID3D11Asynchronous *)bgo.queries[1]);
    bgo.current.cpu_begin=bgo_qpc();bgo.active=1;bgo.last_serial=ticket.serial;bgo.last_ms=now;
done:
    SetLastError(saved);
}
void bank_gpu_observe_end(void *context,EyeGpuTicket ticket,unsigned draw,unsigned clear) {
    DWORD saved=GetLastError();
    if(!bgo.terminal && bgo.active && bgo.thread==GetCurrentThreadId() && bgo.context==context &&
       bgo_same(bgo.current.ticket,ticket)) {
        ID3D11DeviceContext_End(bgo.context,(ID3D11Asynchronous *)bgo.queries[2]);
        ID3D11DeviceContext_End(bgo.context,(ID3D11Asynchronous *)bgo.queries[0]);
        bgo.current.cpu_end=bgo_qpc();bgo.current.draw_mask=draw;bgo.current.clear_mask=clear;
        bgo.active=0;bgo.pending=1;
    }
    SetLastError(saved);
}
int bank_gpu_observe_take(unsigned *cursor,BankGpuObservation *out) {
    if(!cursor || !out || *cursor>=BGO_CAPACITY ||
       !__atomic_load_n(&bgo.rows[*cursor].ready,__ATOMIC_ACQUIRE))return 0;
    *out=bgo.rows[*cursor].row; ++*cursor;return 1;
}
#endif
#endif
