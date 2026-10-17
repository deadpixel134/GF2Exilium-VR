#ifndef GF2_XR_EYE_SIZE_H
#define GF2_XR_EYE_SIZE_H
/* XR writer, Unity reader. One atomic numeric pair; no XR handles cross threads.
   Zero at session close/open. Size does not authorize render/session use. */
typedef struct {unsigned long long packed;} XrEyeSize;
static void xr_eye_size_clear(XrEyeSize *s) {
    __atomic_store_n(&s->packed,0,__ATOMIC_RELEASE);
}
static int xr_eye_size_read(const XrEyeSize *s,unsigned *width,unsigned *height) {
    unsigned long long packed=__atomic_load_n(&s->packed,__ATOMIC_ACQUIRE);
    *width=(unsigned)packed;*height=(unsigned)(packed>>32);return packed!=0;
}
static int xr_eye_size_open(XrEyeSize *s,PFN_xrGetInstanceProcAddr get,XrInstance instance,XrSystemId system) {
    xr_eye_size_clear(s);
    PFN_xrEnumerateViewConfigurationViews enumerate=0;
    if(!get || !instance || !system ||
       get(instance,"xrEnumerateViewConfigurationViews",(PFN_xrVoidFunction *)&enumerate)!=XR_SUCCESS || !enumerate) return 0;
    XrViewConfigurationView views[2]={{0},{0}};unsigned count=0;
    views[0].type=views[1].type=XR_TYPE_VIEW_CONFIGURATION_VIEW;
    if(enumerate(instance,system,XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,2,&count,views)!=XR_SUCCESS || count!=2) return 0;
    unsigned width=0,height=0;
    for(unsigned i=0;i<2;++i) {
        XrViewConfigurationView *v=&views[i];
        if(!v->recommendedImageRectWidth || !v->recommendedImageRectHeight ||
           v->recommendedImageRectWidth>v->maxImageRectWidth || v->recommendedImageRectHeight>v->maxImageRectHeight ||
           !v->recommendedSwapchainSampleCount || !v->maxSwapchainSampleCount ||
           v->recommendedSwapchainSampleCount>v->maxSwapchainSampleCount) return 0;
        if(width<v->recommendedImageRectWidth) width=v->recommendedImageRectWidth;
        if(height<v->recommendedImageRectHeight) height=v->recommendedImageRectHeight;
    }
    /* Shared pair extent must fit BOTH eyes; never silently clamp quality. */
    if(width>16384 || height>16384) return 0;
    for(unsigned i=0;i<2;++i) if(width>views[i].maxImageRectWidth || height>views[i].maxImageRectHeight) return 0;
    __atomic_store_n(&s->packed,((unsigned long long)height<<32)|width,__ATOMIC_RELEASE);return 1;
}
#ifdef GF2VR_RENDER_SCALE
#include "render_scale_config.h"
/* Same sole-XR extent publication, with requested per-axis quality scaling.
   The common pair must fit both views and the actual D3D11 2D texture limit. */
static int xr_eye_size_open_scaled(XrEyeSize *s,PFN_xrGetInstanceProcAddr get,XrInstance instance,XrSystemId system,double scale){
    xr_eye_size_clear(s);
    if(!__builtin_isfinite(scale)||scale<=0)return 0;
    PFN_xrEnumerateViewConfigurationViews enumerate=0;
    if(!get||!instance||!system||get(instance,"xrEnumerateViewConfigurationViews",(PFN_xrVoidFunction *)&enumerate)!=XR_SUCCESS||!enumerate)return 0;
    XrViewConfigurationView views[2]={{0},{0}};unsigned count=0;
    views[0].type=views[1].type=XR_TYPE_VIEW_CONFIGURATION_VIEW;
    if(enumerate(instance,system,XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,2,&count,views)!=XR_SUCCESS||count!=2)return 0;
    unsigned base_width=0,base_height=0,max_width=16384,max_height=16384;
    for(unsigned i=0;i<2;++i){XrViewConfigurationView *v=&views[i];
        if(!v->recommendedImageRectWidth||!v->recommendedImageRectHeight||
           v->recommendedImageRectWidth>v->maxImageRectWidth||v->recommendedImageRectHeight>v->maxImageRectHeight||
           !v->recommendedSwapchainSampleCount||!v->maxSwapchainSampleCount||
           v->recommendedSwapchainSampleCount>v->maxSwapchainSampleCount)return 0;
        if(base_width<v->recommendedImageRectWidth)base_width=v->recommendedImageRectWidth;
        if(base_height<v->recommendedImageRectHeight)base_height=v->recommendedImageRectHeight;
        if(max_width>v->maxImageRectWidth)max_width=v->maxImageRectWidth;
        if(max_height>v->maxImageRectHeight)max_height=v->maxImageRectHeight;
    }
    unsigned width=0,height=0;
    if(!render_scale_dimension(base_width,max_width,scale,&width)||!render_scale_dimension(base_height,max_height,scale,&height))return 0;
    __atomic_store_n(&s->packed,((unsigned long long)height<<32)|width,__ATOMIC_RELEASE);return 1;
}
#endif
#endif
