#ifndef GF2_COMPLETED_PRESENTATION_H
#define GF2_COMPLETED_PRESENTATION_H
/* Sole XR-thread presentation owner. Copies immutable completed metadata only,
   not Unity objects or current-flight leases. Capture is called AFTER successful
   fresh End, real GPU drains and image release. Replays never become production
   completions or refresh the deadline. The producer writes the other bank. */
typedef struct {
    unsigned valid;
    U64 generation,epoch,frame,serial,tick,cancel_clock;
    XrSession session;
    XrSpace space;
    XrCompositionLayerProjectionView views[2];
} CompletedPresentation;
static CompletedPresentation stereo_completed;
static void completed_presentation_cancel(CompletedPresentation *c) {
    c->valid=0;
}
static int completed_presentation_capture(CompletedPresentation *c,XrEyeOutput *o,
        const XrEyeInput *input,const XrCompositionLayerProjection *layer,U64 generation,U64 tick) {
    if(!c || !o || !input || !input->valid || !generation || !tick ||
       !o->enabled || !o->hold_enabled || !o->session || o->owner_thread!=GetCurrentThreadId() || !layer || !layer->space ||
       layer->type!=XR_TYPE_COMPOSITION_LAYER_PROJECTION || layer->viewCount!=2 || !layer->views ||
       !input->frame_id || !input->reference_epoch || !o->ticket.serial ||
       o->ticket.generation!=generation || o->ticket.epoch!=input->reference_epoch ||
       __atomic_load_n(&o->state,__ATOMIC_ACQUIRE)!=XEO_IDLE ||
       o->acquired[0] || o->acquired[1])return 0;
    /* A duplicate fresh notification cannot renew the same image's age. */
    if(c->epoch==input->reference_epoch && c->frame>=input->frame_id)return 0;
    for(unsigned i=0;i<2;++i)
        if(!o->chains[i] || layer->views[i].subImage.swapchain!=o->chains[i] ||
           !xr_eye_pose_valid(&layer->views[i].pose))return 0;
    if(o->chains[0]==o->chains[1])return 0;
    CompletedPresentation next={.valid=1,.generation=generation,.epoch=input->reference_epoch,
        .frame=input->frame_id,.serial=o->ticket.serial,.tick=tick,
        .cancel_clock=__atomic_load_n(&stereo_completed_cancel_clock,__ATOMIC_ACQUIRE),
        .session=o->session,.space=layer->space};
    next.views[0]=layer->views[0];next.views[1]=layer->views[1];
    *c=next;o->bank_protected=1;return 1;
}
static int completed_presentation_layer(CompletedPresentation *c,const XrEyeOutput *o,PanelXr *p,
        const XrEyeInput *input,XrSpace space,const XrFrameEndInfo *end,
        const U64 *publication,U64 tick,XrCompositionLayerProjection *layer,
        XrCompositionLayerProjectionView views[2]) {
    if(!c || !c->valid || !o || !p || !input || !end || !publication || !layer || !views)return 0;
    U64 generation=__atomic_load_n(publication,__ATOMIC_ACQUIRE);
    if(!generation || generation!=c->generation || !o->enabled || !o->hold_enabled ||
       c->cancel_clock!=__atomic_load_n(&stereo_completed_cancel_clock,__ATOMIC_ACQUIRE) ||
       c->session!=p->session || o->session!=p->session || o->owner_thread!=GetCurrentThreadId() || !space || space!=c->space ||
       !xr_eye_matches(input,c->epoch,input->frame_id,end->displayTime) ||
       tick<c->tick || tick-c->tick>=250 ||
       __atomic_load_n(&o->state,__ATOMIC_ACQUIRE)==XEO_FAULT) {
        completed_presentation_cancel(c);return 0;
    }
    for(unsigned i=0;i<2;++i) {
        /* Never show the active bank: it can contain unfinished or unsuccessfully
           released replacement pixels. The retained pair is independently safe. */
        if(!o->spare_chains[i] || c->views[i].subImage.swapchain!=o->spare_chains[i] ||
           o->spare_chains[i]==o->chains[0] || o->spare_chains[i]==o->chains[1]) {
            completed_presentation_cancel(c);return 0;
        }
    }
    views[0]=c->views[0];views[1]=c->views[1];
    *layer=(XrCompositionLayerProjection){.type=XR_TYPE_COMPOSITION_LAYER_PROJECTION,
        .space=c->space,.viewCount=2,.views=views};return 1;
}
#ifdef GF2VR_ASYNC_PRESENTATION
/* A later successful display is NOT a new capture instant. The prepared-bank
   clock owns the deadline; replay/duplicate/failure cannot mint another250ms. */
static inline int completed_presentation_capture_late(CompletedPresentation *c,XrEyeOutput *o,
        const XrEyeInput *original,const XrCompositionLayerProjection *layer,
        U64 generation,U64 prepared_tick,U64 now,U64 cancellation_at_build,
        XrResult actual_end_result) {
    if(actual_end_result!=XR_SUCCESS || !prepared_tick || now<prepared_tick || now-prepared_tick>=250 ||
       cancellation_at_build!=__atomic_load_n(&stereo_completed_cancel_clock,__ATOMIC_ACQUIRE))return 0;
    return completed_presentation_capture(c,o,original,layer,generation,prepared_tick);
}
#endif
#endif
