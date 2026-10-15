#ifndef GF2_BANK_EYE_WRITE_OBSERVE_H
#define GF2_BANK_EYE_WRITE_OBSERVE_H
#include "eye_gpu_copy.h"
#define BEW_CAPACITY 32u
enum { BEW_VALID=1,BEW_DISJOINT,BEW_ZERO_FREQUENCY,BEW_REVERSED,
       BEW_CREATE_ERROR,BEW_READ_ERROR,BEW_MISSING_WRITES,BEW_RECLEARED };
typedef struct {
    EyeGpuTicket ticket;
    unsigned long long gpu_begin,gpu_end,gpu_frequency,cpu_begin,cpu_end,cpu_read,device,context;
    unsigned long long gpu_eye[2],cpu_eye[2];
    unsigned thread,width,height,format,draw_mask,clear_mask,status,result,pending_reads;
    unsigned seen_mask,reclear_mask,first_mask,draws[3];
} BankEyeWriteObservation;
void bank_eye_write_observe_begin(void *,void *,EyeGpuTicket,EyeGpuDescription,int);
void bank_eye_write_observe_end(void *,EyeGpuTicket,unsigned,unsigned);
void bank_eye_write_observe_draw(void *,EyeGpuTicket,unsigned);
void bank_eye_write_observe_clear(void *,EyeGpuTicket,unsigned);
int bank_eye_write_observe_take(unsigned *,BankEyeWriteObservation *);
#ifdef GF2_BANK_EYE_WRITE_IMPLEMENTATION
#ifndef BEW_TICK
#define BEW_TICK() GetTickCount64()
#endif
/* Exact original render producer, numeric main consumer. This never owns
   Unity/XR eye resources or supplies product completion. Five query objects
   are reused only after their preceding issued bracket has completed. */
static struct {
    ID3D11Device *device;ID3D11DeviceContext *context;ID3D11Query *queries[5];
    BankEyeWriteObservation current;
    unsigned thread,active,pending,terminal,claimed;
    unsigned long long last_serial,last_ms;
    struct {unsigned ready;BankEyeWriteObservation row;} rows[BEW_CAPACITY];
} bew;
static unsigned long long bew_qpc(void) {
    LARGE_INTEGER q={0};QueryPerformanceCounter(&q);return (unsigned long long)q.QuadPart;
}
static int bew_same(EyeGpuTicket a,EyeGpuTicket b) {
    return a.serial && a.serial==b.serial && a.generation==b.generation && a.epoch==b.epoch;
}
static int bew_owner(void *context,EyeGpuTicket ticket) {
    return !bew.terminal && bew.active && bew.thread==GetCurrentThreadId() &&
        bew.context==context && bew_same(bew.current.ticket,ticket);
}
static void bew_publish(unsigned status,HRESULT result) {
    if(bew.claimed>=BEW_CAPACITY)return;
    unsigned n=bew.claimed++;bew.current.status=status;bew.current.result=(unsigned)result;
    bew.rows[n].row=bew.current;__atomic_store_n(&bew.rows[n].ready,1,__ATOMIC_RELEASE);
}
static void bew_release_completed(void) {
    for(unsigned i=0;i<5;++i)if(bew.queries[i]) {ID3D11Query_Release(bew.queries[i]);bew.queries[i]=0;}
    if(bew.context){ID3D11DeviceContext_Release(bew.context);bew.context=0;}
    if(bew.device){ID3D11Device_Release(bew.device);bew.device=0;}
    bew.terminal=1;
}
static void bew_collect(void) {
    if(!bew.pending)return;
    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT frequency={0};UINT64 begin=0,end=0,eyes[2]={0};
    void *outputs[]={&frequency,&begin,&end,&eyes[0],&eyes[1]};
    unsigned sizes[]={sizeof(frequency),sizeof(begin),sizeof(end),sizeof(eyes[0]),sizeof(eyes[1])};
    for(unsigned i=0;i<5;++i) {
        if(i>=3 && !(bew.current.seen_mask&(1u<<(i-3))))continue; /* never read unissued query */
        HRESULT hr=ID3D11DeviceContext_GetData(bew.context,(ID3D11Asynchronous *)bew.queries[i],
            outputs[i],sizes[i],D3D11_ASYNC_GETDATA_DONOTFLUSH);
        if(hr==S_FALSE){++bew.current.pending_reads;return;}
        if(hr!=S_OK){bew.current.cpu_read=bew_qpc();bew_publish(BEW_READ_ERROR,hr);bew.terminal=1;return;}
    }
    bew.current.gpu_begin=begin;bew.current.gpu_end=end;bew.current.gpu_frequency=frequency.Frequency;
    bew.current.gpu_eye[0]=eyes[0];bew.current.gpu_eye[1]=eyes[1];bew.current.cpu_read=bew_qpc();
    int reversed=end<begin;
    for(unsigned i=0;i<2;++i)if(bew.current.seen_mask&(1u<<i))reversed|=eyes[i]<begin || eyes[i]>end;
    if(bew.current.seen_mask==3)reversed|=
        (bew.current.first_mask==1 && eyes[0]>eyes[1]) ||
        (bew.current.first_mask==2 && eyes[1]>eyes[0]);
    unsigned status=frequency.Disjoint?BEW_DISJOINT:!frequency.Frequency?BEW_ZERO_FREQUENCY:
        reversed?BEW_REVERSED:bew.current.reclear_mask?BEW_RECLEARED:
        bew.current.seen_mask!=3 || bew.current.draw_mask!=3 || bew.current.clear_mask!=3?BEW_MISSING_WRITES:BEW_VALID;
    bew_publish(status,S_OK);bew.pending=0;
    if(bew.claimed>=BEW_CAPACITY)bew_release_completed();
}
void bank_eye_write_observe_begin(void *device,void *context,EyeGpuTicket ticket,EyeGpuDescription desc,int allowed) {
    DWORD saved=GetLastError();unsigned thread=GetCurrentThreadId();
    if(!allowed || bew.terminal || !device || !context || !ticket.serial || !ticket.generation || !ticket.epoch ||
       (bew.thread && bew.thread!=thread) || (bew.context && bew.context!=context) ||
       (bew.device && bew.device!=device))goto done;
    bew_collect();
    if(bew.terminal || bew.active || bew.pending || bew.claimed>=BEW_CAPACITY ||
       (bew.last_serial && (ticket.serial<=bew.last_serial || ticket.serial-bew.last_serial<4)))goto done;
    unsigned long long now=BEW_TICK();
    if(bew.last_ms && (now<bew.last_ms || now-bew.last_ms<100))goto done;
    bew.current=(BankEyeWriteObservation){0};bew.current.ticket=ticket;bew.current.thread=thread;
    bew.current.width=desc.width;bew.current.height=desc.height;bew.current.format=desc.format;
    bew.current.device=(unsigned long long)device;bew.current.context=(unsigned long long)context;
    if(!bew.device) {
        bew.device=device;bew.context=context;bew.thread=thread;
        ID3D11Device_AddRef(bew.device);ID3D11DeviceContext_AddRef(bew.context);
        for(unsigned i=0;i<5;++i) {
            D3D11_QUERY_DESC q={i?D3D11_QUERY_TIMESTAMP:D3D11_QUERY_TIMESTAMP_DISJOINT,0};
            HRESULT hr=ID3D11Device_CreateQuery(bew.device,&q,&bew.queries[i]);
            if(FAILED(hr) || !bew.queries[i]) {bew_publish(BEW_CREATE_ERROR,hr);bew_release_completed();goto done;}
        }
    }
    ID3D11DeviceContext_Begin(bew.context,(ID3D11Asynchronous *)bew.queries[0]);
    ID3D11DeviceContext_End(bew.context,(ID3D11Asynchronous *)bew.queries[1]);
    bew.current.cpu_begin=bew_qpc();bew.active=1;bew.last_serial=ticket.serial;bew.last_ms=now;
done:
    SetLastError(saved);
}
void bank_eye_write_observe_draw(void *context,EyeGpuTicket ticket,unsigned mask) {
    DWORD saved=GetLastError();
    if(mask<=3 && bew_owner(context,ticket)) {
        if(!mask && bew.current.draws[2]!=~0u)++bew.current.draws[2];
        if(mask && !bew.current.seen_mask)bew.current.first_mask=mask;
        for(unsigned i=0;i<2;++i)if(mask&(1u<<i)) {
            if(bew.current.draws[i]!=~0u)++bew.current.draws[i];
            if(!(bew.current.seen_mask&(1u<<i))) {
                ID3D11DeviceContext_End(bew.context,(ID3D11Asynchronous *)bew.queries[3+i]);
                bew.current.cpu_eye[i]=bew_qpc();bew.current.seen_mask|=1u<<i;
            }
        }
    }
    SetLastError(saved);
}
void bank_eye_write_observe_clear(void *context,EyeGpuTicket ticket,unsigned mask) {
    DWORD saved=GetLastError();
    if(mask<=3 && bew_owner(context,ticket))bew.current.reclear_mask|=mask&bew.current.seen_mask;
    SetLastError(saved);
}
void bank_eye_write_observe_end(void *context,EyeGpuTicket ticket,unsigned draw,unsigned clear) {
    DWORD saved=GetLastError();
    if(bew_owner(context,ticket)) {
        ID3D11DeviceContext_End(bew.context,(ID3D11Asynchronous *)bew.queries[2]);
        ID3D11DeviceContext_End(bew.context,(ID3D11Asynchronous *)bew.queries[0]);
        bew.current.cpu_end=bew_qpc();bew.current.draw_mask=draw;bew.current.clear_mask=clear;
        bew.active=0;bew.pending=1;
    }
    SetLastError(saved);
}
int bank_eye_write_observe_take(unsigned *cursor,BankEyeWriteObservation *out) {
    if(!cursor || !out || *cursor>=BEW_CAPACITY || !__atomic_load_n(&bew.rows[*cursor].ready,__ATOMIC_ACQUIRE))return 0;
    *out=bew.rows[*cursor].row;++*cursor;return 1;
}
#endif
#endif
