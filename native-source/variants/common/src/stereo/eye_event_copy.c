#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include "eye_event_copy.h"
#ifdef GF2VR_BANK_DISPATCH_OBSERVE
#include "bank_fence_observe.h"
#define BDO_COPY_NOTE(p,phase,value) do {if(!(p)->drain_only)bank_dispatch_observe_record((p)->ticket.serial,(p)->ticket.generation,(p)->ticket.epoch,phase,value);} while(0)
#else
#define BDO_COPY_NOTE(p,phase,value) ((void)0)
#endif
static int eye_event_prepare_kind(EyeEventCopy *p,int id,EyeGpuTicket ticket,unsigned render_thread,
                     int order_ready,void *device,void *context,void *left,void *right,
                     void *dl,void *dr,unsigned drain_only
#ifdef GF2VR_RENDER_OWNER_COPY
                     ,unsigned prime
#endif
                     ) {
    if(!p || __atomic_load_n(&p->state,__ATOMIC_ACQUIRE)!=EEC_IDLE ||
       !order_ready || !render_thread || id<=0 || id<=p->last_event_id ||
       !ticket.serial || !ticket.generation || !ticket.epoch ||
       (p->owner_thread && p->owner_thread!=GetCurrentThreadId()) ||
       (p->render_thread && p->render_thread!=render_thread) ||
       !device || !context || !left || !right || (!drain_only && (!dl || !dr))) return 0;
    /* Cross-thread native device/context ownership requires a multithread device.
       Serializing actual immediate-context commands remains the adapter's duty. */
    if(ID3D11Device_GetCreationFlags((ID3D11Device *)device)&D3D11_CREATE_DEVICE_SINGLETHREADED) return 0;
    void *refs[]={device,context,left,right,dl,dr};
    for(unsigned i=0;i<6;++i) {if(refs[i])IUnknown_AddRef((IUnknown *)refs[i]);p->refs[i]=refs[i];}
    if(!p->owner_thread)p->owner_thread=GetCurrentThreadId();
    if(!p->render_thread)p->render_thread=render_thread;
    p->event_id=id;p->last_event_id=id;p->ticket=ticket;p->drain_only=drain_only;
#ifdef GF2VR_BANK_COPY_DELIVERY_OBSERVE
    p->timing=(EyeGpuTiming){0};
#endif
#ifdef GF2VR_RENDER_OWNER_COPY
    /* Publish immutable metadata once. Native service must never observe a
       transient QUEUED request while main is still initializing its payload. */
    p->native_dispatch=prime;
    if(prime)p->drain_only=0;
    __atomic_store_n(&p->state,prime?EEC_PRIMED:EEC_QUEUED,__ATOMIC_RELEASE);
#else
    __atomic_store_n(&p->state,EEC_QUEUED,__ATOMIC_RELEASE);
#endif
    return 1;
}
int eye_event_prepare(EyeEventCopy *p,int id,EyeGpuTicket ticket,unsigned render_thread,
                     int order_ready,void *device,void *context,void *left,void *right,
                     void *dl,void *dr) {
    return eye_event_prepare_kind(p,id,ticket,render_thread,order_ready,device,context,left,right,dl,dr,0
#ifdef GF2VR_RENDER_OWNER_COPY
        ,0
#endif
        );
}
int eye_event_prepare_drain(EyeEventCopy *p,int id,EyeGpuTicket ticket,unsigned render_thread,
                           int order_ready,void *device,void *context,void *left,void *right) {
    return eye_event_prepare_kind(p,id,ticket,render_thread,order_ready,device,context,left,right,0,0,1
#ifdef GF2VR_RENDER_OWNER_COPY
        ,0
#endif
        );
}
#ifdef GF2VR_RENDER_OWNER_COPY
int eye_event_prime(EyeEventCopy *p,int id,EyeGpuTicket ticket,unsigned render_thread,
    int ordered,void *device,void *context,void *left,void *right) {
    DWORD saved=GetLastError();
    /* Retain validated sources, then publish PRIMED directly; issued stays zero. */
    int ok=eye_event_prepare_kind(p,id,ticket,render_thread,ordered,device,context,left,right,0,0,1,1);
    SetLastError(saved);return ok;
}
int eye_event_post_native(EyeEventCopy *p,EyeGpuTicket ticket,void *left,void *right) {
    if(!p || !left || !right || left==right ||
       __atomic_load_n(&p->state,__ATOMIC_ACQUIRE)!=EEC_PRIMED)return 0;
    if(!p->native_dispatch || !p->owner_thread || !p->render_thread ||
       ticket.serial!=p->ticket.serial || ticket.generation!=p->ticket.generation ||
       ticket.epoch!=p->ticket.epoch || !ticket.serial)return 0;
    unsigned expected=EEC_PRIMED;
    if(!__atomic_compare_exchange_n(&p->state,&expected,EEC_RUNNING,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE))return 0;
    DWORD saved=GetLastError();
    IUnknown_AddRef((IUnknown *)left);IUnknown_AddRef((IUnknown *)right);
    p->refs[4]=left;p->refs[5]=right;
    /* COPY_ISSUED and exact waited leases are already pinned by the backend.
       issued is eligibility for native service, not a Unity enqueue. */
    __atomic_store_n(&p->issued,1,__ATOMIC_RELEASE);
    __atomic_store_n(&p->state,EEC_QUEUED,__ATOMIC_RELEASE);
    SetLastError(saved);return 1;
}
void eye_event_service_native(EyeEventCopy *p,void *device,void *context) {
    if(!p || !device || !context ||
       __atomic_load_n(&p->state,__ATOMIC_ACQUIRE)!=EEC_QUEUED ||
       !p->native_dispatch || GetCurrentThreadId()!=p->render_thread ||
       p->refs[0]!=device || p->refs[1]!=context)return;
    eye_event_callback(p,p->event_id); /* existing CAS/GPU submission/fault path */
}
int eye_event_cancel_prime(EyeEventCopy *p) {
    if(!p || GetCurrentThreadId()!=p->owner_thread)return 0;
    unsigned expected=EEC_PRIMED;
    if(!__atomic_compare_exchange_n(&p->state,&expected,EEC_RUNNING,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE))return 0;
    DWORD saved=GetLastError();
    for(unsigned i=0;i<4;++i){IUnknown_Release((IUnknown *)p->refs[i]);p->refs[i]=0;}
    p->ticket=(EyeGpuTicket){0};p->event_id=0;p->native_dispatch=0;
    __atomic_store_n(&p->state,EEC_IDLE,__ATOMIC_RELEASE);
    SetLastError(saved);return 1;
}
#endif
int eye_event_issue(EyeEventCopy *p,void (*issue)(void (*)(int),int),void (*callback)(int)) {
    if(!p || !issue || !callback || __atomic_load_n(&p->state,__ATOMIC_ACQUIRE)!=EEC_QUEUED ||
       GetCurrentThreadId()!=p->owner_thread || p->issued) return 0;
    /* Publish issuance before calling Unity: direct-mode callbacks may reenter. */
    __atomic_store_n(&p->issued,1,__ATOMIC_RELEASE);
    DWORD saved=GetLastError();
    /* Timestamp BEFORE Unity issue: a direct callback may run inside the call. */
    BDO_COPY_NOTE(p,12,(unsigned)p->event_id);
    issue(callback,p->event_id);SetLastError(saved);return 1;
}
void eye_event_callback(EyeEventCopy *p,int id) {
    if(!p || __atomic_load_n(&p->state,__ATOMIC_ACQUIRE)!=EEC_QUEUED ||
       !__atomic_load_n(&p->issued,__ATOMIC_ACQUIRE)) return;
    if(GetCurrentThreadId()!=p->render_thread) return;
    unsigned expected=EEC_QUEUED;
    if(!__atomic_compare_exchange_n(&p->state,&expected,EEC_RUNNING,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE)) return;
    if(id!=p->event_id){__atomic_store_n(&p->state,EEC_QUEUED,__ATOMIC_RELEASE);return;}
    BDO_COPY_NOTE(p,13,(unsigned)id);
    int ok=p->drain_only ? eye_gpu_drain(&p->gpu,p->refs[0],p->refs[1],p->refs[2],p->refs[3],p->ticket) :
        eye_gpu_submit(&p->gpu,p->refs[0],p->refs[1],p->refs[2],p->refs[3],p->refs[4],p->refs[5],p->ticket);
    BDO_COPY_NOTE(p,14,ok!=0); /* CPU command submission result, NOT GPU drain. */
    /* Even pre-copy failure doesn't drain earlier Unity render work. Keep all
       request refs, XR acquisition and engine borrow quarantined. */
    __atomic_store_n(&p->state,ok?EEC_PENDING:EEC_FAULT,__ATOMIC_RELEASE);
}
void eye_event_poll(EyeEventCopy *p) {
    if(!p || __atomic_load_n(&p->state,__ATOMIC_ACQUIRE)!=EEC_PENDING ||
       GetCurrentThreadId()!=p->render_thread) return;
    unsigned expected=EEC_PENDING;
    if(!__atomic_compare_exchange_n(&p->state,&expected,EEC_RUNNING,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE)) return;
    int state=eye_gpu_poll(&p->gpu);
    unsigned next=EEC_PENDING;
    if(state==EGC_DRAINED) {
#ifdef GF2VR_BANK_COPY_DELIVERY_OBSERVE
        p->timing=p->gpu.timing; /* retain scalar proof BEFORE GPU disposal */
#endif
        next=eye_gpu_dispose(&p->gpu)?EEC_DONE:EEC_FAULT;
    }
    else if(state!=EGC_PENDING) next=EEC_FAULT;
    /* Dispose backend references on their render owner BEFORE publishing done.
       Request references survive for owner-thread acknowledgement. */
    __atomic_store_n(&p->state,next,__ATOMIC_RELEASE);
}
#if defined(GF2VR_PERF_FENCE_DELIVERY) && defined(GF2VR_BANK_FENCE_COMPLETION)
void eye_event_fence_complete(EyeEventCopy *p) {
    if(!p)return;
    unsigned expected=EEC_PENDING;
    if(!__atomic_compare_exchange_n(&p->state,&expected,EEC_RUNNING,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE))return;
    DWORD saved=GetLastError();unsigned next=EEC_PENDING;
    /* PENDING release proves the matching callback has finished submission.
       CAS excludes render poll; no pending refs or request refs are released. */
    if(!p->ticket.serial || p->gpu.ticket.serial!=p->ticket.serial ||
       p->gpu.ticket.generation!=p->ticket.generation || p->gpu.ticket.epoch!=p->ticket.epoch ||
       p->gpu.drain_only!=p->drain_only)next=EEC_FAULT;
    else {
        int state=eye_gpu_poll_fenced(&p->gpu);
        if(state==EGC_DRAINED) {
#ifdef GF2VR_BANK_COPY_DELIVERY_OBSERVE
            p->timing=p->gpu.timing;
#endif
            next=eye_gpu_dispose_fenced(&p->gpu)?EEC_DONE:EEC_FAULT;
        }
        else if(state!=EGC_PENDING)next=EEC_FAULT;
    }
    __atomic_store_n(&p->state,next,__ATOMIC_RELEASE);SetLastError(saved);
}
#endif
static int eye_event_take_kind(EyeEventCopy *p,EyeGpuTicket *out,unsigned drain_only) {
    if(out) *out=(EyeGpuTicket){0};
    if(!p || !out || __atomic_load_n(&p->state,__ATOMIC_ACQUIRE)!=EEC_DONE ||
       GetCurrentThreadId()!=p->owner_thread || p->drain_only!=drain_only) return 0;
    *out=p->ticket;
    for(unsigned i=0;i<6;++i) {if(p->refs[i])IUnknown_Release((IUnknown *)p->refs[i]);p->refs[i]=0;}
    p->ticket=(EyeGpuTicket){0};p->event_id=0;p->drain_only=0;
#ifdef GF2VR_BANK_COPY_DELIVERY_OBSERVE
    p->timing=(EyeGpuTiming){0};
#endif
#ifdef GF2VR_RENDER_OWNER_COPY
    p->native_dispatch=0;
#endif
    __atomic_store_n(&p->issued,0,__ATOMIC_RELEASE);
    __atomic_store_n(&p->state,EEC_IDLE,__ATOMIC_RELEASE);return 1;
}
int eye_event_take(EyeEventCopy *p,EyeGpuTicket *out) {return eye_event_take_kind(p,out,0);}
int eye_event_take_drain(EyeEventCopy *p,EyeGpuTicket *out) {return eye_event_take_kind(p,out,1);}
