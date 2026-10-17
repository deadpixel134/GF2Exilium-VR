#ifndef GF2_CONTROLLER_RUNTIME_H
#define GF2_CONTROLLER_RUNTIME_H
#include "controller_platform.h"
#ifdef GF2VR_ACTIVITY_DIALOGUE_RAY
#include "dialogue_ray_surface.h"
#endif
/* XR-worker owns cursor swapchain, policy and OS sink. One static image is
   filled by Present and released only after its independent query completes. */
static struct {
    PanelXr cursor;
    ControllerInput input;
    ControllerWin32 sink;
    ControllerHit hit;
    U64 sample_tick,token,last_tick,generation_identity,epoch_identity,surface_identity;
#ifdef GF2VR_ACTIVITY_DIALOGUE_RAY
    U64 dialogue_frame;
#endif
    XrSwapchain panel_identity;
    HWND window_identity;
    unsigned width_identity,height_identity,mode_identity,hit_valid,disabled,ready;
} stereo_controller;
static void controller_runtime_cancel(void) {
    (void)controller_input_cancel(&stereo_controller.input,controller_win32_emit,&stereo_controller.sink);
    (void)controller_win32_cancel(&stereo_controller.sink);
    stereo_controller.hit_valid=0;stereo_controller.last_tick=0;
}
static void controller_runtime_open(void) {
    /* Retained failed releases must never be discarded on session recovery. */
    controller_runtime_cancel();stereo_controller.disabled=stereo_controller.ready=0;
    ++stereo_controller.token;stereo_controller.panel_identity=0;stereo_controller.window_identity=0;
    HINSTANCE user=GetModuleHandleW(L"user32.dll");ControllerWin32Api *a=&stereo_controller.sink.api;
#define UI_LOAD(field,name) a->field=user?(void *)GetProcAddress(user,name):0
    UI_LOAD(Foreground,"GetForegroundWindow");UI_LOAD(WindowProcess,"GetWindowThreadProcessId");
    UI_LOAD(ClientRect,"GetClientRect");UI_LOAD(ClientScreen,"ClientToScreen");UI_LOAD(CursorSet,"SetCursorPos");
    UI_LOAD(CursorGet,"GetCursorPos");UI_LOAD(KeyState,"GetAsyncKeyState");UI_LOAD(Inject,"SendInput");
#undef UI_LOAD
}
static void controller_runtime_cursor(PanelXr *panel) {
    if(stereo_controller.disabled||stereo_controller.ready)return;
    if(!panel->api.CreateSwapchain||!panel->api.EnumerateSwapchainImages||!panel->api.AcquireSwapchainImage||
       !panel->api.WaitSwapchainImage||!panel->api.ReleaseSwapchainImage||!panel->api.DestroySwapchain)return;
    PanelXr *c=&stereo_controller.cursor;
    if(!c->swapchain) {
        c->api=panel->api;c->session=panel->session;
        XrSwapchainCreateInfo info={0};info.type=XR_TYPE_SWAPCHAIN_CREATE_INFO;
        info.createFlags=XR_SWAPCHAIN_CREATE_STATIC_IMAGE_BIT;
        info.usageFlags=XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT|XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
        info.format=29;info.sampleCount=1;info.width=info.height=64;info.faceCount=info.arraySize=info.mipCount=1;
        uint32_t count=0;
        if(c->api.CreateSwapchain(c->session,&info,&c->swapchain)!=XR_SUCCESS||!c->swapchain||
           c->api.EnumerateSwapchainImages(c->swapchain,0,&count,0)!=XR_SUCCESS||!count||count>16)goto fail;
        for(unsigned i=0;i<count;++i)c->images[i].type=XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR;
        if(c->api.EnumerateSwapchainImages(c->swapchain,count,&count,(XrSwapchainImageBaseHeader *)c->images)!=XR_SUCCESS)goto fail;
        c->image_count=count;
        XrSwapchainImageAcquireInfo acquire={0};acquire.type=XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO;
        if(c->api.AcquireSwapchainImage(c->swapchain,&acquire,&c->index)!=XR_SUCCESS)goto fail;
        c->acquired=1;
    }
    if(c->acquired&&!c->waited) {
        XrSwapchainImageWaitInfo wait={0};wait.type=XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO;wait.timeout=0;
        XrResult result=c->api.WaitSwapchainImage(c->swapchain,&wait);
        if(result==XR_TIMEOUT_EXPIRED)return;
        if(result!=XR_SUCCESS||c->index>=c->image_count||!c->images[c->index].texture)goto fail;
        c->waited=1;controller_cursor_destination=c->images[c->index].texture;
        __atomic_store_n(&controller_cursor_state,PC_REQUEST,__ATOMIC_RELEASE);
    }
    unsigned state=__atomic_load_n(&controller_cursor_state,__ATOMIC_ACQUIRE);
    if(state==PC_DONE) {
        if(!panel_image_release(c))goto fail;
        stereo_controller.ready=1;event("controller_cursor_ready",0,GetCurrentThreadId());
    } else if(state==PC_FAULT||state==PC_SKIPPED)goto fail;
    return;
fail:
    stereo_controller.disabled=1;controller_runtime_cancel();
    event("controller_cursor_unavailable",0,GetCurrentThreadId());
}
static const XrCompositionLayerQuad *controller_runtime_panel(const XrFrameEndInfo *end) {
    if(!end||!end->layers)return 0;
    const XrCompositionLayerQuad *quad=0;
    for(unsigned i=0;i<end->layerCount;++i)if(end->layers[i]&&end->layers[i]->type==XR_TYPE_COMPOSITION_LAYER_QUAD) {
        if(quad)return 0;quad=(const XrCompositionLayerQuad *)end->layers[i];
    }
    return quad;
}
static int controller_runtime_prepare_geometry(PanelXr *panel,int focused,unsigned mode,
    XrSwapchain identity,U64 owner,const ControllerPanel *presented,XrCompositionLayerQuad *cursor) {
#ifdef GF2VR_ACTIVITY_DIALOGUE_RAY_OBSERVE
#define DRO_STAGE(n) do{if(mode==2)dialogue_ray_observation.stage=(n);}while(0)
    DRO_STAGE(31);
#endif
    stereo_controller.hit_valid=0;
#ifdef GF2VR_CONTROLLER_GAME_MODE
    if(controller_game_runtime_busy()){controller_runtime_cancel();return 0;}
#endif
    ControllerActions *actions=&stereo_aux.pointer;ControllerWin32 *sink=&stereo_controller.sink;
    U64 now=GetTickCount64();
#ifdef GF2VR_ACTIVITY_DIALOGUE_RAY_OBSERVE
    if(mode==2)dialogue_ray_observation.prepare_flags=(focused!=0)|((actions->tracked!=0)<<1)|
        ((stereo_controller.sample_tick!=0)<<2)|
        ((now>=stereo_controller.sample_tick && now-stereo_controller.sample_tick<=250)<<3);
#endif
    if(!focused||!presented||!cursor||!actions->tracked||
       !stereo_controller.sample_tick||now<stereo_controller.sample_tick||now-stereo_controller.sample_tick>250)return 0;
#ifdef GF2VR_ACTIVITY_DIALOGUE_RAY_OBSERVE
    DRO_STAGE(32);
#endif
    HWND window=__atomic_load_n(&controller_game_window,__ATOMIC_ACQUIRE);RECT rect={0};DWORD process=0;
    if(!window||!sink->api.Foreground||sink->api.Foreground()!=window||!sink->api.WindowProcess||
       !sink->api.WindowProcess(window,&process)||process!=GetCurrentProcessId()||
       !sink->api.ClientRect||!sink->api.ClientRect(window,&rect)||rect.left||rect.top||rect.right<=0||rect.bottom<=0)return 0;
    unsigned width=(unsigned)rect.right,height=(unsigned)rect.bottom;
#ifdef GF2VR_ACTIVITY_DIALOGUE_RAY_OBSERVE
    if(mode==2){dialogue_ray_observation.width=width;dialogue_ray_observation.height=height;}
    DRO_STAGE(33);
#endif
    if(presented->client_width && (presented->client_width!=width || presented->client_height!=height))return 0;
    U64 generation=mode?__atomic_load_n(&stereo_publication_generation,__ATOMIC_ACQUIRE):0;
    U64 epoch=__atomic_load_n(&stereo_eye_runtime.mailbox.epoch,__ATOMIC_ACQUIRE);
    controller_runtime_cursor(panel);
#ifdef GF2VR_ACTIVITY_DIALOGUE_RAY_OBSERVE
    DRO_STAGE(34);
#endif
    if(!stereo_controller.ready)return 0;
    if(stereo_controller.panel_identity!=identity||stereo_controller.surface_identity!=owner||stereo_controller.window_identity!=window||
       stereo_controller.width_identity!=width||stereo_controller.height_identity!=height||stereo_controller.mode_identity!=mode||
       stereo_controller.generation_identity!=generation||stereo_controller.epoch_identity!=epoch||
       (stereo_controller.last_tick&&(now<stereo_controller.last_tick||now-stereo_controller.last_tick>250))) {
        controller_runtime_cancel();++stereo_controller.token;
        stereo_controller.panel_identity=identity;stereo_controller.surface_identity=owner;stereo_controller.window_identity=window;
        stereo_controller.width_identity=width;stereo_controller.height_identity=height;stereo_controller.mode_identity=mode;
        stereo_controller.generation_identity=generation;stereo_controller.epoch_identity=epoch;
    }
    if(sink->escape_owned||(!stereo_controller.input.pressed&&sink->mouse_owned)) {
#ifdef GF2VR_ACTIVITY_DIALOGUE_RAY_OBSERVE
        DRO_STAGE(35);
#endif
        if(!controller_win32_cancel(sink))return 0;
    }
    sink->window=window;sink->process=process;sink->width=width;sink->height=height;
    ControllerPanel geometry=*presented;
    geometry.client_width=width;geometry.client_height=height;
    ControllerPose aim={{actions->pose.position.x,actions->pose.position.y,actions->pose.position.z},
        {actions->pose.orientation.x,actions->pose.orientation.y,actions->pose.orientation.z,actions->pose.orientation.w}};
#ifdef GF2VR_ACTIVITY_DIALOGUE_RAY_OBSERVE
    if(mode==2){dialogue_ray_observation.view=geometry;dialogue_ray_observation.aim=aim;}
    DRO_STAGE(36);
#endif
    if(!controller_panel_hit(&aim,&geometry,&stereo_controller.hit))return 0;
    /* Cursor and injected input use the same stabilized client coordinate.
       Reproject onto this CURRENT panel, not an old moving-panel world point. */
    unsigned display_x,display_y;
    controller_input_coordinates(&stereo_controller.input,GetTickCount64(),&stereo_controller.hit,
        actions->face_down||actions->trigger_down,&display_x,&display_y);
    cursor->layerFlags=XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
    cursor->subImage.swapchain=stereo_controller.cursor.swapchain;
    cursor->subImage.imageRect=(XrRect2Di){{0,0},{64,64}};cursor->subImage.imageArrayIndex=0;
    float u=width>1?(float)display_x/(float)(width-1):0.5f;
    float v=height>1?(float)display_y/(float)(height-1):0.5f;
    ControllerVector n=controller_rotate(geometry.pose.orientation,
        (ControllerVector){(u-.5f)*geometry.width,(.5f-v)*geometry.height,.002f});
    cursor->pose.position=(XrVector3f){geometry.pose.position.x+n.x,geometry.pose.position.y+n.y,geometry.pose.position.z+n.z};
    cursor->size.width=cursor->size.height=geometry.width*0.018f;
#ifdef GF2VR_ACTIVITY_DIALOGUE_RAY_OBSERVE
    DRO_STAGE(40);
#undef DRO_STAGE
#endif
    stereo_controller.hit_valid=1;return 1;
}
static int controller_runtime_prepare(PanelXr *panel,const XrFrameEndInfo *end,int focused,unsigned mode,XrCompositionLayerQuad *cursor) {
    stereo_controller.hit_valid=0;
    const XrCompositionLayerQuad *q=controller_runtime_panel(end);
    if(!q || q->space!=panel->space || !q->subImage.swapchain || q->subImage.imageArrayIndex ||
       q->subImage.imageRect.offset.x || q->subImage.imageRect.offset.y ||
       q->subImage.imageRect.extent.width!=(int)panel->width || q->subImage.imageRect.extent.height!=(int)panel->height)return 0;
    ControllerPanel geometry={0};geometry.pose=(ControllerPose){
        {q->pose.position.x,q->pose.position.y,q->pose.position.z},
        {q->pose.orientation.x,q->pose.orientation.y,q->pose.orientation.z,q->pose.orientation.w}};
    geometry.width=q->size.width;geometry.height=q->size.height;geometry.u1=geometry.v1=1;
    *cursor=*q;return controller_runtime_prepare_geometry(panel,focused,mode,q->subImage.swapchain,0,&geometry,cursor);
}
#ifdef GF2VR_ACTIVITY_DIALOGUE_RAY
static int controller_runtime_prepare_dialogue(PanelXr *panel,int focused,
    const DialogueRaySurface *surface,const XrPosef *head,XrCompositionLayerQuad *cursor) {
#ifdef GF2VR_ACTIVITY_DIALOGUE_RAY_OBSERVE
    dialogue_ray_observation.stage=21;
#endif
    stereo_controller.hit_valid=0;
    if(!surface || !surface->valid || !head || !xr_eye_pose_valid(head) || stereo_aux.shown)return 0;
    RECT r={0};ControllerWin32 *sink=&stereo_controller.sink;
    HWND window=__atomic_load_n(&controller_game_window,__ATOMIC_ACQUIRE);
#ifdef GF2VR_ACTIVITY_DIALOGUE_RAY_OBSERVE
    dialogue_ray_observation.stage=22;
#endif
    if(!window || !sink->api.ClientRect || !sink->api.ClientRect(window,&r) || r.left || r.top || r.right<=0 || r.bottom<=0)return 0;
    ControllerPose current={{head->position.x,head->position.y,head->position.z},
        {head->orientation.x,head->orientation.y,head->orientation.z,head->orientation.w}};
    ControllerPanel geometry;
#ifdef GF2VR_ACTIVITY_DIALOGUE_RAY_OBSERVE
    dialogue_ray_observation.width=(unsigned)r.right;dialogue_ray_observation.height=(unsigned)r.bottom;
    dialogue_ray_observation.stage=23;
#endif
    if(!dialogue_ray_view(surface,&current,(unsigned)r.right,(unsigned)r.bottom,&geometry))return 0;
    *cursor=(XrCompositionLayerQuad){.type=XR_TYPE_COMPOSITION_LAYER_QUAD,.space=panel->space,
        .eyeVisibility=XR_EYE_VISIBILITY_BOTH,
        .pose={{geometry.pose.orientation.x,geometry.pose.orientation.y,geometry.pose.orientation.z,geometry.pose.orientation.w},
               {geometry.pose.position.x,geometry.pose.position.y,geometry.pose.position.z}}};
    stereo_controller.dialogue_frame=surface->frame;
    return controller_runtime_prepare_geometry(panel,focused,2,0,surface->owner,&geometry,cursor);
}
#endif
static void controller_runtime_end(XrResult result,int cursor_submitted) {
#ifdef GF2VR_CONTROLLER_GAME_MODE
    controller_game_runtime_end(result);
    if(controller_game_runtime_busy()){controller_runtime_cancel();return;}
#endif
    U64 now=GetTickCount64();ControllerActions *a=&stereo_aux.pointer;
#ifdef GF2VR_ACTIVITY_DIALOGUE_RAY
    if(stereo_controller.mode_identity==2){DialogueRaySurface check;
        if(stereo_aux.shown || __atomic_load_n(&stereo_publication_generation,__ATOMIC_ACQUIRE)!=stereo_controller.generation_identity ||
           stereo_eye_runtime.owner.epoch!=stereo_controller.epoch_identity ||
           stereo_eye_runtime.owner.head_reference_epoch!=stereo_controller.epoch_identity ||
           stereo_eye_runtime.owner.changing || stereo_eye_runtime.owner.retiring ||
           !dialogue_ray_read(&dialogue_ray_mailbox,stereo_controller.generation_identity,stereo_controller.epoch_identity,
                stereo_controller.dialogue_frame,now,&check) || check.owner!=stereo_controller.surface_identity)cursor_submitted=0;
    }
#endif
    static unsigned prior_hit;
    unsigned hit=result==XR_SUCCESS&&cursor_submitted&&stereo_controller.hit_valid;
#ifdef GF2VR_ACTIVITY_DIALOGUE_RAY_OBSERVE
    dialogue_ray_observation.post_flags=(result==XR_SUCCESS)|((cursor_submitted!=0)<<1)|
        ((stereo_controller.hit_valid!=0)<<2)|((stereo_controller.mode_identity==2)<<3);
#endif
#ifdef GF2VR_ACTIVITY_DIALOGUE_RAY
    static unsigned prior_route=3,route_reports;unsigned route=hit?stereo_controller.mode_identity:3;
    if(route!=prior_route && route_reports<64){++route_reports;prior_route=route;
        event("controller_surface",route,stereo_controller.generation_identity);}
#endif
    if(hit!=prior_hit){event("controller_hit",hit,GetCurrentThreadId());prior_hit=hit;}
    if(result!=XR_SUCCESS||!cursor_submitted||!stereo_controller.hit_valid) {controller_runtime_cancel();return;}
    stereo_controller.last_tick=now;
    unsigned before=stereo_controller.input.pressed;
    int ok=controller_input_step(&stereo_controller.input,stereo_controller.token,now,1,&stereo_controller.hit,
        a->face_down,a->trigger_down,a->back_down,controller_win32_emit,&stereo_controller.sink);
    if(!ok)controller_runtime_cancel();
#ifdef GF2VR_ACTIVITY_DIALOGUE_RAY_OBSERVE
    dialogue_ray_observation.input_flags=(ok!=0)|((stereo_controller.input.pressed!=0)<<1)|
        ((a->trigger_down!=0)<<2)|((a->face_down!=0)<<3)|((stereo_controller.input.primary_armed!=0)<<4)|
        ((stereo_controller.input.stable!=0)<<5);
#endif
    if(before!=stereo_controller.input.pressed)event("controller_primary",stereo_controller.input.pressed,GetCurrentThreadId());
}
static int controller_runtime_close(void) {
    controller_runtime_cancel();
    if(stereo_controller.input.pressed||stereo_controller.sink.mouse_owned||stereo_controller.sink.escape_owned)return 0;
    unsigned expected=PC_REQUEST;
    __atomic_compare_exchange_n(&controller_cursor_state,&expected,PC_SKIPPED,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE);
    unsigned state=__atomic_load_n(&controller_cursor_state,__ATOMIC_ACQUIRE);
    if(state==PC_COPYING||state==PC_INFLIGHT||state==PC_FAULT)return 0;
    PanelXr *c=&stereo_controller.cursor;
    if(c->acquired&&!c->waited) {
        XrSwapchainImageWaitInfo wait={0};wait.type=XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO;wait.timeout=0;
        if(c->api.WaitSwapchainImage(c->swapchain,&wait)!=XR_SUCCESS)return 0;c->waited=1;
    }
    if(c->acquired&&!panel_image_release(c))return 0;
    if(c->swapchain) {if(c->api.DestroySwapchain(c->swapchain)!=XR_SUCCESS)return 0;c->swapchain=0;}
    *c=(PanelXr){0};controller_cursor_destination=0;
    __atomic_store_n(&controller_cursor_state,PC_IDLE,__ATOMIC_RELEASE);
    stereo_controller.ready=0;return 1;
}
#endif
