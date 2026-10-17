#ifndef GF2_WORLD_DRAG_ACTIONS_H
#define GF2_WORLD_DRAG_ACTIONS_H
#include "world_drag.h"
/* Child resources of the existing AuxPanel action set. Never attaches/syncs
   separately. LOCAL space is borrowed from eye owner, not recreated here. */
typedef struct {
    PFN_xrCreateAction create;PFN_xrStringToPath path;PFN_xrCreateActionSpace space;
    PFN_xrDestroySpace destroy;PFN_xrGetActionStateFloat analog;
    PFN_xrGetActionStatePose pose;PFN_xrLocateSpace locate;
} WorldDragActionApi;
typedef struct {
    WorldDragActionApi api;XrSession session;XrAction grip_pose,grip,trigger;
    XrPath hand[2];XrSpace spaces[2];unsigned ready;WorldDragFrame frame;
} WorldDragActions;
static int world_drag_actions_close(WorldDragActions *a) {
    a->ready=0;a->frame=(WorldDragFrame){0};
    for(unsigned h=0;h<2;++h)if(a->spaces[h]) {
        if(!a->api.destroy||XR_FAILED(a->api.destroy(a->spaces[h])))return 0;
        a->spaces[h]=XR_NULL_HANDLE;
    }
    *a=(WorldDragActions){0};return 1; /* parent destroys child actions */
}
static void world_drag_text(char *out,const char *in) {while((*out++=*in++)) {}}
static int world_drag_actions_create(WorldDragActions *a,WorldDragActionApi api,XrInstance instance,XrSession session,XrActionSet set) {
    if(a->session||a->spaces[0]||a->spaces[1]||!api.create||!api.path||!api.space||!api.destroy||!api.analog||!api.pose||!api.locate)return 0;
    a->api=api;a->session=session;
    if(api.path(instance,"/user/hand/left",&a->hand[0])!=XR_SUCCESS||api.path(instance,"/user/hand/right",&a->hand[1])!=XR_SUCCESS)return 0;
    XrActionCreateInfo c={0};c.type=XR_TYPE_ACTION_CREATE_INFO;c.countSubactionPaths=2;c.subactionPaths=a->hand;
    c.actionType=XR_ACTION_TYPE_POSE_INPUT;world_drag_text(c.actionName,"world_drag_hand");world_drag_text(c.localizedActionName,"World drag hand");
    if(api.create(set,&c,&a->grip_pose)!=XR_SUCCESS)return 0;
    c.actionType=XR_ACTION_TYPE_FLOAT_INPUT;world_drag_text(c.actionName,"world_drag_grip");world_drag_text(c.localizedActionName,"World drag grip");
    if(api.create(set,&c,&a->grip)!=XR_SUCCESS)return 0;
    world_drag_text(c.actionName,"world_drag_trigger");world_drag_text(c.localizedActionName,"World drag trigger");
    if(api.create(set,&c,&a->trigger)!=XR_SUCCESS)return 0;
    for(unsigned h=0;h<2;++h) {
        XrActionSpaceCreateInfo s={0};s.type=XR_TYPE_ACTION_SPACE_CREATE_INFO;s.action=a->grip_pose;s.subactionPath=a->hand[h];s.poseInActionSpace.orientation.w=1;
        XrResult r=api.space(session,&s,&a->spaces[h]);if(r!=XR_SUCCESS||!a->spaces[h])return 0;
    }
    a->ready=1;return 1;
}
static unsigned world_drag_actions_bindings(WorldDragActions *a,XrInstance instance,unsigned profile,XrActionSuggestedBinding *bindings,unsigned capacity) {
    if(!a->ready||!bindings||capacity<6||profile>3)return 0;
    const char *paths[6]={"/user/hand/left/input/grip/pose","/user/hand/left/input/squeeze/value","/user/hand/left/input/trigger/value",
        "/user/hand/right/input/grip/pose","/user/hand/right/input/squeeze/value","/user/hand/right/input/trigger/value"};
    if(profile>=2) {paths[1]="/user/hand/left/input/squeeze/click";paths[4]="/user/hand/right/input/squeeze/click";}
    for(unsigned i=0;i<6;++i) {
        XrPath p=0;if(a->api.path(instance,paths[i],&p)!=XR_SUCCESS)return 0;
        bindings[i]=(XrActionSuggestedBinding){i%3==0?a->grip_pose:i%3==1?a->grip:a->trigger,p};
    }
    return 6;
}
static int world_drag_actions_sample(WorldDragActions *a,XrSpace local,XrReferenceSpaceType type,XrTime time,unsigned long long now) {
    a->frame=(WorldDragFrame){0};a->frame.time_ms=now;
    if(!a->ready||!local||time<=0||(type!=XR_REFERENCE_SPACE_TYPE_LOCAL&&type!=XR_REFERENCE_SPACE_TYPE_STAGE))return 0;
    a->frame.sample_id=(unsigned long long)time;
    for(unsigned h=0;h<2;++h) {
        XrActionStateGetInfo get={0};get.type=XR_TYPE_ACTION_STATE_GET_INFO;get.subactionPath=a->hand[h];
        for(unsigned button=0;button<2;++button) {
            XrActionStateFloat f={0};f.type=XR_TYPE_ACTION_STATE_FLOAT;get.action=button?a->trigger:a->grip;
            if(a->api.analog(a->session,&get,&f)!=XR_SUCCESS)continue;
            a->frame.active[h*2+button]=f.isActive!=0;
            if(f.isActive)a->frame.values[h*2+button]=world_drag_finite(f.currentState)?f.currentState:0;
        }
        XrActionStatePose p={0};p.type=XR_TYPE_ACTION_STATE_POSE;get.action=a->grip_pose;
        XrSpaceLocation location={0};location.type=XR_TYPE_SPACE_LOCATION;
        if(a->api.pose(a->session,&get,&p)!=XR_SUCCESS||!p.isActive||
           a->api.locate(a->spaces[h],local,time,&location)!=XR_SUCCESS||
           (location.locationFlags&(XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_POSITION_TRACKED_BIT))!=
                (XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_POSITION_TRACKED_BIT))continue;
        WorldDragVector v={location.pose.position.x,location.pose.position.y,location.pose.position.z};
        if(world_drag_position_valid(v)) {a->frame.position[h]=v;a->frame.tracked[h]=1;}
    }
    return 1;
}
#endif
