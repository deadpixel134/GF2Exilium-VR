#ifndef GF2_SOURCE_RENDER_FRAME_H
#define GF2_SOURCE_RENDER_FRAME_H
#include "camera_data_owner.h"
#include "source_pose_apply.h"
#include "pose_render_check.h"
#include "camera_pipeline_scope.h"
/* Long-lived owner-thread selection published by the real topology classifier.
   This module neither selects by camera name nor invents game units/metre. */
typedef struct {
    void *source;
    unsigned long long generation;
    float units_per_meter;
    unsigned eligible;
} SourceRenderSelection;
#ifdef GF2VR_ACTIVITY_STACK
typedef struct {
    void *context;
    int (*apply)(void *,SourcePoseApi *,SourceAnchorState *,PoseMathBinding *,XrEyeMailbox *,const PoseAppliedTicket *);
    int (*borrow)(void *,unsigned long long);
    int (*current)(void *);
    void (*release)(void *);
    int (*cpu_return)(void *);
} SourceRenderGroupApi;
#endif
/* One in-flight eye pair. This storage, selection, data/eye owner and services
   survive the outer runtime_invoke and (when injected) the GPU drain boundary.
   Never wrapper-stack scratch. No borrowed render context is stored. */
typedef struct {
    CameraDataOwner *data;
    ManagedEyes *eyes;
    CameraPipelineScope *scope;
    const SourceRenderSelection *selection;
    ManagedEyeApi calls;
    PoseRenderCheck check;
    unsigned long long serial,epoch;
    float units;
    unsigned pipeline_root,busy,data_guard,borrowed,armed,root_done,injected,cpu_ok;
#ifdef GF2VR_ACTIVITY_STACK
    const SourceRenderGroupApi *group;
    unsigned group_borrowed;
#endif
#if defined(GF2VR_MAIN_HANDOFF_OBSERVE) || defined(GF2VR_NATIVE_BOUNDARY_OBSERVE)
    unsigned observed_reason;
#endif
} SourceRenderFrame;
/* Camera settings/target attachment belong to the source/data lifetime, not
   the availability of a particular XR prediction. A temporary mailbox gap
   must reject that frame without destroying otherwise valid private cameras. */
static int source_render_source_current(void *context,unsigned long long generation) {
    const SourceRenderFrame *f=context;
    return f && f->data && f->data->binding && !f->data->retiring && f->selection &&
        f->selection->eligible && generation && generation==f->data->generation &&
        generation==f->selection->generation && f->selection->source==f->data->source_camera &&
        f->units>0 && f->units==f->selection->units_per_meter;
}
static int source_render_selection_current(void *context,unsigned long long generation) {
    const SourceRenderFrame *f=context;
    return source_render_source_current(context,generation) && f->check.mailbox &&
        __atomic_load_n(&f->check.mailbox->epoch,__ATOMIC_ACQUIRE)==f->epoch && f->epoch;
}
/* Read-only/no managed calls. Also used after Camera[] allocation by the actual
   camera_pipeline_forward path. GPU publication needs this AND pair freshness,
   successful CPU scope and genuine GPU completion evidence. */
static int source_render_frame_current(void *context) {
#ifdef GF2VR_ACTIVITY_STACK
    SourceRenderFrame *group_frame=context;
    if(group_frame && group_frame->group && (!group_frame->group_borrowed ||
       !group_frame->group->current(group_frame->group->context)))return 0;
#endif
#if defined(GF2VR_MAIN_HANDOFF_OBSERVE) || defined(GF2VR_NATIVE_BOUNDARY_OBSERVE)
    SourceRenderFrame *f=context;unsigned reason=0;
    if(!f)return 0;
    if(f->busy)reason=1;
    else if(!f->borrowed)reason=2;
    else if(!f->data || !f->data->ready)reason=3;
    else if(!source_render_source_current(f,f->check.ticket.source_generation))reason=4;
    else if(!f->check.mailbox ||
        __atomic_load_n(&f->check.mailbox->epoch,__ATOMIC_ACQUIRE)!=f->epoch || !f->epoch)reason=5;
    else if(!pose_render_current(&f->check))reason=6;
    if(reason && !f->observed_reason)f->observed_reason=reason;
    return reason==0;
#else
    const SourceRenderFrame *f=context;
    return f && !f->busy && f->borrowed && f->data && f->data->ready &&
        source_render_selection_current((void *)f,f->check.ticket.source_generation) &&
        pose_render_current((void *)&f->check);
#endif
}
/* Only for a pair never passed to the engine, or AFTER authoritative GPU drain.
   Kept private to the state transitions below; CPU return is not GPU drain. */
static void source_render_frame_release(SourceRenderFrame *f) {
#if defined(GF2VR_MAIN_HANDOFF_OBSERVE) || defined(GF2VR_NATIVE_BOUNDARY_OBSERVE)
    unsigned reason=f->observed_reason,pose_reason=f->check.observed_reason;
#endif
    if(f->pipeline_root) f->calls.unpin(f->pipeline_root);
    if(f->borrowed) managed_eyes_return(f->eyes);
#ifdef GF2VR_ACTIVITY_STACK
    if(f->group_borrowed)f->group->release(f->group->context);
#endif
    *f=(SourceRenderFrame){0};
#if defined(GF2VR_MAIN_HANDOFF_OBSERVE) || defined(GF2VR_NATIVE_BOUNDARY_OBSERVE)
    /* Preserve only diagnostics until root capture; the next arm resets them. */
    f->observed_reason=reason;f->check.observed_reason=pose_reason;
#endif
}
#ifdef GF2VR_ACTIVITY_STACK
static int source_render_frame_arm_with(SourceRenderFrame *f,CameraDataOwner *data,
#else
static int source_render_frame_arm(SourceRenderFrame *f,CameraDataOwner *data,
#endif
    const SourceRenderSelection *selection,SourcePoseApi *reader,SourceAnchorState *anchor,
    PoseMathBinding *math,XrEyeMailbox *mailbox,CameraPipelineScope *scope,unsigned long thread,
    const CameraBatchApi *batch,void *pipeline,void *method
#ifdef GF2VR_ACTIVITY_STACK
    ,const SourceRenderGroupApi *group
#endif
    ) {
    GF2VR_PREPARE_STEP(400);
    if(!f || !scope || !thread || thread!=scope->owner_thread) return 0;
    GF2VR_PREPARE_STEP(401);
    if(f->serial || f->busy || !scope->render.active || scope->armed || scope->render.attempted ||
       !data || !data->binding || !data->ready || data->busy || data->retiring ||
       !data->eyes || data->eyes->borrowed || !data->eyes->targets_ready ||
       !selection || !reader || !anchor || !math || !mailbox || !batch || !pipeline || !method) return 0;
    *f=(SourceRenderFrame){0};f->busy=1;f->data=data;f->eyes=data->eyes;f->scope=scope;
    f->selection=selection;f->units=selection->units_per_meter;f->serial=scope->render.serial;
    f->calls=data->binding->refs.settings.calls;f->check.mailbox=mailbox;f->check.eyes=f->eyes;
    f->check.source_generation=&selection->generation;
    f->epoch=__atomic_load_n(&mailbox->epoch,__ATOMIC_ACQUIRE);
#ifdef GF2VR_ACTIVITY_STACK
    f->group=group;
    if(group && (!group->context || !group->apply || !group->borrow || !group->current || !group->release || !group->cpu_return))goto failed;
#endif
    GF2VR_PREPARE_STEP(402);
    if(!source_render_selection_current(f,data->generation)) goto failed;
    GF2VR_PREPARE_STEP(403);
    f->pipeline_root=f->calls.pin(pipeline,1);if(!f->pipeline_root) goto failed;
    GF2VR_PREPARE_STEP(404);
    if(!camera_data_owner_refresh(data,source_render_source_current,f)) goto failed;
    /* Pose getters/setters can reenter Unity. Retain data/eye ownership through
       that entire interval; retirement marks intent but must not destroy them. */
    data->busy=1;f->data_guard=1;
    GF2VR_PREPARE_STEP(405);
    if(!source_pose_apply(reader,selection->source,selection->generation,f->units,anchor,math,
                          mailbox,&f->calls,f->eyes,&f->check.ticket) ||
       (GF2VR_PREPARE_STEP(408),!source_render_selection_current(f,f->check.ticket.source_generation))) goto failed;
#ifdef GF2VR_ACTIVITY_STACK
    if(group && !group->apply(group->context,reader,anchor,math,mailbox,&f->check.ticket))goto failed;
#endif
    data->busy=0;f->data_guard=0;
    void *left=0,*right=0;
    GF2VR_PREPARE_STEP(406);
#ifdef GF2VR_ACTIVITY_STACK
    if(group) {
        if(!group->borrow(group->context,f->check.ticket.frame_id))goto failed;
        f->group_borrowed=1;
    }
#endif
    if(!managed_eyes_borrow(f->eyes,f->check.ticket.frame_id,&left,&right)) goto failed;
    f->borrowed=1;f->busy=0;
    GF2VR_PREPARE_STEP(407);
    if(!source_render_frame_current(f) ||
       (GF2VR_PREPARE_STEP(409),!camera_pipeline_arm(scope,thread,batch,pipeline,method,left,right,source_render_frame_current,f))) goto failed;
    scope->render.required_source=data->source_camera;
    f->armed=1;return 1;
failed:
    /* The scope still forwards original cameras and is closed by its root owner.
       Data refresh may have retired its owner; f->eyes/calls remain independent. */
    if(f->data_guard) {data->busy=0;f->data_guard=0;}
    f->eyes->matrices_ready=0;
    if(data->binding && data->retiring && !f->borrowed) camera_data_owner_retire(data);
    source_render_frame_release(f);return 0;
}
#ifdef GF2VR_ACTIVITY_STACK
static int source_render_frame_arm(SourceRenderFrame *f,CameraDataOwner *data,
    const SourceRenderSelection *selection,SourcePoseApi *reader,SourceAnchorState *anchor,
    PoseMathBinding *math,XrEyeMailbox *mailbox,CameraPipelineScope *scope,unsigned long thread,
    const CameraBatchApi *batch,void *pipeline,void *method) {
    return source_render_frame_arm_with(f,data,selection,reader,anchor,math,mailbox,scope,thread,batch,pipeline,method
#ifdef GF2VR_ACTIVITY_STACK
        ,0
#endif
    );
}
#endif
/* Call instead of bare camera_pipeline_end for this armed frame, at the outer
   runtime_invoke return, including managed-exception return. Returns CPU status
   only. An injected failing call may already have submitted GPU work, so it
   retains the borrow exactly like a successful call. */
static int source_render_frame_finish(SourceRenderFrame *f,unsigned long thread,
                                     unsigned long long serial,int root_success) {
    if(!f || !f->armed || f->root_done || f->busy || !f->scope ||
       thread!=f->scope->owner_thread || serial!=f->serial || !f->scope->render.active ||
       serial!=f->scope->render.serial) return 0;
    f->injected=f->scope->render.injected;
    f->cpu_ok=camera_pipeline_end(f->scope,thread,serial,root_success);f->root_done=1;
#ifdef GF2VR_ACTIVITY_STACK
    /* Only activation has a CPU scope: URP has finished reading isActiveAndEnabled.
       Camera/data/matrix/target roots still remain borrowed until actual GPU drain. */
    if(f->group_borrowed && !f->group->cpu_return(f->group->context))f->cpu_ok=0;
#endif
    if(f->pipeline_root) {f->calls.unpin(f->pipeline_root);f->pipeline_root=0;}
    int ok=f->cpu_ok;
    if(!f->injected) source_render_frame_release(f);
    return ok;
}
/* Invoke ONLY when the backend proves no pending engine/GPU use of these eyes.
   This serial prevents a stale completion from releasing a different frame.
   The event-copy adapter reconciles its frozen ticket before calling this. */
static int source_render_frame_gpu_drained(SourceRenderFrame *f,unsigned long long serial) {
    if(!f || !f->serial || f->serial!=serial || !f->root_done || !f->injected || f->busy) return 0;
    source_render_frame_release(f);return 1;
}
#endif
