#ifndef GF2_CAMERA_PIPELINE_SCOPE_H
#define GF2_CAMERA_PIPELINE_SCOPE_H
#include "camera_render_scope.h"
/* Immutable owner_thread is assigned before hook publication. All other state
   is owner-thread only. Caller roots the selected pipeline and prepared eyes
   through outer root return; this bridge owns only the private Camera[] roots.
   No borrowed ScriptableRenderContext is stored. */
typedef struct {
    unsigned long owner_thread;
    CameraRenderScope render;
    const CameraBatchApi *api;
    void *pipeline,*method,*left,*right,*check_owner;
    CameraRenderCurrent current;
    unsigned armed;
} CameraPipelineScope;

static int camera_pipeline_begin(CameraPipelineScope *s,unsigned long thread,
                                 unsigned long long serial) {
    if(!thread || thread!=s->owner_thread) return 0;
    return camera_render_scope_begin(&s->render,serial);
}
/* Called inside the selected root, after producer preparation/borrow. Arm at
   most once. The check_owner (e.g. PoseRenderCheck) must survive outer return,
   including managed exception unwinding; it cannot be wrapper-stack scratch. */
static int camera_pipeline_arm(CameraPipelineScope *s,unsigned long thread,
                               const CameraBatchApi *api,void *pipeline,void *method,
                               void *left,void *right,CameraRenderCurrent current,void *owner) {
    if(!thread || thread!=s->owner_thread || !s->render.active ||
       s->armed || s->render.attempted || !api || !pipeline || !method ||
       !left || !right || left==right || !current) return 0;
    s->api=api;s->pipeline=pipeline;s->method=method;s->left=left;s->right=right;
    s->current=current;s->check_owner=owner;s->armed=1;return 1;
}
static void camera_pipeline_forward(CameraPipelineScope *s,unsigned long thread,
                                    CameraRenderForward original,void *pipeline,
                                    void *context,void *cameras,void *method) {
    /* A foreign thread must short-circuit before reading mutable owner state. */
    if(!thread || thread!=s->owner_thread || !s->render.active || !s->armed ||
       pipeline!=s->pipeline || method!=s->method) {
        original(pipeline,context,cameras,method);return;
    }
    camera_render_forward_checked(&s->render,s->api,original,pipeline,context,cameras,method,
                                  s->left,s->right,s->current,s->check_owner);
}
/* Wire to the selected outer runtime_invoke's after-return path, even when its
   exception output is set. No replay and no managed camera/target retirement.
   Success is CPU scope evidence only, never target freshness/GPU completion. */
static int camera_pipeline_end(CameraPipelineScope *s,unsigned long thread,
                               unsigned long long serial,int root_success) {
    if(!thread || thread!=s->owner_thread || !s->render.active || s->render.serial!=serial) return 0;
    int ok=0;
    if(s->armed) ok=camera_render_scope_end(&s->render,s->api,serial,root_success);
    else {s->render.active=0;s->render.serial=0;}
    s->api=0;s->pipeline=s->method=s->left=s->right=s->check_owner=0;
    s->current=0;s->armed=0;return ok;
}
#endif
