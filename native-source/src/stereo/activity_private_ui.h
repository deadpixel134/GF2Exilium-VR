#ifndef GF2_ACTIVITY_PRIVATE_UI_H
#define GF2_ACTIVITY_PRIVATE_UI_H
#include "activity_ui_geometry.h"
#ifndef GF2VR_ACTIVITY_UI_VISIBLE
#define GF2VR_ACTIVITY_UI_VISIBLE() 0
#endif
#ifndef GF2VR_ACTIVITY_UI_REPORT
#define GF2VR_ACTIVITY_UI_REPORT(g,f,s,p) ((void)0)
#endif
static unsigned activity_ui_layer_count(void){
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
    return 2;
#else
    return 1;
#endif
}
static ActivityUiLayer *activity_ui_layer(ActivityEyeGroup *g,unsigned n){
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
    if(n)return &g->dialogue_ui;
#else
    (void)n;
#endif
    return &g->ui;
}
static ActivityPrimarySource *activity_ui_layer_source(ActivityEyeGroup *g,unsigned n){
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
    if(n)return &g->source->active.dialogue;
#else
    (void)n;
#endif
    return &g->source->active.primary;
}
static void *activity_ui_owner_key(ActivityEyeGroup *g,ActivityUiLayer *l){return l==&g->ui?(void *)g:(void *)l;}
static unsigned activity_ui_linked_count(ActivityEyeGroup *g){
    unsigned n=0;for(unsigned i=0;i<activity_ui_layer_count();++i)n+=activity_ui_layer(g,i)->linked;return n;
}
static void *activity_ui_list_camera(ActivityEyeGroup *g,unsigned eye,unsigned index,unsigned shown){
    if(index<g->count-1)return g->eyes[index].eyes[eye].camera;
    index-=g->count-1;
    for(unsigned i=0;i<activity_ui_layer_count();++i){ActivityUiLayer *l=activity_ui_layer(g,i);
        if((shown?l->data.binding!=0:l->linked!=0)){if(!index)return l->eyes.eyes[eye].camera;--index;}
    }return 0;
}
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
static void activity_dialogue_ui_report(ActivityEyeGroup *g,unsigned stage,unsigned long long frame){
    static unsigned long long generations[6];static unsigned reports;
    if(!g || stage>=6 || reports>=64 || generations[stage]==g->generation)return;
    generations[stage]=g->generation;++reports;
    event("activity_dialogue_ui_stage",(g->generation<<8)|stage,frame);
}
/* Original orthographic source is read-only. Set/read back perspective only on
   script-free privately owned UI cameras, after every Camera.CopyFrom. */
static int activity_dialogue_private_perspective(ActivityEyeGroup *g,void *camera){
    static void *setter;static unsigned attempted;
    const ManagedEyeApi *c=&g->base->binding->refs.settings.calls;
    if(!attempted){attempted=1;setter=eye_binding_method(c->camera_class,"set_orthographic","System.Boolean","System.Void",0,g->base->binding->refs.generic);}
    if(!setter || !activity_group_link_valid(g))return 0;
    unsigned char value=0,actual=1;void *args[]={&value};SourcePoseApi read={0};read.calls=*c;
    return managed_eye_call(c,setter,camera,args,0) && source_pose_value(&read,g->source->topology->api->orthographic,
        camera,g->source->topology->api->bool_class,&actual,1) && actual==0 && activity_group_link_valid(g);
}
#endif
/* Entire UI camera is a private optional contribution. Source roles and source
   lists are immutable. Each slot retains its UI objects through the same real
   render/copy/GPU drain as the complete world stack. */
static int activity_group_ui_link(ActivityEyeGroup *g,unsigned shown) {
    if(!activity_group_link_valid(g) || g->borrowed || g->count>=8)return 0;
    unsigned available=0;
    for(unsigned i=0;i<activity_ui_layer_count();++i)available+=activity_ui_layer(g,i)->data.binding!=0;
    unsigned linked=activity_ui_linked_count(g),desired=shown?available:0;
    if(!available)return !desired && !linked;
    if(g->count+desired>8)return 0;
    const CameraDataReferencesApi *a=&g->base->binding->refs;const ManagedEyeApi *c=&a->settings.calls;
    void *lists[2][2]={{0}},*source_lists[6]={0};CameraDataListApi methods[2][2]={0};unsigned roots[10]={0},nr=0,ns=2;int ok=0;
    for(unsigned l=0;l<2;++l) {
        if(!camera_data_stack_list(a,g->base->components[0],l?a->get_stack:a->get_cameras,&source_lists[l]) || !source_lists[l])goto end;
        roots[nr]=c->pin(source_lists[l],1);if(!roots[nr++])goto end;
    }
    for(unsigned i=0;i<activity_ui_layer_count();++i){ActivityUiLayer *layer=activity_ui_layer(g,i);
        if(!layer->data.binding)continue;
#ifdef GF2VR_ACTIVITY_UI_LIFETIME
        /* Unlinking a drained private list must not call a removed original
           UI owner. Its private lists were independently copied/verified on
           attachment; still verify original Base and both private eye lists. */
        if(!shown)continue;
#endif
        for(unsigned l=0;l<2;++l){
            if(!camera_data_stack_list(a,layer->data.components[0],l?a->get_stack:a->get_cameras,&source_lists[ns]))goto end;
            if(source_lists[ns]){roots[nr]=c->pin(source_lists[ns],1);if(!roots[nr++])goto end;}++ns;
        }
    }
    for(unsigned eye=0;eye<2;++eye)for(unsigned l=0;l<2;++l) {
        void *list=0;unsigned n=0;
        if(!camera_data_stack_list(a,g->base->components[eye+1],l?a->get_stack:a->get_cameras,&list) || !list)goto end;
        roots[nr]=c->pin(list,1);if(!roots[nr++])goto end;lists[eye][l]=list;
        unsigned alias=0;for(unsigned i=0;i<ns;++i)if(list==source_lists[i])alias=1;
        if(alias ||
           (eye && (list==lists[0][0] || list==lists[0][1])) ||
           !csm_bind_list(a,list,&methods[eye][l]) || !camera_data_u32(a,methods[eye][l].count,list,a->int_class,&n) ||
           n!=g->count-1+linked)goto end;
        for(unsigned i=0;i<n;++i) {
            int index=(int)i;void *args[]={&index},*item=0;
            void *expected=activity_ui_list_camera(g,eye,i,0);
            if(!managed_eye_call(c,methods[eye][l].item,list,args,&item) || item!=expected)goto end;
        }
    }
    if(desired==linked){ok=1;goto end;}
    /* Full preflight above precedes any private write. World prefix and order
       are rebuilt exactly; aliased lists within one eye are written once. */
    for(unsigned eye=0;eye<2;++eye)for(unsigned l=0;l<2;++l) {
        void *list=lists[eye][l];CameraDataListApi *m=&methods[eye][l];
        if(l && list==lists[eye][0])continue;
        if(!activity_group_link_valid(g) || !managed_eye_call(c,m->clear,list,0,0))goto end;
        for(unsigned i=0;i<g->count-1+desired;++i) {
            void *item=activity_ui_list_camera(g,eye,i,shown);void *args[]={item};
            if(!activity_group_link_valid(g) || !managed_eye_call(c,m->add,list,args,0))goto end;
        }
    }
    for(unsigned eye=0;eye<2;++eye)for(unsigned l=0;l<2;++l) {
        unsigned count=0;void *list=lists[eye][l];CameraDataListApi *m=&methods[eye][l];
        if(!camera_data_u32(a,m->count,list,a->int_class,&count) || count!=g->count-1+desired)goto end;
        for(unsigned i=0;i<count;++i) {
            int index=(int)i;void *args[]={&index},*item=0;
            if(!managed_eye_call(c,m->item,list,args,&item) || item!=activity_ui_list_camera(g,eye,i,shown))goto end;
        }
    }
    if(activity_group_link_valid(g)){
        for(unsigned i=0;i<activity_ui_layer_count();++i){ActivityUiLayer *l=activity_ui_layer(g,i);l->linked=shown && l->data.binding;}ok=1;
    }
end:
    while(nr){unsigned root=roots[--nr];if(root)c->unpin(root);}return ok;
}
static int activity_group_ui_refresh_layer(ActivityEyeGroup *g,ActivityUiLayer *l) {
    if(!l->data.binding)return 1;
    CameraDataOwner *d=&l->data;ManagedEyes *e=&l->eyes;
    if(!activity_group_link_valid(g) || e->borrowed || e->targets_ready || d->retiring)return 0;
    const CameraDataSettingsApi *s=&d->binding->refs.settings;const ManagedEyeApi *c=&s->calls;
#ifdef GF2VR_ACTIVITY_TARGET_SURFACE
    unsigned surface_index=l==&g->ui?7:8;
    if(!activity_target_surface_clear(c,&g->surfaces[surface_index],e))return 0;
#endif
#ifdef GF2VR_ACTIVITY_UI_FRAME_ELIGIBLE
    /* Reject foreign/retired enrollment before copying any camera data. */
    if(l->enrolled)for(unsigned eye=0;eye<2;++eye)
        if(!uicr_current(e->eyes[eye].camera,activity_ui_owner_key(g,l),g->generation))return 0;
#endif
    d->ready=e->ready=e->matrices_ready=0;unsigned role=1;unsigned char clear=1;
    if(!camera_data_settings_copy(s,d->components[0],d->components[1],d->components[2]) ||
       !camera_data_references_copy_mode(&d->binding->refs,d->components[0],d->components[1],d->components[2],1))return 0;
    for(unsigned eye=0;eye<2;++eye) {
        ManagedEyeSlot *slot=&e->eyes[eye];unsigned char enabled=0;
#ifdef GF2VR_ACTIVITY_UI_FRAME_ELIGIBLE
        /* Copy/refresh must not revoke the current normal-frame eligibility
           established by borrow. Unenrolled startup cameras remain disabled. */
        if(l->enrolled) {
            if(!uicr_current(slot->camera,activity_ui_owner_key(g,l),g->generation))return 0;
            enabled=1;
        }
#endif
        void *copy[]={d->source_camera},*enabled_args[]={&enabled},*null_target[]={0};
        void *overlay[]={&role},*depth[]={&clear};unsigned actual=0;
        if(slot->camera==e->eyes[1-eye].camera || slot->object==e->eyes[1-eye].object)return 0;
        for(unsigned i=0;i<activity_ui_layer_count();++i){ActivityUiLayer *other=activity_ui_layer(g,i);
            if(other==l || !other->data.binding)continue;
            if(slot->camera==other->data.source_camera)return 0;
            for(unsigned n=0;n<2;++n)if(slot->camera==other->eyes.eyes[n].camera || slot->object==other->eyes.eyes[n].object)return 0;
        }
        for(unsigned i=0;i<g->count;++i) {
            if(slot->camera==g->members[i].source || slot->camera==g->members[i].left || slot->camera==g->members[i].right)return 0;
        }
        if(!slot->camera_root || slot->target || slot->target_root || slot->camera==d->source_camera ||
           !managed_eye_call(c,c->copy_from,slot->camera,copy,0) ||
           !managed_eye_call(c,c->set_enabled,slot->camera,enabled_args,0) || !managed_eye_call(c,c->set_target,slot->camera,null_target,0) ||
           !managed_eye_call(c,s->set[4],d->components[eye+1],overlay,0) ||
           !managed_eye_call(c,s->set[10],d->components[eye+1],depth,0) ||
           !camera_data_u32(&d->binding->refs,s->get[4],d->components[eye+1],s->value_class[4],&actual) || actual!=1 ||
           !activity_group_link_valid(g))return 0;
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
        if(l==&g->dialogue_ui && !activity_dialogue_private_perspective(g,slot->camera))return 0;
#endif
        unsigned char actual_clear=0;SourcePoseApi values={0};values.calls=*c;
        if(!source_pose_value(&values,s->get[10],d->components[eye+1],s->value_class[10],&actual_clear,1) || actual_clear!=1)return 0;
#ifdef GF2VR_ACTIVITY_UI_FRAME_ELIGIBLE
        if(enabled) {
            unsigned active=0;
            if(!uicr_current(slot->camera,activity_ui_owner_key(g,l),g->generation) ||
               !generic_source_active(g->source,slot->camera,&active) || active!=1 ||
               !activity_group_link_valid(g))return 0;
        }
#endif
        ActivityViewport rect,before,after;
#ifdef GF2VR_ACTIVITY_TARGET_SURFACE
        if(g->base_eyes->eyes[eye].target && (!g->source->topology || !g->source->topology->api ||
           !activity_target_surface_bind(c,g->source->topology->api->target,&g->surfaces[surface_index],
               e,g->base_eyes,eye,activity_group_link_valid,g)))return 0;
#endif
        if(g->base_eyes->eyes[eye].target && (!activity_viewport_read(c,g->base_eyes->eyes[eye].camera,&rect) ||
           !activity_viewport_align(c,slot->camera,&rect,&before,&after,activity_group_link_valid,g)))return 0;
    }
    d->ready=e->ready=1;return activity_group_link_valid(g);
}
static int activity_group_ui_refresh(ActivityEyeGroup *g){
    for(unsigned i=0;i<activity_ui_layer_count();++i)if(!activity_group_ui_refresh_layer(g,activity_ui_layer(g,i)))return 0;
    return 1;
}

static int activity_group_ui_attach_current(void *context,unsigned long long generation) {
    ActivityEyeGroup *g=context;return activity_group_link_valid(g) && g->generation==generation;
}
static int activity_group_ui_prepare_layer(ActivityEyeGroup *g,const ManagedEyeApi *calls,unsigned index) {
    ActivityUiLayer *l=activity_ui_layer(g,index);ActivityPrimarySource *u=activity_ui_layer_source(g,index);if(!u->canvas)return 1;
    unsigned contributions=0;for(unsigned i=0;i<activity_ui_layer_count();++i)contributions+=activity_ui_layer_source(g,i)->canvas!=0;
    if(!u->canvas_root || !u->camera_root || g->count+contributions>8)return 0;
    CameraDataComponentApi *b=&l->binding;b->refs=g->base->binding->refs;b->type=g->base->binding->type;
    b->get_component=g->base->binding->get_component;b->type_root=calls->pin(b->type,1);if(!b->type_root)return 0;
    return managed_eyes_create(calls,&l->eyes,u->entry.identity.source) && activity_group_source_valid(g) &&
        camera_data_owner_attach(b,&l->data,&l->eyes,u->entry.identity.source,g->generation,activity_group_ui_attach_current,g);
}
static int activity_group_ui_prepare(ActivityEyeGroup *g,const ManagedEyeApi *calls){
    for(unsigned i=0;i<activity_ui_layer_count();++i)if(!activity_group_ui_prepare_layer(g,calls,i))return 0;
    return 1;
}

static int activity_group_ui_apply(ActivityEyeGroup *g,const PoseMath *math,
    SourcePoseApi *reader,const UnityTrackingInput *tracking,const PoseAppliedTicket *ticket) {
    unsigned shown=GF2VR_ACTIVITY_UI_VISIBLE()!=0,available=0;
#ifdef GF2VR_ACTIVITY_UI_RAY_CAPTURE
    float ray_geometry[20];ManagedEyePose ray_poses[2];unsigned ray_ready=0;
#endif
    for(unsigned i=0;i<activity_ui_layer_count();++i)available+=activity_ui_layer(g,i)->data.binding!=0;
    if(shown && available){
        struct {ManagedEyePose poses[2];ManagedEyeMatrix views[2],projections[2];float near_z,far_z;SourcePoseSnapshot source;} frames[2];
#ifdef GF2VR_ACTIVITY_UI_GENERATION_LEASE
        if(!GF2VR_ACTIVITY_UI_GENERATION_PREPARE(g->source,g->generation))return 0;
#endif
        for(unsigned i=0;i<activity_ui_layer_count();++i){ActivityUiLayer *l=activity_ui_layer(g,i);
            ActivityPrimarySource *u=activity_ui_layer_source(g,i);if(!l->data.binding)continue;
            SourcePoseSnapshot source;ManagedEyePose *poses=frames[i].poses;
            ManagedEyeMatrix *views=frames[i].views,*projections=frames[i].projections;
            if(!source_pose_read(reader,l->data.source_camera,&source))return 0;
            frames[i].near_z=source.near_z;frames[i].far_z=source.far_z;
            frames[i].source=source;
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
            if(i){float geometry[20];
                if(!GF2VR_ACTIVITY_DIALOGUE_GEOMETRY(g->source,g->generation,geometry) ||
                   !activity_ui_planar_optics(math,tracking,&source,geometry,poses,views,projections)){
                    activity_dialogue_ui_report(g,3,ticket->frame_id);return 0;}
            }else
#endif
            if(!(u->plane>source.near_z && u->plane<source.far_z) ||
               !activity_ui_eye_poses(math,tracking,&source.pose,u->plane,2.0f,poses) ||
               !eye_optics_build(poses,tracking->tangents,source.near_z,source.far_z,views,projections))return 0;
        }
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
        if(g->dialogue_ui.data.binding){float primary[20],dialogue[20],reference_depth;
#ifdef GF2VR_ACTIVITY_UI_ISOLATION_OBSERVE
            unsigned attempted=0,passed=0;
#define AI_CHECK(bit,expr) ((attempted|=(bit)),((expr)?((passed|=(bit)),1):0))
            int isolated=AI_CHECK(1,GF2VR_ACTIVITY_PRIMARY_GEOMETRY(g->source,g->generation,primary)) &&
                AI_CHECK(2,GF2VR_ACTIVITY_DIALOGUE_GEOMETRY(g->source,g->generation,dialogue)) &&
                AI_CHECK(4,activity_ui_root_excluded(dialogue,frames[0].views,frames[0].projections)) &&
                AI_CHECK(8,activity_ui_root_excluded(primary,frames[1].views,frames[1].projections));
#undef AI_CHECK
            GF2VR_ACTIVITY_UI_ISOLATION_REPORT(g,ticket,attempted,passed,primary,dialogue,
                frames[0].views,frames[0].projections,frames[1].views,frames[1].projections);
#else
            int isolated=GF2VR_ACTIVITY_PRIMARY_GEOMETRY(g->source,g->generation,primary) &&
                GF2VR_ACTIVITY_DIALOGUE_GEOMETRY(g->source,g->generation,dialogue) &&
                activity_ui_root_excluded(dialogue,frames[0].views,frames[0].projections) &&
                activity_ui_reference_depth(math,&frames[0].source,primary,dialogue,&reference_depth) &&
                activity_ui_planar_optics_at_depth(math,tracking,&frames[1].source,dialogue,reference_depth,
                    frames[1].poses,frames[1].views,frames[1].projections) &&
                activity_ui_depth_isolate(dialogue,primary,frames[1].near_z,frames[1].far_z,frames[1].views,frames[1].projections) &&
                activity_ui_root_excluded(primary,frames[1].views,frames[1].projections);
#endif
            if(!isolated){activity_dialogue_ui_report(g,4,ticket->frame_id);return 0;}
#ifdef GF2VR_ACTIVITY_UI_RAY_CAPTURE
            for(unsigned n=0;n<20;++n)ray_geometry[n]=dialogue[n];
            ray_poses[0]=frames[1].poses[0];ray_poses[1]=frames[1].poses[1];ray_ready=1;
#endif
        }
#endif
        for(unsigned i=0;i<activity_ui_layer_count();++i){ActivityUiLayer *l=activity_ui_layer(g,i);if(!l->data.binding)continue;
            if(!managed_eyes_matrices_mode(&l->binding.refs.settings.calls,&l->eyes,ticket->frame_id,
               frames[i].views,frames[i].projections,frames[i].poses,1))return 0;
        }
    }
    if(!activity_group_ui_link(g,shown))return 0;
    for(unsigned i=0;i<activity_ui_layer_count();++i){ActivityUiLayer *l=activity_ui_layer(g,i);l->shown=shown && l->data.binding;}
#ifdef GF2VR_ACTIVITY_UI_RAY_CAPTURE
    GF2VR_ACTIVITY_UI_RAY_CAPTURE(g,math,tracking,ticket,ray_ready?ray_geometry:0,ray_ready?ray_poses:0,shown && ray_ready);
#endif
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
    if(g->dialogue_ui.data.binding)activity_dialogue_ui_report(g,shown?2:5,ticket->frame_id);
#endif
    GF2VR_ACTIVITY_UI_REPORT(g->generation,ticket->frame_id,g->ui_shown,g->source->active.primary.plane);return 1;
}

#endif
