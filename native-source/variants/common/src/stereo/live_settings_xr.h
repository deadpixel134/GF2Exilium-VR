#ifndef GF2VR_LIVE_SETTINGS_XR_H
#define GF2VR_LIVE_SETTINGS_XR_H
/* Session-owned, two-bank UI swapchains. The displayed bank is immutable;
   updates acquire/wait the other bank, upload on Present, then release/switch. */
static struct {
    PanelXr banks[2];unsigned front,have_front,uploading,upload_bank,slot,disabled,hit,host_fault,used;
    U64 sequence[2],open_until,input_sequence;
    DWORD thread;ControllerHit point;ControllerInput input;LiveSettingsInputGate gate;
} live_settings_xr;
static unsigned live_settings_game_allowed;
#ifdef GF2VR_LIVE_SETTINGS_ENTRY_OBSERVE
static U64 live_settings_entry_xr_state;
#endif
static unsigned live_settings_modal(void){
    LiveSettingsSurface *s=__atomic_load_n(&live_settings_wire.surface,__ATOMIC_ACQUIRE);
    U64 now=GetTickCount64();
    if(live_settings_xr.host_fault)return 0;
    if(now<live_settings_xr.open_until)return 1;
    if(!s||!__atomic_load_n(&s->mode,__ATOMIC_ACQUIRE))return 0;
    U64 beat=__atomic_load_n(&s->heartbeat,__ATOMIC_ACQUIRE);
    return beat&&now>=beat&&now-beat<3000;
}
static void live_settings_ui_cancel(void){
    LiveSettingsSurface *s=__atomic_load_n(&live_settings_wire.surface,__ATOMIC_ACQUIRE);
    if(s&&(live_settings_xr.input.token||live_settings_xr.input.pressed)){
        __atomic_add_fetch(&s->session,1,__ATOMIC_ACQ_REL);(void)live_settings_surface_input(s,3,0,0,0);
    }
    live_settings_xr.input=(ControllerInput){0};live_settings_xr.hit=0;
}
static void live_settings_toggle(void){
    unsigned was=live_settings_modal();live_settings_ui_cancel();live_settings_xr.gate.blocked=1;
    live_settings_xr.used=1;
    live_settings_xr.host_fault=0;
    if(!was)live_settings_xr.open_until=GetTickCount64()+3000;
    __atomic_store_n(&live_settings_wire.open_request,was?2:1,__ATOMIC_RELEASE);
}
static int live_settings_ui_emit(void *context,unsigned kind,unsigned x,unsigned y){
    (void)context;LiveSettingsSurface *s=__atomic_load_n(&live_settings_wire.surface,__ATOMIC_ACQUIRE);if(!s)return 0;
    unsigned type=kind==CONTROLLER_DOWN?1:kind==CONTROLLER_UP?2:kind==CONTROLLER_BACK?6:0;
    return !type||live_settings_surface_input(s,type,(int)x,(int)y,(unsigned)live_settings_xr.sequence[live_settings_xr.front]);
}
static int live_settings_surface_bank(PanelXr *parent,PanelXr *c){
    if(c->swapchain)return c->session==parent->session;
    c->api=parent->api;c->session=parent->session;c->space=parent->space;c->width=LSS_WIDTH;c->height=LSS_HEIGHT;
    int64_t formats[64];unsigned count=0;
    if(!c->api.EnumerateSwapchainFormats||c->api.EnumerateSwapchainFormats(c->session,64,&count,formats)!=XR_SUCCESS||count>64)return 0;
    const unsigned choices[]={87,91,28,29};
    for(unsigned preferred=0;preferred<4&&!c->format;++preferred)
        for(unsigned n=0;n<count;++n)if(formats[n]==choices[preferred]){c->format=choices[preferred];break;}
    if(!c->format)return 0;
    XrSwapchainCreateInfo info={0};info.type=XR_TYPE_SWAPCHAIN_CREATE_INFO;
    info.usageFlags=XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT|XR_SWAPCHAIN_USAGE_SAMPLED_BIT;info.format=c->format;
    info.sampleCount=1;info.width=LSS_WIDTH;info.height=LSS_HEIGHT;info.faceCount=info.arraySize=info.mipCount=1;
    if(c->api.CreateSwapchain(c->session,&info,&c->swapchain)!=XR_SUCCESS||!c->swapchain)return 0;
    count=0;if(c->api.EnumerateSwapchainImages(c->swapchain,0,&count,0)!=XR_SUCCESS||!count||count>16)return 0;
    for(unsigned n=0;n<count;++n)c->images[n].type=XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR;
    if(c->api.EnumerateSwapchainImages(c->swapchain,count,&count,(XrSwapchainImageBaseHeader *)c->images)!=XR_SUCCESS)return 0;
    c->image_count=count;return 1;
}
static void live_settings_surface_upload(PanelXr *parent){
    LiveSettingsSurface *s=__atomic_load_n(&live_settings_wire.surface,__ATOMIC_ACQUIRE);
    if(!s||live_settings_xr.disabled)return;
    if(!live_settings_xr.uploading){
        int slot=live_settings_surface_claim(s);if(slot<0)return;
        unsigned bank=live_settings_xr.have_front?live_settings_xr.front^1u:0;
        live_settings_xr.uploading=1;live_settings_xr.upload_bank=bank;live_settings_xr.slot=(unsigned)slot;
        live_settings_xr.sequence[bank]=s->sequences[slot];
        if(!live_settings_surface_bank(parent,&live_settings_xr.banks[bank])){live_settings_xr.disabled=1;return;}
    }
    PanelXr *c=&live_settings_xr.banks[live_settings_xr.upload_bank];
    if(!c->acquired){
        XrSwapchainImageAcquireInfo acquire={0};acquire.type=XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO;
        if(c->api.AcquireSwapchainImage(c->swapchain,&acquire,&c->index)!=XR_SUCCESS){live_settings_xr.disabled=1;return;}c->acquired=1;
    }
    if(!c->waited){
        XrSwapchainImageWaitInfo wait={0};wait.type=XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO;wait.timeout=0;
        XrResult result=c->api.WaitSwapchainImage(c->swapchain,&wait);if(result==XR_TIMEOUT_EXPIRED)return;
        if(result!=XR_SUCCESS||c->index>=c->image_count||!c->images[c->index].texture){live_settings_xr.disabled=1;return;}c->waited=1;
        live_settings_gpu.surface=s;live_settings_gpu.slot=live_settings_xr.slot;live_settings_gpu.destination=c->images[c->index].texture;
        live_settings_gpu.format=c->format;__atomic_store_n(&live_settings_gpu.state,PC_REQUEST,__ATOMIC_RELEASE);
    }
    unsigned state=__atomic_load_n(&live_settings_gpu.state,__ATOMIC_ACQUIRE);
    if(state==PC_DONE){
        if(!panel_image_release(c)){live_settings_xr.disabled=1;return;}
        c->have_image=1;
        live_settings_surface_release(s,live_settings_xr.slot);live_settings_xr.uploading=0;
        live_settings_xr.front=live_settings_xr.upload_bank;live_settings_xr.have_front=1;
        __atomic_store_n(&live_settings_gpu.state,PC_IDLE,__ATOMIC_RELEASE);
    }else if(state==PC_FAULT||state==PC_SKIPPED)live_settings_xr.disabled=1;
}
static void live_settings_xr_tick(PanelXr *parent,int focused){
    live_settings_xr.thread=GetCurrentThreadId();
    LiveSettingsSurface *s=__atomic_load_n(&live_settings_wire.surface,__ATOMIC_ACQUIRE);U64 now=GetTickCount64();
    if(s&&__atomic_load_n(&s->mode,__ATOMIC_ACQUIRE)&&now>=live_settings_xr.open_until){
        U64 beat=__atomic_load_n(&s->heartbeat,__ATOMIC_ACQUIRE);
        if(!beat||now<beat||now-beat>=3000){live_settings_xr.host_fault=1;live_settings_ui_cancel();}
    }
    unsigned modal=(live_settings_modal()&&stereo_aux.shown)||live_settings_runtime_busy();
    if(modal)live_settings_xr.used=1;
    unsigned neutral=1;
    for(unsigned n=0;n<4;++n){float v=stereo_aux.drag.frame.values[n];if(!(v>=0&&v<.1f))neutral=0;}
    for(unsigned n=0;n<2;++n){
        float x=stereo_aux.navigation.frame.x[n],y=stereo_aux.navigation.frame.y[n];
        if(!(x>-.1f&&x<.1f&&y>-.1f&&y<.1f)||stereo_aux.game.first[n]||stereo_aux.game.second[n])neutral=0;
    }
    if(stereo_aux.pointer.face_down||stereo_aux.pointer.trigger_down||stereo_aux.pointer.back_down||stereo_aux.down)neutral=0;
    unsigned owned=stereo_controller.sink.mouse_owned||stereo_controller.sink.escape_owned||stereo_game.keys.owned||stereo_game.mode.axis_owned||stereo_game.mode.owned_keys;
    if(live_settings_xr.gate.blocked&&(!stereo_aux.pointer.tracked||owned))neutral=0;
    // Preserve the independent per-hand policies until this feature owns a modal/apply transition.
    live_settings_game_allowed=live_settings_xr.used?live_settings_input_allowed(&live_settings_xr.gate,modal,focused,neutral):1;
    if(!live_settings_game_allowed){controller_runtime_cancel();controller_game_runtime_cancel();stereo_navigation_cancel();stereo_world_drag_reset();}
    if(!modal||!focused)live_settings_ui_cancel();
    if(live_settings_modal())live_settings_surface_upload(parent);
#ifdef GF2VR_LIVE_SETTINGS_ENTRY_OBSERVE
    unsigned slots=s?__atomic_load_n(&s->slots[0],__ATOMIC_ACQUIRE)|(__atomic_load_n(&s->slots[1],__ATOMIC_ACQUIRE)<<3):0;
    unsigned age=3;
    if(s){U64 beat=__atomic_load_n(&s->heartbeat,__ATOMIC_ACQUIRE);if(beat&&now>=beat)age=(unsigned)((now-beat)/1000);if(age>3)age=3;}
    U64 observed=live_settings_modal()|((U64)live_settings_xr.disabled<<1)|((U64)live_settings_xr.have_front<<2)|
        ((U64)live_settings_xr.uploading<<3)|((U64)live_settings_xr.host_fault<<4)|((U64)stereo_aux.shown<<5)|
        ((U64)stereo_aux.tracked<<6)|((U64)(focused!=0)<<7)|((U64)(s!=0)<<8)|
        ((U64)(s?__atomic_load_n(&s->mode,__ATOMIC_ACQUIRE):0)<<9)|((U64)age<<10)|((U64)slots<<12)|
        ((U64)__atomic_load_n(&live_settings_gpu.state,__ATOMIC_ACQUIRE)<<20)|
        ((U64)(s?__atomic_load_n(&s->sequence,__ATOMIC_ACQUIRE):0)<<32);
    __atomic_store_n(&live_settings_entry_xr_state,observed,__ATOMIC_RELEASE);
#endif
}
static int live_settings_xr_quad(PanelXr *parent,int focused,XrCompositionLayerQuad *quad,XrCompositionLayerQuad *cursor){
    live_settings_xr.hit=0;
    if(!live_settings_modal()||!aux_visible(&stereo_aux)||!live_settings_xr.have_front)return 0;
    PanelXr *c=&live_settings_xr.banks[live_settings_xr.front];
    if(c->session!=parent->session||!c->have_image)return 0;
    *quad=(XrCompositionLayerQuad){0};quad->type=XR_TYPE_COMPOSITION_LAYER_QUAD;quad->eyeVisibility=XR_EYE_VISIBILITY_BOTH;
    quad->space=parent->space;quad->pose=(XrPosef){stereo_aux.orientation,stereo_aux.position};
    quad->subImage.swapchain=c->swapchain;quad->subImage.imageRect.extent=(XrExtent2Di){LSS_WIDTH,LSS_HEIGHT};
    quad->size=(XrExtent2Df){.65f,.65f*LSS_HEIGHT/LSS_WIDTH};
    if(stereo_aux.placement.configured&&!aux_placement_size(&stereo_aux.placement,LSS_WIDTH,LSS_HEIGHT,&quad->size.width,&quad->size.height))return 0;
    ControllerActions *a=&stereo_aux.pointer;
    if(!focused||!a->tracked)return 1;
    ControllerPanel geometry={0};geometry.pose=(ControllerPose){{quad->pose.position.x,quad->pose.position.y,quad->pose.position.z},
        {quad->pose.orientation.x,quad->pose.orientation.y,quad->pose.orientation.z,quad->pose.orientation.w}};
    geometry.width=quad->size.width;geometry.height=quad->size.height;geometry.u1=geometry.v1=1;geometry.client_width=LSS_WIDTH;geometry.client_height=LSS_HEIGHT;
    ControllerPose aim={{a->pose.position.x,a->pose.position.y,a->pose.position.z},{a->pose.orientation.x,a->pose.orientation.y,a->pose.orientation.z,a->pose.orientation.w}};
    if(!controller_panel_hit(&aim,&geometry,&live_settings_xr.point))return 1;
    live_settings_xr.hit=1;controller_runtime_cursor(parent);if(!stereo_controller.ready)return 1;
    *cursor=*quad;cursor->layerFlags=XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
    cursor->subImage.swapchain=stereo_controller.cursor.swapchain;cursor->subImage.imageRect.extent=(XrExtent2Di){64,64};
    ControllerVector point=controller_rotate(geometry.pose.orientation,(ControllerVector){(live_settings_xr.point.u-.5f)*geometry.width,(.5f-live_settings_xr.point.v)*geometry.height,.002f});
    cursor->pose.position=(XrVector3f){quad->pose.position.x+point.x,quad->pose.position.y+point.y,quad->pose.position.z+point.z};
    cursor->size.width=cursor->size.height=geometry.width*.018f;return 2;
}
static void live_settings_xr_end(XrResult result,int displayed){
    static unsigned previous;
    unsigned shown=result==XR_SUCCESS&&displayed;
    if(shown!=previous){event("live_settings_panel_displayed",shown,GetCurrentThreadId());previous=shown;}
    if(result!=XR_SUCCESS||!displayed||!live_settings_xr.hit){live_settings_ui_cancel();return;}
    U64 sequence=live_settings_xr.sequence[live_settings_xr.front];
    if(sequence!=live_settings_xr.input_sequence){live_settings_ui_cancel();live_settings_xr.input_sequence=sequence;}
    ControllerActions *a=&stereo_aux.pointer;
    if(!controller_input_step(&live_settings_xr.input,sequence,GetTickCount64(),1,&live_settings_xr.point,
        a->face_down,a->trigger_down,a->back_down,live_settings_ui_emit,0))live_settings_ui_cancel();
}
static int live_settings_compose(PanelXr *parent,int visible,int projected,XrFrameEndInfo *end,
    XrCompositionLayerQuad *quad,XrCompositionLayerQuad *cursor,const XrCompositionLayerBaseHeader **layers){
    int count=visible?live_settings_xr_quad(parent,stereo_hold_focused,quad,cursor):0;
    if(!count||end->layerCount>3)return 0;
    unsigned retained=projected?1:end->layerCount;
    for(unsigned n=0;n<retained;++n)layers[n]=end->layers[n];
    layers[retained++]=(const XrCompositionLayerBaseHeader *)quad;
    if(count==2&&retained<4)layers[retained++]=(const XrCompositionLayerBaseHeader *)cursor;
    end->layers=layers;end->layerCount=retained;return 1;
}
static int live_settings_xr_close(void){
    live_settings_ui_cancel();live_settings_game_allowed=0;
    unsigned expected=PC_REQUEST;
    (void)__atomic_compare_exchange_n(&live_settings_gpu.state,&expected,PC_SKIPPED,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE);
    unsigned state=__atomic_load_n(&live_settings_gpu.state,__ATOMIC_ACQUIRE);
    if(state==PC_COPYING||state==PC_INFLIGHT||state==PC_FAULT)return 0;
    for(unsigned n=0;n<2;++n){PanelXr *c=&live_settings_xr.banks[n];
        if(c->acquired&&!c->waited){XrSwapchainImageWaitInfo wait={0};wait.type=XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO;wait.timeout=0;
            if(c->api.WaitSwapchainImage(c->swapchain,&wait)!=XR_SUCCESS)return 0;c->waited=1;}
        if(c->acquired&&!panel_image_release(c))return 0;
        if(c->swapchain){if(c->api.DestroySwapchain(c->swapchain)!=XR_SUCCESS)return 0;c->swapchain=0;}
    }
    LiveSettingsSurface *s=__atomic_load_n(&live_settings_wire.surface,__ATOMIC_ACQUIRE);
    if(s&&live_settings_xr.uploading)live_settings_surface_release(s,live_settings_xr.slot);
    live_settings_gpu.destination=0;live_settings_gpu.surface=0;__atomic_store_n(&live_settings_gpu.state,PC_IDLE,__ATOMIC_RELEASE);
    live_settings_xr=(typeof(live_settings_xr)){0};return 1;
}
#endif
