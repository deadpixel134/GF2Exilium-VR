#ifndef GF2_XR_EYE_OUTPUT_H
#define GF2_XR_EYE_OUTPUT_H
#include "eye_gpu_copy.h"
#ifdef GF2VR_PERF_PRODUCER_OBSERVE
#include "producer_timing.h"
#endif
/* Included after panel_xr.h: reuse the sole XR worker/API, never its panel
   swapchain. Config is fixed before enabling.
   XR owns handles; the atomic grant retains both acquired/waited images while
   Unity/render/GPU uses them. DONE means no further destination use, not pixels
   suitable for presentation. Fault retains the session and all handles. */
enum { XEO_IDLE,XEO_OFFERED,XEO_CLAIMED,XEO_DONE,XEO_FAULT };
typedef struct {
    unsigned enabled,width,height,format,owner_thread,state;
    XrSession session;
    XrSwapchain chains[2];
    XrSwapchainImageD3D11KHR images[2][16];
    unsigned counts[2],indices[2],acquired[2],waited[2];
    unsigned long long frame,epoch;
    void *textures[2];
    EyeGpuTicket ticket;
    unsigned presentable; /* Published with DONE, never inferred from cancellation. */
#ifdef GF2VR_PERF_PRODUCER_OBSERVE
    /* Same payload ownership as frame/ticket: offer -> claim -> DONE. */
    unsigned long long trace_offer,trace_claim,trace_ready;
#endif
#ifdef GF2VR_MAIN_TRANSITION_HOLD
    /* XR-owner-only alternate bank. Unity accesses only the claimed active
       bank. Never acquire/write a bank retained for transition presentation. */
    unsigned hold_enabled,bank_protected;
    XrSwapchain spare_chains[2];
    XrSwapchainImageD3D11KHR spare_images[2][16];
    unsigned spare_counts[2];
#endif
} XrEyeOutput;
/* Sole XR owner, before first enable/acquisition. The description must come
   from the current prepared native pair; this function does not infer Unity's
   default format, resize live chains, or enable presentation. Caller transfers
   only this numeric value across threads under its configuration handshake. */
static int xr_eye_output_configure(XrEyeOutput *o,const EyeGpuDescription *d) {
    if(!o || !d || o->enabled || o->session || o->chains[0] || o->chains[1] ||
       __atomic_load_n(&o->state,__ATOMIC_ACQUIRE)!=XEO_IDLE ||
       o->acquired[0] || o->acquired[1] || o->waited[0] || o->waited[1] ||
       !d->width || !d->height || d->width>16384 || d->height>16384 ||
       (d->format!=28 && d->format!=29 && d->format!=87 && d->format!=91 && d->format!=10)) return 0;
    o->width=d->width;o->height=d->height;o->format=d->format;return 1;
}
static int xr_eye_output_fault(XrEyeOutput *o) {
    __atomic_store_n(&o->state,XEO_FAULT,__ATOMIC_RELEASE);return 0;
}
static int xr_eye_output_release(XrEyeOutput *o,PanelXr *p) {
    for(unsigned i=0;i<2;++i) if(o->acquired[i]) {
        if(!o->waited[i]) return 0;
        XrSwapchainImageReleaseInfo info={0};info.type=XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO;
        if(p->api.ReleaseSwapchainImage(o->chains[i],&info)!=XR_SUCCESS) return xr_eye_output_fault(o);
        o->acquired[i]=o->waited[i]=0;
    }
    o->textures[0]=o->textures[1]=0;o->frame=o->epoch=0;
    __atomic_store_n(&o->state,XEO_IDLE,__ATOMIC_RELEASE);return 1;
}
/* XR thread only, also called before EndSession/DestroySession. Cancelling an
   unclaimed offer races safely with Unity's CAS; a claimed grant never cancels.
   A timed-out acquisition is waited again, never released as if wait succeeded. */
static int xr_eye_output_reclaim(XrEyeOutput *o,PanelXr *p) {
    if(!o->session) return 1;
    if(GetCurrentThreadId()!=o->owner_thread || p->session!=o->session) return 0;
    unsigned expected=XEO_OFFERED;
    __atomic_compare_exchange_n(&o->state,&expected,XEO_DONE,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE);
    unsigned state=__atomic_load_n(&o->state,__ATOMIC_ACQUIRE);
    if(state==XEO_CLAIMED || state==XEO_FAULT) return 0;
    for(unsigned i=0;i<2;++i) if(o->acquired[i] && !o->waited[i]) {
        XrSwapchainImageWaitInfo wait={0};wait.type=XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO;
        XrResult result=p->api.WaitSwapchainImage(o->chains[i],&wait);
        if(result==XR_TIMEOUT_EXPIRED) return 0;
        if(result!=XR_SUCCESS) return xr_eye_output_fault(o);
        o->waited[i]=1;
    }
    return xr_eye_output_release(o,p);
}
static int xr_eye_output_create(XrEyeOutput *o,PanelXr *p) {
    if(!o->enabled || !o->width || !o->height || o->width>16384 || o->height>16384 ||
       !o->format || !p->session || o->session) return 0;
    o->session=p->session;o->owner_thread=GetCurrentThreadId();
    int64_t formats[64];uint32_t count=0;unsigned found=0;
    if(p->api.EnumerateSwapchainFormats(p->session,64,&count,formats)!=XR_SUCCESS || count>64)
        return xr_eye_output_fault(o);
    for(unsigned i=0;i<count;++i) if(formats[i]==o->format) found=1;
    if(!found) return xr_eye_output_fault(o);
    for(unsigned i=0;i<2;++i) {
        XrSwapchainCreateInfo info={0};info.type=XR_TYPE_SWAPCHAIN_CREATE_INFO;
        info.usageFlags=XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT|XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
        info.format=o->format;info.width=o->width;info.height=o->height;
        info.sampleCount=info.faceCount=info.arraySize=info.mipCount=1;
        XrResult result=p->api.CreateSwapchain(p->session,&info,&o->chains[i]);
        if(result!=XR_SUCCESS || !o->chains[i]) return xr_eye_output_fault(o);
        for(unsigned j=0;j<16;++j) o->images[i][j].type=XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR;
        if(p->api.EnumerateSwapchainImages(o->chains[i],16,&o->counts[i],
            (XrSwapchainImageBaseHeader *)o->images[i])!=XR_SUCCESS || !o->counts[i] || o->counts[i]>16)
            return xr_eye_output_fault(o);
    }
    if(o->chains[0]==o->chains[1]) return xr_eye_output_fault(o);
    return 1;
}
#ifdef GF2VR_MAIN_TRANSITION_HOLD
static __attribute__((noinline)) int xr_eye_output_next_bank(XrEyeOutput *o,PanelXr *p) {
    if(!o->hold_enabled || !o->bank_protected) return 1;
    if(__atomic_load_n(&o->state,__ATOMIC_ACQUIRE)!=XEO_IDLE ||
       o->acquired[0] || o->acquired[1]) return 0;
    if(!o->spare_chains[0] && !o->spare_chains[1]) {
        XrEyeOutput spare={0};spare.enabled=1;
        spare.width=o->width;spare.height=o->height;spare.format=o->format;
        int ok=xr_eye_output_create(&spare,p);
        for(unsigned i=0;i<2;++i) {
            o->spare_chains[i]=spare.chains[i];o->spare_counts[i]=spare.counts[i];
            for(unsigned j=0;j<16;++j)o->spare_images[i][j]=spare.images[i][j];
        }
        if(!ok)return xr_eye_output_fault(o); /* keep partial handles for close */
    }
    for(unsigned i=0;i<2;++i) {
        XrSwapchain chain=o->chains[i];o->chains[i]=o->spare_chains[i];o->spare_chains[i]=chain;
        unsigned count=o->counts[i];o->counts[i]=o->spare_counts[i];o->spare_counts[i]=count;
        for(unsigned j=0;j<16;++j) {
            XrSwapchainImageD3D11KHR image=o->images[i][j];
            o->images[i][j]=o->spare_images[i][j];o->spare_images[i][j]=image;
        }
    }
    o->bank_protected=0;return 1;
}
#endif
static void xr_eye_output_frame(XrEyeOutput *o,PanelXr *p,unsigned long long frame,
                                unsigned long long epoch) {
    if(!o->enabled) return;
    if(!frame || !epoch) {(void)xr_eye_output_reclaim(o,p);return;}
    if(!o->session && !xr_eye_output_create(o,p)) return;
    if(!xr_eye_output_reclaim(o,p) || !frame || !epoch) return;
#ifdef GF2VR_MAIN_TRANSITION_HOLD
    if(!xr_eye_output_next_bank(o,p))return;
#endif
    for(unsigned i=0;i<2;++i) {
        XrSwapchainImageAcquireInfo acquire={0};acquire.type=XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO;
        XrResult result=p->api.AcquireSwapchainImage(o->chains[i],&acquire,&o->indices[i]);
        if(XR_SUCCEEDED(result)) o->acquired[i]=1;
        if(result!=XR_SUCCESS || o->indices[i]>=o->counts[i]) {xr_eye_output_fault(o);return;}
        XrSwapchainImageWaitInfo wait={0};wait.type=XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO;
        result=p->api.WaitSwapchainImage(o->chains[i],&wait);
        if(result==XR_TIMEOUT_EXPIRED) return;
        if(result!=XR_SUCCESS) {xr_eye_output_fault(o);return;}
        o->waited[i]=1;o->textures[i]=o->images[i][o->indices[i]].texture;
        if(!o->textures[i]) {xr_eye_output_fault(o);return;}
    }
    if(o->textures[0]==o->textures[1]) {xr_eye_output_fault(o);return;}
    o->frame=frame;o->epoch=epoch;
    o->presentable=0;
#ifdef GF2VR_PERF_PRODUCER_OBSERVE
    o->trace_offer=producer_clock();o->trace_claim=o->trace_ready=0;
#endif
    __atomic_store_n(&o->state,XEO_OFFERED,__ATOMIC_RELEASE);
}
/* Legacy Unity owner, or the isolated two-flight sole-XR destination adapter.
   CAS grants exclusive payload access before checking it,
   avoiding an ABA read of an offer already reclaimed/replaced by XR. */
static int xr_eye_output_claim(XrEyeOutput *o,unsigned long long frame,EyeGpuTicket ticket,void **out) {
    out[0]=out[1]=0;
    if(!frame || !ticket.serial || !ticket.generation || !ticket.epoch) return 0;
    unsigned expected=XEO_OFFERED;
    if(!__atomic_compare_exchange_n(&o->state,&expected,XEO_CLAIMED,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE)) {
        GF2VR_PREPARE_STEP(1200+expected);return 0;
    }
    if(o->frame!=frame || o->epoch!=ticket.epoch) {
        GF2VR_PREPARE_STEP(o->epoch!=ticket.epoch?1211:1210);
        __atomic_store_n(&o->state,XEO_DONE,__ATOMIC_RELEASE);return 0;
    }
    GF2VR_PREPARE_STEP(1212);
    o->ticket=ticket;out[0]=o->textures[0];out[1]=o->textures[1];
#ifdef GF2VR_PERF_PRODUCER_OBSERVE
    o->trace_claim=producer_clock();
#endif
    return 1;
}
/* Exclusive completion owner (render, or fence-backed XR/main handoff).
   Also valid for main-owner pre-submit cancellation when no GPU/event
   has referenced these destinations. Never use it as source-eye drain evidence. */
static int xr_eye_output_finish(XrEyeOutput *o,EyeGpuTicket ticket,int presentable) {
    if(__atomic_load_n(&o->state,__ATOMIC_ACQUIRE)!=XEO_CLAIMED ||
       o->ticket.serial!=ticket.serial || o->ticket.generation!=ticket.generation ||
       o->ticket.epoch!=ticket.epoch) return 0;
    o->presentable=presentable!=0;
#ifdef GF2VR_PERF_PRODUCER_OBSERVE
    o->trace_ready=producer_clock();
#endif
    __atomic_store_n(&o->state,XEO_DONE,__ATOMIC_RELEASE);return 1;
}
static int xr_eye_output_complete(XrEyeOutput *o,EyeGpuTicket ticket) {
    return xr_eye_output_finish(o,ticket,0);
}
/* XR owner, immediately before its one EndFrame. Storage supplied by caller
   survives that call. Late/stale completion is drained without a layer. No
   reacquisition or re-use of a last-good image on another prediction.
   source_generation is an atomic invalidation contract supplied by the source
   owner; zero disables publication. Root return must never populate it. */
static int xr_eye_output_projection(XrEyeOutput *o,PanelXr *p,const XrEyeInput *input,
        XrSpace space,const XrFrameEndInfo *end,const unsigned long long *source_generation,
        XrCompositionLayerProjection *layer,XrCompositionLayerProjectionView views[2]) {
    *layer=(XrCompositionLayerProjection){0};
    views[0]=views[1]=(XrCompositionLayerProjectionView){0};
    if(!o->session || GetCurrentThreadId()!=o->owner_thread || p->session!=o->session ||
       __atomic_load_n(&o->state,__ATOMIC_ACQUIRE)!=XEO_DONE) return 0;
    int visible=o->enabled && o->presentable && space && end &&
        end->type==XR_TYPE_FRAME_END_INFO && source_generation && input &&
        xr_eye_matches(input,o->epoch,o->frame,end->displayTime) &&
        o->ticket.generation && __atomic_load_n(source_generation,__ATOMIC_ACQUIRE)==o->ticket.generation &&
        o->ticket.epoch==o->epoch && o->acquired[0] && o->acquired[1] && o->waited[0] && o->waited[1];
    EyeGpuTicket ticket=o->ticket;
    if(!xr_eye_output_release(o,p) || !visible) return 0;
    /* Runtime release calls may overlap source invalidation. Check again at
       publication; source owner must publish zero before retiring that source. */
    if(__atomic_load_n(source_generation,__ATOMIC_ACQUIRE)!=ticket.generation) return 0;
    for(unsigned i=0;i<2;++i) {
        views[i].type=XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
        views[i].pose=input->poses[i];views[i].fov=input->fov[i];
        views[i].subImage.swapchain=o->chains[i];
        views[i].subImage.imageRect.extent.width=(int)o->width;
        views[i].subImage.imageRect.extent.height=(int)o->height;
    }
    layer->type=XR_TYPE_COMPOSITION_LAYER_PROJECTION;layer->space=space;
    layer->viewCount=2;layer->views=views;return 1;
}
static int xr_eye_output_close(XrEyeOutput *o,PanelXr *p) {
    if(!xr_eye_output_reclaim(o,p)) return 0;
    for(unsigned i=0;i<2;++i) if(o->chains[i]) {
        if(p->api.DestroySwapchain(o->chains[i])!=XR_SUCCESS) return xr_eye_output_fault(o);
        o->chains[i]=XR_NULL_HANDLE;
    }
#ifdef GF2VR_MAIN_TRANSITION_HOLD
    for(unsigned i=0;i<2;++i) if(o->spare_chains[i]) {
        if(p->api.DestroySwapchain(o->spare_chains[i])!=XR_SUCCESS)return xr_eye_output_fault(o);
        o->spare_chains[i]=XR_NULL_HANDLE;
    }
    o->bank_protected=0;
#endif
    o->session=XR_NULL_HANDLE;return 1;
}
#endif
