#ifndef GF2VR_LIVE_SETTINGS_RUNTIME_H
#define GF2VR_LIVE_SETTINGS_RUNTIME_H
#include "live_settings_transaction.h"
static LiveSettingsTransaction live_settings_transaction;
static struct {
    const LiveSettingsDocuments *documents;const LiveSettingsValues *values;
    unsigned rollback,stage,xr_request,xr_done,xr_ok,xr_active,xr_session_serial,seen_session;
    unsigned freeze,await_new_frame,has_applied,reconcile_session;
    U64 first_frame,extent,limits_base,limits_max;
    PFN_xrGetInstanceProcAddr get;XrInstance instance;XrSystemId system;
    int before_w,before_h,before_mode,wanted_w,wanted_h,wanted_mode;
    unsigned screen_captured,immersive,eye_changed,contact_uncertain;
    PcOutput before_pc;
} live_settings_runtime;
static unsigned live_settings_runtime_busy(void){return __atomic_load_n(&live_settings_runtime.freeze,__ATOMIC_ACQUIRE);}
static void live_settings_note_frame(U64 serial){
    if(serial&&!live_settings_runtime_busy()&&__atomic_load_n(&live_settings_runtime.await_new_frame,__ATOMIC_ACQUIRE)&&!__atomic_load_n(&live_settings_runtime.first_frame,__ATOMIC_ACQUIRE))
        __atomic_store_n(&live_settings_runtime.first_frame,serial,__ATOMIC_RELEASE);
}
static unsigned live_settings_projection_allowed(unsigned fresh,unsigned held,U64 serial){
    if(live_settings_runtime_busy())return 0;
    if(!__atomic_load_n(&live_settings_runtime.await_new_frame,__ATOMIC_ACQUIRE))return 1;
    U64 first=__atomic_load_n(&live_settings_runtime.first_frame,__ATOMIC_ACQUIRE);
    if(!fresh||held||!first||serial<first)return 0;
    __atomic_store_n(&live_settings_runtime.await_new_frame,0,__ATOMIC_RELEASE);return 1;
}
static void live_settings_runtime_xr_open(PFN_xrGetInstanceProcAddr get,XrInstance instance,XrSystemId system){
    LiveSettingsSurface *surface=__atomic_load_n(&live_settings_wire.surface,__ATOMIC_ACQUIRE);
    if(surface){__atomic_add_fetch(&surface->session,1,__ATOMIC_ACQ_REL);__atomic_add_fetch(&surface->redraw,1,__ATOMIC_ACQ_REL);}
    live_settings_runtime.get=get;live_settings_runtime.instance=instance;live_settings_runtime.system=system;
    PFN_xrEnumerateViewConfigurationViews enumerate=0;XrViewConfigurationView views[2]={{0},{0}};unsigned count=0;
    views[0].type=views[1].type=XR_TYPE_VIEW_CONFIGURATION_VIEW;
    unsigned bw=0,bh=0,mw=16384,mh=16384;
    if(get&&get(instance,"xrEnumerateViewConfigurationViews",(PFN_xrVoidFunction *)&enumerate)==XR_SUCCESS&&enumerate&&
       enumerate(instance,system,XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,2,&count,views)==XR_SUCCESS&&count==2){
        for(unsigned n=0;n<2;++n){if(bw<views[n].recommendedImageRectWidth)bw=views[n].recommendedImageRectWidth;
            if(bh<views[n].recommendedImageRectHeight)bh=views[n].recommendedImageRectHeight;
            if(mw>views[n].maxImageRectWidth)mw=views[n].maxImageRectWidth;if(mh>views[n].maxImageRectHeight)mh=views[n].maxImageRectHeight;}
    }
    __atomic_store_n(&live_settings_runtime.limits_max,((U64)mh<<32)|mw,__ATOMIC_RELEASE);
    __atomic_store_n(&live_settings_runtime.limits_base,((U64)bh<<32)|bw,__ATOMIC_RELEASE);
    __atomic_add_fetch(&live_settings_runtime.xr_session_serial,1,__ATOMIC_ACQ_REL);
    __atomic_store_n(&live_settings_runtime.xr_active,1,__ATOMIC_RELEASE);
    if(__atomic_load_n(&live_settings_runtime.has_applied,__ATOMIC_ACQUIRE)){
        __atomic_store_n(&live_settings_runtime.freeze,1,__ATOMIC_RELEASE);
        __atomic_store_n(&live_settings_runtime.reconcile_session,1,__ATOMIC_RELEASE);
    }
}
static void live_settings_runtime_xr_closed(void){
    __atomic_store_n(&live_settings_runtime.xr_active,0,__ATOMIC_RELEASE);
    live_settings_runtime.get=0;live_settings_runtime.instance=0;live_settings_runtime.system=0;
}
static int live_settings_contact_call(void *method,const LiveSettingsDocument *document){
    if(!method||!document||document->length>4096||!touch_p2_bind())return 0;
    CompositionImageApi *a=&t2.api;unsigned length=document->length;void *array=a->array_new(a->byte_class,length);if(!array)return 0;
    unsigned root=a->pin(array,1);if(!root)return 0;int ok=a->object_class(array)==a->byte_array_class&&a->array_length(array)==length;
    for(unsigned n=0;ok&&n<length;++n){
        uint8_t value=(uint8_t)document->text[n];int32_t index=(int32_t)n;void *box=a->value_box(a->byte_class,&value);
        if(!box||a->object_class(box)!=a->byte_class){ok=0;break;}
        void *args[]={box,&index},*exception=0;a->invoke(a->set_value,array,args,&exception);if(exception){ok=0;break;}
        void *get[]={&index};void *read=a->invoke(a->get_value,array,get,&exception);
        const uint8_t *actual=read&&!exception&&a->object_class(read)==a->byte_class?a->unbox(read):0;
        if(!actual||*actual!=value)ok=0;
    }
    int status=-999;if(ok){void *args[]={array,&status};ok=touch_p2_void(method,args,&status)&&status==1;}
    a->unpin(root);return ok;
}
static int live_settings_prepare_values(const LiveSettingsDocuments *docs,const LiveSettingsValues *v){
    if(render_guard||stereo_pipeline.render.active||stereo_start.busy)return 0;
    if(__atomic_load_n(&live_settings_runtime.xr_active,__ATOMIC_ACQUIRE)&&
       __atomic_load_n(&live_settings_runtime.xr_request,__ATOMIC_ACQUIRE)!=__atomic_load_n(&live_settings_runtime.xr_done,__ATOMIC_ACQUIRE))return 0;
    U64 base=__atomic_load_n(&live_settings_runtime.limits_base,__ATOMIC_ACQUIRE),maximum=__atomic_load_n(&live_settings_runtime.limits_max,__ATOMIC_ACQUIRE);
    if(__atomic_load_n(&live_settings_runtime.xr_active,__ATOMIC_ACQUIRE)){
        unsigned w=0,h=0;
        if(!base||!render_scale_dimension((unsigned)base,(unsigned)maximum,v->eye_scale,&w)||
           !render_scale_dimension((unsigned)(base>>32),(unsigned)(maximum>>32),v->eye_scale,&h))return 0;
    }
    if(!performance_runtime.initialized)performance_bind();
    if(!performance_runtime.set||!performance_runtime.width||!performance_runtime.height||!performance_runtime.mode)return 0;
#ifdef GF2VR_DIRTY_CONTACT
    touch_p2_config_load();if(!touch_p2_bind())return 0;
    HINSTANCE game=GetModuleHandleW(L"GameAssembly.dll");
    unsigned char (*generic)(void *)=game?(void *)il2cpp_export(game,"il2cpp_method_is_generic"):0;
    const char *args[]={"System.Byte[]","System.Int32&"};
    void *validate=generic?eye_binding_method_args(t2.image.bridge_class,"ValidateContact",2,args,"System.Void",1,generic):0;
    if(!live_settings_contact_call(validate,&docs->files[LSD_CONTACT]))return 0;
#else
    (void)docs;
#endif
    // A fresh transaction captures restoration authority once, before any consumer changes.
    live_settings_runtime.documents=0;live_settings_runtime.values=0;live_settings_runtime.stage=0;live_settings_runtime.screen_captured=0;
    return 1;
}
static int live_settings_xr_values(const LiveSettingsValues *v){
    if(stereo_aux.ready){
        if(v->panel.configured&&(!stereo_aux.placement_math.sin||!stereo_aux.placement_math.cos||!stereo_aux.placement_math.sqrt||!stereo_aux.placement_math.atan2)){
            HINSTANCE module=0;if(!GetModuleHandleExW(0,L"ucrtbase.dll",&module))return 0;
            stereo_aux.placement_module=module;
            stereo_aux.placement_math=(AuxPlacementMath){(void *)GetProcAddress(module,"sin"),(void *)GetProcAddress(module,"cos"),
                (void *)GetProcAddress(module,"sqrt"),(void *)GetProcAddress(module,"atan2")};
            if(!stereo_aux.placement_math.sin||!stereo_aux.placement_math.cos||!stereo_aux.placement_math.sqrt||!stereo_aux.placement_math.atan2)return 0;
        }
        unsigned changed=stereo_aux.panel_hand!=v->roles||stereo_aux.pointer.face_swap!=v->face||
            stereo_aux.placement.configured!=v->panel.configured||stereo_aux.placement.viewer_facing!=v->panel.viewer_facing||
            stereo_aux.placement.apply_angles_when_facing!=v->panel.apply_angles_when_facing||
            stereo_aux.placement.maximum_width!=v->panel.maximum_width||stereo_aux.placement.maximum_height!=v->panel.maximum_height||
            stereo_aux.placement.screen_tilt!=v->panel.screen_tilt;
        for(unsigned n=0;n<3;++n)if(stereo_aux.placement.offset[n]!=v->panel.offset[n]||stereo_aux.placement.rotation[n]!=v->panel.rotation[n])changed=1;
        controller_runtime_cancel();controller_game_runtime_cancel();live_settings_ui_cancel();live_settings_xr.gate.blocked=1;
        stereo_aux.panel_hand=v->roles;stereo_aux.pointer.hand=v->roles;stereo_aux.pointer.face_swap=v->face;
        stereo_aux.placement=v->panel;
        if(changed){stereo_aux.tracked=0;controller_actions_invalidate(&stereo_aux.pointer);stereo_aux.armed=stereo_aux.down=0;live_settings_gesture_reset(&stereo_aux.settings_grip);}
#ifdef GF2VR_DIRTY_CONTACT
        if(stereo_aux.haptics.settings.amplitude!=v->haptics.amplitude||stereo_aux.haptics.settings.duration!=v->haptics.duration)
            contact_haptic_cancel(&stereo_aux.haptics);
        stereo_aux.haptics.settings=v->haptics;
#endif
    }
    stereo_drag_settings=v->drag;stereo_world_drag_reset();stereo_navigation_cancel();
    XrEyeSize candidate={0};
    if(!xr_eye_size_open_scaled(&candidate,live_settings_runtime.get,live_settings_runtime.instance,live_settings_runtime.system,v->eye_scale))return 0;
    U64 packed=__atomic_load_n(&candidate.packed,__ATOMIC_ACQUIRE);
    __atomic_store_n(&stereo_xr_size.packed,packed,__ATOMIC_RELEASE);__atomic_store_n(&live_settings_runtime.extent,packed,__ATOMIC_RELEASE);return 1;
}
static void live_settings_runtime_xr_tick(void){
    unsigned request=__atomic_load_n(&live_settings_runtime.xr_request,__ATOMIC_ACQUIRE);
    if(!request||request==__atomic_load_n(&live_settings_runtime.xr_done,__ATOMIC_ACQUIRE))return;
    int ok=live_settings_xr_values(live_settings_runtime.values);
    __atomic_store_n(&live_settings_runtime.xr_ok,ok,__ATOMIC_RELEASE);
    __atomic_store_n(&live_settings_runtime.xr_done,request,__ATOMIC_RELEASE);
}
static int live_settings_json_same(const char *a,unsigned an,const char *b,unsigned bn){
    unsigned ai=0,bi=0,quoted=0,escaped=0;
    if(an>=3&&(unsigned char)a[0]==239&&(unsigned char)a[1]==187&&(unsigned char)a[2]==191)ai=3;
    if(bn>=3&&(unsigned char)b[0]==239&&(unsigned char)b[1]==187&&(unsigned char)b[2]==191)bi=3;
    for(;;){
        if(!quoted){while(ai<an&&(a[ai]==' '||a[ai]=='\r'||a[ai]=='\n'||a[ai]=='\t'))++ai;
            while(bi<bn&&(b[bi]==' '||b[bi]=='\r'||b[bi]=='\n'||b[bi]=='\t'))++bi;}
        if(ai==an||bi==bn)return ai==an&&bi==bn;
        char c=a[ai++];if(c!=b[bi++])return 0;
        if(quoted){if(escaped)escaped=0;else if(c=='\\')escaped=1;else if(c=='"')quoted=0;}else if(c=='"')quoted=1;
    }
}
static int live_settings_main_values(const LiveSettingsDocuments *docs,const LiveSettingsValues *v){
    stereo_world_scale_settings=v->world;stereo_world_scale_loaded=stereo_world_scale_valid=1;
    stereo_depth_requested=v->depth;stereo_depth_loaded=stereo_depth_config_valid=1;
    stereo_navigation_settings=v->navigation;stereo_navigation_config_loaded=1;stereo_stick_navigation.move_armed=stereo_stick_navigation.turn_armed=0;
    stereo_navigation_cancel();world_drag_handoff_clear(&stereo_drag_handoff);
    __atomic_store_n(&controller_game_move_hand,v->navigation.locomotion_hand,__ATOMIC_RELEASE);
    stereo_camera_follow_config_loaded=1;stereo_camera_follow_state.disabled=!v->follow;
    primary_ui_loaded=1;primary_ui_enabled=v->activity;
    stereo_late_config(v->performance.late_relocate);performance_runtime.config=v->performance;
    touch_p2_config_load();unsigned hands_changed=t2.enabled!=v->hands;
#ifdef GF2VR_DIRTY_CONTACT
    const LiveSettingsDocument *contact=&docs->files[LSD_CONTACT];
    unsigned contact_changed=live_settings_runtime.contact_uncertain||!t2.contact_configured||
        !live_settings_json_same(t2.contact_config,t2.contact_config_length,contact->text,contact->length);
    if(hands_changed||contact_changed)touch_p2_reset();
    if(contact_changed){
        live_settings_runtime.contact_uncertain=1;
        if(!live_settings_contact_call(t2.contact_configure,contact)||t2.fault)return 0;
        for(unsigned n=0;n<contact->length;++n)t2.contact_config[n]=contact->text[n];
        t2.contact_config_length=contact->length;t2.contact_configured=1;live_settings_runtime.contact_uncertain=0;
    }
#else
    (void)docs;if(hands_changed)touch_p2_reset();
#endif
    t2.enabled=v->hands;
    if(v->logging){static unsigned short path[1024];unsigned n=gf2vr_path_length(gf2vr_paths.root);
        if(gf2vr_path_join(path,gf2vr_paths.root,n,L"vrmod\\logs"))CreateDirectoryW(path,0);
        if(gf2vr_path_join(path,gf2vr_paths.root,n,L"vrmod\\logs\\bootstrap"))CreateDirectoryW(path,0);}
    __atomic_store_n(&gf2vr_runtime_logging_enabled,v->logging,__ATOMIC_RELEASE);return 1;
}
static int live_settings_apply_values(const LiveSettingsDocuments *docs,const LiveSettingsValues *v,unsigned rollback){
    if(render_guard||stereo_pipeline.render.active||stereo_start.busy)return -1;
    unsigned session=__atomic_load_n(&live_settings_runtime.xr_session_serial,__ATOMIC_ACQUIRE);
    if(live_settings_runtime.values!=v||live_settings_runtime.rollback!=rollback||live_settings_runtime.seen_session!=session){
        // Never replace an immutable XR request while its consumer can still read it.
        if(__atomic_load_n(&live_settings_runtime.xr_request,__ATOMIC_ACQUIRE)!=__atomic_load_n(&live_settings_runtime.xr_done,__ATOMIC_ACQUIRE)&&
           __atomic_load_n(&live_settings_runtime.xr_active,__ATOMIC_ACQUIRE))return -1;
        live_settings_runtime.documents=docs;live_settings_runtime.values=v;live_settings_runtime.rollback=rollback;live_settings_runtime.stage=0;
        live_settings_runtime.seen_session=session;
    }
    if(live_settings_runtime.stage==0){
        __atomic_store_n(&live_settings_runtime.freeze,1,__ATOMIC_RELEASE);
        __atomic_store_n(&live_settings_runtime.await_new_frame,1,__ATOMIC_RELEASE);
        __atomic_store_n(&live_settings_runtime.first_frame,0,__ATOMIC_RELEASE);
        if(!live_settings_runtime.screen_captured){
            if(!performance_screen_read(0,&live_settings_runtime.before_w,&live_settings_runtime.before_h,&live_settings_runtime.before_mode))return 0;
            live_settings_runtime.before_pc=performance_runtime.pc;live_settings_runtime.screen_captured=1;
            live_settings_runtime.immersive=stereo_source.manual&&stereo_source.manual->state==MTG_ACTIVE&&!performance_manual_flat;
        }
        int wanted=v->performance.pc.fixed&&(v->performance.pc.always||live_settings_runtime.immersive);
        live_settings_runtime.wanted_w=rollback?live_settings_runtime.before_w:wanted?(int)v->performance.pc.width:
            live_settings_runtime.before_pc.captured?live_settings_runtime.before_pc.width:live_settings_runtime.before_w;
        live_settings_runtime.wanted_h=rollback?live_settings_runtime.before_h:wanted?(int)v->performance.pc.height:
            live_settings_runtime.before_pc.captured?live_settings_runtime.before_pc.height:live_settings_runtime.before_h;
        live_settings_runtime.wanted_mode=live_settings_runtime.before_mode;
        int w=0,h=0,mode=0;if(!performance_screen_read(0,&w,&h,&mode))return 0;
        if((w!=live_settings_runtime.wanted_w||h!=live_settings_runtime.wanted_h||mode!=live_settings_runtime.wanted_mode)&&
           !performance_screen_set(0,live_settings_runtime.wanted_w,live_settings_runtime.wanted_h,live_settings_runtime.wanted_mode))return 0;
        // Main, managed and XR changes are hidden behind the same display/input barrier.
        if(!live_settings_main_values(docs,v))return 0;
        U64 old=__atomic_load_n(&stereo_xr_size.packed,__ATOMIC_ACQUIRE);live_settings_runtime.eye_changed=0;
        if(__atomic_load_n(&live_settings_runtime.xr_active,__ATOMIC_ACQUIRE)){
            __atomic_store_n(&live_settings_runtime.xr_ok,0,__ATOMIC_RELEASE);
            __atomic_store_n(&live_settings_runtime.extent,old,__ATOMIC_RELEASE);__atomic_add_fetch(&live_settings_runtime.xr_request,1,__ATOMIC_ACQ_REL);
        }else{__atomic_store_n(&live_settings_runtime.xr_ok,1,__ATOMIC_RELEASE);
            __atomic_store_n(&live_settings_runtime.xr_done,__atomic_load_n(&live_settings_runtime.xr_request,__ATOMIC_ACQUIRE),__ATOMIC_RELEASE);}
        live_settings_runtime.stage=1;return -1;
    }
    if(live_settings_runtime.stage==1){
        if(__atomic_load_n(&live_settings_runtime.xr_active,__ATOMIC_ACQUIRE)&&
           __atomic_load_n(&live_settings_runtime.xr_request,__ATOMIC_ACQUIRE)!=__atomic_load_n(&live_settings_runtime.xr_done,__ATOMIC_ACQUIRE))return -1;
        if(!__atomic_load_n(&live_settings_runtime.xr_ok,__ATOMIC_ACQUIRE))return 0;
        int w=0,h=0,mode=0;if(!performance_screen_read(0,&w,&h,&mode))return 0;
        if(w!=live_settings_runtime.wanted_w||h!=live_settings_runtime.wanted_h||mode!=live_settings_runtime.wanted_mode)return -1;
        PcOutput previous=live_settings_runtime.before_pc;
        if(rollback)performance_runtime.pc=previous;
        else performance_runtime.pc=(PcOutput){.captured=1,.applied=v->performance.pc.fixed&&(v->performance.pc.always||live_settings_runtime.immersive),
            .width=previous.captured?previous.width:live_settings_runtime.before_w,.height=previous.captured?previous.height:live_settings_runtime.before_h,
            .mode=live_settings_runtime.before_mode,.current_width=w,.current_height=h};
        live_settings_runtime.stage=2;__atomic_store_n(&live_settings_runtime.freeze,0,__ATOMIC_RELEASE);
    }
    if(live_settings_runtime.stage==2){
        U64 extent=__atomic_load_n(&live_settings_runtime.extent,__ATOMIC_ACQUIRE);
        // Resizing temporarily revokes/detaches the source. Keep waiting for the
        // captured immersive intent unless the user explicitly selects flat mode.
        if(live_settings_runtime.immersive&&!performance_manual_flat&&
           __atomic_load_n(&live_settings_runtime.xr_active,__ATOMIC_ACQUIRE)&&extent){
            unsigned w=(unsigned)extent,h=(unsigned)(extent>>32);
            if(stereo_start.failed)return 0;
            if(!stereo_live||!stereo_live->ready||stereo_live->width!=(int)w||stereo_live->height!=(int)h)return -1;
            if(__atomic_load_n(&stereo_xr_config.state,__ATOMIC_ACQUIRE)!=XEC_READY||
               stereo_xr_config.description.width!=w||stereo_xr_config.description.height!=h)return -1;
            if(!__atomic_load_n(&live_settings_runtime.first_frame,__ATOMIC_ACQUIRE)||
               __atomic_load_n(&live_settings_runtime.await_new_frame,__ATOMIC_ACQUIRE))return -1;
        }
        live_settings_runtime.stage=3;
    }
    __atomic_store_n(&live_settings_runtime.has_applied,1,__ATOMIC_RELEASE);
    __atomic_store_n(&live_settings_runtime.reconcile_session,0,__ATOMIC_RELEASE);
    return 1;
}
static void live_settings_finish_values(unsigned committed){
    (void)committed;__atomic_store_n(&live_settings_runtime.freeze,0,__ATOMIC_RELEASE);
    event(committed?"live_settings_committed":"live_settings_rolled_back",live_settings_transaction.revision,GetCurrentThreadId());
}
static void live_settings_main_tick(void){
    if(!render_installed||!gf2vr_paths.ready||GetCurrentThreadId()!=__atomic_load_n(&first_tid,__ATOMIC_ACQUIRE))return;
    DWORD saved=GetLastError();
    if(performance_stopping()){
        if(__atomic_load_n(&live_settings_wire.ready,__ATOMIC_ACQUIRE)==1)live_settings_reply(&live_settings_wire,LSW_UNAVAILABLE,0,live_settings_transaction.revision);
        SetLastError(saved);return;
    }
    live_settings_service_start(gf2vr_paths.root,GF2VR_OBSERVER_ID,
#ifdef GF2VR_DIRTY_CONTACT
        1
#else
        0
#endif
    );
    const LiveSettingsAdapter adapter={live_settings_prepare_values,live_settings_apply_values,live_settings_finish_values};
    if(live_settings_transaction.initialized&&live_settings_transaction.phase<=1&&
       __atomic_load_n(&live_settings_runtime.reconcile_session,__ATOMIC_ACQUIRE)){
        LiveSettingsBank *current=&live_settings_transaction.banks[live_settings_transaction.current];
        int confirmed=live_settings_apply_values(&current->documents,&current->values,0);
        if(confirmed==0){__atomic_store_n(&live_settings_runtime.reconcile_session,0,__ATOMIC_RELEASE);event("live_settings_session_reconcile_failed",live_settings_transaction.revision,GetCurrentThreadId());}
    }
    unsigned initialized=live_settings_transaction.initialized;
    live_settings_transaction_tick(&live_settings_transaction,&live_settings_wire,&adapter,GetTickCount64());
    if(!initialized&&live_settings_transaction.initialized)event("live_settings_service_ready",live_settings_transaction.revision,GetCurrentThreadId());
#ifdef GF2VR_LIVE_SETTINGS_ENTRY_OBSERVE
    static U64 host_status=~0ull,host_process=~0ull,xr_state=~0ull,transport=~0ull;
    U64 status=__atomic_load_n(&live_settings_wire.entry_host_status,__ATOMIC_ACQUIRE);
    U64 process=__atomic_load_n(&live_settings_wire.entry_host_process,__ATOMIC_ACQUIRE);
    U64 state=__atomic_load_n(&live_settings_entry_xr_state,__ATOMIC_ACQUIRE);
    U64 connection=__atomic_load_n(&live_settings_wire.entry_transport,__ATOMIC_ACQUIRE);
    if(connection!=transport){event("settings_entry_transport",connection,GetCurrentThreadId());transport=connection;}
    if(status!=host_status||process!=host_process){event("settings_entry_host_status",status,GetCurrentThreadId());event("settings_entry_host_process",process,GetCurrentThreadId());host_status=status;host_process=process;}
    if(state!=xr_state){event("settings_entry_surface_state",state,GetCurrentThreadId());xr_state=state;}
    static U64 gpu_descriptor=~0ull,gpu_format=~0ull,gpu_result=~0ull;
    U64 descriptor=__atomic_load_n(&live_settings_entry_gpu_descriptor,__ATOMIC_ACQUIRE);
    U64 format=__atomic_load_n(&live_settings_entry_gpu_format,__ATOMIC_ACQUIRE);
    U64 result=__atomic_load_n(&live_settings_entry_gpu_result,__ATOMIC_ACQUIRE);
    if(descriptor!=gpu_descriptor){event("settings_entry_gpu_descriptor",descriptor,GetCurrentThreadId());gpu_descriptor=descriptor;}
    if(format!=gpu_format){event("settings_entry_gpu_format",format,GetCurrentThreadId());gpu_format=format;}
    if(result!=gpu_result){event("settings_entry_gpu_result",result,GetCurrentThreadId());gpu_result=result;}
#endif
    SetLastError(saved);
}
#endif
