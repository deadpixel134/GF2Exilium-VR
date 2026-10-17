#ifndef GF2_ACTIVITY_VIEWPORT_H
#define GF2_ACTIVITY_VIEWPORT_H
#include <float.h>
#include "managed_eye_camera.h"
/* Rect is the verified sequential XMin/YMin/Width/Height value type. Private
   Overlay target stays null. Equal actual pixel rectangles prevent the game's
   InitializeAdditionalCameraData from replacing the assigned XR projection. */
typedef struct {float v[4];} ActivityViewport;
static int activity_viewport_equal(const ActivityViewport *a,const ActivityViewport *b) {
    for(unsigned i=0;i<4;++i)if(a->v[i]!=b->v[i])return 0;
    return 1;
}
static int activity_viewport_read(const ManagedEyeApi *a,void *camera,ActivityViewport *out) {
    if(!a || !a->rect_class || !a->get_pixel_rect || !a->invoke || !a->pin || !a->unpin ||
       !a->unbox || !a->object_class || !camera || !out)return 0;
    void *boxed=0;
    if(!managed_eye_call(a,a->get_pixel_rect,camera,0,&boxed) || !boxed)return 0;
    unsigned root=a->pin(boxed,1);if(!root)return 0;
    int ok=0;
    if(a->object_class(boxed)==a->rect_class) {
        const float *value=a->unbox(boxed);
        if(value) {
            *out=(ActivityViewport){{value[0],value[1],value[2],value[3]}};
            ok=1;
            for(unsigned i=0;i<4;++i)if(!(out->v[i]>=-FLT_MAX && out->v[i]<=FLT_MAX))ok=0;
            if(!(out->v[2]>0 && out->v[3]>0))ok=0;
        }
    }
    a->unpin(root);return ok;
}
#ifdef GF2VR_ACTIVITY_SCALE_PREPARE_OBSERVE
static int activity_viewport_align(const ManagedEyeApi *a,void *camera,
    const ActivityViewport *base,ActivityViewport *before,ActivityViewport *after,
    int (*current)(void *),void *context) {
    unsigned attempted=0,passed=0;
#define AVP_CHECK(bit,expression) (attempted|=(bit),(expression)?(passed|=(bit),1):0)
#define AVP_REPORT() GF2VR_ACTIVITY_VIEWPORT_BOUNDARY_REPORT(context,camera,attempted,passed,base,((passed&2)?before:0),((passed&64)?after:0))
    if(!a || !a->set_pixel_rect || !base || !before || !after || !current ||
       !AVP_CHECK(1,current(context)) || !AVP_CHECK(2,activity_viewport_read(a,camera,before)) ||
       !AVP_CHECK(4,current(context))) {AVP_REPORT();return 0;}
    if(!AVP_CHECK(8,activity_viewport_equal(base,before))) {
        ActivityViewport value=*base;void *args[]={&value};
        if(!AVP_CHECK(16,managed_eye_call(a,a->set_pixel_rect,camera,args,0)) ||
           !AVP_CHECK(32,current(context))) {AVP_REPORT();return 0;}
    }
    int ok=AVP_CHECK(64,activity_viewport_read(a,camera,after)) &&
        AVP_CHECK(128,current(context)) && AVP_CHECK(256,activity_viewport_equal(base,after));
    AVP_REPORT();
#undef AVP_REPORT
#undef AVP_CHECK
    return ok;
}
#else
static int activity_viewport_align(const ManagedEyeApi *a,void *camera,
    const ActivityViewport *base,ActivityViewport *before,ActivityViewport *after,
    int (*current)(void *),void *context) {
    if(!a || !a->set_pixel_rect || !base || !before || !after || !current || !current(context) ||
       !activity_viewport_read(a,camera,before) || !current(context))return 0;
    if(!activity_viewport_equal(base,before)) {
        ActivityViewport value=*base;void *args[]={&value};
        if(!managed_eye_call(a,a->set_pixel_rect,camera,args,0) || !current(context))return 0;
    }
    return activity_viewport_read(a,camera,after) && current(context) && activity_viewport_equal(base,after);
}
#endif
#endif
