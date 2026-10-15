#ifndef GF2_SOURCE_POSE_APPLY_H
#define GF2_SOURCE_POSE_APPLY_H
#include "source_pose_reader.h"
#include "source_anchor.h"
#ifdef GF2VR_CAMERA_FOLLOW_TOGGLE
#include "camera_follow_source.h"
#endif
#include "pose_math_binding.h"
/* Unity-main-thread only. The actual producer owns/validates source identity
   and generation, and retires this anchor on source replacement. */
static int source_pose_apply(const SourcePoseApi *reader,void *source,
                             unsigned long long generation,float units_per_meter,
                             SourceAnchorState *anchor,PoseMathBinding *math,
                             XrEyeMailbox *mailbox,const ManagedEyeApi *api,ManagedEyes *eyes,
                             PoseAppliedTicket *ticket) {
    GF2VR_PREPARE_STEP(500);
    if(!ticket) return 0;
    *ticket=(PoseAppliedTicket){0};
    GF2VR_PREPARE_STEP(501);
    if(!eyes || eyes->borrowed) return 0;
    eyes->matrices_ready=0;
    GF2VR_PREPARE_STEP(502);
    if(!anchor || !math || !math->module || math->borrowed || !generation) return 0;
    GF2VR_PREPARE_STEP(503);
    if(anchor->anchor.source_generation && !source_anchor_current(anchor,generation,units_per_meter)) return 0;
    SourcePoseSnapshot snapshot;
    math->borrowed=1; /* Managed getters may reenter; keep CRT resident. */
    GF2VR_PREPARE_STEP(504);
    int ok=source_pose_read(reader,source,&snapshot);
    if(ok && !anchor->anchor.source_generation)
        ok=(GF2VR_PREPARE_STEP(505),source_anchor_capture(anchor,&math->api,generation,&snapshot.pose,units_per_meter));
#ifdef GF2VR_MAIN_CAMERA_FOLLOW
#ifdef GF2VR_CAMERA_FOLLOW_TOGGLE
    if(ok){int returning=stereo_camera_follow_prepare(mailbox,generation);
        ok=camera_follow_source(&stereo_camera_follow_state,anchor,&math->api,generation,&snapshot.pose,units_per_meter,returning);}
#else
    if(ok) ok=source_anchor_follow(anchor,&math->api,generation,&snapshot.pose,units_per_meter);
#endif
#endif
    math->borrowed=0;
    if(!ok) return 0;
#ifdef GF2VR_PLACEMENT_SOURCE
    GF2VR_PLACEMENT_SOURCE(source,generation,&snapshot,&anchor->anchor);
#endif
    GF2VR_PREPARE_STEP(506);
#ifdef GF2VR_PLACEMENT_APPLIED
    int applied=pose_math_apply(math,mailbox,&anchor->anchor,api,eyes,snapshot.near_z,snapshot.far_z,ticket);
    GF2VR_PLACEMENT_APPLIED(ticket,applied);
    return applied;
#else
    return pose_math_apply(math,mailbox,&anchor->anchor,api,eyes,snapshot.near_z,snapshot.far_z,ticket);
#endif
}
#endif
