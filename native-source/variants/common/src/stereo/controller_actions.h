#ifndef GF2_CONTROLLER_ACTIONS_H
#define GF2_CONTROLLER_ACTIONS_H
/* Extend the EXISTING session action-set owner before its single attachment.
   This transport does not attach/sync a second set or own parent XR lifetime.
   Profile bindings must be appended to the panel owner's suggestion list. */
typedef struct {
    PFN_xrCreateAction CreateAction;
    PFN_xrStringToPath StringToPath;
    PFN_xrCreateActionSpace CreateActionSpace;
    PFN_xrDestroySpace DestroySpace;
    PFN_xrGetActionStatePose GetActionStatePose;
    PFN_xrGetActionStateBoolean GetActionStateBoolean;
    PFN_xrLocateSpace LocateSpace;
} ControllerActionApi;
typedef struct {
    ControllerActionApi api;XrSession session;
    XrAction aim,primary,trigger,back;XrSpace space;
    unsigned hand,ready,tracked,face_down,trigger_down,back_down;
#ifdef GF2VR_CONTROLLER_FACE_CONFIG
    unsigned face_swap; /* Captured before shared-set profile binding/attachment. */
#endif
    XrPosef pose;
} ControllerActions;
static void controller_actions_invalidate(ControllerActions *c) {
    c->tracked=c->face_down=c->trigger_down=c->back_down=0;c->pose=(XrPosef){0};
}
/* Successful space destruction precedes parent action-set/session teardown.
   Failure retains the handle; caller must retry instead of zeroing storage. */
static int controller_actions_close(ControllerActions *c) {
    controller_actions_invalidate(c);c->ready=0;
    if(c->space) {
        if(!c->api.DestroySpace || c->api.DestroySpace(c->space)!=XR_SUCCESS)return 0;
        c->space=XR_NULL_HANDLE;
    }
    *c=(ControllerActions){0};return 1;
}
static void controller_action_text(char *out,const char *in) {while((*out++=*in++)) {}}
/* Creating the four child actions is atomic from the parent's perspective:
   on failure destroy the parent set after controller_actions_close. */
#ifdef GF2VR_CONTROLLER_HAND_ROLES
static int controller_actions_create_selected(ControllerActions *c,const ControllerActionApi *api,
    XrInstance instance,XrSession session,XrActionSet set,unsigned hand,unsigned selected_face) {
#else
static int controller_actions_create(ControllerActions *c,const ControllerActionApi *api,
    XrInstance instance,XrSession session,XrActionSet set,unsigned hand) {
#endif
    if(!c||!api||!instance||!session||!set||hand>1||c->space||c->session||
       !api->CreateAction||!api->StringToPath||!api->CreateActionSpace||!api->DestroySpace||
       !api->GetActionStatePose||!api->GetActionStateBoolean||!api->LocateSpace)return 0;
    c->api=*api;c->session=session;c->hand=hand;
#ifdef GF2VR_CONTROLLER_FACE_CONFIG
    /* Runtime configuration supersedes the historical fixed test-build flag. */
#ifdef GF2VR_CONTROLLER_HAND_ROLES
    c->face_swap=selected_face<=1?selected_face:GF2VR_CONTROLLER_FACE_SWAP_REQUEST()!=0;
#else
    c->face_swap=GF2VR_CONTROLLER_FACE_SWAP_REQUEST()!=0;
#endif
#endif
    const char *names[]={"ui_aim","ui_primary","ui_trigger","ui_back"};
    XrAction *handles[]={&c->aim,&c->primary,&c->trigger,&c->back};
    for(unsigned i=0;i<4;++i) {
        XrActionCreateInfo info={0};info.type=XR_TYPE_ACTION_CREATE_INFO;
        info.actionType=i?XR_ACTION_TYPE_BOOLEAN_INPUT:XR_ACTION_TYPE_POSE_INPUT;
        controller_action_text(info.actionName,names[i]);controller_action_text(info.localizedActionName,names[i]);
        if(api->CreateAction(set,&info,handles[i])!=XR_SUCCESS || !*handles[i])return 0;
    }
    XrActionSpaceCreateInfo space={0};space.type=XR_TYPE_ACTION_SPACE_CREATE_INFO;
    space.action=c->aim;space.poseInActionSpace.orientation.w=1;
    if(api->CreateActionSpace(session,&space,&c->space)!=XR_SUCCESS || !c->space)return 0;
    c->ready=1;return 1;
}
#ifdef GF2VR_CONTROLLER_HAND_ROLES
static int controller_actions_create(ControllerActions *c,const ControllerActionApi *api,
    XrInstance instance,XrSession session,XrActionSet set,unsigned hand) {
    return controller_actions_create_selected(c,api,instance,session,set,hand,~0u);
}
#endif
/* profile:0=Touch,1=Index,2=Vive,3=Microsoft. Never use system/menu buttons.
   Face actions remain inactive where the profile has no approved A/B or X/Y.
   Passing too-small storage or an invalid path returns no usable suggestion. */
static unsigned controller_actions_bindings(ControllerActions *c,XrInstance instance,
    unsigned profile,XrActionSuggestedBinding *out,unsigned capacity) {
    unsigned count=profile<2?4:2;
    if(!c||!c->ready||!instance||profile>3||!out||capacity<count)return 0;
    /* Keep immutable logical Primary/Back actions and exchange only their two
       within-hand paths. Trigger/aim/hand roles and unsupported profiles stay. */
    const char *right[]={"/user/hand/right/input/aim/pose","/user/hand/right/input/trigger/value",
        "/user/hand/right/input/a/click","/user/hand/right/input/b/click"};
    const char *left_touch[]={"/user/hand/left/input/aim/pose","/user/hand/left/input/trigger/value",
        "/user/hand/left/input/x/click","/user/hand/left/input/y/click"};
    const char *left_index[]={"/user/hand/left/input/aim/pose","/user/hand/left/input/trigger/value",
        "/user/hand/left/input/a/click","/user/hand/left/input/b/click"};
    unsigned swap=0;
#if defined(GF2VR_CONTROLLER_FACE_CONFIG)
    swap=c->face_swap;
#elif defined(GF2VR_FACE_SWAP)
    swap=1;
#endif
    const char **paths=c->hand?(profile==0?left_touch:left_index):right;
    XrAction actions[]={c->aim,c->trigger,c->primary,c->back};
    for(unsigned i=0;i<count;++i) {
        XrPath path=0;
        unsigned index=swap && i>=2?(i==2?3:2):i;
        if(c->api.StringToPath(instance,paths[index],&path)!=XR_SUCCESS || !path)return 0;
        out[i]=(XrActionSuggestedBinding){actions[i],path};
    }
    return count;
}
/* Called AFTER the shared owner's successful SyncActions, once per predicted
   frame, in the EXACT space used by the submitted panel. Inactive buttons do
   not poison a valid aim for trigger-only profiles; API errors fail closed. */
static int controller_actions_sample(ControllerActions *c,XrSpace panel_space,XrTime time,int focused) {
    if(!c)return 0;controller_actions_invalidate(c);
    if(!c->ready||!focused||!panel_space||time<=0)return 0;
    XrActionStateGetInfo get={0};get.type=XR_TYPE_ACTION_STATE_GET_INFO;get.action=c->aim;
    XrActionStatePose pose={0};pose.type=XR_TYPE_ACTION_STATE_POSE;
    XrSpaceLocation loc={0};loc.type=XR_TYPE_SPACE_LOCATION;
    XrSpaceLocationFlags required=XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT|
        XR_SPACE_LOCATION_POSITION_TRACKED_BIT|XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT;
    if(c->api.GetActionStatePose(c->session,&get,&pose)!=XR_SUCCESS||!pose.isActive||
       c->api.LocateSpace(c->space,panel_space,time,&loc)!=XR_SUCCESS || (loc.locationFlags&required)!=required)return 0;
    XrAction actions[]={c->primary,c->trigger,c->back};unsigned states[3]={0};
    for(unsigned i=0;i<3;++i) {
        XrActionStateBoolean button={0};button.type=XR_TYPE_ACTION_STATE_BOOLEAN;get.action=actions[i];
        if(c->api.GetActionStateBoolean(c->session,&get,&button)!=XR_SUCCESS)return 0;
        states[i]=button.isActive && button.currentState;
    }
    c->pose=loc.pose;c->tracked=1;c->face_down=states[0];c->trigger_down=states[1];c->back_down=states[2];return 1;
}
#endif
