#ifndef GF2_CONTROLLER_GAME_ACTIONS_H
#define GF2_CONTROLLER_GAME_ACTIONS_H
/* Optional child actions of the existing shared set. No spaces, attach or
   SyncActions. Both physical hands are independent of normal face exchange. */
typedef struct {
    ControllerActionApi api;XrSession session;XrAction primary,secondary;
    XrPath hand[2];unsigned ready,bound_profiles,active[2],first[2],second[2];
} ControllerGameActions;
static int controller_game_actions_create(ControllerGameActions *a,const ControllerActionApi *api,
    XrInstance instance,XrSession session,XrActionSet set){
    *a=(ControllerGameActions){0};a->api=*api;a->session=session;
    if(api->StringToPath(instance,"/user/hand/left",&a->hand[0])!=XR_SUCCESS ||
       api->StringToPath(instance,"/user/hand/right",&a->hand[1])!=XR_SUCCESS)return 0;
    XrActionCreateInfo c={0};c.type=XR_TYPE_ACTION_CREATE_INFO;c.actionType=XR_ACTION_TYPE_BOOLEAN_INPUT;
    c.countSubactionPaths=2;c.subactionPaths=a->hand;
    controller_action_text(c.actionName,"game_physical_first");controller_action_text(c.localizedActionName,"Game X A");
    if(api->CreateAction(set,&c,&a->primary)!=XR_SUCCESS || !a->primary)return 0;
    controller_action_text(c.actionName,"game_physical_second");controller_action_text(c.localizedActionName,"Game Y B");
    if(api->CreateAction(set,&c,&a->secondary)!=XR_SUCCESS || !a->secondary)return 0;
    a->ready=1;return 1;
}
static unsigned controller_game_actions_bindings(ControllerGameActions *a,XrInstance instance,unsigned profile,
    XrActionSuggestedBinding *out,unsigned capacity){
    if(!a||!a->ready||profile>1||capacity<4)return 0;
    const char *touch[]={"/user/hand/left/input/x/click","/user/hand/left/input/y/click",
        "/user/hand/right/input/a/click","/user/hand/right/input/b/click"};
    const char *index[]={"/user/hand/left/input/a/click","/user/hand/left/input/b/click",
        "/user/hand/right/input/a/click","/user/hand/right/input/b/click"};
    for(unsigned n=0;n<4;++n){XrPath p=0;
        if(a->api.StringToPath(instance,(profile?index:touch)[n],&p)!=XR_SUCCESS || !p)return 0;
        out[n]=(XrActionSuggestedBinding){n%2?a->secondary:a->primary,p};
    }
    return 4;
}
static void controller_game_actions_sample(ControllerGameActions *a){
    for(unsigned h=0;h<2;++h){a->active[h]=a->first[h]=a->second[h]=0;
        if(!a->ready||!a->bound_profiles)continue;
        XrActionStateGetInfo get={0};get.type=XR_TYPE_ACTION_STATE_GET_INFO;get.subactionPath=a->hand[h];
        XrActionStateBoolean first={0},second={0};first.type=second.type=XR_TYPE_ACTION_STATE_BOOLEAN;
        get.action=a->primary;if(a->api.GetActionStateBoolean(a->session,&get,&first)!=XR_SUCCESS)continue;
        get.action=a->secondary;if(a->api.GetActionStateBoolean(a->session,&get,&second)!=XR_SUCCESS)continue;
        if(!first.isActive||!second.isActive)continue;
        a->active[h]=1;a->first[h]=first.currentState!=0;a->second[h]=second.currentState!=0;
    }
}
#endif
