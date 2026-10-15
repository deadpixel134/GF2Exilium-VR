#ifndef GF2_ACTIVITY_EYE_GROUP_H
#define GF2_ACTIVITY_EYE_GROUP_H
#include "activity_source_group.h"
#include "camera_stack_managed.h"
#ifdef GF2VR_ACTIVITY_TARGET_SURFACE
#include "activity_target_surface.h"
#endif
#ifdef GF2VR_ACTIVITY_VIEWPORT
#include "activity_viewport.h"
#ifndef GF2VR_ACTIVITY_VIEWPORT_REPORT
#define GF2VR_ACTIVITY_VIEWPORT_REPORT(g,m,e,b,o,a) ((void)0)
#endif
#endif
/* One fixed numeric slot owns one complete private stereo stack. Only the Base
   owns targets; Overlays use the engine's stack output. Never fake target-ready
   or release this group from a Base End / CPU return / elapsed time. */
#ifdef GF2VR_ACTIVITY_PRIMARY_UI
typedef struct {
    CameraDataComponentApi binding;CameraDataOwner data;ManagedEyes eyes;
    unsigned linked,shown,activated;
#ifdef GF2VR_ACTIVITY_UI_FRAME_ELIGIBLE
    unsigned enrolled;
#endif
} ActivityUiLayer;
#endif
typedef struct {
    ActivitySourceGroup *source;
    CameraDataOwner *base;
    ManagedEyes *base_eyes;
    CameraDataComponentApi binding[7];
    CameraDataOwner data[7];
    ManagedEyes eyes[7];
    CameraStackMember members[8];
    CameraStackManaged adapter;
    CameraStackScratch scratch;
    SourceRenderGroupApi frame_api;
#ifdef GF2VR_ACTIVITY_PRIMARY_UI
    /* Legacy field aliases retain established observer/test interfaces. */
    union {ActivityUiLayer ui;struct {
        CameraDataComponentApi ui_binding;CameraDataOwner ui_data;ManagedEyes ui_eyes;
        unsigned ui_linked,ui_shown,ui_activated;
#ifdef GF2VR_ACTIVITY_UI_FRAME_ELIGIBLE
        unsigned ui_enrolled;
#endif
    };};
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
    ActivityUiLayer dialogue_ui;
#endif
#endif
    unsigned count,ready,busy,retiring,linked,borrowed,activated;
    unsigned long long generation,frame;
#ifdef GF2VR_ACTIVITY_TARGET_SURFACE
    ActivityTargetSurface surfaces[9]; /* seven world Overlays, two optional UI layers */
#endif
} ActivityEyeGroup;
static int activity_group_source_valid(ActivityEyeGroup *g) {
    return g && !g->retiring && g->generation && g->count>1 &&
        activity_source_current(g->source,g->generation) &&
        g->source->active.count==g->count;
}
static int activity_group_link_valid(void *context) {
    ActivityEyeGroup *g=context;
    return g && g->busy && activity_group_source_valid(g) && g->base && !g->base->retiring;
}
#ifdef GF2VR_ACTIVITY_PRIMARY_UI
#include "activity_private_ui.h"
#ifdef GF2VR_ACTIVITY_UI_FRAME_ELIGIBLE
#include "activity_ui_frame_eligibility.h"
#endif
#ifdef GF2VR_ACTIVITY_UI_LIFETIME
#include "activity_ui_transition_group.h"
#endif
#endif
#ifdef GF2VR_ACTIVITY_TARGET_SURFACE
static int activity_group_surface_clear_all(ActivityEyeGroup *g) {
    if(!g)return 0;
    unsigned live=0;for(unsigned i=0;i<9;++i)live|=g->surfaces[i].live_mask;
    if(!live)return 1;
    if(!g->base_eyes || g->base_eyes->borrowed || g->borrowed)return 0;
    int clean=1;
    for(unsigned i=0;i<7;++i)if(g->surfaces[i].live_mask &&
       !activity_target_surface_clear(&g->binding[i].refs.settings.calls,&g->surfaces[i],&g->eyes[i]))clean=0;
#ifdef GF2VR_ACTIVITY_PRIMARY_UI
    for(unsigned i=0;i<activity_ui_layer_count();++i){ActivityUiLayer *l=activity_ui_layer(g,i);
        if(g->surfaces[7+i].live_mask &&
           !activity_target_surface_clear(&l->binding.refs.settings.calls,&g->surfaces[7+i],&l->eyes))clean=0;}
#endif
    return clean;
}
#endif
#ifdef GF2VR_ACTIVITY_VIEWPORT
static int activity_group_viewports(ActivityEyeGroup *g) {
    if(!activity_group_link_valid(g) || g->count>8 || !g->base->binding ||
       g->borrowed || !g->base_eyes || g->base_eyes->borrowed)return 0;
    const ManagedEyeApi *a=&g->base->binding->refs.settings.calls;
    for(unsigned eye=0;eye<2;++eye) {
        ManagedEyeSlot *b=&g->base_eyes->eyes[eye];ActivityViewport base,check;
        if(!b->camera_root || !b->target_root || !b->target ||
           !activity_viewport_read(a,b->camera,&base) || !activity_group_link_valid(g))return 0;
        for(unsigned i=1;i<g->count;++i) {
            ManagedEyes *eyes=&g->eyes[i-1];ManagedEyeSlot *e=&eyes->eyes[eye];
            if(eyes->borrowed || eyes->targets_ready || e->target || e->target_root ||
               !e->camera_root || !e->camera || e->camera==b->camera)return 0;
            /* No original member or opposite-eye alias can become a setter target. */
            for(unsigned m=0;m<g->count;++m) {
                if(e->camera==g->members[m].source || b->camera==g->members[m].source)return 0;
                void *left=m?g->eyes[m-1].eyes[0].camera:g->base_eyes->eyes[0].camera;
                void *right=m?g->eyes[m-1].eyes[1].camera:g->base_eyes->eyes[1].camera;
                if(e->camera==(eye?left:right))return 0;
                if(m!=i && e->camera==(eye?right:left))return 0;
            }
            ActivityViewport before,after;
#ifdef GF2VR_ACTIVITY_TARGET_SURFACE
            if(!g->source->topology || !g->source->topology->api ||
               !activity_target_surface_bind(a,g->source->topology->api->target,&g->surfaces[i-1],
                   eyes,g->base_eyes,eye,activity_group_link_valid,g))return 0;
#endif
            if(!activity_viewport_align(a,e->camera,&base,&before,&after,activity_group_link_valid,g))return 0;
            GF2VR_ACTIVITY_VIEWPORT_REPORT(g->generation,i,eye,base.v,before.v,after.v);
        }
        if(!activity_viewport_read(a,b->camera,&check) || !activity_group_link_valid(g) ||
           !activity_viewport_equal(&base,&check))return 0;
    }
    return activity_group_link_valid(g);
}
#endif
static void activity_group_invalidate(ActivityEyeGroup *g) {
    if(!g)return;
    g->retiring=1;g->ready=0;
#ifdef GF2VR_ACTIVITY_PRIMARY_UI
    for(unsigned i=0;i<activity_ui_layer_count();++i){ActivityUiLayer *l=activity_ui_layer(g,i);
        l->data.retiring=1;l->data.ready=l->eyes.ready=l->eyes.matrices_ready=0;}
#endif
    if(g->base){g->base->retiring=1;g->base->ready=0;}
    for(unsigned i=0;i<7;++i){g->data[i].retiring=1;g->data[i].ready=0;g->eyes[i].ready=g->eyes[i].matrices_ready=0;}
}
static int activity_group_retire(void *context) {
    ActivityEyeGroup *g=context;if(!g)return 0;
    activity_group_invalidate(g);
    if(g->busy || g->borrowed || (g->base && (g->base->busy || g->base_eyes->borrowed)))return 0;
    for(unsigned i=0;i<7;++i)if(g->data[i].busy || g->eyes[i].borrowed)return 0;
#ifdef GF2VR_ACTIVITY_PRIMARY_UI
    for(unsigned i=0;i<activity_ui_layer_count();++i){ActivityUiLayer *l=activity_ui_layer(g,i);
        if(l->data.busy || l->eyes.borrowed)return 0;}
#endif
    int clean=1;g->busy=1;
#ifdef GF2VR_ACTIVITY_PRIMARY_UI
    #ifdef GF2VR_ACTIVITY_UI_FRAME_ELIGIBLE
    if(!activity_ui_stop_eligibility(g)){g->busy=0;return 0;}
    #endif
#endif
#ifdef GF2VR_ACTIVITY_TARGET_SURFACE
    /* Unknown destination references retain every Base root. Never destroy the
       shared target merely because CPU returned or private destruction began. */
    if(!activity_group_surface_clear_all(g)){g->busy=0;return 0;}
#endif
#ifdef GF2VR_ACTIVITY_PRIMARY_UI
    for(unsigned i=0;i<activity_ui_layer_count();++i){ActivityUiLayer *l=activity_ui_layer(g,i);
        if(!camera_data_owner_retire_single(&l->data))clean=0;
        if(l->binding.type_root && (!managed_eyes_retire(&l->binding.refs.settings.calls,&l->eyes) ||
           !camera_data_component_release(&l->binding)))clean=0;
    }
#endif
    for(unsigned i=7;i>0;--i) {
        unsigned n=i-1;
        if(!camera_data_owner_retire_single(&g->data[n])){clean=0;continue;}
        if(g->binding[n].type_root) {
            if(!managed_eyes_retire(&g->binding[n].refs.settings.calls,&g->eyes[n]) ||
               !camera_data_component_release(&g->binding[n])){clean=0;continue;}
            g->eyes[n]=(ManagedEyes){0};
        }
    }
    if(g->base && !camera_data_owner_retire_single(g->base))clean=0;
    g->busy=0;if(clean)*g=(ActivityEyeGroup){0};return clean;
}
static int activity_group_refresh(void *context,int (*valid)(void *,unsigned long long),void *owner) {
    ActivityEyeGroup *g=context;
    if(!g || g->busy || g->borrowed || !activity_group_source_valid(g) || !g->base ||
       !g->base->binding || g->base->busy || g->base_eyes->borrowed || !valid ||
       !valid(owner,g->generation))return 0;
    g->busy=1;g->ready=0;unsigned targets=g->base_eyes->targets_ready;int ok=0;
    g->base->busy=1;g->base->ready=0;g->base_eyes->ready=g->base_eyes->matrices_ready=0;
    g->base_eyes->targets_ready=0;
    if(!activity_source_recheck(g->source,g->generation))goto done;
#ifdef GF2VR_ACTIVITY_TARGET_SURFACE
    if(!activity_group_surface_clear_all(g))goto done;
#endif
#ifdef GF2VR_ACTIVITY_PRIMARY_UI
    if(activity_ui_linked_count(g) && !activity_group_ui_link(g,0))goto done;
    for(unsigned i=0;i<activity_ui_layer_count();++i)activity_ui_layer(g,i)->shown=0;
#endif
    if(targets && !camera_data_owner_targets(g->base,0,valid,owner))goto done;
    /* Preserve Camera and AdditionalCameraData per member, including masks,
       renderer/clear-depth/pass scalars. No source setter or mask union. */
    for(unsigned i=0;i<g->count;++i) {
        CameraDataOwner *d=i?&g->data[i-1]:g->base;
        ManagedEyes *e=i?&g->eyes[i-1]:g->base_eyes;
        if(!d->binding || d->retiring || e->borrowed)goto done;
        d->busy=1;d->ready=0;e->ready=e->matrices_ready=0;
        const ManagedEyeApi *c=&d->binding->refs.settings.calls;
        for(unsigned eye=0;eye<2;++eye) {
            unsigned char off=0;void *source_args[]={d->source_camera},*off_args[]={&off},*null_args[]={0};
            if(!managed_eye_call(c,c->copy_from,e->eyes[eye].camera,source_args,0) ||
               !managed_eye_call(c,c->set_enabled,e->eyes[eye].camera,off_args,0) ||
               !managed_eye_call(c,c->set_target,e->eyes[eye].camera,null_args,0))goto done;
        }
        if(!camera_data_settings_copy(&d->binding->refs.settings,d->components[0],d->components[1],d->components[2]) ||
           !camera_data_references_copy_mode(&d->binding->refs,d->components[0],d->components[1],d->components[2],1) ||
           !valid(owner,g->generation) || !activity_group_source_valid(g))goto done;
    }
    if(!camera_stack_managed_prepare_with(&g->adapter,&g->scratch,g->linked) ||
       !activity_source_recheck(g->source,g->generation) || !valid(owner,g->generation))goto done;
    if(targets && !camera_data_owner_targets(g->base,1,valid,owner))goto done;
#ifdef GF2VR_ACTIVITY_VIEWPORT
    if(targets && !activity_group_viewports(g))goto done;
#endif
#ifdef GF2VR_ACTIVITY_PRIMARY_UI
    if(!activity_group_ui_refresh(g))goto done;
#endif
    ok=activity_group_source_valid(g) && valid(owner,g->generation);
done:
    g->base->busy=0;
    for(unsigned i=0;i<7;++i)g->data[i].busy=0;
    g->busy=0;
    if(!ok){activity_group_invalidate(g);source_generation_invalidate(g->source->source);return 0;}
    g->base_eyes->targets_ready=targets;g->base->ready=g->base_eyes->ready=1;
    for(unsigned i=1;i<g->count;++i)g->data[i-1].ready=g->eyes[i-1].ready=1;
    g->linked=1;g->ready=1;return 1;
}
/* Authored member-relative translation and yaw/pitch survive the same physical
   packet. Authored roll is excluded; physical reference is read, never reset. */
static int activity_overlay_poses(const PoseMath *m,const ManagedEyePose *base,
    const ManagedEyePose *member,const ManagedEyePose eyes[2],ManagedEyePose out[2]) {
    double bq[4],mq[4],ba[3],ma[3],local[3],delta[3],inverse[4];
    if(!pose_read(m,base,bq) || !pose_read(m,member,mq) || !pose_angles(m,bq,ba) || !pose_angles(m,mq,ma))return 0;
    double y[4]={0,m->sin(ba[0]/2),0,m->cos(ba[0]/2)};
    double p[4]={m->sin(ba[1]/2),0,0,m->cos(ba[1]/2)};pose_multiply(y,p,bq);
    for(unsigned i=0;i<4;++i)inverse[i]=i<3?-bq[i]:bq[i];
    for(unsigned i=0;i<3;++i)delta[i]=(double)member->position[i]-base->position[i];
    pose_rotate(inverse,delta,local);
    double my[4]={0,m->sin(ma[0]/2),0,m->cos(ma[0]/2)};
    double mp[4]={m->sin(ma[1]/2),0,0,m->cos(ma[1]/2)},relative[4];
    pose_multiply(my,mp,mq);pose_multiply(inverse,mq,relative);
    for(unsigned eye=0;eye<2;++eye) {
        double q[4],offset[3],final[4];
        if(!pose_read(m,&eyes[eye],q))return 0;
        pose_rotate(q,local,offset);
        /* Apply the authored relative orientation in the final eye's local
           basis. Euler differences would lose that basis under physical roll. */
        pose_multiply(q,relative,final);
        for(unsigned i=0;i<3;++i)if(!eye_optics_float(eyes[eye].position[i]+offset[i],&out[eye].position[i]))return 0;
        for(unsigned i=0;i<4;++i)if(!eye_optics_float(final[i],&out[eye].rotation[i]))return 0;
    }
    return 1;
}
static int activity_group_apply(void *context,SourcePoseApi *reader,SourceAnchorState *anchor,
    PoseMathBinding *math,XrEyeMailbox *mailbox,const PoseAppliedTicket *ticket) {
    ActivityEyeGroup *g=context;
    if(!g || !g->ready || g->busy || g->borrowed || !activity_group_source_valid(g) ||
       !math || !math->module || math->borrowed || !ticket || ticket->source_generation!=g->generation ||
       !pose_ticket_current(mailbox,ticket,g->generation))return 0;
    g->busy=1;math->borrowed=1;int ok=0;
    XrEyeInput packet;UnityTrackingInput tracking;ManagedEyePose base_eyes[2];SourcePoseSnapshot base;
    if(!xr_eye_mailbox_read(mailbox,&packet) || packet.frame_id!=ticket->frame_id ||
       !xr_eye_unity_tracking(&packet,&tracking) || !pose_compose(&math->api,&tracking,&anchor->anchor,base_eyes) ||
       !source_pose_read(reader,g->base->source_camera,&base))goto done;
    for(unsigned i=1;i<g->count;++i) {
        CameraDataOwner *d=&g->data[i-1];ManagedEyes *e=&g->eyes[i-1];SourcePoseSnapshot authored;
        ManagedEyePose poses[2];ManagedEyeMatrix views[2],projections[2];d->busy=1;
        if(!source_pose_read(reader,d->source_camera,&authored) ||
           !activity_overlay_poses(&math->api,&base.pose,&authored.pose,base_eyes,poses) ||
           !eye_optics_build(poses,tracking.tangents,authored.near_z,authored.far_z,views,projections) ||
           !managed_eyes_matrices_mode(&d->binding->refs.settings.calls,e,ticket->frame_id,
               views,projections,poses,1) || !activity_group_source_valid(g))goto done;
        d->busy=0;
    }
#ifdef GF2VR_ACTIVITY_PRIMARY_UI
    if(!activity_group_ui_apply(g,&math->api,reader,&tracking,ticket))goto done;
#endif
    ok=pose_ticket_current(mailbox,ticket,g->generation) && activity_group_source_valid(g);
done:
    for(unsigned i=0;i<7;++i){g->data[i].busy=0;if(!ok)g->eyes[i].matrices_ready=0;}
    math->borrowed=0;g->busy=0;return ok;
}
static int activity_group_cpu_return(void *context);
static int activity_group_borrow(void *context,unsigned long long frame) {
    ActivityEyeGroup *g=context;
    if(!g || !g->ready || g->busy || g->borrowed || !activity_group_source_valid(g))return 0;
    for(unsigned i=1;i<g->count;++i) {
        ManagedEyes *e=&g->eyes[i-1];
        if(!g->data[i-1].ready || g->data[i-1].retiring || e->borrowed || !e->ready ||
           !e->matrices_ready || e->matrix_frame!=frame || e->targets_ready ||
           e->eyes[0].target_root || e->eyes[1].target_root)return 0;
    }
    /* Actual cached URP IL00c1/01ae requires isActiveAndEnabled on Overlays.
       The original input is already materialized. Only our script-free private
       Overlay objects are enabled for this selected CPU render. */
#ifdef GF2VR_ACTIVITY_PRIMARY_UI
    for(unsigned i=0;i<activity_ui_layer_count();++i){ActivityUiLayer *l=activity_ui_layer(g,i);
        if(l->shown && (!l->linked || !l->data.ready || !l->eyes.ready || l->eyes.borrowed ||
           !l->eyes.matrices_ready || l->eyes.matrix_frame!=frame || l->eyes.targets_ready))return 0;}
#endif
    g->busy=1;g->activated=1;int ok=1;
    for(unsigned i=1;i<g->count && ok;++i)for(unsigned eye=0;eye<2 && ok;++eye) {
        const ManagedEyeApi *c=&g->binding[i-1].refs.settings.calls;
        unsigned char on=1;void *args[]={&on};
        ok=managed_eye_call(c,c->set_active,g->eyes[i-1].eyes[eye].object,args,0) &&
            managed_eye_call(c,c->set_enabled,g->eyes[i-1].eyes[eye].camera,args,0) && activity_group_source_valid(g);
    }
#ifdef GF2VR_ACTIVITY_PRIMARY_UI
    for(unsigned i=0;i<activity_ui_layer_count() && ok;++i){ActivityUiLayer *l=activity_ui_layer(g,i);
        if(!l->shown)continue;const ManagedEyeApi *c=&l->binding.refs.settings.calls;
#ifdef GF2VR_ACTIVITY_UI_FRAME_ELIGIBLE
        if(!l->enrolled && !activity_ui_enroll_layer(g,l))ok=0;
#endif
        l->activated=1;unsigned char on=1;void *args[]={&on};
        for(unsigned eye=0;eye<2 && ok;++eye)ok=managed_eye_call(c,c->set_active,l->eyes.eyes[eye].object,args,0) &&
            managed_eye_call(c,c->set_enabled,l->eyes.eyes[eye].camera,args,0) && activity_group_source_valid(g);
#ifdef GF2VR_ACTIVITY_UI_FRAME_ELIGIBLE
        if(ok)ok=activity_ui_verify_layer(g,l);
#endif
    }
#endif
    g->busy=0;
    if(!ok) {
#ifdef GF2VR_ACTIVITY_UI_FRAME_ELIGIBLE
        (void)activity_ui_stop_eligibility(g);
#endif
        (void)activity_group_cpu_return(g);
        source_generation_invalidate(g->source->source);return 0;
    }
    for(unsigned i=1;i<g->count;++i){g->eyes[i-1].borrowed=1;g->eyes[i-1].matrices_ready=0;}
#ifdef GF2VR_ACTIVITY_PRIMARY_UI
    for(unsigned i=0;i<activity_ui_layer_count();++i){ActivityUiLayer *l=activity_ui_layer(g,i);
        if(l->shown){l->eyes.borrowed=1;l->eyes.matrices_ready=0;}}
#endif
    g->borrowed=1;g->frame=frame;return 1;
}
static int activity_group_frame_current(void *context) {
    ActivityEyeGroup *g=context;
    if(!g || !g->ready || !g->borrowed || !activity_group_source_valid(g))return 0;
#ifdef GF2VR_ACTIVITY_PRIMARY_UI
    for(unsigned i=0;i<activity_ui_layer_count();++i){ActivityUiLayer *l=activity_ui_layer(g,i);
        if(l->shown && (!l->eyes.borrowed || l->eyes.matrix_frame!=g->frame || !l->data.ready || l->data.retiring))return 0;}
#endif
    for(unsigned i=1;i<g->count;++i)if(!g->data[i-1].ready || g->data[i-1].retiring ||
       !g->eyes[i-1].borrowed || g->eyes[i-1].matrix_frame!=g->frame)return 0;
    return 1;
}
static void activity_group_release(void *context) {
    ActivityEyeGroup *g=context;
    (void)activity_group_cpu_return(g);
    for(unsigned i=1;i<g->count;++i)managed_eyes_return(&g->eyes[i-1]);
#ifdef GF2VR_ACTIVITY_PRIMARY_UI
    for(unsigned i=0;i<activity_ui_layer_count();++i){ManagedEyes *e=&activity_ui_layer(g,i)->eyes;
        if(e->borrowed)managed_eyes_return(e);}
#endif
    g->borrowed=0;g->frame=0;
}
static int activity_group_cpu_return(void *context) {
    ActivityEyeGroup *g=context;if(!g || g->busy)return 0;
#ifdef GF2VR_ACTIVITY_UI_FRAME_ELIGIBLE
    if(!g->activated && activity_ui_enrolled_any(g) && !activity_group_source_valid(g)) {
        g->busy=1;int ok=activity_ui_stop_eligibility(g);g->busy=0;return ok;
    }
#endif
    if(!g->activated)return 1;
    g->busy=1;int ok=1;
    for(unsigned i=1;i<g->count;++i)for(unsigned eye=0;eye<2;++eye) {
        const ManagedEyeApi *c=&g->binding[i-1].refs.settings.calls;
        unsigned char off=0;void *args[]={&off};
        if(!managed_eye_call(c,c->set_enabled,g->eyes[i-1].eyes[eye].camera,args,0))ok=0;
        if(!managed_eye_call(c,c->set_active,g->eyes[i-1].eyes[eye].object,args,0))ok=0;
    }
#ifdef GF2VR_ACTIVITY_PRIMARY_UI
    for(unsigned i=0;i<activity_ui_layer_count();++i){ActivityUiLayer *l=activity_ui_layer(g,i);
        if(!l->activated)continue;
#ifdef GF2VR_ACTIVITY_UI_FRAME_ELIGIBLE
        if(!l->enrolled || !activity_group_source_valid(g)){
            if(!activity_ui_stop_layer(g,l))ok=0;
        }
#else
        const ManagedEyeApi *c=&l->binding.refs.settings.calls;unsigned char off=0;void *args[]={&off};
        for(unsigned eye=0;eye<2;++eye){
            if(!managed_eye_call(c,c->set_enabled,l->eyes.eyes[eye].camera,args,0))ok=0;
            if(!managed_eye_call(c,c->set_active,l->eyes.eyes[eye].object,args,0))ok=0;
        }if(ok)l->activated=0;
#endif
    }
#endif
    g->busy=0;if(ok)g->activated=0;
    else {activity_group_invalidate(g);source_generation_invalidate(g->source->source);}
    return ok;
}
static int activity_group_prepare(ActivityEyeGroup *g,ActivitySourceGroup *source,
    SourceEyePreparation *p,unsigned long thread,int width,int height) {
    SourceGeneration *s=p?p->source:0;
    if(!g || !source || !p || !s || thread!=s->owner_thread || s->busy || width<=0 || height<=0 ||
       !activity_source_current(source,s->selection.generation))return 0;
    CameraDataOwner *base=source_eye_data(p);
    if(g->generation)return g->ready && !g->retiring && !g->borrowed && !p->eyes->borrowed &&
        g->generation==s->selection.generation && p->generation==g->generation &&
        p->width==width && p->height==height && base->group==g;
    if(base->binding || p->generation || p->eyes->ready || p->eyes->borrowed || p->binding->in_use)return 0;
    g->source=source;g->base=base;g->base_eyes=p->eyes;g->count=source->active.count;
    g->generation=s->selection.generation;
#ifdef GF2VR_ACTIVITY_SCALE_PREPARE_OBSERVE
    const unsigned long long observed_generation=g->generation;
    const unsigned observed_count=g->count;
    unsigned observed_stage=10,observed_member=0;
#define AGP_STAGE(n) (observed_stage=(n))
#else
#define AGP_STAGE(n) ((void)0)
#endif
    g->frame_api=(SourceRenderGroupApi){g,activity_group_apply,activity_group_borrow,activity_group_frame_current,activity_group_release,activity_group_cpu_return};
    s->busy=1;s->cancelled=0;int ok=0;
    const ManagedEyeApi *calls=&p->binding->refs.settings.calls;
    if(!managed_eyes_create(calls,p->eyes,s->selection.source) ||
       (AGP_STAGE(11),!source_eye_current(p,g->generation)) ||
       (AGP_STAGE(12),!camera_data_owner_attach(p->binding,base,p->eyes,
           s->selection.source,g->generation,source_eye_current,p)))goto done;
    base->group=g;base->group_refresh=activity_group_refresh;base->group_retire=activity_group_retire;
    for(unsigned i=1;i<g->count;++i) {
#ifdef GF2VR_ACTIVITY_SCALE_PREPARE_OBSERVE
        observed_member=i;
#endif
        AGP_STAGE(20);
        unsigned n=i-1;CameraDataComponentApi *b=&g->binding[n];
        b->refs=p->binding->refs;b->type=p->binding->type;b->get_component=p->binding->get_component;
        b->type_root=calls->pin(b->type,1);if(!b->type_root)goto done;
        void *camera=source->active.entries[i].identity.source;
        if((AGP_STAGE(21),!managed_eyes_create(calls,&g->eyes[n],camera)) ||
           (AGP_STAGE(22),!source_eye_current(p,g->generation)) ||
           (AGP_STAGE(23),!camera_data_owner_attach(b,&g->data[n],&g->eyes[n],camera,g->generation,source_eye_current,p)))goto done;
    }
#ifdef GF2VR_ACTIVITY_PRIMARY_UI
    AGP_STAGE(30);
    g->busy=1;int ui_prepared=activity_group_ui_prepare(g,calls);g->busy=0;
    if(!ui_prepared || !source_eye_current(p,g->generation))goto done;
#endif
    for(unsigned i=0;i<g->count;++i) {
        CameraDataOwner *d=i?&g->data[i-1]:base;ManagedEyes *e=i?&g->eyes[i-1]:p->eyes;
        g->members[i]=(CameraStackMember){d->source_camera,e->eyes[0].camera,e->eyes[1].camera};
        for(unsigned j=0;j<3;++j)g->adapter.components[i][j]=d->components[j];
    }
    g->adapter.refs=&p->binding->refs;g->adapter.members=g->members;g->adapter.count=g->count;
    g->adapter.context=g;g->adapter.valid=activity_group_link_valid;g->adapter.bind_list=csm_bind_list;
    if((AGP_STAGE(40),!activity_group_refresh(g,source_eye_current,p)) ||
       (AGP_STAGE(50),!managed_eyes_targets(calls,p->eyes,width,height)) ||
       (AGP_STAGE(55),!source_eye_current(p,g->generation)))goto done;
#ifdef GF2VR_ACTIVITY_VIEWPORT
    AGP_STAGE(60);
    g->busy=1;int viewport_ok=activity_group_viewports(g);g->busy=0;
    if(!viewport_ok || !source_eye_current(p,g->generation))goto done;
#endif
#ifdef GF2VR_ACTIVITY_PRIMARY_UI
    AGP_STAGE(70);
    g->busy=1;int ui_viewport=activity_group_ui_refresh(g);g->busy=0;
    if(!ui_viewport || !source_eye_current(p,g->generation))goto done;
#endif
    p->generation=g->generation;p->width=width;p->height=height;ok=1;
done:
#ifdef GF2VR_ACTIVITY_SCALE_PREPARE_OBSERVE
    GF2VR_ACTIVITY_PREPARE_BOUNDARY_REPORT(observed_generation,observed_count,observed_member,
        observed_stage,width,height,ok);
#endif
#undef AGP_STAGE
    if(!ok){activity_group_invalidate(g);source_generation_invalidate(s);}
    s->busy=0;if(!ok)(void)source_generation_collect(s,thread);return ok;
}
#endif
