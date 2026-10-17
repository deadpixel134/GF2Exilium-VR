#ifndef GF2_XR_PRESENTATION_CACHE_H
#define GF2_XR_PRESENTATION_CACHE_H
#include "xr_presentation_flights.h"
/* XR-only completed metadata; no source/native borrow, fresh completion or
   HMD neutral. Bank pool owns protection. Preparation age gates fresh output;
   the HELD fresh-display clock gates replay when its approved route is selected. */
typedef struct {
    unsigned valid,index;
    EyeFlightToken token;
    unsigned long long serial,tick,cancel_clock;
    XrSession session;XrSpace space;
    XrCompositionLayerProjectionView views[2];
} XrBankPresentation;
static inline int xpc_capture(XrBankPresentation *cache,XrPresentationFlights *flights,PanelXr *panel,
        EyeFlightToken token,const XrCompositionLayerProjection *layer,
        const unsigned long long *cancel_clock,unsigned long long cancel_at_build,XrResult actual_end) {
    if(!cache || !flights || !layer || !cancel_clock || actual_end!=XR_SUCCESS ||
       !xpb_owner(&flights->storage,panel) || !flights->storage.ready || flights->storage.closing ||
       __atomic_load_n(cancel_clock,__ATOMIC_ACQUIRE)!=cancel_at_build ||
       layer->type!=XR_TYPE_COMPOSITION_LAYER_PROJECTION || layer->next || !layer->space ||
       layer->viewCount!=2 || !layer->views)return 0;
    unsigned i;if(!pbp_held(&flights->storage.leases,GetTickCount64(),&i))return 0;
    PresentationBank *bank=&flights->storage.leases.banks[i];XrEyeOutput *o=&flights->storage.outputs[i];
    if(!pbp_same(bank->token,token) || !bank->ready_serial ||
       (cache->token.epoch==token.epoch && cache->token.frame>=token.frame))return 0;
    for(unsigned eye=0;eye<2;++eye) {
        const XrCompositionLayerProjectionView *v=&layer->views[eye];
        if(v->type!=XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW || v->next ||
           v->subImage.swapchain!=o->chains[eye] || !xr_eye_pose_valid(&v->pose) ||
           o->acquired[eye] || o->waited[eye] ||
           v->subImage.imageRect.extent.width!=(int)o->width ||
           v->subImage.imageRect.extent.height!=(int)o->height)return 0;
    }
    *cache=(XrBankPresentation){.valid=1,.index=i,.token=token,.serial=bank->ready_serial,
        .tick=
#ifdef GF2VR_HELD_FRESH_CLOCK
        bank->held_ms,
#else
        bank->prepared_ms,
#endif
        .cancel_clock=cancel_at_build,.session=panel->session,.space=layer->space};
    cache->views[0]=layer->views[0];cache->views[1]=layer->views[1];return 1;
}
static inline int xpc_layer(XrBankPresentation *cache,XrPresentationFlights *flights,PanelXr *panel,
        const XrEyeInput *current,XrSpace space,const XrFrameEndInfo *end,
        const unsigned long long *publication,const unsigned long long *cancel_clock,
        XrCompositionLayerProjection *layer,XrCompositionLayerProjectionView views[2]) {
    if(layer)*layer=(XrCompositionLayerProjection){0};
    if(views)views[0]=views[1]=(XrCompositionLayerProjectionView){0};
    if(!cache || !cache->valid || !flights || !panel || !current || !end || !publication ||
       !cancel_clock || !layer || !views)return 0;
    unsigned i;
    int valid=xpb_owner(&flights->storage,panel) && flights->storage.ready && !flights->storage.closing &&
        cache->session==panel->session && space && space==cache->space &&
        __atomic_load_n(publication,__ATOMIC_ACQUIRE)==cache->token.generation &&
        __atomic_load_n(cancel_clock,__ATOMIC_ACQUIRE)==cache->cancel_clock &&
        end->type==XR_TYPE_FRAME_END_INFO && xr_eye_matches(current,cache->token.epoch,current->frame_id,end->displayTime) &&
        pbp_held(&flights->storage.leases,GetTickCount64(),&i) && i==cache->index;
    if(valid) {
        PresentationBank *bank=&flights->storage.leases.banks[i];
        valid=pbp_same(bank->token,cache->token) && bank->ready_serial==cache->serial &&
#ifdef GF2VR_HELD_FRESH_CLOCK
            bank->held_ms==cache->tick;
#else
            bank->prepared_ms==cache->tick;
#endif
        for(unsigned j=0;j<PBP_BANKS;++j)
            if(__atomic_load_n(&flights->storage.outputs[j].state,__ATOMIC_ACQUIRE)==XEO_FAULT)valid=0;
        for(unsigned eye=0;eye<2;++eye)
            if(cache->views[eye].subImage.swapchain!=flights->storage.outputs[i].chains[eye] ||
               !xpf_chain_allowed(flights,panel,cache->views[eye].subImage.swapchain))valid=0;
    }
    if(!valid){cache->valid=0;return 0;}
    views[0]=cache->views[0];views[1]=cache->views[1];
    *layer=(XrCompositionLayerProjection){.type=XR_TYPE_COMPOSITION_LAYER_PROJECTION,
        .space=cache->space,.viewCount=2,.views=views};return 1;
}
#endif
