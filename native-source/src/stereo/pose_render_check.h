#ifndef GF2_POSE_RENDER_CHECK_H
#define GF2_POSE_RENDER_CHECK_H
#include "pose_apply.h"
/* Stack-owned for one synchronous camera_render_forward_checked call. Source
   generation and eyes are Unity-owner-thread state; only mailbox is shared.
   Copy the applied ticket before borrowing; never point at mutable next-frame
   scratch. The producer still owns camera eligibility and GPU drain. */
typedef struct {
    const XrEyeMailbox *mailbox;
    PoseAppliedTicket ticket;
    const ManagedEyes *eyes;
    const unsigned long long *source_generation;
#if defined(GF2VR_MAIN_HANDOFF_OBSERVE) || defined(GF2VR_NATIVE_BOUNDARY_OBSERVE)
    unsigned observed_reason; /* Owner-thread; first failed existing predicate. */
#endif
} PoseRenderCheck;
static int pose_render_current(void *owner) {
#if defined(GF2VR_MAIN_HANDOFF_OBSERVE) || defined(GF2VR_NATIVE_BOUNDARY_OBSERVE)
    PoseRenderCheck *check=owner;unsigned reason=0;unsigned long long generation=0;
    if(!check)return 0;
    if(!check->eyes || !check->source_generation || !check->mailbox)reason=1;
    else if(!check->eyes->borrowed)reason=2;
    else if(!check->eyes->ready)reason=3;
    else if(!check->eyes->targets_ready)reason=4;
    else if(check->eyes->matrix_frame!=check->ticket.frame_id)reason=5;
    else if(!(generation=*check->source_generation) || check->ticket.source_generation!=generation)reason=6;
    else if(check->ticket.display_time<=0)reason=7;
    else if(!check->ticket.reference_epoch || !check->ticket.frame_id)reason=8;
    else if(__atomic_load_n(&check->mailbox->epoch,__ATOMIC_ACQUIRE)!=check->ticket.reference_epoch)reason=9;
    else if(__atomic_load_n(&check->mailbox->frame,__ATOMIC_ACQUIRE)!=check->ticket.frame_id)reason=10;
    if(reason && !check->observed_reason)check->observed_reason=reason;
    return reason==0;
#else
    const PoseRenderCheck *check=owner;
    return check && check->eyes && check->source_generation &&
        check->eyes->borrowed && check->eyes->ready && check->eyes->targets_ready &&
        check->eyes->matrix_frame==check->ticket.frame_id &&
        pose_ticket_current(check->mailbox,&check->ticket,*check->source_generation);
#endif
}
#endif
