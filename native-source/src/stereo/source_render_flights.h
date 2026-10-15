#ifndef GF2_SOURCE_RENDER_FLIGHTS_H
#define GF2_SOURCE_RENDER_FLIGHTS_H
#include "source_eye_pool.h"
#include "eye_pose_pool.h"
/* Unity owner-thread adapter for the real SourceRenderFrame/CameraPipelineScope
   functions. Slots are address-stable; numeric index i ALWAYS selects managed
   pair i. Each slot publishes its immutable pose to its own mailbox exactly
   once, never to the live latest-frame mailbox. No XR/input/reference calls.
   The native adapter must prove child queue ordering and both GPU boundaries;
   this adapter cannot manufacture that evidence. */
typedef struct {
    EyeFlightOwner *owner;
    EyeFlightToken token;
    XrEyeMailbox mailbox;
    CameraPipelineScope scope;
    SourceRenderFrame frame;
    unsigned occupied;
#if defined(GF2VR_ASYNC_VALIDITY_OBSERVE) || defined(GF2VR_MAIN_HANDOFF_OBSERVE) || defined(GF2VR_NATIVE_BOUNDARY_OBSERVE)
    /* Main-only facts from the EXISTING current check; no second snapshot. */
    unsigned observed_current_reason,observed_current_flags;
    int observed_current_result;
#endif
#ifdef GF2VR_MAIN_HANDOFF_OBSERVE
    unsigned observed_bridge_reason;
    int observed_bridge_result;
#endif
} SourceRenderFlight;
typedef struct { SourceRenderFlight slots[EFO_CAPACITY]; } SourceRenderFlights;
#if defined(GF2VR_TWO_FLIGHT_ARM_RETRY)
#define GF2VR_ARM_READ_ATTEMPTS 4u
#else
#define GF2VR_ARM_READ_ATTEMPTS 1u
#endif
#if defined(GF2VR_SOURCE_FLIGHT_RETRY_TEST)
/* Deterministic contention fixture only; absent from shipped builds. */
static void (*source_flight_retry_test)(unsigned stage,unsigned attempt);
#define GF2VR_ARM_READ_BUSY(stage,attempt) do { \
    if(source_flight_retry_test)source_flight_retry_test(stage,attempt); \
} while(0)
#else
#define GF2VR_ARM_READ_BUSY(stage,attempt) ((void)0)
#endif
static int source_render_flight_production_live(const EyeFlightSnapshot *n) {
    if(n->flags&(EFF_CANCELLED|EFF_FAULT|EFF_RECLAIMING|EFF_PRESENTATION_RETIRED))return 0;
    if(!(n->flags&EFF_XR_ENDED))return 1;
#ifdef GF2VR_ASYNC_PRESENTATION
    /* Only the explicitly retained, excluded-bank producer contract survives
       End before CPU preparation. Ordinary End never becomes render admission. */
    unsigned retained=EFF_DISPLAY_DETACHED|EFF_PRODUCER_RETAINED;
    return (n->flags&retained)==retained;
#else
    return 0;
#endif
}
static int source_render_flight_current(void *context) {
    SourceRenderFlight *s=context;EyeFlightSnapshot n;
#if defined(GF2VR_ASYNC_VALIDITY_OBSERVE) || defined(GF2VR_MAIN_HANDOFF_OBSERVE) || defined(GF2VR_NATIVE_BOUNDARY_OBSERVE)
    unsigned reason=0;int result=EFO_REJECTED;unsigned flags=0;
    if(!s || !s->occupied || !s->owner)reason=1;
    else {
        result=eye_flight_snapshot(s->owner,s->token,&n);
        if(result!=EFO_OK)reason=2;
        else {
            flags=n.flags;
            if(!source_render_flight_production_live(&n))reason=3;
            else if(n.ticket.serial!=s->frame.serial)reason=4;
            else if(!source_render_frame_current(&s->frame))reason=5;
        }
    }
    if(s && reason && !s->observed_current_reason) {
        s->observed_current_reason=reason;s->observed_current_flags=flags;
        s->observed_current_result=result;
    }
    return reason==0;
#else
    return s && s->occupied && s->owner &&
        eye_flight_snapshot(s->owner,s->token,&n)==EFO_OK &&
        source_render_flight_production_live(&n) &&
        n.ticket.serial==s->frame.serial && source_render_frame_current(&s->frame);
#endif
}
/* Caller reserves the numeric render ticket first, prepares its fixed managed
   pair and calls here inside the selected real root. Failure never returns an
   injected borrow; finish still closes a root that has begun. */
static int source_render_flight_arm(SourceRenderFlights *flights,SourceEyePool *pool,
    EyeFlightOwner *owner,EyePosePool *poses,EyeFlightToken token,unsigned long thread,
    SourcePoseApi *reader,SourceAnchorState *anchor,PoseMathBinding *math,
    const CameraBatchApi *batch,void *pipeline,void *method) {
    GF2VR_PREPARE_STEP(300);
    if(!flights || !pool || !pool->source || !thread || thread!=pool->source->owner_thread)
        return EFO_REJECTED;
    GF2VR_PREPARE_STEP(301);
    /* Retry only side-effect-free metadata reads. Never replay a managed arm,
       reserve a new ticket, or weaken stale/cancelled/reference validation.
       Persistent contention still returns BUSY without creating a borrow. */
    unsigned index;int result=EFO_BUSY;
    for(unsigned attempt=1;attempt<=GF2VR_ARM_READ_ATTEMPTS;++attempt) {
        result=eye_flight_slot(owner,token,&index);
        if(result!=EFO_BUSY)break;
        GF2VR_ARM_READ_BUSY(301,attempt);
    }
    if(result!=EFO_OK)return result;
    GF2VR_PREPARE_STEP(302);
    SourceRenderFlight *s=&flights->slots[index];
    if(s->occupied || s->frame.serial || s->scope.render.active)return EFO_REJECTED;
    GF2VR_PREPARE_STEP(303);
    EyeFlightSnapshot n;XrEyeInput pose;
    for(unsigned attempt=1;attempt<=GF2VR_ARM_READ_ATTEMPTS;++attempt) {
        result=eye_pose_pool_read(poses,owner,token,&n,&pose);
        if(result!=EFO_BUSY)break;
        GF2VR_ARM_READ_BUSY(303,attempt);
    }
    if(result!=EFO_OK)return result;
    GF2VR_PREPARE_STEP(304);
    unsigned arm_flags=EFF_RESERVED|EFF_XR_BEGUN;
#ifdef GF2VR_ASYNC_PRESENTATION
    if((n.flags&(EFF_XR_ENDED|EFF_DISPLAY_DETACHED|EFF_PRODUCER_RETAINED))==
       (EFF_XR_ENDED|EFF_DISPLAY_DETACHED|EFF_PRODUCER_RETAINED))
        arm_flags|=EFF_XR_ENDED|EFF_DISPLAY_DETACHED|EFF_PRODUCER_RETAINED;
#endif
    if(!source_render_flight_production_live(&n) || !(n.flags&EFF_RESERVED) || (n.flags&~arm_flags) ||
       !n.ticket.serial || token.generation!=pool->source->selection.generation ||
       !pool->source->selection.eligible || !pool->data[index].ready ||
       pool->data[index].retiring || pool->eyes[index].borrowed)return EFO_REJECTED;
    if(s->scope.owner_thread && s->scope.owner_thread!=thread)return EFO_REJECTED;
    GF2VR_PREPARE_STEP(305);
    s->scope.owner_thread=thread;
    GF2VR_PREPARE_STEP(306);
    if(!camera_pipeline_begin(&s->scope,thread,n.ticket.serial))return EFO_REJECTED;
    s->owner=owner;s->token=token;s->occupied=1;
    GF2VR_PREPARE_STEP(307);
    if(!xr_eye_mailbox_publish(&s->mailbox,&pose))return EFO_REJECTED;
    GF2VR_PREPARE_STEP(308);
#ifdef GF2VR_ACTIVITY_STACK
    if(!source_render_frame_arm_with(&s->frame,&pool->data[index],&pool->source->selection,
#else
    if(!source_render_frame_arm(&s->frame,&pool->data[index],&pool->source->selection,
#endif
        reader,anchor,math,&s->mailbox,&s->scope,thread,batch,pipeline,method
#ifdef GF2VR_ACTIVITY_STACK
        ,pool->data[index].group?&pool->groups[index].frame_api:0
#endif
        ))return EFO_REJECTED;
    /* Include cancellation/reference ownership in BOTH checks around Camera[]
       construction. Reentrant cancellation cannot inject the cached packet. */
    s->scope.current=source_render_flight_current;s->scope.check_owner=s;
    GF2VR_PREPARE_STEP(309);
    return source_render_flight_current(s)?EFO_OK:EFO_REJECTED;
}
static void source_render_flight_forward(SourceRenderFlight *s,unsigned long thread,
    CameraRenderForward original,void *pipeline,void *context,void *cameras,void *method) {
    camera_pipeline_forward(&s->scope,thread,original,pipeline,context,cameras,method);
}
/* CPU status only. Backend must acknowledge numeric CPU return, and issue a
   validated private tail for injected work even when this returns false. */
static int source_render_flight_finish(SourceRenderFlight *s,unsigned long thread,
    unsigned long long serial,int success) {
    if(!s || !thread || thread!=s->scope.owner_thread)return 0;
    if(s->frame.armed)return source_render_frame_finish(&s->frame,thread,serial,success);
    return camera_pipeline_end(&s->scope,thread,serial,success);
}
/* AFTER numeric take and actual native backend cleanup, BEFORE pose-pool
   retirement acknowledgement. No numeric reuse acknowledgement here. */
static int source_render_flight_release(SourceRenderFlights *f,EyeFlightOwner *owner,
    EyeFlightToken token,unsigned long thread) {
    if(!f || !owner)return EFO_REJECTED;
    unsigned index;int result=eye_flight_slot(owner,token,&index);
    if(result!=EFO_OK)return result;
    SourceRenderFlight *s=&f->slots[index];EyeFlightSnapshot n;
    result=eye_flight_snapshot(owner,token,&n);if(result!=EFO_OK)return result;
    if(!(n.flags&EFF_RECLAIMING))return EFO_REJECTED;
    if(!thread || (s->scope.owner_thread && thread!=s->scope.owner_thread))return EFO_REJECTED;
    /* A flight that never armed has no managed payload to clean. */
    if(!s->occupied)return s->frame.serial || s->scope.render.active || s->scope.armed?EFO_REJECTED:EFO_OK;
    if(s->owner!=owner ||
       s->token.frame!=token.frame || s->token.generation!=token.generation || s->token.epoch!=token.epoch ||
       s->scope.render.active || s->scope.armed)return EFO_REJECTED;
    if(s->frame.serial && !source_render_frame_gpu_drained(&s->frame,n.ticket.serial))return EFO_REJECTED;
    xr_eye_mailbox_invalidate(&s->mailbox);
    s->occupied=0;s->owner=0;s->token=(EyeFlightToken){0};return EFO_OK;
}
#endif
