#ifndef GF2_XR_EYE_OWNER_H
#define GF2_XR_EYE_OWNER_H
#include "xr_eye_input.h"
#ifdef GF2VR_AUTO_STEREO
#include "xr_level_reference.h"
#endif
/* One XR worker owns these handles. Camera/source generations never call open
   or close. Close only after all users of the XR space/session have drained. */
typedef struct {
    PFN_xrCreateReferenceSpace create;
    PFN_xrDestroySpace destroy;
    PFN_xrLocateViews locate;
    PFN_xrLocateSpace locate_space;
    double (*tangent)(double);
} XrEyeOwnerApi;
typedef struct {
    XrEyeOwnerApi api;
    XrSession session;
    XrSpace space,head_space;
    unsigned long long serial,epoch,last_frame;
    XrTime change_time;
    unsigned changing,retiring;
    unsigned require_calibration,calibration_request;
#ifdef GF2VR_AUTO_STEREO
    unsigned level_initial_reference;
#endif
    unsigned long long head_reference_epoch;
    XrPosef head_reference;
} XrEyeOwner;

static int xr_eye_owner_bind(PFN_xrGetInstanceProcAddr get,XrInstance instance,
                             double (*tangent)(double),XrEyeOwnerApi *out) {
    if(!out) return 0;
    *out=(XrEyeOwnerApi){0};
    if(!get || !instance || !tangent) return 0;
    XrEyeOwnerApi api={0};PFN_xrVoidFunction fn=0;
    if(get(instance,"xrCreateReferenceSpace",&fn)!=XR_SUCCESS || !fn) return 0;
    api.create=(PFN_xrCreateReferenceSpace)fn;fn=0;
    if(get(instance,"xrDestroySpace",&fn)!=XR_SUCCESS || !fn) return 0;
    api.destroy=(PFN_xrDestroySpace)fn;fn=0;
    if(get(instance,"xrLocateViews",&fn)!=XR_SUCCESS || !fn) return 0;
    api.locate=(PFN_xrLocateViews)fn;fn=0;
    if(get(instance,"xrLocateSpace",&fn)!=XR_SUCCESS || !fn) return 0;
    api.locate_space=(PFN_xrLocateSpace)fn;api.tangent=tangent;*out=api;return 1;
}
static int xr_eye_owner_open(XrEyeOwner *o,const XrEyeOwnerApi *api,XrSession session) {
    if(!o || !api || !session || o->space || o->head_space || o->session || o->retiring ||
       !api->create || !api->destroy || !api->locate || !api->locate_space || !api->tangent ||
       o->serial==~0ull) return 0;
    XrReferenceSpaceCreateInfo info={0};info.type=XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
    info.referenceSpaceType=XR_REFERENCE_SPACE_TYPE_LOCAL;info.poseInReferenceSpace.orientation.w=1;
    XrSpace space=XR_NULL_HANDLE;
    XrResult result=api->create(session,&info,&space);
    /* Even a success-class loss-pending result may have allocated a space.
       Retain it for explicit cleanup, but never expose it for new eye work. */
    if(XR_SUCCEEDED(result) && space) {
        o->api=*api;o->session=session;o->space=space;
        o->last_frame=0;o->change_time=0;o->changing=0;
        if(result!=XR_SUCCESS) {o->retiring=1;return 0;}
        info.referenceSpaceType=XR_REFERENCE_SPACE_TYPE_VIEW;
        XrSpace head=XR_NULL_HANDLE;
        result=api->create(session,&info,&head);
        if(XR_SUCCEEDED(result)) o->head_space=head;
        if(result!=XR_SUCCESS || !head) {o->retiring=1;return 0;}
        o->head_reference_epoch=0;o->epoch=++o->serial;return 1;
    }
    return 0;
}
static int xr_eye_owner_close(XrEyeOwner *o) {
    if(!o) return 0;
    o->epoch=0;o->retiring=1;
    if(o->head_space) {
        if(XR_FAILED(o->api.destroy(o->head_space))) return 0;
        o->head_space=XR_NULL_HANDLE;
    }
    if(o->space && XR_FAILED(o->api.destroy(o->space))) return 0;
    /* Keep serial across session recreation to prevent handle/token reuse. */
    unsigned long long serial=o->serial;
    *o=(XrEyeOwner){0};o->serial=serial;return 1;
}
static int xr_eye_owner_change(XrEyeOwner *o,const XrEventDataReferenceSpaceChangePending *event) {
    if(!o || !event || !o->space || o->retiring ||
       event->type!=XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING ||
       event->session!=o->session || event->referenceSpaceType!=XR_REFERENCE_SPACE_TYPE_LOCAL) return 0;
    /* Conservative fallback from notification until the latest announced
       change is effective. No frame-time HMD neutral is sampled. The future
       pose composer must treat the next epoch as an actual origin change. */
    o->epoch=0;o->calibration_request=0;
    if(!o->changing || event->changeTime>o->change_time) o->change_time=event->changeTime;
    o->changing=1;return 1;
}
static int xr_eye_owner_capture(XrEyeOwner *o,unsigned long long frame_id,
                                const XrFrameState *frame,XrEyeInput *out) {
    /* Confirmation is for this attempt only; an invalid frame must not defer
       calibration until the user has moved to an unrelated posture. */
    unsigned confirmed=o?o->calibration_request:0;
    if(o) o->calibration_request=0;
    if(!out) return 0;
    *out=(XrEyeInput){0};
    if(!o || !o->space || o->retiring || !frame || frame->type!=XR_TYPE_FRAME_STATE ||
       !frame_id || frame_id<=o->last_frame) return 0;
    o->last_frame=frame_id;
    if(o->changing) {
        if(frame->predictedDisplayTime<o->change_time || !frame->shouldRender ||
           frame->predictedDisplayTime<=0 || o->serial==~0ull) return 0;
        o->epoch=++o->serial;o->changing=0;
    }
    if(!o->head_space) return 0;
    if(o->require_calibration && o->head_reference_epoch!=o->epoch && !confirmed) return 0;
    XrEyeInput packet={0};
    if(!xr_eye_locate(o->api.locate,o->session,o->space,XR_REFERENCE_SPACE_TYPE_LOCAL,
                     o->epoch,frame_id,frame,o->api.tangent,&packet)) return 0;
    XrSpaceLocation head={0};head.type=XR_TYPE_SPACE_LOCATION;
    XrSpaceLocationFlags required=XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
    if(o->api.locate_space(o->head_space,o->space,packet.display_time,&head)!=XR_SUCCESS ||
       (head.locationFlags&required)!=required || !xr_eye_pose_valid(&head.pose)) return 0;
    /* Deliberate XR activation/recovery boundary, independent of any game source.
       Ordinary invalid frames, pause/resume and camera changes never reseed. */
    if(o->head_reference_epoch!=o->epoch) {
        XrSpaceLocationFlags tracked=XR_SPACE_LOCATION_POSITION_TRACKED_BIT|XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT;
        if(o->require_calibration && (head.locationFlags&tracked)!=tracked) return 0;
        XrPosef reference=head.pose;
#ifdef GF2VR_AUTO_STEREO
        if(o->level_initial_reference && !xr_level_reference(&head.pose,&reference))return 0;
#endif
        o->head_reference=reference;o->head_reference_epoch=o->epoch;
    }
    packet.head=head.pose;packet.reference_head=o->head_reference;packet.head_valid=1;
#ifdef GF2VR_TOUCH_PRIVATE
    touch_p2_capture(o,&packet);
#endif
    *out=packet;return 1;
#line 130
}
/* XR worker only. A calibrated origin cannot be overwritten by this startup
   confirmation; actual recenter is a separate product action. */
static int xr_eye_owner_confirm_initial(XrEyeOwner *o) {
    if(!o || !o->require_calibration || !o->session || !o->space || !o->epoch ||
       o->changing || o->retiring || o->head_reference_epoch==o->epoch) return 0;
    o->calibration_request=1;return 1;
}
#endif
