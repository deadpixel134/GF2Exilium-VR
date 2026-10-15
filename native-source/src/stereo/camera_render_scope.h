#ifndef GF2_CAMERA_RENDER_SCOPE_H
#define GF2_CAMERA_RENDER_SCOPE_H
#include "managed_camera_batch.h"
/* Actual selected wrapper ABI: pipeline, borrowed context, Camera[], MethodInfo.
   Install only on the verified wrapper, inside the attributed owner's root
   runtime_invoke scope. No retained context or recursive pipeline invocation. */
typedef void (*CameraRenderForward)(void *,void *,void *,void *);
/* Owner-thread, non-throwing, read-only check: no managed calls. The producer
   checks its source generation and XR ticket here. Never holds an XR lock
   across the original render; publication still requires a fresh check. */
typedef int (*CameraRenderCurrent)(void *);
typedef struct {
    CameraBatch batch;
    void *required_source; /* Borrowed from the armed producer's rooted owner. */
    unsigned long long serial,last_serial;
    unsigned active,attempted,injected,returned;
#ifdef GF2VR_MAIN_HANDOFF_OBSERVE
    unsigned observed_injection;
#endif
} CameraRenderScope;

static int camera_render_scope_begin(CameraRenderScope *s,unsigned long long serial) {
    if(s->active || !serial || serial<=s->last_serial) return 0;
    s->serial=s->last_serial=serial;s->active=1;
#ifdef GF2VR_MAIN_HANDOFF_OBSERVE
    s->observed_injection=0;
#endif
    s->attempted=s->injected=s->returned=0;s->required_source=0;return 1;
}
/* Caller supplies already-borrowed, fully prepared eyes. This scope owns only
   temporary array roots. It never releases the producer's eye/GPU borrow. */
static void camera_render_forward_checked(CameraRenderScope *s,const CameraBatchApi *api,
                                  CameraRenderForward original,
                                  void *pipeline,void *context,void *cameras,void *method,
                                  void *left,void *right,
                                  CameraRenderCurrent current,void *owner) {
    void *selected=cameras;
    int inject=0;
    if(s->active && !s->attempted) {
        /* Set before managed allocation/invoke: reentrant wrappers forward only. */
        s->attempted=1;
#ifdef GF2VR_MAIN_HANDOFF_OBSERVE
        s->observed_injection=1;
        if(left && right && current) {
            if(!current(owner))s->observed_injection=2;
            else if(!(s->required_source ? camera_batch_build_source(api,&s->batch,cameras,left,right,s->required_source)
                                        : camera_batch_build(api,&s->batch,cameras,left,right)))s->observed_injection=3;
            else if(!current(owner))s->observed_injection=4;
            else {selected=s->batch.array;s->injected=1;inject=1;s->observed_injection=5;}
        }
#else
        if(left && right && current && current(owner) &&
           (s->required_source ? camera_batch_build_source(api,&s->batch,cameras,left,right,s->required_source)
                               : camera_batch_build(api,&s->batch,cameras,left,right)) && current(owner)) {
            selected=s->batch.array;s->injected=1;inject=1;
        }
#endif
    }
    original(pipeline,context,selected,method);
    /* A native managed exception may unwind past here into the outer public
       runtime_invoke catcher. Cleanup therefore belongs to scope_end below. */
    if(inject) s->returned=1;
}
/* Legacy fixture/adapter entry. Live pose producers must use the checked
   entry above; allocation and managed setters can overlap XR invalidation. */
static int camera_render_unchecked(void *owner) {(void)owner;return 1;}
static void camera_render_forward(CameraRenderScope *s,const CameraBatchApi *api,
                                  CameraRenderForward original,
                                  void *pipeline,void *context,void *cameras,void *method,
                                  void *left,void *right) {
    camera_render_forward_checked(s,api,original,pipeline,context,cameras,method,
                                 left,right,camera_render_unchecked,0);
}
/* Call AFTER outer runtime_invoke returns, including its managed-exception path.
   Result means only injected wrapper returned and outer scope had no exception.
   It is not target freshness, GPU completion, or permission to destroy eyes.
   A failing invocation is never replayed with the original camera array. */
static int camera_render_scope_end(CameraRenderScope *s,const CameraBatchApi *api,
                                   unsigned long long serial,int root_success) {
    if(!s->active || s->serial!=serial) return 0;
    int success=root_success && s->injected && s->returned;
    camera_batch_release(api,&s->batch);
    s->active=0;s->serial=0;s->injected=s->returned=0;s->required_source=0;
    return success;
}
#endif
