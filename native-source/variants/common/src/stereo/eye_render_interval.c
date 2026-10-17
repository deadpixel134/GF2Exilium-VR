#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include "eye_render_interval.h"
#ifdef GF2VR_ROOT_INTERVAL_TIMING_OBSERVE
#include "root_interval_observe.h"
static void rit_native(EyeRenderInterval *p,unsigned phase,int result) {
    rit_record((RootIntervalTag){(unsigned long long)p,0,p->ticket.generation,
        p->ticket.epoch,p->ticket.serial},phase,result);
}
#endif
#ifdef GF2VR_BANK_GPU_OBSERVE
#define GF2_BANK_GPU_IMPLEMENTATION
#include "bank_gpu_observe.h"
int bank_cycle_observe_ready(void);
#endif
#ifdef GF2VR_BANK_EYE_WRITE_OBSERVE
#define GF2_BANK_EYE_WRITE_IMPLEMENTATION
#include "bank_eye_write_observe.h"
int bank_cycle_observe_ready(void);
#endif
static int eri_same(EyeGpuTicket a,EyeGpuTicket b) {
    return a.serial && a.generation && a.epoch && a.serial==b.serial &&
        a.generation==b.generation && a.epoch==b.epoch;
}
static void eri_reset(EyeRenderInterval *p) {
    for(unsigned i=0;i<2;++i) {if(p->initial_views[i]) IUnknown_Release((IUnknown *)p->initial_views[i]);p->initial_views[i]=0;}
    for(unsigned i=0;i<6;++i) {if(p->refs[i]) IUnknown_Release((IUnknown *)p->refs[i]);p->refs[i]=0;}
    p->ticket=(EyeGpuTicket){0};p->begin_id=p->end_id=0;
    p->draw_mask=p->clear_mask=p->copy_allowed=p->copied=0;
    p->fault_reason=ERIF_NONE;
    __atomic_store_n(&p->begin_issued,0,__ATOMIC_RELEASE);
    __atomic_store_n(&p->end_issued,0,__ATOMIC_RELEASE);
    __atomic_store_n(&p->abandoned,0,__ATOMIC_RELEASE);
    __atomic_store_n(&p->state,ERI_IDLE,__ATOMIC_RELEASE);
}
static int eri_prepare(EyeRenderInterval *p,int begin,int end,EyeGpuTicket ticket,
                      unsigned thread,int ordered,void *d,void *c,void *l,void *r,void *dl,void *dr,unsigned format) {
    if(!p || __atomic_load_n(&p->state,__ATOMIC_ACQUIRE)!=ERI_IDLE || !ordered || !thread ||
       begin<=0 || begin<=p->last_id || end<=begin || !eri_same(ticket,ticket) ||
       !d || !c || !l || !r || (p->owner_thread && p->owner_thread!=GetCurrentThreadId()) ||
       (p->render_thread && p->render_thread!=thread)) return 0;
    unsigned expected=ERI_IDLE;
    if(!__atomic_compare_exchange_n(&p->state,&expected,ERI_PREPARING,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE)) return 0;
    DWORD saved=GetLastError();EyeGpuDescription description={0};
    if((ID3D11Device_GetCreationFlags((ID3D11Device *)d)&D3D11_CREATE_DEVICE_SINGLETHREADED) ||
       !eye_gpu_describe(d,l,r,&description) || ((dl || dr) && !eye_gpu_preflight(d,l,r,dl,dr))) {
        __atomic_store_n(&p->state,ERI_IDLE,__ATOMIC_RELEASE);SetLastError(saved);return 0;
    }
    if(format) {
        int compatible=description.format==format ||
            (description.format==DXGI_FORMAT_R8G8B8A8_TYPELESS &&
             (format==DXGI_FORMAT_R8G8B8A8_UNORM || format==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB));
        int ok=compatible && (format==28 || format==29 || format==87 || format==91 || format==10);
        D3D11_RENDER_TARGET_VIEW_DESC view={0};view.Format=(DXGI_FORMAT)format;
        view.ViewDimension=D3D11_RTV_DIMENSION_TEXTURE2D;
        void *eyes[]={l,r};
        for(unsigned i=0;ok && i<2;++i)
            ok=SUCCEEDED(ID3D11Device_CreateRenderTargetView((ID3D11Device *)d,
                (ID3D11Resource *)eyes[i],&view,(ID3D11RenderTargetView **)&p->initial_views[i])) && p->initial_views[i];
        if(!ok) {eri_reset(p);SetLastError(saved);return 0;}
    }
    void *refs[]={d,c,l,r,dl,dr};
    for(unsigned i=0;i<6;++i) {if(refs[i]) IUnknown_AddRef((IUnknown *)refs[i]);p->refs[i]=refs[i];}
    p->owner_thread=GetCurrentThreadId();p->render_thread=thread;
    p->begin_id=begin;p->end_id=end;p->last_id=end;p->ticket=ticket;
#if defined(GF2VR_BANK_GPU_OBSERVE) || defined(GF2VR_BANK_EYE_WRITE_OBSERVE)
    p->gpu_description=description;
#endif
    __atomic_store_n(&p->state,ERI_QUEUED,__ATOMIC_RELEASE);SetLastError(saved);return 1;
}
int eye_interval_prepare(EyeRenderInterval *p,int begin,int end,EyeGpuTicket ticket,
                         unsigned thread,int ordered,void *d,void *c,void *l,void *r) {
    return eri_prepare(p,begin,end,ticket,thread,ordered,d,c,l,r,0,0,0);
}
int eye_interval_prepare_copy(EyeRenderInterval *p,int begin,int end,EyeGpuTicket ticket,
                              unsigned thread,int ordered,void *d,void *c,void *l,void *r,void *dl,void *dr) {
    if(!dl || !dr) return 0;
    return eri_prepare(p,begin,end,ticket,thread,ordered,d,c,l,r,dl,dr,0);
}
int eye_interval_prepare_initialized(EyeRenderInterval *p,int begin,int end,EyeGpuTicket ticket,
    unsigned thread,int ordered,void *d,void *c,void *l,void *r,unsigned format) {
    if(!format)return 0;
    return eri_prepare(p,begin,end,ticket,thread,ordered,d,c,l,r,0,0,format);
}
int eye_interval_prepare_initialized_copy(EyeRenderInterval *p,int begin,int end,EyeGpuTicket ticket,
    unsigned thread,int ordered,void *d,void *c,void *l,void *r,void *dl,void *dr,unsigned format) {
    if(!dl || !dr || !format) return 0;
    return eri_prepare(p,begin,end,ticket,thread,ordered,d,c,l,r,dl,dr,format);
}
int eye_interval_cancel(EyeRenderInterval *p,EyeGpuTicket ticket) {
    if(!p || __atomic_load_n(&p->state,__ATOMIC_ACQUIRE)!=ERI_QUEUED ||
       p->owner_thread!=GetCurrentThreadId() || !eri_same(ticket,p->ticket) ||
       __atomic_load_n(&p->begin_issued,__ATOMIC_ACQUIRE)) return 0;
    __atomic_store_n(&p->state,ERI_PREPARING,__ATOMIC_RELEASE);
    DWORD saved=GetLastError();eri_reset(p);SetLastError(saved);return 1;
}
int eye_interval_issue_begin(EyeRenderInterval *p,void (*issue)(void (*)(int),int),void (*cb)(int)) {
    if(!p || !issue || !cb || __atomic_load_n(&p->state,__ATOMIC_ACQUIRE)!=ERI_QUEUED ||
       p->owner_thread!=GetCurrentThreadId() || __atomic_load_n(&p->abandoned,__ATOMIC_ACQUIRE) ||
       __atomic_exchange_n(&p->begin_issued,1,__ATOMIC_ACQ_REL)) return 0;
    DWORD saved=GetLastError();issue(cb,p->begin_id);SetLastError(saved);return 1;
}
static int eri_issue_end(EyeRenderInterval *p,EyeGpuTicket ticket,int ordered,int copy,
                         void (*issue)(void (*)(int),int),void (*cb)(int)) {
    if(!p || !issue || !cb || !ordered || p->owner_thread!=GetCurrentThreadId() ||
       !eri_same(ticket,p->ticket) || !__atomic_load_n(&p->begin_issued,__ATOMIC_ACQUIRE) ||
       __atomic_load_n(&p->abandoned,__ATOMIC_ACQUIRE)) return 0;
    unsigned state=__atomic_load_n(&p->state,__ATOMIC_ACQUIRE);
    if(state!=ERI_QUEUED && state!=ERI_RUNNING && state!=ERI_ACTIVE) return 0;
    if(__atomic_load_n(&p->end_issued,__ATOMIC_ACQUIRE)) return 0;
    p->copy_allowed=copy && p->refs[4] && p->refs[5];
    __atomic_store_n(&p->end_issued,1,__ATOMIC_RELEASE);
    DWORD saved=GetLastError();issue(cb,p->end_id);SetLastError(saved);return 1;
}
int eye_interval_issue_end(EyeRenderInterval *p,EyeGpuTicket ticket,int ordered,
                          void (*issue)(void (*)(int),int),void (*cb)(int)) {
    return eri_issue_end(p,ticket,ordered,1,issue,cb);
}
int eye_interval_issue_retire(EyeRenderInterval *p,EyeGpuTicket ticket,int ordered,
                             void (*issue)(void (*)(int),int),void (*cb)(int)) {
    return eri_issue_end(p,ticket,ordered,0,issue,cb);
}
static int eri_end_watch(EyeRenderInterval *p) {
    if(p->attached) {if(!eye_target_hooks_detach(&p->watch)) {
#ifdef GF2VR_BANK_GPU_OBSERVE
        bank_gpu_observe_end(p->refs[1],p->ticket,0,0);
#endif
#ifdef GF2VR_BANK_EYE_WRITE_OBSERVE
        bank_eye_write_observe_end(p->refs[1],p->ticket,0,0);
#endif
        return 0;
    }p->attached=0;}
    int ok=p->watch.state!=ETW_ACTIVE ||
        eye_target_watch_end(&p->watch,p->ticket,&p->draw_mask,&p->clear_mask);
#ifdef GF2VR_BANK_GPU_OBSERVE
    bank_gpu_observe_end(p->refs[1],p->ticket,ok?p->draw_mask:0,ok?p->clear_mask:0);
#endif
#ifdef GF2VR_BANK_EYE_WRITE_OBSERVE
    bank_eye_write_observe_end(p->refs[1],p->ticket,ok?p->draw_mask:0,ok?p->clear_mask:0);
#endif
    return ok;
}
void eye_interval_callback(EyeRenderInterval *p,int id) {
    if(!p) return;
    unsigned state=__atomic_load_n(&p->state,__ATOMIC_ACQUIRE);
    if((state!=ERI_QUEUED && state!=ERI_ACTIVE) || p->render_thread!=GetCurrentThreadId()) return;
    /* An old callback can overlap a newly prepared but unpublished request.
       Do not read its mutable IDs while the main owner can still cancel it. */
    if(!__atomic_load_n(&p->begin_issued,__ATOMIC_ACQUIRE)) return;
    DWORD saved=GetLastError();
    if(id==p->begin_id && __atomic_load_n(&p->begin_issued,__ATOMIC_ACQUIRE)) {
        unsigned expected=ERI_QUEUED;
        if(__atomic_compare_exchange_n(&p->state,&expected,ERI_RUNNING,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE)) {
#ifdef GF2VR_ROOT_INTERVAL_TIMING_OBSERVE
            rit_native(p,RIT_BEGIN_ENTER,1);
#endif
            if(!__atomic_load_n(&p->abandoned,__ATOMIC_ACQUIRE) &&
               eye_target_watch_begin(&p->watch,p->refs[1],p->refs[2],p->refs[3],p->ticket))
                p->attached=eye_target_hooks_attach(&p->watch);
            if(p->attached && p->initial_views[0] && p->initial_views[1]) {
#ifdef GF2VR_BANK_GPU_OBSERVE
                bank_gpu_observe_begin(p->refs[0],p->refs[1],p->ticket,p->gpu_description,bank_cycle_observe_ready());
#endif
#ifdef GF2VR_BANK_EYE_WRITE_OBSERVE
                bank_eye_write_observe_begin(p->refs[0],p->refs[1],p->ticket,p->gpu_description,bank_cycle_observe_ready());
#endif
                const FLOAT black[4]={0,0,0,1};
                for(unsigned i=0;i<2;++i) {
                    ID3D11DeviceContext_ClearRenderTargetView((ID3D11DeviceContext *)p->refs[1],
                        (ID3D11RenderTargetView *)p->initial_views[i],black);
                    /* Record the command we actually issued, even if Unity's
                       current dispatch variant is installed only at pre-Draw.
                       Identity checks and later-clear invalidation still apply. */
                    eye_target_watch_clear(&p->watch,p->refs[1],p->initial_views[i]);
                }
            }
            /* Hook/begin rejection is not render completion. A valid tail can
               still drain the interval; observations stay ineligible. */
#ifdef GF2VR_ROOT_INTERVAL_TIMING_OBSERVE
            rit_native(p,RIT_BEGIN_RETURN,p->attached!=0);
#endif
            __atomic_store_n(&p->state,ERI_ACTIVE,__ATOMIC_RELEASE);
        }
    } else if(id==p->end_id && __atomic_load_n(&p->end_issued,__ATOMIC_ACQUIRE)) {
        unsigned expected=ERI_ACTIVE;
        if(__atomic_compare_exchange_n(&p->state,&expected,ERI_RUNNING,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE)) {
#ifdef GF2VR_ROOT_INTERVAL_TIMING_OBSERVE
            rit_native(p,RIT_TAIL_ENTER,1);
#endif
            int ok=eri_end_watch(p);
            if(!ok) p->fault_reason=ERIF_END_WATCH;
            if(ok) {
                p->copied=p->copy_allowed && !__atomic_load_n(&p->abandoned,__ATOMIC_ACQUIRE) &&
                    p->draw_mask==3 && p->clear_mask==3;
                ok=p->copied ? eye_gpu_submit(&p->gpu,p->refs[0],p->refs[1],p->refs[2],p->refs[3],
                    p->refs[4],p->refs[5],p->ticket) :
                    eye_gpu_drain(&p->gpu,p->refs[0],p->refs[1],p->refs[2],p->refs[3],p->ticket);
                if(!ok) p->fault_reason=ERIF_GPU_SUBMIT;
            }
#ifdef GF2VR_ROOT_INTERVAL_TIMING_OBSERVE
            rit_native(p,RIT_TAIL_RETURN,ok);
#endif
            __atomic_store_n(&p->state,ok?ERI_PENDING:ERI_FAULT,__ATOMIC_RELEASE);
        } else if(expected==ERI_QUEUED) {
            /* Reversed events disprove the caller's ordering contract. */
            p->fault_reason=ERIF_REVERSED;
            __atomic_store_n(&p->state,ERI_FAULT,__ATOMIC_RELEASE);
        }
    }
    SetLastError(saved);
}
void eye_interval_abandon(EyeRenderInterval *p,EyeGpuTicket ticket) {
    if(p && __atomic_load_n(&p->state,__ATOMIC_ACQUIRE)!=ERI_IDLE &&
       p->owner_thread==GetCurrentThreadId() && eri_same(ticket,p->ticket))
        __atomic_store_n(&p->abandoned,1,__ATOMIC_RELEASE);
}
int eye_interval_submitted(const EyeRenderInterval *p,EyeIntervalResult *out) {
    if(out)*out=(EyeIntervalResult){0};
    if(!p || !out)return 0;
    unsigned state=__atomic_load_n(&p->state,__ATOMIC_ACQUIRE);
    if((state!=ERI_PENDING && state!=ERI_DONE) ||
       __atomic_load_n(&p->abandoned,__ATOMIC_ACQUIRE) ||
       p->draw_mask!=3 || p->clear_mask!=3 ||
       !p->ticket.serial || !p->ticket.generation || !p->ticket.epoch)return 0;
    *out=(EyeIntervalResult){p->ticket,p->draw_mask,p->clear_mask,p->copied};
    return 1;
}
void eye_interval_poll_notify(EyeRenderInterval *p,void (*notify)(const EyeIntervalResult *)) {
    if(!p) return;
    unsigned state=__atomic_load_n(&p->state,__ATOMIC_ACQUIRE);
    if((state!=ERI_ACTIVE && state!=ERI_PENDING) || p->render_thread!=GetCurrentThreadId()) return;
    DWORD saved=GetLastError();
    if(state==ERI_ACTIVE && __atomic_load_n(&p->abandoned,__ATOMIC_ACQUIRE)) {
        __atomic_store_n(&p->state,ERI_RUNNING,__ATOMIC_RELEASE);
        (void)eri_end_watch(p);p->fault_reason=ERIF_ABANDONED;
        __atomic_store_n(&p->state,ERI_FAULT,__ATOMIC_RELEASE);
    } else if(state==ERI_PENDING) {
        unsigned expected=ERI_PENDING;
        if(!__atomic_compare_exchange_n(&p->state,&expected,ERI_RUNNING,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE)) {
            SetLastError(saved);return;
        }
        int gpu=eye_gpu_poll(&p->gpu);unsigned next=ERI_PENDING;
        if(gpu==EGC_DRAINED) {
#ifdef GF2VR_ROOT_INTERVAL_TIMING_OBSERVE
            rit_native(p,RIT_GPU_DRAIN,1);
#endif
#ifdef GF2VR_PERF_FENCE_TIMING
            p->timing=p->gpu.timing;
#endif
            int ok=eye_gpu_dispose(&p->gpu);
            if(!ok) p->fault_reason=ERIF_GPU_DISPOSE;
            if(ok && p->watch.state==ETW_ENDED) {
                ok=eye_target_watch_release(&p->watch,p->ticket,1);
                if(!ok) p->fault_reason=ERIF_WATCH_RELEASE;
            }
#ifdef GF2VR_ROOT_INTERVAL_TIMING_OBSERVE
            rit_native(p,RIT_NATIVE_CLEAN,ok);
#endif
            next=ok?ERI_DONE:ERI_FAULT;
            if(ok && notify) {
                EyeIntervalResult result={p->ticket,p->draw_mask,p->clear_mask,p->copied};
                notify(&result);
            }
        } else if(gpu!=EGC_PENDING) {p->fault_reason=ERIF_GPU_POLL;next=ERI_FAULT;}
        __atomic_store_n(&p->state,next,__ATOMIC_RELEASE);
    }
    SetLastError(saved);
}
#ifdef GF2VR_PERF_FENCE_DELIVERY
void eye_interval_fence_notify(EyeRenderInterval *p,void (*notify)(const EyeIntervalResult *)) {
    if(!p) return;
    unsigned expected=ERI_PENDING;
    if(!__atomic_compare_exchange_n(&p->state,&expected,ERI_RUNNING,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE)) return;
    DWORD saved=GetLastError();unsigned next=ERI_PENDING;
    /* PENDING release-publication proves tail/end/detach finished and makes
       the immutable payload accessible. This CAS also excludes normal poll. */
    if(p->attached || (p->watch.state!=ETW_ENDED && (p->watch.state!=ETW_IDLE || p->copied)) ||
       !eri_same(p->gpu.ticket,p->ticket)) {
        p->fault_reason=ERIF_GPU_POLL;next=ERI_FAULT;
    } else {
        int gpu=eye_gpu_poll_fenced(&p->gpu);
        if(gpu==EGC_DRAINED) {
#ifdef GF2VR_ROOT_INTERVAL_TIMING_OBSERVE
            rit_native(p,RIT_GPU_DRAIN,1);
#endif
#ifdef GF2VR_PERF_FENCE_TIMING
            p->timing=p->gpu.timing;
#endif
            int ok=eye_gpu_dispose_fenced(&p->gpu);
            if(!ok) p->fault_reason=ERIF_GPU_DISPOSE;
            if(ok && p->watch.state==ETW_ENDED) {
                ok=eye_target_watch_release_completed(&p->watch,p->ticket);
                if(!ok) p->fault_reason=ERIF_WATCH_RELEASE;
            }
#ifdef GF2VR_ROOT_INTERVAL_TIMING_OBSERVE
            rit_native(p,RIT_NATIVE_CLEAN,ok);
#endif
            next=ok?ERI_DONE:ERI_FAULT;
            if(ok && notify) {
                EyeIntervalResult result={p->ticket,p->draw_mask,p->clear_mask,p->copied};notify(&result);
            }
        } else if(gpu!=EGC_PENDING) {p->fault_reason=ERIF_GPU_POLL;next=ERI_FAULT;}
    }
    __atomic_store_n(&p->state,next,__ATOMIC_RELEASE);SetLastError(saved);
}
#endif
void eye_interval_poll(EyeRenderInterval *p) {eye_interval_poll_notify(p,0);}
void eye_interval_service_notify(EyeRenderInterval *p,void (*notify)(const EyeIntervalResult *)) {
    if(!p || GetCurrentThreadId()!=p->render_thread ||
       __atomic_load_n(&p->state,__ATOMIC_ACQUIRE)!=ERI_PENDING ||
       p->gpu.thread!=GetCurrentThreadId() || p->gpu.state!=EGC_PENDING ||
       !p->gpu.context || !p->gpu.device) return;
    DWORD saved=GetLastError();LARGE_INTEGER frequency,start,now;
    if(!QueryPerformanceFrequency(&frequency) || frequency.QuadPart<=0 ||
       !QueryPerformanceCounter(&start) ||
       FAILED(ID3D11Device_GetDeviceRemovedReason((ID3D11Device *)p->gpu.device))) {
        eye_interval_poll_notify(p,notify);SetLastError(saved);return;
    }
    /* Flush is submission, never completion evidence. GetData still owns the
       completion decision, watch disposal and publication on this thread. */
    ID3D11DeviceContext_Flush((ID3D11DeviceContext *)p->gpu.context);
    for(unsigned i=0;i<64;++i) {
        if(!QueryPerformanceCounter(&now) || now.QuadPart<start.QuadPart ||
           now.QuadPart-start.QuadPart>=frequency.QuadPart/500) break;
        eye_interval_poll_notify(p,notify);
        if(__atomic_load_n(&p->state,__ATOMIC_ACQUIRE)!=ERI_PENDING) break;
        Sleep(0); /* yield; no timer-based1ms sleep and no unbounded spin */
    }
    SetLastError(saved);
}
int eye_interval_take(EyeRenderInterval *p,EyeIntervalResult *out) {
    if(out) *out=(EyeIntervalResult){0};
    if(!p || !out || __atomic_load_n(&p->state,__ATOMIC_ACQUIRE)!=ERI_DONE ||
       p->owner_thread!=GetCurrentThreadId()) return 0;
    __atomic_store_n(&p->state,ERI_PREPARING,__ATOMIC_RELEASE);
    DWORD saved=GetLastError();*out=(EyeIntervalResult){p->ticket,p->draw_mask,p->clear_mask,p->copied};
    eri_reset(p);SetLastError(saved);return 1;
}
