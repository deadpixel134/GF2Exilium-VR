#ifndef GF2_CAMERA_RETURN_RUNTIME_H
#define GF2_CAMERA_RETURN_RUNTIME_H
#include "camera_return_policy.h"
#ifdef GF2VR_CAMERA_FOLLOW_TOGGLE
#ifdef GF2VR_CAMERA_FOLLOW_CONFIG
#include "camera_follow_config.h"
static unsigned stereo_camera_follow_config_loaded;
#endif
/* Main source-update boundary: select follow before composing either eye.
   LEFT refreshes the frozen base once, so return does not leave an artificial
   base-difference offset that would be counted again on subsequent ON. */
static int stereo_camera_follow_prepare(const void *mailbox,U64 generation){
#ifdef GF2VR_CAMERA_FOLLOW_CONFIG
    unsigned config=camera_follow_config_load(&stereo_camera_follow_config_loaded,&stereo_camera_follow_state.disabled,GF2VR_NAVIGATION_CONFIG);
    if(config){DWORD saved=GetLastError();event(config==3?"camera_follow_config_invalid_default_on":"camera_follow_config_ready",config,GetCurrentThreadId());SetLastError(saved);}
#endif
    if(!mailbox||!generation)return 0;
    XrEyeInput packet={0};UnityTrackingInput tracking;DWORD saved=GetLastError();U64 now=GetTickCount64();
    if(!xr_eye_mailbox_read((XrEyeMailbox *)mailbox,&packet)||!xr_eye_unity_tracking(&packet,&tracking)){
        SetLastError(saved);return 0;
    }
    unsigned toggle=camera_follow_take(&stereo_camera_follow,packet.reference_epoch,generation,now,packet.display_time);
    int returning=0;
    if(stereo_world_drag_game_focused()){
        if(toggle){stereo_camera_follow_state.disabled^=1;event("camera_follow_enabled",!stereo_camera_follow_state.disabled,GetCurrentThreadId());}
        returning=camera_follow_return_pending(&stereo_camera_return,packet.reference_epoch,generation,now,packet.display_time);
    }
    SetLastError(saved);return returning;
}
#endif
/* Called only on the existing XR actual-End route, independently of whether
   stick movement is inhibited by an active WorldDrag/UI gesture. */
static int stereo_camera_return_end(int eligible,U64 epoch,U64 generation,U64 tick,U64 sample){
    const NavigationSample *n=&stereo_aux.navigation.frame;
    int active=n->camera_return_active&&n->tracked[0]&&n->tick_ms>=tick&&n->tick_ms-tick<=250&&(U64)n->display_time==sample;
#ifdef GF2VR_CAMERA_FOLLOW_TOGGLE
    int follow_active=n->camera_follow_active&&n->tracked[1]&&n->tick_ms>=tick&&n->tick_ms-tick<=250&&(U64)n->display_time==sample;
    camera_follow_publish(&stereo_camera_follow,eligible,follow_active,n->camera_follow_down,epoch,generation,n->tick_ms,sample);
#endif
    if(!camera_return_publish(&stereo_camera_return,eligible,active,n->camera_return_down,epoch,generation,n->tick_ms,sample))return 0;
    stereo_world_drag_reset(); /* XR-owned latch; held Grip requires release. */
    DWORD saved=GetLastError();event("camera_return_requested",stereo_camera_return.serial,GetCurrentThreadId());SetLastError(saved);return 1;
}
/* Main owns accumulated navigation. Read the actual currently validated source
   pose and physical deltas; consume the command only at/after its eye time. */
static int stereo_camera_return_pose(const void *math,const void *input,const void *anchor,
    const double *physical,double source_yaw,double reference_yaw,const void *settings,const void *raw,U64 now){
    const PoseMath *m=math;const UnityTrackingInput *i=input;const PoseSourceAnchor *a=anchor;
    const NavigationSettings *cfg=settings;const NavigationSample *sample=raw;
    CameraReturnCommand c=camera_return_take(&stereo_camera_return,i->reference_epoch,a->source_generation,now,i->display_time);
    if(!c.serial)return 1;
    if(!stereo_world_drag_game_focused())return 1; /* consume, never replay on refocus */
    double angles[2],offset[3],scale=1;
#ifdef GF2VR_WORLD_SCALE
    scale=stereo_world_head_scale();
#endif
    if(!camera_return_target(m,i,a,physical,source_yaw,reference_yaw,scale,angles,offset)){
        DWORD saved=GetLastError();event("camera_return_rejected_geometry",c.serial,GetCurrentThreadId());SetLastError(saved);return 0;
    }
    NavigationState next={0};next.epoch=i->reference_epoch;next.generation=a->source_generation;
    next.settings=*cfg;next.settings_valid=1;next.sequence=sample->sequence;
    next.yaw=angles[0];next.pitch=angles[1];for(unsigned n=0;n<3;++n)next.offset[n]=offset[n];
    stereo_stick_navigation=next;
    stereo_navigation.epoch=i->reference_epoch;stereo_navigation.generation=a->source_generation;stereo_navigation.offset=(WorldDragVector){0};
    world_drag_handoff_clear(&stereo_drag_handoff); /* shared POD, no XR state access */
    DWORD saved=GetLastError();event("camera_return_composed",c.serial,GetCurrentThreadId());SetLastError(saved);
    return 1;
}
#endif
