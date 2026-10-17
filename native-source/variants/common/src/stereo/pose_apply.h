#ifndef GF2_POSE_APPLY_H
#define GF2_POSE_APPLY_H
#include "pose_composition.h"
#include "xr_eye_mailbox.h"
/* Unity-main-thread consumer. Caller owns an already-prepared eye generation,
   a generation-stable source anchor and lifetime-safe math function pointers.
   Returned ticket is not CPU-render/eye-freshness/GPU-completion evidence. */
typedef struct {
    unsigned long long reference_epoch,frame_id,source_generation;
    XrTime display_time;
} PoseAppliedTicket;
#ifdef GF2VR_MAIN_HANDOFF_OBSERVE
/* The observed pose check expands these same read-only predicates for reasons. */
__attribute__((unused))
#endif
static int pose_ticket_current(const XrEyeMailbox *mailbox,const PoseAppliedTicket *ticket,
                                unsigned long long current_source_generation) {
    if(!mailbox || !ticket || !current_source_generation ||
       ticket->source_generation!=current_source_generation || ticket->display_time<=0) return 0;
    XrEyeInput packet={0};packet.valid=1;packet.reference_epoch=ticket->reference_epoch;
    packet.frame_id=ticket->frame_id;
    return xr_eye_mailbox_current(mailbox,&packet);
}
static int pose_apply(const PoseMath *math,XrEyeMailbox *mailbox,const PoseSourceAnchor *anchor,
                       const ManagedEyeApi *api,ManagedEyes *eyes,float near_z,float far_z,
                       PoseAppliedTicket *ticket) {
    GF2VR_PREPARE_STEP(520);
    if(!ticket) return 0;
    *ticket=(PoseAppliedTicket){0};
    if(!eyes || eyes->borrowed) return 0;
    eyes->matrices_ready=0;
    if(!mailbox || !anchor || !api) return 0;
    XrEyeInput packet;UnityTrackingInput tracking;ManagedEyePose poses[2];
    GF2VR_PREPARE_STEP(521);
    if(!xr_eye_mailbox_read(mailbox,&packet) ||
       (GF2VR_PREPARE_STEP(522),!xr_eye_unity_tracking(&packet,&tracking)) ||
       (GF2VR_PREPARE_STEP(523),!pose_compose(math,&tracking,anchor,poses)) ||
       (GF2VR_PREPARE_STEP(524),!xr_eye_mailbox_current(mailbox,&packet))) return 0;
    GF2VR_PREPARE_STEP(525);
    if(!managed_eyes_optics(api,eyes,packet.frame_id,poses,tracking.tangents,near_z,far_z)) return 0;
    /* Managed invocation may overlap EndFrame/reference retirement. Partial
       application remains private; never permit a borrow of its old matrices. */
    GF2VR_PREPARE_STEP(526);
    if(!xr_eye_mailbox_current(mailbox,&packet)) {eyes->matrices_ready=0;return 0;}
    *ticket=(PoseAppliedTicket){packet.reference_epoch,packet.frame_id,anchor->source_generation,packet.display_time};
#ifdef GF2VR_NAVIGATION_BLOCK_APPLIED
    GF2VR_NAVIGATION_BLOCK_APPLIED(&tracking,anchor,poses,ticket);
#endif
#ifdef GF2VR_STEREO_DEPTH_APPLIED
    GF2VR_STEREO_DEPTH_APPLIED(&tracking,anchor,poses,ticket);
#endif
#ifdef GF2VR_POSE_OBSERVE
    GF2VR_POSE_OBSERVE(&packet,anchor);
#endif
    return 1;
}
#endif
