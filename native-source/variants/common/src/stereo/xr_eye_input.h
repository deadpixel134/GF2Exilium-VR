#ifndef GF2_XR_EYE_INPUT_H
#define GF2_XR_EYE_INPUT_H
#include <openxr/openxr.h>
#include "eye_optics.h"
/* XR worker only; the caller owns session/space validity and synchronization.
   No WaitFrame/BeginFrame calls here: use the existing frame owner's prediction.
   Copy the completed packet through a synchronized handoff before Unity uses it.
   The packet stores raw XR coordinates, NOT a final Unity-world pose. */
typedef struct {
    unsigned valid;
    unsigned long long reference_epoch,frame_id;
    XrTime display_time;
    XrViewStateFlags flags;
    unsigned head_valid;
    XrPosef head,reference_head; /* Same LOCAL epoch; reference acquired by XR owner. */
    XrPosef poses[2];
    XrFovf fov[2]; /* Preserve unmodified optics for eventual XR submission. */
    EyeFrustumTangents tangents[2];
} XrEyeInput;

/* Double-precision standard tangent, supplied by the runtime composition.
   No approximate FOV reconstruction from a symmetric/source camera FOV. */
static int xr_eye_fov(XrFovf fov,double (*tangent)(double),EyeFrustumTangents *out) {
    if(!tangent || !out) return 0;
    double angles[4]={fov.angleLeft,fov.angleRight,fov.angleDown,fov.angleUp};
    float values[4];
    for(unsigned i=0;i<4;++i) {
        /* Perspective rays must lie strictly in the forward hemisphere;
           reject unsupported ranges rather than clamp runtime calibration. */
        if(!(angles[i]>-1.5707963267948966 && angles[i]<1.5707963267948966) ||
           !eye_optics_float(tangent(angles[i]),&values[i])) return 0;
    }
    if(!(values[1]>values[0] && values[3]>values[2])) return 0;
    *out=(EyeFrustumTangents){values[0],values[1],values[2],values[3]};return 1;
}
static int xr_eye_pose_valid(const XrPosef *pose) {
    float checked;
    if(!eye_optics_float(pose->position.x,&checked) ||
       !eye_optics_float(pose->position.y,&checked) ||
       !eye_optics_float(pose->position.z,&checked)) return 0;
    double q[4]={pose->orientation.x,pose->orientation.y,pose->orientation.z,pose->orientation.w};
    double norm=0;
    for(unsigned i=0;i<4;++i) {
        if(!eye_optics_float(q[i],&checked)) return 0;
        norm+=q[i]*q[i];
    }
    return norm>=0.999 && norm<=1.001;
}
static int xr_eye_locate(PFN_xrLocateViews locate,XrSession session,XrSpace space,
                         XrReferenceSpaceType space_type,unsigned long long epoch,
                         unsigned long long frame_id,const XrFrameState *frame,
                         double (*tangent)(double),XrEyeInput *output) {
    if(!output) return 0;
    *output=(XrEyeInput){0}; /* Failure cannot leave last-frame data valid. */
    if(!locate || !session || !space || !epoch || !frame_id || !tangent || !frame ||
       frame->type!=XR_TYPE_FRAME_STATE || !frame->shouldRender || frame->predictedDisplayTime<=0 ||
       (space_type!=XR_REFERENCE_SPACE_TYPE_LOCAL && space_type!=XR_REFERENCE_SPACE_TYPE_STAGE)) return 0;
    XrViewLocateInfo info={0};info.type=XR_TYPE_VIEW_LOCATE_INFO;
    info.viewConfigurationType=XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    info.displayTime=frame->predictedDisplayTime;info.space=space;
    XrViewState state={0};state.type=XR_TYPE_VIEW_STATE;
    XrView views[2]={0};views[0].type=views[1].type=XR_TYPE_VIEW;
    uint32_t count=0;
#ifdef GF2VR_LATENCY_OBSERVE
    LatU64 latency_before=latency_on()?latency_clock():0;
#endif
    XrResult result=locate(session,&info,&state,2,&count,views);
#ifdef GF2VR_LATENCY_OBSERVE
    LatU64 latency_after=latency_on()?latency_clock():0;
    latency_located(epoch,frame_id,info.displayTime,latency_before,latency_after,result==XR_SUCCESS && count==2);
#endif
    /* Loss-pending is success-class in XR but cannot seed new eye work here. */
    XrViewStateFlags required=XR_VIEW_STATE_ORIENTATION_VALID_BIT|XR_VIEW_STATE_POSITION_VALID_BIT;
    if(result!=XR_SUCCESS || count!=2 || (state.viewStateFlags&required)!=required) return 0;
    XrEyeInput packet={0};
    for(unsigned i=0;i<2;++i) {
        if(!xr_eye_pose_valid(&views[i].pose) || !xr_eye_fov(views[i].fov,tangent,&packet.tangents[i])) return 0;
        packet.poses[i]=views[i].pose;packet.fov[i]=views[i].fov;
    }
    packet.reference_epoch=epoch;packet.frame_id=frame_id;packet.display_time=info.displayTime;
    packet.flags=state.viewStateFlags;packet.valid=1;*output=packet;return 1;
}
static int xr_eye_matches(const XrEyeInput *p,unsigned long long epoch,
                          unsigned long long frame_id,XrTime display_time) {
    return p && p->valid && epoch && frame_id && display_time>0 &&
        p->reference_epoch==epoch && p->frame_id==frame_id && p->display_time==display_time;
}
#endif
