#ifndef GF2_XR_DISPLAY_EXCLUSION_H
#define GF2_XR_DISPLAY_EXCLUSION_H
#include <openxr/openxr.h>
/* Read-only prerequisite for detached display End. Known projection/quad
   layers only, bounded by the existing world+panel+cursor composition.
   Unknown types/extension chains reject instead of guessing their resources.
   Caller must still validate exact session/frame/time/reference and real End. */
static int xr_display_excludes(const XrFrameEndInfo *end,XrSwapchain left,XrSwapchain right) {
    if(!end || end->type!=XR_TYPE_FRAME_END_INFO || end->next || !left || !right || left==right ||
       end->layerCount>3 || (end->layerCount && !end->layers))return 0;
    for(unsigned i=0;i<end->layerCount;++i) {
        const XrCompositionLayerBaseHeader *base=end->layers[i];
        if(!base || base->next)return 0;
        if(base->type==XR_TYPE_COMPOSITION_LAYER_PROJECTION) {
            const XrCompositionLayerProjection *p=(const XrCompositionLayerProjection *)base;
            if(!p->space || p->viewCount!=2 || !p->views)return 0;
            for(unsigned eye=0;eye<2;++eye) {
                const XrCompositionLayerProjectionView *v=&p->views[eye];
                if(v->type!=XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW || v->next ||
                   !v->subImage.swapchain || v->subImage.swapchain==left || v->subImage.swapchain==right)return 0;
            }
        } else if(base->type==XR_TYPE_COMPOSITION_LAYER_QUAD) {
            const XrCompositionLayerQuad *q=(const XrCompositionLayerQuad *)base;
            if(!q->space || !q->subImage.swapchain || q->subImage.swapchain==left || q->subImage.swapchain==right)return 0;
        } else return 0;
    }
    return 1;
}
#endif
