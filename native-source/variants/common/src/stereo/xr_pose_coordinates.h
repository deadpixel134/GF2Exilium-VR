#ifndef GF2_XR_POSE_COORDINATES_H
#define GF2_XR_POSE_COORDINATES_H
#include "xr_eye_input.h"
/* Coordinate conversion only, not source/neutral pose composition.
   Reflect Z: position S*p, rotation S*R*S with S=diag(1,1,-1).
   This preserves runtime calibration/cant; no symmetric-IPD reconstruction. */
static int xr_pose_to_unity(const XrPosef *xr,ManagedEyePose *unity) {
    if(!xr || !unity || !xr_eye_pose_valid(xr)) return 0;
    *unity=(ManagedEyePose){{xr->position.x,xr->position.y,-xr->position.z},
                          {-xr->orientation.x,-xr->orientation.y,xr->orientation.z,xr->orientation.w}};
    return 1;
}
typedef struct {
    ManagedEyePose head,reference_head,eyes[2];
    EyeFrustumTangents tangents[2];
    unsigned long long reference_epoch,frame_id;
    XrTime display_time;
} UnityTrackingInput;
static int xr_eye_unity_tracking(const XrEyeInput *packet,UnityTrackingInput *out) {
    if(!out) return 0;
    *out=(UnityTrackingInput){0};
    if(!packet || !packet->head_valid ||
       !xr_eye_matches(packet,packet->reference_epoch,packet->frame_id,packet->display_time)) return 0;
    UnityTrackingInput input={0};
    if(!xr_pose_to_unity(&packet->head,&input.head) ||
       !xr_pose_to_unity(&packet->reference_head,&input.reference_head)) return 0;
    for(unsigned i=0;i<2;++i) {
        if(!xr_pose_to_unity(&packet->poses[i],&input.eyes[i])) return 0;
        input.tangents[i]=packet->tangents[i];
    }
    input.reference_epoch=packet->reference_epoch;input.frame_id=packet->frame_id;
    input.display_time=packet->display_time;*out=input;return 1;
}
#endif
