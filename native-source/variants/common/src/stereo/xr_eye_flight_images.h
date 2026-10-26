#ifndef GF2_XR_EYE_FLIGHT_IMAGES_H
#define GF2_XR_EYE_FLIGHT_IMAGES_H
#include "eye_flight_owner.h"
#include "eye_flight_backend.h"
/* Included after xr_eye_output.h. Sole XR owner adapter; uses its real image
   API/handles and protected alternate bank, but NEVER its legacy frame/reclaim
   route while a flight is attached. Source and copy drain are separate.
   A BUSY acknowledgement retains actual API progress: never reacquire/release
   twice. This adapter does not call Begin/End, sample input or reset reference. */
typedef struct {
    EyeFlightToken token;
    unsigned thread,actual_released,fault,projected;
#ifdef GF2VR_ASYNC_PRESENTATION
    unsigned detached,pending_offer;
    XrSwapchain writing_chains[2];
    unsigned long long prepared_tick;
#endif
#ifdef GF2VR_TWO_FLIGHT_OBSERVE
    unsigned projection_reason;
#endif
} XrEyeFlightImages;
static int xfi_same(EyeFlightToken a,EyeFlightToken b) {
    return a.frame && a.frame==b.frame && a.generation==b.generation && a.epoch==b.epoch;
}
static int xfi_fault(XrEyeFlightImages *f,XrEyeOutput *o,EyeFlightOwner *owner) {
    f->fault=1;(void)xr_eye_output_fault(o);
    int r=eye_flight_fault(owner,f->token);return r==EFO_BUSY?r:EFO_REJECTED;
}
/* Reconcile API facts before performing another API call. Core lock ordering
   never encloses an OpenXR call; retry uses retained acquired/waited/released
   facts, even if the real Release reentered another source invalidation. */
static int xfi_sync(XrEyeFlightImages *f,XrEyeOutput *o,EyeFlightOwner *owner) {
    if(!f || !o || !owner || !f->token.frame || GetCurrentThreadId()!=f->thread)return EFO_REJECTED;
    if(f->fault)return xfi_fault(f,o,owner);
    if(o->owner_thread!=f->thread || o->frame!=f->token.frame || o->epoch!=f->token.epoch ||
       o->ticket.generation!=f->token.generation || o->ticket.epoch!=f->token.epoch)return xfi_fault(f,o,owner);
#ifdef GF2VR_ASYNC_PRESENTATION
    if(o->chains[0]!=f->writing_chains[0] || o->chains[1]!=f->writing_chains[1])return xfi_fault(f,o,owner);
#endif
    EyeFlightSnapshot s;int r=eye_flight_snapshot(owner,f->token,&s);
    if(r!=EFO_OK)return r;
#ifdef GF2VR_ASYNC_PRESENTATION
    if(s.flags&EFF_DISPLAY_DETACHED)f->detached=1;
    if(f->pending_offer && !o->ticket.serial && s.ticket.serial) {
        /* Admission can occur after bank reservation. Adopt its one immutable
           ticket only before any destination acquisition/claim, never rotate
           the protected bank or renew its original preparation clock. */
        if(!(s.flags&EFF_RESERVED) || o->acquired[0] || o->acquired[1] ||
           s.acquired_mask || f->actual_released ||
           __atomic_load_n(&o->state,__ATOMIC_ACQUIRE)!=XEO_IDLE ||
           s.ticket.generation!=f->token.generation || s.ticket.epoch!=f->token.epoch)
            return xfi_fault(f,o,owner);
        o->ticket=s.ticket;
    }
    if(f->pending_offer && o->ticket.serial!=s.ticket.serial)return xfi_fault(f,o,owner);
#endif
    for(unsigned i=0;i<2;++i) {
        unsigned bit=1u<<i;
        if((o->acquired[i] || (f->actual_released&bit)) && !(s.acquired_mask&bit)) {
            r=eye_flight_acquired(owner,f->token,bit);if(r!=EFO_OK)return r;
        }
        if((o->waited[i] || (f->actual_released&bit)) && !(s.waited_mask&bit)) {
            r=eye_flight_waited(owner,f->token,bit);if(r!=EFO_OK)return r;
        }
        if((f->actual_released&bit) && !(s.released_mask&bit)) {
            r=eye_flight_released(owner,f->token,bit);if(r!=EFO_OK)return r;
        }
    }
    return EFO_OK;
}
/* Reserve the exact writing bank BEFORE excluded-layer End, without acquiring
   an image or claiming GPU completion. Retry on the same ticket cannot rotate
   banks again. The captured handles stay fixed through late copy/disposition. */
static int xfi_prepare_mode(XrEyeFlightImages *f,XrEyeOutput *o,PanelXr *p,
                       EyeFlightOwner *owner,EyeFlightToken token,int pending) {
    if(!f || !o || !p || !owner || !o->enabled)return EFO_REJECTED;
    EyeFlightSnapshot s;int r=eye_flight_snapshot(owner,token,&s);if(r!=EFO_OK)return r;
    unsigned required=EFF_RENDER_ISSUED|EFF_CPU_RETURNED|EFF_XR_BEGUN;
    int cpu_ready=s.cpu_ok;
#ifdef GF2VR_PRODUCER_TRANSFER
    if((s.flags&EFF_PRIVATE_PRODUCER) && !(s.flags&EFF_CPU_RETURNED)) {
        required &= ~EFF_CPU_RETURNED;cpu_ready=1;
    }
#endif
#ifdef GF2VR_ASYNC_PRESENTATION
    if(pending) {
        required=EFF_XR_BEGUN;
        cpu_ready=!(s.flags&EFF_CPU_RETURNED) || s.cpu_ok;
        if(s.acquired_mask || s.waited_mask || s.released_mask ||
           s.flags&EFF_COPY_ISSUED)return EFO_REJECTED;
    }
#else
    if(pending)return EFO_REJECTED;
#endif
    unsigned ended=s.flags&EFF_XR_ENDED;
#ifdef GF2VR_ASYNC_PRESENTATION
    if(s.flags&EFF_DISPLAY_DETACHED && !(s.flags&EFF_PRESENTATION_RETIRED))ended=0;
#endif
    if((s.flags&required)!=required || !cpu_ready || !s.renderable || ended ||
       s.flags&(EFF_CANCELLED|EFF_FAULT|EFF_RECLAIMING))return EFO_REJECTED;
    if(!f->token.frame) {
#ifdef GF2VR_ASYNC_PRESENTATION
        /* Never choose a new writing bank/clock after its display already Ended.
           Unprepared detached production may only drain/discard, not publish. */
        if(s.flags&EFF_XR_ENDED)return EFO_REJECTED;
#endif
        if(__atomic_load_n(&o->state,__ATOMIC_ACQUIRE)!=XEO_IDLE || o->acquired[0] || o->acquired[1])return EFO_REJECTED;
        f->token=token;f->thread=GetCurrentThreadId();
        if(!o->session && !xr_eye_output_create(o,p))return xfi_fault(f,o,owner);
        if(o->owner_thread!=f->thread || o->session!=p->session)return xfi_fault(f,o,owner);
#ifdef GF2VR_MAIN_TRANSITION_HOLD
        if(!xr_eye_output_next_bank(o,p))return xfi_fault(f,o,owner);
#endif
        o->frame=token.frame;o->epoch=token.epoch;o->ticket=s.ticket;o->presentable=0;
#ifdef GF2VR_ASYNC_PRESENTATION
        f->pending_offer=pending!=0;
        if(pending && !s.ticket.serial)o->ticket=(EyeGpuTicket){0,token.generation,token.epoch};
        f->writing_chains[0]=o->chains[0];f->writing_chains[1]=o->chains[1];
        f->prepared_tick=GetTickCount64();
#endif
    }
    if(!xfi_same(f->token,token) || f->thread!=GetCurrentThreadId() ||
       o->session!=p->session || f->actual_released)return EFO_REJECTED;
    return xfi_sync(f,o,owner);
}
static int xfi_prepare(XrEyeFlightImages *f,XrEyeOutput *o,PanelXr *p,
                       EyeFlightOwner *owner,EyeFlightToken token) {
    return xfi_prepare_mode(f,o,p,owner,token,0);
}
#ifdef GF2VR_ASYNC_PRESENTATION
static inline int xfi_prepare_offer(XrEyeFlightImages *f,XrEyeOutput *o,PanelXr *p,
                       EyeFlightOwner *owner,EyeFlightToken token) {
    return xfi_prepare_mode(f,o,p,owner,token,1);
}
#endif
static int xfi_acquire(XrEyeFlightImages *f,XrEyeOutput *o,PanelXr *p,
                       EyeFlightOwner *owner,EyeFlightToken token) {
    if(!f || !o || !p || !owner || !o->enabled)return EFO_REJECTED;
    EyeFlightSnapshot s;int r=eye_flight_snapshot(owner,token,&s);
    if(r!=EFO_OK)return r;
    unsigned required=EFF_RENDER_ISSUED|EFF_CPU_RETURNED|EFF_XR_BEGUN;
#ifdef GF2VR_PRODUCER_TRANSFER
    int producer_ready=(s.flags&EFF_PRIVATE_PRODUCER) &&
        (!(s.flags&EFF_CPU_RETURNED) || s.cpu_ok);
    if(producer_ready)required &= ~EFF_CPU_RETURNED;
#endif
#ifdef GF2VR_TWO_FLIGHT_EARLY_COPY
#ifdef GF2VR_PRODUCER_TRANSFER
    if(!producer_ready && !(s.flags&(EFF_RENDER_SUBMITTED|EFF_RENDER_DRAINED)))return EFO_REJECTED;
#else
    if(!(s.flags&(EFF_RENDER_SUBMITTED|EFF_RENDER_DRAINED)))return EFO_REJECTED;
#endif
#else
    required|=EFF_RENDER_DRAINED;
#endif
    unsigned ended=s.flags&EFF_XR_ENDED;
#ifdef GF2VR_ASYNC_PRESENTATION
    if(s.flags&EFF_DISPLAY_DETACHED && !(s.flags&EFF_PRESENTATION_RETIRED))ended=0;
#endif
    if((s.flags&required)!=required ||
#ifdef GF2VR_PRODUCER_TRANSFER
       (!s.cpu_ok && !producer_ready) ||
#else
       !s.cpu_ok ||
#endif
       !s.renderable || ended ||
       s.flags&(EFF_CANCELLED|EFF_FAULT|EFF_RECLAIMING))return EFO_REJECTED;
    r=xfi_prepare(f,o,p,owner,token);if(r!=EFO_OK)return r;
    unsigned state=__atomic_load_n(&o->state,__ATOMIC_ACQUIRE);
    if(state==XEO_OFFERED || state==XEO_CLAIMED)return EFO_OK;
    if(state!=XEO_IDLE)return EFO_REJECTED;
    for(unsigned i=0;i<2;++i) {
        if(!o->acquired[i]) {
            XrSwapchainImageAcquireInfo info={.type=XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
            XrResult result=p->api.AcquireSwapchainImage(o->chains[i],&info,&o->indices[i]);
            if(XR_SUCCEEDED(result))o->acquired[i]=1;
            if(result!=XR_SUCCESS || o->indices[i]>=o->counts[i])return xfi_fault(f,o,owner);
            r=xfi_sync(f,o,owner);if(r!=EFO_OK)return r;
        }
        if(!o->waited[i]) {
            XrSwapchainImageWaitInfo info={.type=XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
            XrResult result=p->api.WaitSwapchainImage(o->chains[i],&info);
            if(result==XR_TIMEOUT_EXPIRED)return EFO_BUSY;
            if(result!=XR_SUCCESS)return xfi_fault(f,o,owner);
            o->waited[i]=1;
            r=xfi_sync(f,o,owner);if(r!=EFO_OK)return r;
        }
        o->textures[i]=o->images[i][o->indices[i]].texture;
        if(!o->textures[i])return xfi_fault(f,o,owner);
    }
    if(o->textures[0]==o->textures[1])return xfi_fault(f,o,owner);
    __atomic_store_n(&o->state,XEO_OFFERED,__ATOMIC_RELEASE);return EFO_OK;
}
/* Only real COPY_DRAINED authorizes completing a claimed destination grant.
   Backend copy posting remains exact-ticket, retryable and separately owned. */
static int xfi_complete(XrEyeFlightImages *f,XrEyeOutput *o,EyeFlightOwner *owner) {
    int r=xfi_sync(f,o,owner);if(r!=EFO_OK)return r;
    EyeFlightSnapshot s;r=eye_flight_snapshot(owner,f->token,&s);if(r!=EFO_OK)return r;
    if(!(s.flags&EFF_COPY_DRAINED))return EFO_REJECTED;
#ifdef GF2VR_PRODUCER_TRANSFER
    /* Neither early commands nor both GPU drains prove the outer CPU result. */
    if(!(s.flags&EFF_CPU_RETURNED))return EFO_BUSY;
    if(!s.cpu_ok)return EFO_REJECTED;
#endif
#ifdef GF2VR_TWO_FLIGHT_EARLY_COPY
    /* Copy's queue fence never substitutes for the independent render drain. */
    if(s.flags&(EFF_CANCELLED|EFF_FAULT|EFF_RECLAIMING))return EFO_REJECTED;
    if(!(s.flags&EFF_RENDER_DRAINED))return EFO_BUSY;
#endif
#ifdef GF2VR_LATENCY_OBSERVE
    latency_mark(f->token.epoch,f->token.frame,f->token.generation,3);
#endif
    unsigned state=__atomic_load_n(&o->state,__ATOMIC_ACQUIRE);
    if(state==XEO_DONE)return EFO_OK;
    if(state==XEO_IDLE && f->actual_released==3)return EFO_OK; /* retained projection retry */
    if(state!=XEO_CLAIMED)return EFO_REJECTED;
    return xr_eye_output_finish(o,s.ticket,!(s.flags&(EFF_CANCELLED|EFF_FAULT)))?EFO_OK:EFO_REJECTED;
}
/* XR-driven deferred-copy handshake. Claim after BOTH real waits, retain the
   claim on backend contention, and post exact destinations once. Native issue
   still happens on the verified main/render queue, never on this XR thread. */
static int xfi_copy_tick(XrEyeFlightImages *f,XrEyeOutput *o,PanelXr *p,
                         EyeFlightOwner *owner,EyeFlightBackend *backend,EyeFlightToken token) {
    EyeFlightSnapshot s;int r=eye_flight_snapshot(owner,token,&s);if(r!=EFO_OK)return r;
    if(s.flags&EFF_COPY_ISSUED) {
        if(!xfi_same(f->token,token))return EFO_REJECTED;
        return s.flags&EFF_COPY_DRAINED?xfi_complete(f,o,owner):EFO_BUSY;
    }
#ifdef GF2VR_PRODUCER_TRANSFER
    /* Sole XR owner can perform both real waits while the private render tail
       is in flight. No command copy until actual draw/clear submission below. */
    if(s.flags&EFF_PRIVATE_PRODUCER) {
        r=xfi_acquire(f,o,p,owner,token);if(r!=EFO_OK)return r;
    }
#endif
#ifdef GF2VR_XR_SUBMISSION_ACK
    if(!(s.flags&(EFF_RENDER_SUBMITTED|EFF_RENDER_DRAINED))) {
        /* Only the owner of this already prepared bank can shorten the main
           acknowledgment dependency. Existing acquire validates all numeric
           CPU/current/cancellation/image guards again after publication. */
        if(!f || !o || !p || !xfi_same(f->token,token) ||
           f->thread!=GetCurrentThreadId())return EFO_REJECTED;
        r=eye_flight_backend_publish_submission(backend,owner,token);
        if(r!=EFO_OK)return r;
    }
#endif
    r=xfi_acquire(f,o,p,owner,token);if(r!=EFO_OK)return r;
    unsigned state=__atomic_load_n(&o->state,__ATOMIC_ACQUIRE);
    if(state==XEO_OFFERED) {
        void *destinations[2];
        if(!xr_eye_output_claim(o,token.frame,s.ticket,destinations))return EFO_REJECTED;
    } else if(state!=XEO_CLAIMED)return EFO_REJECTED;
    r=eye_flight_backend_offer_copy(backend,owner,token,o->textures[0],o->textures[1]);
    return r==EFO_OK?EFO_BUSY:r; /* posted != completed */
}
/* Also drains unused partial acquisitions after cancellation. A timeout is
   retry, not evidence of Wait/Release success. Outstanding copy rejects release. */
static int xfi_release(XrEyeFlightImages *f,XrEyeOutput *o,PanelXr *p,EyeFlightOwner *owner) {
    if(!p || !f || !o || f->thread!=GetCurrentThreadId() || p->session!=o->session)return EFO_REJECTED;
    int r=xfi_sync(f,o,owner);if(r!=EFO_OK)return r;
    for(unsigned i=0;i<2;++i)if(o->acquired[i]) {
        unsigned bit=1u<<i;
        if(!o->waited[i]) {
            XrSwapchainImageWaitInfo info={.type=XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
            XrResult result=p->api.WaitSwapchainImage(o->chains[i],&info);
            if(result==XR_TIMEOUT_EXPIRED)return EFO_BUSY;
            if(result!=XR_SUCCESS)return xfi_fault(f,o,owner);
            o->waited[i]=1;r=xfi_sync(f,o,owner);if(r!=EFO_OK)return r;
        }
        r=eye_flight_can_release(owner,f->token,bit);if(r!=EFO_OK)return r;
        XrSwapchainImageReleaseInfo info={.type=XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
        if(p->api.ReleaseSwapchainImage(o->chains[i],&info)!=XR_SUCCESS)return xfi_fault(f,o,owner);
        f->actual_released|=bit;o->acquired[i]=o->waited[i]=0;
        r=xfi_sync(f,o,owner);if(r!=EFO_OK)return r;
    }
    o->textures[0]=o->textures[1]=0;
    __atomic_store_n(&o->state,XEO_IDLE,__ATOMIC_RELEASE);return EFO_OK;
}
static int xfi_projection_mode(XrEyeFlightImages *f,XrEyeOutput *o,PanelXr *p,EyeFlightOwner *owner,
        const XrEyeInput *input,XrSpace space,const XrFrameEndInfo *end,
        const unsigned long long *generation,XrCompositionLayerProjection *layer,
        XrCompositionLayerProjectionView views[2],int detached) {
    *layer=(XrCompositionLayerProjection){0};views[0]=views[1]=(XrCompositionLayerProjectionView){0};
#ifdef GF2VR_TWO_FLIGHT_OBSERVE
    if(f)f->projection_reason=1;
#endif
    if(!f || f->projected || !space || !end || end->type!=XR_TYPE_FRAME_END_INFO || !generation)return EFO_REJECTED;
#ifdef GF2VR_ASYNC_PRESENTATION
    if(detached) {
        EyeFlightSnapshot original;int check=eye_flight_snapshot(owner,f->token,&original);
        if(check!=EFO_OK)return check;
        if(!(original.flags&EFF_DISPLAY_DETACHED) || original.flags&EFF_PRESENTATION_RETIRED ||
           end->displayTime<=original.display_time ||
           (unsigned long long)end->displayTime-(unsigned long long)original.display_time>=250000000ull ||
           !xr_eye_matches(input,f->token.epoch,f->token.frame,original.display_time))return EFO_REJECTED;
    } else
#else
    (void)detached;
#endif
    if(!xr_eye_matches(input,f->token.epoch,f->token.frame,end->displayTime))return EFO_REJECTED;
#ifdef GF2VR_TWO_FLIGHT_OBSERVE
    f->projection_reason=2;
#endif
    int r=xfi_complete(f,o,owner);if(r!=EFO_OK)return r;
    int visible=o->enabled && o->presentable &&
        __atomic_load_n(generation,__ATOMIC_ACQUIRE)==f->token.generation;
#ifdef GF2VR_TWO_FLIGHT_OBSERVE
    f->projection_reason=3;
#endif
    r=xfi_release(f,o,p,owner);if(r!=EFO_OK)return r;
    if(!visible || __atomic_load_n(generation,__ATOMIC_ACQUIRE)!=f->token.generation)
        {
#ifdef GF2VR_TWO_FLIGHT_OBSERVE
            f->projection_reason=4;
#endif
            f->projected=1;return EFO_REJECTED;} /* drained output cannot be revived */
#ifdef GF2VR_TWO_FLIGHT_OBSERVE
    f->projection_reason=5;
#endif
#ifdef GF2VR_ASYNC_PRESENTATION
    if(detached)r=eye_flight_can_detached_dispose(owner,f->token,1);
    else
#endif
        r=eye_flight_can_end(owner,f->token,end->displayTime,1);
    if(r!=EFO_OK){if(r!=EFO_BUSY)f->projected=1;return r;}
    for(unsigned i=0;i<2;++i) {
        views[i].type=XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
        views[i].pose=input->poses[i];views[i].fov=input->fov[i];
        views[i].subImage.swapchain=o->chains[i];
        views[i].subImage.imageRect.extent.width=(int)o->width;
        views[i].subImage.imageRect.extent.height=(int)o->height;
    }
    layer->type=XR_TYPE_COMPOSITION_LAYER_PROJECTION;layer->space=space;
    layer->viewCount=2;layer->views=views;f->projected=1;
#ifdef GF2VR_TWO_FLIGHT_OBSERVE
    f->projection_reason=0;
#endif
    return EFO_OK;
}
static int xfi_projection(XrEyeFlightImages *f,XrEyeOutput *o,PanelXr *p,EyeFlightOwner *owner,
        const XrEyeInput *input,XrSpace space,const XrFrameEndInfo *end,
        const unsigned long long *generation,XrCompositionLayerProjection *layer,
        XrCompositionLayerProjectionView views[2]) {
    return xfi_projection_mode(f,o,p,owner,input,space,end,generation,layer,views,0);
}
#ifdef GF2VR_ASYNC_PRESENTATION
/* Only builds a completed/released pair with its ORIGINAL capture pose. The
   caller must retain it through actual later End and then disposition; this
   constructor alone never permits take/reuse or renews capture freshness. */
static inline int xfi_detached_projection(XrEyeFlightImages *f,XrEyeOutput *o,PanelXr *p,EyeFlightOwner *owner,
        const XrEyeInput *input,XrSpace space,const XrFrameEndInfo *end,
        const unsigned long long *generation,XrCompositionLayerProjection *layer,
        XrCompositionLayerProjectionView views[2]) {
    return xfi_projection_mode(f,o,p,owner,input,space,end,generation,layer,views,1);
}
#endif
/* Sole frame driver calls only after REAL EndFrame OR actual EndSession and
   the corresponding numeric terminal ack (never a stop request alone).
   Numeric slot may already have been retired by main; local image proof remains.
   Never use this to clear the numeric pose/native/source owner. */
static int xfi_retire_mode(XrEyeFlightImages *f,XrEyeOutput *o,int real_end_and_numeric_ack) {
    if(!f || !o || !real_end_and_numeric_ack || f->fault || !f->token.frame ||
       f->thread!=GetCurrentThreadId() || o->acquired[0] || o->acquired[1] ||
       __atomic_load_n(&o->state,__ATOMIC_ACQUIRE)!=XEO_IDLE)return EFO_REJECTED;
    o->frame=o->epoch=0;o->presentable=0;o->ticket=(EyeGpuTicket){0};
    *f=(XrEyeFlightImages){0};return EFO_OK;
}
static int xfi_retire(XrEyeFlightImages *f,XrEyeOutput *o,int real_end_and_numeric_ack) {
#ifdef GF2VR_ASYNC_PRESENTATION
    if(f && f->detached)return EFO_REJECTED;
#endif
    return xfi_retire_mode(f,o,real_end_and_numeric_ack);
}
#ifdef GF2VR_ASYNC_PRESENTATION
static inline int xfi_detached_retire(XrEyeFlightImages *f,XrEyeOutput *o,EyeFlightOwner *owner) {
    if(!f || !f->detached)return EFO_REJECTED;
    EyeFlightSnapshot s;int r=eye_flight_snapshot(owner,f->token,&s);if(r!=EFO_OK)return r;
    if((s.flags&(EFF_DISPLAY_DETACHED|EFF_PRESENTATION_RETIRED))!=
                (EFF_DISPLAY_DETACHED|EFF_PRESENTATION_RETIRED))return EFO_REJECTED;
    return xfi_retire_mode(f,o,1);
}
/* Finalize immediately after real later End (or discard after the original
   End). Prevalidate XR-local cleanup BEFORE opening numeric main-thread reuse.
   No second snapshot after disposition: main may already have taken/retired
   that numeric slot. BUSY keeps all local proof and repeats no SDK call. */
static inline int xfi_detached_finalize(XrEyeFlightImages *f,XrEyeOutput *o,EyeFlightOwner *owner,
                                       int published,int actual_later_end_success) {
    if(!f || !o || !f->detached || f->fault || !f->token.frame ||
       f->thread!=GetCurrentThreadId() || o->owner_thread!=f->thread ||
       o->frame!=f->token.frame || o->epoch!=f->token.epoch ||
       o->chains[0]!=f->writing_chains[0] || o->chains[1]!=f->writing_chains[1] ||
       o->acquired[0] || o->acquired[1] || __atomic_load_n(&o->state,__ATOMIC_ACQUIRE)!=XEO_IDLE ||
       (published && (!actual_later_end_success || !f->projected)))return EFO_REJECTED;
    int r=eye_flight_detached_dispose(owner,f->token,published);if(r!=EFO_OK)return r;
    return xfi_retire_mode(f,o,1); /* XR-local, no reentrant SDK or numeric read */
}
#endif
#endif
