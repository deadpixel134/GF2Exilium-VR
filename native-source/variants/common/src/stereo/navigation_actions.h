#ifndef GF2_NAVIGATION_ACTIONS_H
#define GF2_NAVIGATION_ACTIONS_H
#include "navigation_policy.h"
/* Optional children of the existing AuxPanel action set: no second attach or
   SyncActions, no spaces. Failure disables only navigation. Parent owns every
   created action, including partial/unbound creation, through set teardown. */
typedef struct {
    PFN_xrCreateAction create;PFN_xrStringToPath path;
    PFN_xrGetActionStateVector2f vector;PFN_xrGetActionStateBoolean boolean;
} NavigationActionApi;
typedef struct {
    NavigationActionApi api;XrSession session;XrAction axis,press;
    XrPath hand[2];unsigned ready,bound_profiles;NavigationSample frame;
#ifdef GF2VR_CAMERA_RETURN
    XrAction camera_return;unsigned camera_return_ready,camera_return_profiles;
#endif
#ifdef GF2VR_CAMERA_FOLLOW_TOGGLE
    XrAction camera_follow;unsigned camera_follow_ready,camera_follow_profiles;
#endif
} NavigationActions;
static void navigation_action_text(char *out,const char *s){while((*out++=*s++)){} }
static int navigation_actions_create(NavigationActions *n,NavigationActionApi api,XrInstance instance,XrSession session,XrActionSet set) {
    if(!n||n->session||!instance||!session||!set||!api.create||!api.path||!api.vector||!api.boolean)return 0;
    n->api=api;n->session=session;
    if(api.path(instance,"/user/hand/left",&n->hand[0])!=XR_SUCCESS||api.path(instance,"/user/hand/right",&n->hand[1])!=XR_SUCCESS)return 0;
    XrActionCreateInfo c={0};c.type=XR_TYPE_ACTION_CREATE_INFO;c.countSubactionPaths=2;c.subactionPaths=n->hand;
    c.actionType=XR_ACTION_TYPE_VECTOR2F_INPUT;navigation_action_text(c.actionName,"vr_navigation_axis");navigation_action_text(c.localizedActionName,"VR navigation axis");
    if(api.create(set,&c,&n->axis)!=XR_SUCCESS||!n->axis)return 0;
    c.actionType=XR_ACTION_TYPE_BOOLEAN_INPUT;navigation_action_text(c.actionName,"vr_navigation_press");navigation_action_text(c.localizedActionName,"Navigation trackpad press");
    if(api.create(set,&c,&n->press)!=XR_SUCCESS||!n->press)return 0;
    n->ready=1;
#ifdef GF2VR_CAMERA_RETURN
    c.countSubactionPaths=1;c.subactionPaths=&n->hand[0];
    navigation_action_text(c.actionName,"vr_return_camera");navigation_action_text(c.localizedActionName,"Return to game camera");
    if(api.create(set,&c,&n->camera_return)==XR_SUCCESS&&n->camera_return)n->camera_return_ready=1;
#endif
#ifdef GF2VR_CAMERA_FOLLOW_TOGGLE
    c.countSubactionPaths=1;c.subactionPaths=&n->hand[1];
    navigation_action_text(c.actionName,"vr_toggle_camera_follow");navigation_action_text(c.localizedActionName,"Toggle game camera follow");
    if(api.create(set,&c,&n->camera_follow)==XR_SUCCESS&&n->camera_follow)n->camera_follow_ready=1;
#endif
    return 1;
}
#ifdef GF2VR_CAMERA_RETURN
static unsigned navigation_camera_return_binding(NavigationActions *n,XrInstance instance,unsigned profile,XrActionSuggestedBinding *out,unsigned capacity){
    if(!n||!n->camera_return_ready||profile>3||profile==2||!out||!capacity)return 0;
    XrPath p=0;if(n->api.path(instance,"/user/hand/left/input/thumbstick/click",&p)!=XR_SUCCESS||!p)return 0;
    *out=(XrActionSuggestedBinding){n->camera_return,p};return 1;
}
#endif
#ifdef GF2VR_CAMERA_FOLLOW_TOGGLE
static unsigned navigation_camera_follow_binding(NavigationActions *n,XrInstance instance,unsigned profile,XrActionSuggestedBinding *out,unsigned capacity){
    if(!n||!n->camera_follow_ready||profile>3||profile==2||!out||!capacity)return 0;
    XrPath p=0;if(n->api.path(instance,"/user/hand/right/input/thumbstick/click",&p)!=XR_SUCCESS||!p)return 0;
    *out=(XrActionSuggestedBinding){n->camera_follow,p};return 1;
}
#endif
static unsigned navigation_actions_bindings(NavigationActions *n,XrInstance instance,unsigned profile,XrActionSuggestedBinding *out,unsigned capacity) {
    unsigned count=profile==2?4:2;
    if(!n||!n->ready||profile>3||!out||capacity<count)return 0;
    const char *stick[]={"/user/hand/left/input/thumbstick","/user/hand/right/input/thumbstick"};
    const char *pad[]={"/user/hand/left/input/trackpad","/user/hand/right/input/trackpad"};
    const char *click[]={"/user/hand/left/input/trackpad/click","/user/hand/right/input/trackpad/click"};
    for(unsigned h=0;h<2;++h) {
        XrPath p=0;if(n->api.path(instance,profile==2?pad[h]:stick[h],&p)!=XR_SUCCESS||!p)return 0;
        out[h]=(XrActionSuggestedBinding){n->axis,p};
        if(profile==2){if(n->api.path(instance,click[h],&p)!=XR_SUCCESS||!p)return 0;out[2+h]=(XrActionSuggestedBinding){n->press,p};}
    }
    return count;
}
/* The caller already performed the sole shared SyncActions. A trackpad press
   action is only bound on Vive; inactive on thumbstick profiles. */
static void navigation_actions_sample(NavigationActions *n,XrTime time,unsigned long long tick) {
    n->frame=(NavigationSample){0};n->frame.display_time=time;n->frame.tick_ms=tick;
    if(!n->ready||time<=0||!tick)return;
#ifdef GF2VR_CAMERA_RETURN
    if(n->camera_return_ready&&n->camera_return_profiles){
        XrActionStateGetInfo get={0};get.type=XR_TYPE_ACTION_STATE_GET_INFO;get.action=n->camera_return;get.subactionPath=n->hand[0];
        XrActionStateBoolean button={0};button.type=XR_TYPE_ACTION_STATE_BOOLEAN;
        if(n->api.boolean(n->session,&get,&button)==XR_SUCCESS&&button.isActive){n->frame.camera_return_active=1;n->frame.camera_return_down=button.currentState!=0;}
    }
#endif
#ifdef GF2VR_CAMERA_FOLLOW_TOGGLE
    if(n->camera_follow_ready&&n->camera_follow_profiles){
        XrActionStateGetInfo get={0};get.type=XR_TYPE_ACTION_STATE_GET_INFO;get.action=n->camera_follow;get.subactionPath=n->hand[1];
        XrActionStateBoolean button={0};button.type=XR_TYPE_ACTION_STATE_BOOLEAN;
        if(n->api.boolean(n->session,&get,&button)==XR_SUCCESS&&button.isActive){n->frame.camera_follow_active=1;n->frame.camera_follow_down=button.currentState!=0;}
    }
#endif
    for(unsigned h=0;h<2;++h) {
        XrActionStateGetInfo get={0};get.type=XR_TYPE_ACTION_STATE_GET_INFO;get.action=n->axis;get.subactionPath=n->hand[h];
        XrActionStateVector2f v={0};v.type=XR_TYPE_ACTION_STATE_VECTOR2F;
        if(n->api.vector(n->session,&get,&v)!=XR_SUCCESS||!v.isActive||!navigation_axis_valid(v.currentState.x,v.currentState.y))continue;
        XrActionStateBoolean press={0};press.type=XR_TYPE_ACTION_STATE_BOOLEAN;get.action=n->press;
        if(n->api.boolean(n->session,&get,&press)!=XR_SUCCESS)continue;
        n->frame.active[h]=1;
        if(!press.isActive||press.currentState){n->frame.x[h]=v.currentState.x;n->frame.y[h]=v.currentState.y;}
    }
}
#endif
