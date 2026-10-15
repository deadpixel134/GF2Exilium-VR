#ifndef GF2_TRANSITION_HOLD_H
#define GF2_TRANSITION_HOLD_H
/* XR-owner-only image presentation state, not a live Unity generation. */
typedef struct {
    unsigned valid;
    U64 generation,epoch,tick;
    U64 transition_from,transition_tick;
    XrSession session;
    XrSpace space;
    XrCompositionLayerProjectionView views[2];
} TransitionHold;
static TransitionHold stereo_hold;
/* XR owns the presentation clock. First observation of the validated source
   token starts it once; continued held frames never renew it. */
static int transition_hold_wait(TransitionHold *h,U64 from,U64 tick) {
#ifdef GF2VR_HOLD_300
    if(!h->valid||!from||from!=h->generation||tick<h->tick)return 0;
    if(h->transition_from!=from) {
        if(tick-h->tick>=250)return 0; /* stale cache cannot begin a session */
        h->transition_from=from;h->transition_tick=tick;
    }
    return tick>=h->transition_tick&&tick-h->transition_tick<300;
#else
    (void)h;(void)from;(void)tick;return 0;
#endif
}
static void transition_hold_capture(TransitionHold *h,XrEyeOutput *o,
        const XrEyeInput *input,const XrCompositionLayerProjection *layer,U64 tick) {
    h->valid=1;h->generation=o->ticket.generation;h->epoch=input->reference_epoch;
    h->tick=tick;h->session=o->session;h->space=layer->space;
    h->transition_from=h->transition_tick=0;
    h->views[0]=layer->views[0];h->views[1]=layer->views[1];
    o->bank_protected=1;
}
static int transition_hold_layer(TransitionHold *h,const XrEyeOutput *o,PanelXr *p,
        const XrEyeInput *input,XrSpace space,const XrFrameEndInfo *end,U64 from,U64 tick,
        XrCompositionLayerProjection *layer,XrCompositionLayerProjectionView views[2]) {
    int expired=tick<h->tick || tick-h->tick>=250;
#ifdef GF2VR_HOLD_300
    (void)transition_hold_wait(h,from,tick);
    /*300ms minimum switch delay plus the existing250ms readiness grace.
      No fresh replacement by550ms -> ordinary safe fallback, never forever. */
    if(h->transition_from==from&&from)
        expired=tick<h->transition_tick||tick-h->transition_tick>=550;
#endif
    if(!h->valid || !from || from!=h->generation || expired ||
       !o->hold_enabled || !o->enabled || o->session!=p->session || h->session!=p->session ||
       !space || space!=h->space || !input || !input->valid ||
       !xr_eye_matches(input,h->epoch,input->frame_id,end->displayTime) ||
       __atomic_load_n(&o->state,__ATOMIC_ACQUIRE)==XEO_FAULT) return 0;
    for(unsigned i=0;i<2;++i) {
        /* Only the inactive bank is safe: current bank may already be claimed
           or have a newer nonpresentable release. No image-index guessing. */
        if(!o->spare_chains[i] || h->views[i].subImage.swapchain!=o->spare_chains[i] ||
           o->spare_chains[i]==o->chains[0] || o->spare_chains[i]==o->chains[1])return 0;
    }
    views[0]=h->views[0];views[1]=h->views[1];
    *layer=(XrCompositionLayerProjection){0};layer->type=XR_TYPE_COMPOSITION_LAYER_PROJECTION;
    layer->space=h->space;layer->viewCount=2;layer->views=views;
    return 1;
}
#endif
