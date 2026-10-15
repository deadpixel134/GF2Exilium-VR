#ifndef GF2_ACTIVITY_CANVAS_SCOPE_RUNTIME_H
#define GF2_ACTIVITY_CANVAS_SCOPE_RUNTIME_H
/* Adapter for the existing synchronous primitive MEMBER_BEGIN5/MEMBER_END6.
   This does not install a render hook/new managed bridge or queue GPU work. */
typedef struct {U64 serial,generation,frame;unsigned slot,owner;int ids[2];} PrimaryUiCanvasPlan;
static PrimaryUiCanvasPlan primary_ui_canvas_plan;
#ifdef GF2VR_ACTIVITY_UI_GENERATION_LEASE
#if defined(GF2VR_ACTIVITY_UI_SPATIAL_OBSERVE) || defined(GF2VR_ACTIVITY_UI_REGISTRATION_OBSERVE)
#error Retired per-camera observers cannot instrument the generation lease
#endif
/* Producer member notifications retain their normal semantics and real drain
   checks. They no longer toggle a Canvas whose native batches live per frame. */
static int primary_ui_canvas_prepare(U64 serial){
#ifdef GF2VR_ACTIVITY_UI_NATIVE_REGISTER_OBSERVE
    acr_prepare(serial);
#else
    (void)serial;
#endif
    return 1;
}
static int primary_ui_canvas_before_notify(U64 serial,int operation,int camera){(void)serial;(void)operation;(void)camera;return 1;}
static int primary_ui_canvas_after_notify(U64 serial,int operation,int camera,int result){
#ifdef GF2VR_ACTIVITY_UI_NATIVE_REGISTER_OBSERVE
    DWORD saved=GetLastError();acr_notice(serial,operation,camera,result);SetLastError(saved);
#else
    (void)serial;(void)operation;(void)camera;
#endif
    return result;
}
static int primary_ui_canvas_finish(U64 serial){
#ifdef GF2VR_ACTIVITY_UI_NATIVE_REGISTER_OBSERVE
    acr_finish(serial);
#else
    (void)serial;
#endif
    return 1;
}
static void primary_ui_canvas_owner_tick(void) {
#ifdef GF2VR_ACTIVITY_UI_LIFETIME
    /* The original root can change between dialogue lines. A main idle tick
       is not GPU/copy drain: defer restoration to the UI transaction. */
    ActivitySourceGroup *owner=primary_ui_generation_context;
    if(owner && GetCurrentThreadId()==stereo_pipeline.owner_thread &&
       activity_source_current(owner,primary_ui_generation_id)) {
        if(!primary_ui_current(owner))activity_ui_request_change(owner);
        return;
    }
#endif
    if((primary_ui_canvas.scope.active
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
        || dialogue_ui_canvas.scope.active
#endif
       ) && GetCurrentThreadId()==stereo_pipeline.owner_thread &&
       !stereo_pipeline.render.active && (!primary_ui_generation_context ||
       !activity_source_current(primary_ui_generation_context,primary_ui_generation_id) ||
       !primary_ui_current(primary_ui_generation_context))) {
        DWORD saved=GetLastError();(void)primary_ui_generation_release(primary_ui_generation_context);SetLastError(saved);
    }
#ifdef GF2VR_ACTIVITY_UI_CANVAS_MEMBERSHIP_OBSERVE
    acm_tick();
#endif
}
#else
static void primary_ui_canvas_report(unsigned operation,int ok,U64 serial,int camera) {
    static unsigned reports;if(reports>=32 && ok)return;
    if(reports>=64)return;++reports;
    char line[900],*p=source_prefix(line,"activity_primary_ui_canvas_scope");
    p=literal(p,",\"serial\":");p=hexadecimal(p,serial);
    p=literal(p,",\"operation\":");p=decimal(p,operation);
    p=literal(p,",\"camera\":");p=hexadecimal(p,(unsigned)camera);
    p=literal(p,",\"ok\":");p=decimal(p,ok!=0);
    p=literal(p,",\"active\":");p=decimal(p,primary_ui_canvas.scope.active);
    p=literal(p,",\"failed\":");p=decimal(p,primary_ui_canvas.scope.failed);
    p=literal(p,",\"optional_disabled\":");p=decimal(p,primary_ui_scope_disabled);
    p=literal(p,"}\n");source_log(line,p);
}
static int primary_ui_canvas_prepare(U64 serial) {
    primary_ui_canvas_plan=(PrimaryUiCanvasPlan){0};
    if(primary_ui_canvas.scope.active)return 0;
    if(!stereo_live || stereo_two.root_slot>=EFO_CAPACITY || stereo_two.root_serial!=serial)return 1;
    ActivityEyeGroup *g=&stereo_live->pool.groups[stereo_two.root_slot];
    if(!g->ui_shown)return 1;
    if(primary_ui_scope_disabled || !g->borrowed || !g->ui_eyes.borrowed || !g->ui_linked ||
       !activity_group_source_valid(g) || !primary_ui_current(g->source) || !primary_ui_canvas_bind())goto fail;
    SourcePoseApi read={0};read.calls=g->ui_binding.refs.settings.calls;int ids[2]={0};
    for(unsigned eye=0;eye<2;++eye)
        if(!source_pose_value(&read,stereo_live->topology_api.instance,g->ui_eyes.eyes[eye].camera,
            stereo_live->topology_api.int_class,&ids[eye],4) || !ids[eye] || !activity_group_source_valid(g))goto fail;
    if(ids[0]==ids[1])goto fail;
    primary_ui_canvas_plan.serial=serial;primary_ui_canvas_plan.generation=g->generation;
    primary_ui_canvas_plan.frame=g->frame;primary_ui_canvas_plan.slot=stereo_two.root_slot;
    primary_ui_canvas_plan.owner=GetCurrentThreadId();
    primary_ui_canvas_plan.ids[0]=ids[0];primary_ui_canvas_plan.ids[1]=ids[1];
#ifdef GF2VR_ACTIVITY_UI_REGISTRATION_OBSERVE
    aur_prepare(g,serial,ids[0]);
#endif
#ifdef GF2VR_ACTIVITY_UI_SPATIAL_OBSERVE
    aus_prepare(g,serial,ids[0]);
#endif
    return 1;
fail:
    primary_ui_scope_disabled=1;primary_ui_canvas_report(0,0,serial,0);return 0;
}
static int primary_ui_canvas_before_notify(U64 serial,int operation,int camera) {
    ActivityCanvasScope *s=&primary_ui_canvas.scope;
    if(!s->active || s->serial!=serial)return 1;
    if(s->owner!=GetCurrentThreadId())return 0; /* never call Unity on foreign owner */
    ActivityCanvasScopeApi ops=primary_ui_canvas_ops();int ok;
    if(operation==6 && camera==s->camera) {
#ifdef GF2VR_ACTIVITY_UI_SPATIAL_OBSERVE
        aus_phase(serial,camera,1);
#endif
        ok=acs_end(s,&ops,GetCurrentThreadId(),serial,camera);
    }
    else {
        /* Fault or an unexpected next boundary must restore before forwarding.
           Do not let psn_current rejection suppress this cleanup. */
        s->failed=1;ok=acs_restore(s,&ops,GetCurrentThreadId());
    }
    if(!ok)primary_ui_scope_disabled=1;
    primary_ui_canvas_release();primary_ui_canvas_report((unsigned)operation,ok,serial,camera);
#ifdef GF2VR_ACTIVITY_UI_REGISTRATION_OBSERVE
    if(operation==6 && ok)aur_phase(serial,camera,2);
#endif
    return ok || operation==4;
}
static int primary_ui_canvas_after_notify(U64 serial,int operation,int camera,int result) {
    if(operation!=5 || result!=1 || primary_ui_canvas_plan.serial!=serial ||
       (camera!=primary_ui_canvas_plan.ids[0] && camera!=primary_ui_canvas_plan.ids[1]))return result;
    if(primary_ui_canvas_plan.owner!=GetCurrentThreadId() || !stereo_live || stereo_two.root_serial!=serial ||
       stereo_two.root_slot!=primary_ui_canvas_plan.slot || stereo_two.root_slot>=EFO_CAPACITY)return 0;
    ActivityEyeGroup *g=&stereo_live->pool.groups[stereo_two.root_slot];
    int valid=g->generation==primary_ui_canvas_plan.generation && g->frame==primary_ui_canvas_plan.frame &&
        g->ui_shown && g->ui_linked && g->borrowed && g->ui_eyes.borrowed &&
        activity_group_source_valid(g) && primary_ui_current(g->source);
#ifdef GF2VR_ACTIVITY_UI_REGISTRATION_OBSERVE
    if(valid)aur_phase(serial,camera,0);
#endif
    int ok=valid && primary_ui_canvas_begin(&g->source->active.primary,serial,camera);
#ifdef GF2VR_ACTIVITY_UI_REGISTRATION_OBSERVE
    if(ok)aur_phase(serial,camera,1);
#endif
#ifdef GF2VR_ACTIVITY_UI_SPATIAL_OBSERVE
    if(ok)aus_phase(serial,camera,0);
#endif
    if(!ok)primary_ui_scope_disabled=1;
    primary_ui_canvas_report(5,ok,serial,camera);return ok;
}
static int primary_ui_canvas_finish(U64 serial) {
    int pending=primary_ui_canvas.scope.active && primary_ui_canvas.scope.serial==serial;
    if(pending && GetCurrentThreadId()!=primary_ui_canvas.scope.owner)return 0;
    if(pending)primary_ui_canvas.scope.failed=1; /* no End: this root is never successful */
    int ok=!pending;
    if(pending){(void)primary_ui_canvas_restore(GetCurrentThreadId());primary_ui_canvas_report(7,0,serial,0);}
    if(primary_ui_canvas_plan.serial==serial)primary_ui_canvas_plan.serial=0;
#ifdef GF2VR_ACTIVITY_UI_REGISTRATION_OBSERVE
    aur_finish(serial);
#endif
#ifdef GF2VR_ACTIVITY_UI_SPATIAL_OBSERVE
    aus_finish(serial);
#endif
    return ok;
}
static void primary_ui_canvas_owner_tick(void) {
    if(primary_ui_canvas.scope.active && GetCurrentThreadId()==primary_ui_canvas.scope.owner &&
       !stereo_pipeline.render.active) {
        DWORD saved=GetLastError();(void)primary_ui_canvas_restore(GetCurrentThreadId());SetLastError(saved);
    }
}
#endif
#endif
