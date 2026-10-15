#ifndef GF2_ACTIVITY_TARGET_SURFACE_H
#define GF2_ACTIVITY_TARGET_SURFACE_H
#include "managed_eye_camera.h"
/* A camera destination reference is not a target owner. The Base retains both
   target roots until all recorded references are verified detached. A setter
   attempt is recorded BEFORE invocation, including throw-after-effect cases. */
typedef struct {
    void *camera[2],*target[2],*getter;
    unsigned live_mask;
} ActivityTargetSurface;
static int activity_target_read(const ManagedEyeApi *a,void *getter,void *camera,void **out) {
    if(!a || !getter || !camera || !out || !a->invoke || !a->pin || !a->unpin ||
       !a->object_class || !a->target_class)return 0;
    void *actual=0;
    if(!managed_eye_call(a,getter,camera,0,&actual))return 0;
    if(actual) {
        unsigned root=a->pin(actual,1);if(!root)return 0;
        int typed=a->object_class(actual)==a->target_class;a->unpin(root);
        if(!typed)return 0;
    }
    *out=actual;return 1; /* compare only with the independently rooted owner */
}
static int activity_target_surface_bind(const ManagedEyeApi *a,void *getter,
    ActivityTargetSurface *r,ManagedEyes *cameras,ManagedEyes *base,unsigned eye,
    int (*current)(void *),void *context) {
    if(!a || !a->set_target || !getter || !r || !cameras || !base || eye>1 ||
       cameras==base || cameras->borrowed || cameras->targets_ready || base->borrowed ||
       !current || !current(context))return 0;
    ManagedEyeSlot *s=&cameras->eyes[eye],*owner=&base->eyes[eye];
    if(!s->camera_root || !s->camera || s->target || s->target_root ||
       !owner->target_root || !owner->target_constructed || !owner->target ||
       owner->target==base->eyes[1-eye].target || s->camera==base->eyes[0].camera ||
       s->camera==base->eyes[1].camera || s->camera==cameras->eyes[1-eye].camera)return 0;
    /* Refresh temporarily clears publication readiness. The constructed,
       rooted Base target and exact camera readback still own this binding. */
    unsigned bit=1u<<eye;void *actual=0;
    if(!activity_target_read(a,getter,owner->camera,&actual) || actual!=owner->target || !current(context))return 0;
    if((r->live_mask&bit) && (r->camera[eye]!=s->camera || r->target[eye]!=owner->target || r->getter!=getter))return 0;
    if(!activity_target_read(a,getter,s->camera,&actual) || !current(context))return 0;
    if(actual && (!(r->live_mask&bit) || actual!=owner->target))return 0;
    r->camera[eye]=s->camera;r->target[eye]=owner->target;r->getter=getter;r->live_mask|=bit;
    if(!actual) {
        void *args[]={owner->target};
        if(!managed_eye_call(a,a->set_target,s->camera,args,0) || !current(context))return 0;
    }
    return activity_target_read(a,getter,s->camera,&actual) && current(context) && actual==owner->target;
}
static int activity_target_surface_clear(const ManagedEyeApi *a,ActivityTargetSurface *r,ManagedEyes *cameras) {
    if(!r || !cameras || cameras->borrowed)return 0;
    if(!r->live_mask)return 1;
    if(!a || !a->set_target || !r->getter)return 0;
    int clean=1;
    for(unsigned eye=0;eye<2;++eye) {
        unsigned bit=1u<<eye;if(!(r->live_mask&bit))continue;
        ManagedEyeSlot *s=&cameras->eyes[eye];
        if(!s->camera_root || !s->camera || s->camera!=r->camera[eye]){clean=0;continue;}
        void *actual=0;
        int read=activity_target_read(a,r->getter,s->camera,&actual);
        if(read && actual && actual!=r->target[eye]){clean=0;continue;} /* foreign owner */
        if(read && actual) {
            void *args[]={0};(void)managed_eye_call(a,a->set_target,s->camera,args,0);
            read=activity_target_read(a,r->getter,s->camera,&actual);
        }
        if(!(read && !actual) && !managed_eye_verified_dead(a,s->camera)){clean=0;continue;}
        r->live_mask&=~bit;r->camera[eye]=r->target[eye]=0;
    }
    if(!r->live_mask)*r=(ActivityTargetSurface){0};
    return clean && !r->live_mask;
}
#endif
