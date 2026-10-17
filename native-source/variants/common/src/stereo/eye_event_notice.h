#ifndef GF2_EYE_EVENT_NOTICE_H
#define GF2_EYE_EVENT_NOTICE_H
#include "eye_gpu_copy.h"
#define EGN_CAPACITY 4u /* render/copy across two source flights; reusable capacity */
typedef struct {EyeGpuTicket ticket;unsigned drain_only,valid;unsigned long long signal,checked;} EyeGpuNotice;
int eye_gpu_notice_take(unsigned *,unsigned *,EyeGpuNotice *); /* sole-XR consumer */
void eye_gpu_notice_return(unsigned,int consumed);
unsigned long long eye_gpu_notice_stat(unsigned);
#ifdef GF2_EYE_EVENT_NOTICE_IMPLEMENTATION
#ifndef EGN_SET_EVENT
#define EGN_SET_EVENT(f,e) ID3D11Fence_SetEventOnCompletion((f),1,(e))
#endif
static struct {HANDLE events[EGN_CAPACITY+1];unsigned init,wait_error;
    unsigned long long posted,completed,dropped;
    struct {unsigned state;ID3D11Fence *fence;ID3D11Device *device;HRESULT armed;
        EyeGpuNotice notice;} slots[EGN_CAPACITY];} egn;
static unsigned long long egn_qpc(void){LARGE_INTEGER q={0};QueryPerformanceCounter(&q);return (unsigned long long)q.QuadPart;}
/* No product pointers/commands/ack/source lifetime on this worker. Only retained
   fence/device facts are published. Handles remain stable until process exit. */
static DWORD WINAPI egn_worker(void *unused){
    (void)unused;unsigned seen[EGN_CAPACITY]={0};
    for(;;){
        DWORD wait=WaitForMultipleObjects(EGN_CAPACITY+1,egn.events,FALSE,INFINITE);
        if(wait<WAIT_OBJECT_0+EGN_CAPACITY)seen[wait-WAIT_OBJECT_0]=1;
        else if(wait!=WAIT_OBJECT_0+EGN_CAPACITY){__atomic_store_n(&egn.wait_error,GetLastError(),__ATOMIC_RELEASE);__atomic_store_n(&egn.init,3,__ATOMIC_RELEASE);return 0;}
        for(unsigned i=0;i<EGN_CAPACITY;++i){
            if(__atomic_load_n(&egn.slots[i].state,__ATOMIC_ACQUIRE)!=2)continue;
            if(!seen[i]&&SUCCEEDED(egn.slots[i].armed))continue;
            EyeGpuNotice *r=&egn.slots[i].notice;r->valid=0;
            if(SUCCEEDED(egn.slots[i].armed)){
                UINT64 value=ID3D11Fence_GetCompletedValue(egn.slots[i].fence);
                HRESULT removed=ID3D11Device_GetDeviceRemovedReason(egn.slots[i].device);
                r->checked=egn_qpc();r->valid=value>=1&&value!=~0ull&&SUCCEEDED(removed);
            }
            ID3D11Fence_Release(egn.slots[i].fence);ID3D11Device_Release(egn.slots[i].device);
            egn.slots[i].fence=0;egn.slots[i].device=0;seen[i]=0;
            __atomic_add_fetch(&egn.completed,1,__ATOMIC_RELAXED);
            __atomic_store_n(&egn.slots[i].state,3,__ATOMIC_RELEASE); /* previous arm returned and event consumed */
        }
    }
}
static int egn_open(void){
    unsigned state=__atomic_load_n(&egn.init,__ATOMIC_ACQUIRE);if(state)return state==2;
    unsigned expected=0;if(!__atomic_compare_exchange_n(&egn.init,&expected,1,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE))return 0;
    for(unsigned i=0;i<=EGN_CAPACITY;++i){egn.events[i]=CreateEventW(0,FALSE,FALSE,0);if(!egn.events[i])goto fail;}
    HANDLE worker=CreateThread(0,0,egn_worker,0,0,0);if(!worker)goto fail;
    CloseHandle(worker);__atomic_store_n(&egn.init,2,__ATOMIC_RELEASE);return 1;
fail:
    __atomic_store_n(&egn.wait_error,GetLastError(),__ATOMIC_RELEASE);
    for(unsigned i=0;i<=EGN_CAPACITY;++i)if(egn.events[i]){CloseHandle(egn.events[i]);egn.events[i]=0;}
    __atomic_store_n(&egn.init,3,__ATOMIC_RELEASE);return 0; /* optional path unavailable; original completion continues */
}
static void eye_gpu_notice_post(EyeGpuCopy *p){
    DWORD saved=GetLastError();
    if(!p||!p->fence_armed||!p->fence||!p->device||p->thread!=GetCurrentThreadId()||
       !p->ticket.serial||!p->ticket.generation||!p->ticket.epoch||!egn_open())goto out;
    unsigned i;
    for(i=0;i<EGN_CAPACITY;++i){unsigned expected=0;if(__atomic_compare_exchange_n(&egn.slots[i].state,&expected,1,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE))break;}
    if(i==EGN_CAPACITY){__atomic_add_fetch(&egn.dropped,1,__ATOMIC_RELAXED);goto out;}
    egn.slots[i].notice=(EyeGpuNotice){.ticket=p->ticket,.drain_only=p->drain_only};
#ifdef GF2VR_PERF_FENCE_TIMING
    egn.slots[i].notice.signal=p->timing.signal;
#endif
    egn.slots[i].fence=p->fence;egn.slots[i].device=p->device;
    ID3D11Fence_AddRef(egn.slots[i].fence);ID3D11Device_AddRef(egn.slots[i].device);
    egn.slots[i].armed=EGN_SET_EVENT(egn.slots[i].fence,egn.events[i]);
    __atomic_store_n(&egn.slots[i].state,2,__ATOMIC_RELEASE);__atomic_add_fetch(&egn.posted,1,__ATOMIC_RELAXED);
    SetEvent(egn.events[EGN_CAPACITY]); /* failed arm/synchronous completion both progress without a frame timer */
out:SetLastError(saved);
}
int eye_gpu_notice_take(unsigned *cursor,unsigned *slot,EyeGpuNotice *out){
    if(!cursor||!slot||!out)return 0;
    for(unsigned n=0;n<EGN_CAPACITY;++n){unsigned i=(*cursor+n)%EGN_CAPACITY,expected=3;
        if(__atomic_compare_exchange_n(&egn.slots[i].state,&expected,4,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE)){
            *out=egn.slots[i].notice;*slot=i;*cursor=(i+1)%EGN_CAPACITY;return 1;}}
    return 0;
}
void eye_gpu_notice_return(unsigned slot,int consumed){
    if(slot>=EGN_CAPACITY)return;unsigned expected=4;
    (void)__atomic_compare_exchange_n(&egn.slots[slot].state,&expected,consumed?0:3,0,__ATOMIC_RELEASE,__ATOMIC_RELAXED);
}
unsigned long long eye_gpu_notice_stat(unsigned i){switch(i){case 0:return __atomic_load_n(&egn.init,__ATOMIC_ACQUIRE);
 case 1:return __atomic_load_n(&egn.posted,__ATOMIC_ACQUIRE);case 2:return __atomic_load_n(&egn.completed,__ATOMIC_ACQUIRE);
 case 3:return __atomic_load_n(&egn.dropped,__ATOMIC_ACQUIRE);case 4:return __atomic_load_n(&egn.wait_error,__ATOMIC_ACQUIRE);default:return 0;}}
#endif
#endif
