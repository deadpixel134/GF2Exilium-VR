#ifndef GF2_AUX_PANEL_H
#define GF2_AUX_PANEL_H
/* XR-worker/session owned. Only pose and Grip transport for the complete
   composite panel; no pointer, synthetic input or navigation ownership. */
#define AUX_API(F) F(CreateActionSet) F(DestroyActionSet) F(CreateAction) \
 F(StringToPath) F(SuggestInteractionProfileBindings) F(AttachSessionActionSets) \
 F(CreateActionSpace) F(DestroySpace) F(SyncActions) F(GetActionStateBoolean) \
 F(GetActionStatePose) F(LocateSpace)
#ifdef GF2VR_WORLD_DRAG
#include "world_drag_actions.h"
#define AUX_DRAG_API(F) F(GetActionStateFloat)
#else
#define AUX_DRAG_API(F)
#endif
#include "aux_panel_config.h"
#ifdef GF2VR_DIRTY_CONTACT
#include "contact_haptics.h"
#endif
#ifdef GF2VR_CONTROLLER_INPUT
#ifdef GF2VR_CONTROLLER_FACE_CONFIG
#include "controller_face_runtime.h"
#define GF2VR_CONTROLLER_FACE_SWAP_REQUEST() controller_face_config_load()
#endif
#include "controller_actions.h"
#ifdef GF2VR_CONTROLLER_GAME_MODE
#include "controller_game_actions.h"
#endif
#endif
#ifdef GF2VR_THUMBSTICK_NAVIGATION
#include "navigation_actions.h"
#endif
typedef struct {
#define AUX_FN(n) PFN_xr##n n;
    AUX_API(AUX_FN)
    AUX_DRAG_API(AUX_FN)
#undef AUX_FN
} AuxApi;
typedef struct {
    AuxApi api; XrActionSet set; XrAction pose,grip; XrSpace space;
#ifdef GF2VR_DIRTY_CONTACT
    ContactHapticActions haptics;
#endif
    XrSession session; DWORD thread; int ready,shown,armed,down,tracked,last_projection;
    unsigned long long generation; XrVector3f position;
    XrQuaternionf orientation;AuxPlacementSettings placement;
    AuxPlacementMath placement_math;HINSTANCE placement_module;
#ifdef GF2VR_CONTROLLER_HAND_ROLES
    unsigned panel_hand;
#endif
#ifdef GF2VR_WORLD_DRAG
    WorldDragActions drag;
#endif
#ifdef GF2VR_CONTROLLER_INPUT
    ControllerActions pointer;
#ifdef GF2VR_CONTROLLER_GAME_MODE
    ControllerGameActions game;
#endif
#endif
#ifdef GF2VR_THUMBSTICK_NAVIGATION
    NavigationActions navigation;
#endif
#ifdef GF2VR_AUX_OBSERVE
    unsigned observe_reason,observe_active,observe_button; XrResult observe_result;
    XrSpaceLocationFlags observe_flags; XrPosef observe_pose;
#endif
} AuxPanel;
static AuxPanel stereo_aux;
#ifdef GF2VR_AUX_OBSERVE
static XrResult aux_observe_call(AuxPanel *a,unsigned reason,XrResult result) {
    a->observe_reason=reason;a->observe_result=result;return result;
}
#define AUX_CALL(a,n,expr) aux_observe_call(a,n,(expr))
#else
#define AUX_CALL(a,n,expr) (expr)
#endif
static void aux_text(char *out,const char *text) {while((*out++=*text++)) {}}
static void aux_reset_visibility(AuxPanel *a) {
#ifdef GF2VR_ACTIVITY_UI_AUX
    GF2VR_ACTIVITY_UI_AUX(0);
#endif
    a->shown=a->armed=a->down=a->tracked=a->last_projection=0;a->generation=0;
}
static int aux_close(AuxPanel *a) {
    if(a->thread && a->thread!=GetCurrentThreadId()) return 0;
    aux_reset_visibility(a);a->ready=0;
#ifdef GF2VR_DIRTY_CONTACT
    contact_haptic_cancel(&a->haptics);
#endif
#ifdef GF2VR_CONTROLLER_INPUT
    if(!controller_actions_close(&a->pointer))return 0;
#endif
#ifdef GF2VR_WORLD_DRAG
    if(!world_drag_actions_close(&a->drag))return 0;
    if(a->space) {if(!a->api.DestroySpace||XR_FAILED(a->api.DestroySpace(a->space)))return 0;a->space=0;}
    if(a->set) {if(!a->api.DestroyActionSet||XR_FAILED(a->api.DestroyActionSet(a->set)))return 0;a->set=0;}
#else
    if(a->space && a->api.DestroySpace)a->api.DestroySpace(a->space);
    if(a->set && a->api.DestroyActionSet)a->api.DestroyActionSet(a->set);
#endif
    if(a->placement_module && !FreeLibrary(a->placement_module))return 0;
    *a=(AuxPanel){0};return 1;
}
static void aux_placement_load(AuxPanel *a) {
    DWORD saved=GetLastError();a->placement=aux_placement_defaults();unsigned status=1;
    HANDLE file=CreateFileW(GF2VR_CONTROLLER_FACE_CONFIG_PATH,0x80000000u,7,0,3,128,0);
    if(file!=(HANDLE)(unsigned long long)-1) {
        static char text[4097];DWORD length=0;int ok=ReadFile(file,text,sizeof(text),&length,0);CloseHandle(file);
        if(!ok||length>=sizeof(text)||!aux_placement_config_parse(text,length,&a->placement)) {
            a->placement=aux_placement_defaults();status=0;
        }else status=2;
    }else if(GetLastError()!=2&&GetLastError()!=3)status=0;
    if(a->placement.configured) {
        HINSTANCE module=0;
        if(GetModuleHandleExW(0,L"ucrtbase.dll",&module)) {
            a->placement_module=module;
            a->placement_math=(AuxPlacementMath){(void *)GetProcAddress(module,"sin"),(void *)GetProcAddress(module,"cos"),
                (void *)GetProcAddress(module,"sqrt"),(void *)GetProcAddress(module,"atan2")};
        }
        if(!a->placement_math.sin||!a->placement_math.cos||!a->placement_math.sqrt||!a->placement_math.atan2) {
            a->placement.configured=0;status=0;
        }
    }
    event("aux_panel_config",((unsigned long long)status<<32)|a->placement.configured|((unsigned long long)a->placement.viewer_facing<<1),GetCurrentThreadId());
    SetLastError(saved);
}
#ifdef GF2VR_DIRTY_CONTACT
static void aux_haptic_load(AuxPanel *a) {
    DWORD saved=GetLastError();ContactHapticSettings settings=contact_haptic_defaults();unsigned status=1;
    static unsigned short path[1024];
#ifdef GF2VR_PORTABLE_BINDING
    int path_ok=gf2vr_paths.ready && gf2vr_path_join(path,gf2vr_paths.root,gf2vr_path_length(gf2vr_paths.root),L"vrmod\\config\\controller_hands.json");
#else
    const unsigned short local[]=L"vrmod\\config\\controller_hands.json";unsigned n=0;do{path[n]=local[n];}while(local[n++]);int path_ok=1;
#endif
    if(!path_ok)status=0;
    else {
        HANDLE file=CreateFileW(path,0x80000000u,7,0,3,128,0);
        if(file!=(HANDLE)(unsigned long long)-1) {
            static char text[4097];DWORD length=0;int ok=ReadFile(file,text,sizeof(text),&length,0);CloseHandle(file);
            status=ok && length<sizeof(text) && contact_haptic_config_parse(text,length,&settings)?2u:0u;
        }else if(GetLastError()!=2 && GetLastError()!=3)status=0;
    }
    if(!status)settings=(ContactHapticSettings){0,0};a->haptics.settings=settings;
    event("contact_haptic_config",status,GetCurrentThreadId());SetLastError(saved);
}
#endif
static XrResult aux_suggest(AuxPanel *a,XrInstance instance,unsigned profile,const XrInteractionProfileSuggestedBinding *suggest) {
#ifdef GF2VR_DIRTY_CONTACT
    return contact_haptic_suggest(&a->haptics,a->api.SuggestInteractionProfileBindings,a->api.StringToPath,instance,profile,suggest);
#else
    (void)profile;return a->api.SuggestInteractionProfileBindings(instance,suggest);
#endif
}
static int aux_open(AuxPanel *a,PFN_xrGetInstanceProcAddr get,XrInstance instance,XrSession session) {
    if(!get || !instance || !session || a->set || a->placement_module) return 0;
    a->thread=GetCurrentThreadId();a->session=session;
    aux_placement_load(a);
#ifdef GF2VR_CONTROLLER_HAND_ROLES
    unsigned selected_face=controller_input_config_load(&a->panel_hand);
#endif
#define AUX_LOAD(n) if(get(instance,"xr" #n,(PFN_xrVoidFunction *)&a->api.n)!=XR_SUCCESS || !a->api.n) goto fail;
    AUX_API(AUX_LOAD)
    AUX_DRAG_API(AUX_LOAD)
#undef AUX_LOAD
    XrActionSetCreateInfo set={0};set.type=XR_TYPE_ACTION_SET_CREATE_INFO;
    aux_text(set.actionSetName,"gf2_aux_panel");aux_text(set.localizedActionSetName,"Game panel");
    if(a->api.CreateActionSet(instance,&set,&a->set)!=XR_SUCCESS) goto fail;
    #ifdef GF2VR_DIRTY_CONTACT
    contact_haptic_open(&a->haptics,get,a->api.CreateAction,a->api.StringToPath,instance,session,a->set);
    aux_haptic_load(a);
#endif
    XrActionCreateInfo action={0};action.type=XR_TYPE_ACTION_CREATE_INFO;
    action.actionType=XR_ACTION_TYPE_POSE_INPUT;
    aux_text(action.actionName,"panel_hand");aux_text(action.localizedActionName,"Panel hand");
    if(a->api.CreateAction(a->set,&action,&a->pose)!=XR_SUCCESS) goto fail;
    action.actionType=XR_ACTION_TYPE_BOOLEAN_INPUT;
    aux_text(action.actionName,"panel_toggle");aux_text(action.localizedActionName,"Panel toggle");
    if(a->api.CreateAction(a->set,&action,&a->grip)!=XR_SUCCESS) goto fail;
#ifdef GF2VR_WORLD_DRAG
    WorldDragActionApi drag_api={a->api.CreateAction,a->api.StringToPath,a->api.CreateActionSpace,a->api.DestroySpace,
        a->api.GetActionStateFloat,a->api.GetActionStatePose,a->api.LocateSpace};
    if(!world_drag_actions_create(&a->drag,drag_api,instance,session,a->set))goto fail;
#endif
#ifdef GF2VR_CONTROLLER_INPUT
    ControllerActionApi pointer_api={a->api.CreateAction,a->api.StringToPath,a->api.CreateActionSpace,a->api.DestroySpace,
        a->api.GetActionStatePose,a->api.GetActionStateBoolean,a->api.LocateSpace};
#ifdef GF2VR_CONTROLLER_HAND_ROLES
    if(!controller_actions_create_selected(&a->pointer,&pointer_api,instance,session,a->set,a->panel_hand,selected_face))goto fail;
#else
    if(!controller_actions_create(&a->pointer,&pointer_api,instance,session,a->set,0))goto fail; /* right pointer */
#endif
#ifdef GF2VR_CONTROLLER_GAME_MODE
    (void)controller_game_actions_create(&a->game,&pointer_api,instance,session,a->set);
#endif
#endif
#ifdef GF2VR_THUMBSTICK_NAVIGATION
    PFN_xrGetActionStateVector2f vector=0;
    if(get(instance,"xrGetActionStateVector2f",(PFN_xrVoidFunction *)&vector)==XR_SUCCESS&&vector) {
        NavigationActionApi nav_api={a->api.CreateAction,a->api.StringToPath,vector,a->api.GetActionStateBoolean};
        (void)navigation_actions_create(&a->navigation,nav_api,instance,session,a->set);
    } /* Partial/unbound optional actions remain owned by the parent set. */
#endif
    const char *profiles[]={"/interaction_profiles/oculus/touch_controller",
        "/interaction_profiles/valve/index_controller","/interaction_profiles/htc/vive_controller",
        "/interaction_profiles/microsoft/motion_controller"};
    unsigned supported=0;
    for(unsigned i=0;i<4;++i) {
        XrPath profile=0,pose=0,grip=0;
#ifdef GF2VR_CONTROLLER_HAND_ROLES
        const char *panel_pose=a->panel_hand?"/user/hand/right/input/grip/pose":"/user/hand/left/input/grip/pose";
        const char *panel_grip=a->panel_hand?(i<2?"/user/hand/right/input/squeeze/value":"/user/hand/right/input/squeeze/click"):
            (i<2?"/user/hand/left/input/squeeze/value":"/user/hand/left/input/squeeze/click");
        if(a->api.StringToPath(instance,profiles[i],&profile)!=XR_SUCCESS ||
           a->api.StringToPath(instance,panel_pose,&pose)!=XR_SUCCESS ||
           a->api.StringToPath(instance,panel_grip,&grip)!=XR_SUCCESS)continue;
#else
        if(a->api.StringToPath(instance,profiles[i],&profile)!=XR_SUCCESS ||
           a->api.StringToPath(instance,"/user/hand/left/input/grip/pose",&pose)!=XR_SUCCESS ||
           a->api.StringToPath(instance,i<2?"/user/hand/left/input/squeeze/value":
                                             "/user/hand/left/input/squeeze/click",&grip)!=XR_SUCCESS) continue;
#endif
#if defined(GF2VR_CAMERA_FOLLOW_TOGGLE) && defined(GF2VR_CONTROLLER_GAME_MODE)
        XrActionSuggestedBinding bindings[24]={{a->pose,pose},{a->grip,grip}};unsigned binding_count=2;
#elif defined(GF2VR_CONTROLLER_GAME_MODE)
        XrActionSuggestedBinding bindings[20]={{a->pose,pose},{a->grip,grip}};unsigned binding_count=2;
#elif defined(GF2VR_THUMBSTICK_NAVIGATION)
        XrActionSuggestedBinding bindings[16]={{a->pose,pose},{a->grip,grip}};unsigned binding_count=2;
#else
        XrActionSuggestedBinding bindings[12]={{a->pose,pose},{a->grip,grip}};unsigned binding_count=2;
#endif
#ifdef GF2VR_WORLD_DRAG
        unsigned extra=world_drag_actions_bindings(&a->drag,instance,i,bindings+2,6);
        if(!extra)continue;binding_count+=extra;
#endif
#ifdef GF2VR_CONTROLLER_INPUT
        unsigned pointer_count=controller_actions_bindings(&a->pointer,instance,i,bindings+binding_count,12-binding_count);
        if(!pointer_count)continue;binding_count+=pointer_count;
#endif
#ifdef GF2VR_THUMBSTICK_NAVIGATION
        unsigned original_count=binding_count;
        unsigned nav_count=navigation_actions_bindings(&a->navigation,instance,i,bindings+binding_count,16-binding_count);
        binding_count+=nav_count;
#endif
#ifdef GF2VR_CAMERA_RETURN
        unsigned before_return=binding_count;
        unsigned return_count=navigation_camera_return_binding(&a->navigation,instance,i,bindings+binding_count,(unsigned)(sizeof(bindings)/sizeof(bindings[0]))-binding_count);
        binding_count+=return_count;
#endif
#ifdef GF2VR_CAMERA_FOLLOW_TOGGLE
        unsigned before_follow=binding_count;
        unsigned follow_count=navigation_camera_follow_binding(&a->navigation,instance,i,bindings+binding_count,(unsigned)(sizeof(bindings)/sizeof(bindings[0]))-binding_count);
        binding_count+=follow_count;
#endif
#ifdef GF2VR_CONTROLLER_GAME_MODE
        unsigned before_game=binding_count;
        unsigned game_count=controller_game_actions_bindings(&a->game,instance,i,bindings+binding_count,20-binding_count);
        binding_count+=game_count;
#endif
        XrInteractionProfileSuggestedBinding suggest={0};suggest.type=XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
        suggest.interactionProfile=profile;suggest.countSuggestedBindings=binding_count;suggest.suggestedBindings=bindings;
#ifdef GF2VR_THUMBSTICK_NAVIGATION
#ifdef GF2VR_CAMERA_FOLLOW_TOGGLE
        /* Independently optional children. Prefer preserving old game keys,
           then LEFT return, while retrying RIGHT without a rejected neighbor.
           All subsets use the same existing action set and single attach. */
        XrActionSuggestedBinding full[24];for(unsigned n=0;n<binding_count;++n)full[n]=bindings[n];
        const unsigned choices[]={7,5,6,4,3,1,2,0};
        unsigned available=(return_count?1u:0u)|(follow_count?2u:0u);
#ifdef GF2VR_CONTROLLER_GAME_MODE
        if(game_count)available|=4;
#endif
        XrResult suggested=XR_ERROR_PATH_UNSUPPORTED;unsigned chosen=0,seen=0;
        for(unsigned choice=0;choice<8;++choice){unsigned mask=choices[choice]&available;
            if(seen&(1u<<mask))continue;seen|=1u<<mask;
            unsigned count=before_return;
            for(unsigned n=0;n<before_return;++n)bindings[n]=full[n];
            if(mask&1)bindings[count++]=full[before_return];
            if(mask&2)bindings[count++]=full[before_follow];
#ifdef GF2VR_CONTROLLER_GAME_MODE
            if(mask&4)for(unsigned n=0;n<game_count;++n)bindings[count++]=full[before_game+n];
#endif
            suggest.countSuggestedBindings=count;suggested=aux_suggest(a,instance,i,&suggest);
            if(suggested==XR_SUCCESS){chosen=mask;break;}
        }
#ifdef GF2VR_CONTROLLER_GAME_MODE
        if(suggested==XR_SUCCESS&&(chosen&4))a->game.bound_profiles|=1u<<i;
#endif
        if(suggested==XR_SUCCESS&&(chosen&1))a->navigation.camera_return_profiles|=1u<<i;
        if(suggested==XR_SUCCESS&&(chosen&2))a->navigation.camera_follow_profiles|=1u<<i;
#else
        XrResult suggested=aux_suggest(a,instance,i,&suggest);
#ifdef GF2VR_CAMERA_RETURN
        if(suggested!=XR_SUCCESS&&return_count){
            /* Retry without only this optional binding first; preserve all
               existing movement, game keys and panel/pointer suggestions. */
            for(unsigned n=before_return+return_count;n<binding_count;++n)bindings[n-return_count]=bindings[n];
            binding_count-=return_count;
#ifdef GF2VR_CONTROLLER_GAME_MODE
            before_game-=return_count;
#endif
            suggest.countSuggestedBindings=binding_count;
            suggested=aux_suggest(a,instance,i,&suggest);return_count=0;
        }
#endif
#ifdef GF2VR_CONTROLLER_GAME_MODE
        if(suggested!=XR_SUCCESS&&game_count){
#ifdef GF2VR_CAMERA_RETURN
            /* If the legacy game-key suggestions failed too, retry return
               independently without them before falling back to navigation. */
            return_count=navigation_camera_return_binding(&a->navigation,instance,i,bindings+before_game,
                (unsigned)(sizeof(bindings)/sizeof(bindings[0]))-before_game);
            if(return_count){suggest.countSuggestedBindings=before_game+return_count;suggested=aux_suggest(a,instance,i,&suggest);}
            if(!return_count||suggested!=XR_SUCCESS){return_count=0;
#endif
            suggest.countSuggestedBindings=before_game;
            suggested=aux_suggest(a,instance,i,&suggest);
#ifdef GF2VR_CAMERA_RETURN
            }
#endif
            game_count=0;
        }
        if(suggested==XR_SUCCESS&&game_count)a->game.bound_profiles|=1u<<i;
#endif
#ifdef GF2VR_CAMERA_RETURN
        if(suggested==XR_SUCCESS&&return_count)a->navigation.camera_return_profiles|=1u<<i;
#endif
#endif /* independent optional camera controls */
        if(suggested!=XR_SUCCESS&&nav_count) {
            suggest.countSuggestedBindings=original_count;
            suggested=aux_suggest(a,instance,i,&suggest);
        }
#ifdef GF2VR_THUMBSTICK_NAVIGATION
        else if(suggested==XR_SUCCESS&&nav_count)a->navigation.bound_profiles|=1u<<i;
#endif
        if(suggested==XR_SUCCESS)++supported;
#else
        if(aux_suggest(a,instance,i,&suggest)==XR_SUCCESS) ++supported;
#endif
    }
#ifdef GF2VR_THUMBSTICK_NAVIGATION
    if(!a->navigation.bound_profiles)a->navigation.ready=0;
#endif
    if(!supported) goto fail;
    XrActionSpaceCreateInfo space={0};space.type=XR_TYPE_ACTION_SPACE_CREATE_INFO;
    space.action=a->pose;space.poseInActionSpace.orientation.w=1;
    if(a->api.CreateActionSpace(session,&space,&a->space)!=XR_SUCCESS) goto fail;
    XrSessionActionSetsAttachInfo attach={0};attach.type=XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO;
    attach.countActionSets=1;attach.actionSets=&a->set;
    if(a->api.AttachSessionActionSets(session,&attach)!=XR_SUCCESS) goto fail;
    a->ready=1;return 1;
fail:
    aux_close(a);return 0;
}
/* No edge on entry or tracking/focus recovery while already holding Grip. */
static void aux_button(AuxPanel *a,int active,int down,unsigned long long generation) {
    if(!generation) {aux_reset_visibility(a);return;}
    if(a->generation!=generation) {aux_reset_visibility(a);a->generation=generation;}
    if(!active) {a->armed=a->down=a->tracked=0;return;}
    if(!down) a->armed=1;
    if(a->armed && down && !a->down) a->shown=!a->shown;
    a->down=down;
#ifdef GF2VR_ACTIVITY_UI_AUX
    GF2VR_ACTIVITY_UI_AUX(a->shown);
#endif
}
static void aux_frame(AuxPanel *a,XrSpace view,XrTime time,unsigned long long generation) {
    a->tracked=0;
#ifdef GF2VR_CONTROLLER_GAME_MODE
    for(unsigned h=0;h<2;++h)a->game.active[h]=a->game.first[h]=a->game.second[h]=0;
#endif
#ifdef GF2VR_CONTROLLER_INPUT
    controller_actions_invalidate(&a->pointer);
#endif
#ifdef GF2VR_WORLD_DRAG
    a->drag.frame=(WorldDragFrame){0};
#endif
#ifdef GF2VR_THUMBSTICK_NAVIGATION
    a->navigation.frame=(NavigationSample){0};
#endif
#ifdef GF2VR_AUX_OBSERVE
    a->observe_reason=1;a->observe_result=XR_SUCCESS;a->observe_active=a->observe_button=0;
    a->observe_flags=0;a->observe_pose=(XrPosef){0};
#endif
    if(!a->ready || a->thread!=GetCurrentThreadId() || !view || !time) {
        aux_button(a,0,0,generation);return;
    }
    XrActiveActionSet active={a->set,XR_NULL_PATH};
    XrActionsSyncInfo sync={0};sync.type=XR_TYPE_ACTIONS_SYNC_INFO;
    sync.countActiveActionSets=1;sync.activeActionSets=&active;
    XrActionStateGetInfo get={0};get.type=XR_TYPE_ACTION_STATE_GET_INFO;get.action=a->grip;
    XrActionStateBoolean button={0};button.type=XR_TYPE_ACTION_STATE_BOOLEAN;
    XrActionStatePose pose={0};pose.type=XR_TYPE_ACTION_STATE_POSE;
    XrSpaceLocation location={0};location.type=XR_TYPE_SPACE_LOCATION;
    if(AUX_CALL(a,2,a->api.SyncActions(a->session,&sync))!=XR_SUCCESS)goto lost;
#ifdef GF2VR_CONTROLLER_INPUT
    (void)controller_actions_sample(&a->pointer,view,time,1);
#ifdef GF2VR_CONTROLLER_GAME_MODE
    controller_game_actions_sample(&a->game);
#endif
#endif
#ifdef GF2VR_WORLD_DRAG
    (void)world_drag_actions_sample(&a->drag,stereo_eye_runtime.owner.space,XR_REFERENCE_SPACE_TYPE_LOCAL,time,GetTickCount64());
#endif
#ifdef GF2VR_THUMBSTICK_NAVIGATION
    navigation_actions_sample(&a->navigation,time,GetTickCount64());
    for(unsigned h=0;h<2;++h)a->navigation.frame.tracked[h]=a->drag.frame.tracked[h];
#endif
    if(AUX_CALL(a,3,a->api.GetActionStateBoolean(a->session,&get,&button))!=XR_SUCCESS || !button.isActive) goto lost;
#ifdef GF2VR_AUX_OBSERVE
    a->observe_active=button.isActive;a->observe_button=button.currentState;
#endif
    get.action=a->pose;
    if(AUX_CALL(a,4,a->api.GetActionStatePose(a->session,&get,&pose))!=XR_SUCCESS || !pose.isActive ||
       AUX_CALL(a,5,a->api.LocateSpace(a->space,view,time,&location))!=XR_SUCCESS ||
       (location.locationFlags & (XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_POSITION_TRACKED_BIT)) !=
           (XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_POSITION_TRACKED_BIT)) goto lost;
#ifdef GF2VR_AUX_OBSERVE
    a->observe_pose=location.pose;a->observe_flags=location.locationFlags;a->observe_reason=6;
#endif
    if(a->placement.configured &&
       (location.locationFlags & (XR_SPACE_LOCATION_ORIENTATION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT)) !=
        (XR_SPACE_LOCATION_ORIENTATION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT))goto lost;
    aux_button(a,1,button.currentState!=0,generation);
    /* Viewer-facing, hand-positioned quad in VIEW space. Comparisons reject
       NaN/infinite and far/behind-head data without importing pose math. */
    XrVector3f p=location.pose.position;p.y+=0.12f;p.z-=0.10f;
    XrQuaternionf orientation={0,0,0,1};
    if(a->placement.configured) {
        AuxPlacementPose hand={{location.pose.position.x,location.pose.position.y,location.pose.position.z},
            {location.pose.orientation.x,location.pose.orientation.y,location.pose.orientation.z,location.pose.orientation.w}},placed;
        if(!aux_placement_pose(&a->placement,&a->placement_math,hand,&placed))goto lost;
        p=(XrVector3f){placed.position.x,placed.position.y,placed.position.z};
        orientation=(XrQuaternionf){placed.orientation.x,placed.orientation.y,placed.orientation.z,placed.orientation.w};
    }
    if(p.z < -0.15f && p.z > -2.0f && p.x > p.z*1.2f && p.x < -p.z*1.2f &&
       p.y > p.z && p.y < -p.z) {a->position=p;a->orientation=orientation;a->tracked=1;}
    return;
lost:
#ifdef GF2VR_AUX_OBSERVE
    a->observe_active=button.isActive;a->observe_button=button.currentState;
    a->observe_pose=location.pose;a->observe_flags=location.locationFlags;
#endif
    aux_button(a,0,0,generation);
}
static int aux_visible(const AuxPanel *a) {return a->ready && a->shown && a->tracked;}
static int aux_copy_needed(const AuxPanel *a,unsigned long long generation) {
    (void)a;(void)generation;
    /* Stereo can become invalid after acquisition in this same XR frame.
       Keep the complete-composite fallback fresh until a separate producer
       can prove that skipping this copy never strands the front panel. */
    return 1;
}
static int aux_quad(const AuxPanel *a,const XrCompositionLayerQuad *front,XrCompositionLayerQuad *out) {
    *out=*front;out->pose.orientation=a->orientation;out->pose.position=a->position;
    out->size.width=0.65f;out->size.height=0.65f*front->size.height/front->size.width;
    if(a->placement.configured && !aux_placement_size(&a->placement,front->size.width,front->size.height,
        &out->size.width,&out->size.height))return 0;
    return 1;
}
#endif
