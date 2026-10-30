#ifndef GF2_ACTIVITY_UI_FRAME_ELIGIBILITY_H
#define GF2_ACTIVITY_UI_FRAME_ELIGIBILITY_H
static int activity_ui_enroll_layer(ActivityEyeGroup *g,ActivityUiLayer *l) {
    if(!g || !activity_group_source_valid(g) || !l->data.binding || !l->data.ready ||
       !g->source->source || GetCurrentThreadId()!=g->source->source->owner_thread)return 0;
    const ManagedEyeApi *a=&l->binding.refs.settings.calls;
    int ids[2]={0};SourcePoseApi read={0};read.calls=*a;
    for(unsigned eye=0;eye<2;++eye) {
        ManagedEyeSlot *e=&l->eyes.eyes[eye];
        if(!e->camera_root || !e->object_root || e->camera==l->data.source_camera ||
           !source_pose_value(&read,g->source->topology->api->instance,e->camera,
               g->source->topology->api->int_class,&ids[eye],4) || !ids[eye])goto fail;
        for(unsigned i=0;i<g->count;++i)
            if(e->camera==g->members[i].source || e->camera==g->members[i].left || e->camera==g->members[i].right)goto fail;
    }
    if(ids[0]==ids[1])goto fail;
    for(unsigned eye=0;eye<2;++eye)
        if(!uicr_add(activity_ui_owner_key(g,l),g->generation,a,l->eyes.eyes[eye].camera,ids[eye]))goto fail;
    l->enrolled=1;return 1;
fail:uicr_retire(activity_ui_owner_key(g,l),g->generation);return 0;
}
static int activity_ui_stop_layer(ActivityEyeGroup *g,ActivityUiLayer *l) {
    if(!g || !l->enrolled)return 1;
    if(GetCurrentThreadId()!=uicr_thread())return 0;
    const ManagedEyeApi *a=&l->binding.refs.settings.calls;
    unsigned char off=0;void *args[]={&off};int ok=1;
    for(unsigned eye=0;eye<2;++eye) {
        ManagedEyeSlot *e=&l->eyes.eyes[eye];
        /* Scene unload can destroy these private objects before group drain.
           A failed setter is safe only when that exact rooted object is
           verified dead; live/unknown failures must retain enrollment. */
        if(e->camera_root && !managed_eye_call(a,a->set_enabled,e->camera,args,0) &&
           !managed_eye_verified_dead(a,e->camera))ok=0;
        if(e->object_root && !managed_eye_call(a,a->set_active,e->object,args,0) &&
           !managed_eye_verified_dead(a,e->object))ok=0;
        if(e->camera_root && !managed_eye_verified_dead(a,e->camera)) {
            unsigned active=1;
            if(!generic_source_active(g->source,e->camera,&active) || active!=0)ok=0;
        }
    }
    if(ok){uicr_retire(activity_ui_owner_key(g,l),g->generation);l->enrolled=0;l->activated=0;}
    return ok;
}
static int activity_ui_verify_layer(ActivityEyeGroup *g,ActivityUiLayer *l) {
    for(unsigned eye=0;eye<2;++eye) {
        unsigned active=0,role=0;
        if(!activity_group_source_valid(g) || uicr_owned(l->eyes.eyes[eye].camera)!=1 ||
           !generic_source_active(g->source,l->eyes.eyes[eye].camera,&active) || active!=1 ||
           !camera_data_u32(&l->binding.refs,l->binding.refs.settings.get[4],
               l->data.components[eye+1],l->binding.refs.settings.value_class[4],&role) || role!=1)return 0;
    }
    return 1;
}
static int activity_ui_enroll(ActivityEyeGroup *g){return activity_ui_enroll_layer(g,&g->ui);}
static int activity_ui_verify_eligibility(ActivityEyeGroup *g){return activity_ui_verify_layer(g,&g->ui);}
static unsigned activity_ui_enrolled_any(ActivityEyeGroup *g){
    for(unsigned i=0;i<activity_ui_layer_count();++i)if(activity_ui_layer(g,i)->enrolled)return 1;return 0;
}
static int activity_ui_stop_eligibility(ActivityEyeGroup *g){
    int ok=1;for(unsigned i=0;i<activity_ui_layer_count();++i)if(!activity_ui_stop_layer(g,activity_ui_layer(g,i)))ok=0;return ok;
}

#endif
