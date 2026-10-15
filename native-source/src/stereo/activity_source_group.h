#ifndef GF2_ACTIVITY_SOURCE_GROUP_H
#define GF2_ACTIVITY_SOURCE_GROUP_H
#include "main_test_grant.h"
#include "main_phase_roles.h"
#include "activity_stack_plan.h"
#ifdef GF2VR_GENERIC_SOURCE_ADAPTER
#include "recent_camera_anchor.h"
#endif
/* A typed, complete-input game adapter. The Base owns stack rendering; it is
   not a generic newest-camera policy. Retain every world member before the
   rooted input returns, then transfer roots only at generation adoption. */
#ifdef GF2VR_ACTIVITY_PRIMARY_UI
typedef struct {
    SourceTopologyEntry entry;PhaseRoleSnapshot role;
    void *canvas;unsigned camera_root,canvas_root;float plane;
} ActivityPrimarySource;
#ifndef GF2VR_ACTIVITY_UI_CAPTURE
#define GF2VR_ACTIVITY_UI_CAPTURE(a,s,e,r) 0
#endif
#ifndef GF2VR_ACTIVITY_UI_CURRENT
#define GF2VR_ACTIVITY_UI_CURRENT(a) 1
#endif
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
#ifndef GF2VR_ACTIVITY_DIALOGUE_CAPTURE
#define GF2VR_ACTIVITY_DIALOGUE_CAPTURE(a,s,e,r) ((void)(a),(void)(s),(void)(e),(void)(r),0)
#endif
#endif
#endif
typedef struct {
    SourceTopologyEntry entries[8];
    PhaseRoleSnapshot roles[8];
    unsigned roots[8],count;
    unsigned long long stamp;
#ifdef GF2VR_ACTIVITY_PRIMARY_UI
    ActivityPrimarySource primary;
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
    ActivityPrimarySource dialogue;
#endif
#endif
} ActivitySourceSnapshot;
typedef struct {
    SourceGeneration *source;
    SourceTopology *topology;
    CameraDataComponentApi *binding;
    MainTestGrant *manual;
    ActivitySourceSnapshot active,pending,reading;
    PhaseRoleSnapshot input_roles[8];
    unsigned busy;
    unsigned long long clock;
#ifdef GF2VR_ACTIVITY_UI_LIFETIME
    /* Optional complete UI changes do not change the complete world stamp.
       Pins stay here until all old managed/native/render/copy borrowers drain. */
    ActivitySourceSnapshot ui_pending;
    unsigned ui_change,ui_pending_ready;
    unsigned long long ui_clock,ui_serial;
#endif
#ifdef GF2VR_GENERIC_SOURCE_ADAPTER
    RecentCameraAnchor recent;
    RcaId generic_unique; /* Compare-only identity of the new unique-world route. */
    RcaCamera recent_input[8];
    void *active_getter;
    unsigned active_attempted;
    unsigned long long recent_clock;
#endif

#ifdef GF2VR_SKILL_SINGLE_SOURCE
    SourceTopologyEntry qualified; /* Compare-only snapshot, never dereference retained objects. */
    unsigned qualified_valid;
#endif
} ActivitySourceGroup;
static int activity_entry_same(const SourceTopologyEntry *a,const SourceTopologyEntry *b) {
    return source_generation_same(&a->identity,&b->identity) &&
        a->orthographic==b->orthographic && a->display==b->display && a->width==b->width &&
        a->height==b->height && a->mask==b->mask && a->has_target==b->has_target;
}
static int activity_role_same(const PhaseRoleSnapshot *a,const PhaseRoleSnapshot *b) {
    if(a->attached!=b->attached || a->complete!=b->complete || a->valid!=b->valid ||
       a->render_type!=b->render_type || a->clear_depth!=b->clear_depth ||
       a->renderer!=b->renderer || a->output!=b->output)return 0;
    for(unsigned l=0;l<2;++l) {
        if(a->lists[l].present!=b->lists[l].present || a->lists[l].complete!=b->lists[l].complete ||
           a->lists[l].count!=b->lists[l].count)return 0;
        for(unsigned i=0;i<a->lists[l].count;++i)if(a->lists[l].cameras[i]!=b->lists[l].cameras[i])return 0;
    }
    return 1;
}
static void activity_snapshot_drop(ActivitySourceGroup *a,ActivitySourceSnapshot *s) {
#ifdef GF2VR_ACTIVITY_PRIMARY_UI
    if(s->primary.canvas_root)a->binding->refs.settings.calls.unpin(s->primary.canvas_root);
    if(s->primary.camera_root)a->binding->refs.settings.calls.unpin(s->primary.camera_root);
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
    if(s->dialogue.canvas_root)a->binding->refs.settings.calls.unpin(s->dialogue.canvas_root);
    if(s->dialogue.camera_root)a->binding->refs.settings.calls.unpin(s->dialogue.camera_root);
#endif
#endif
    for(unsigned i=8;i>0;--i)if(s->roots[i-1])a->binding->refs.settings.calls.unpin(s->roots[i-1]);
    *s=(ActivitySourceSnapshot){0};
}
#ifdef GF2VR_ACTIVITY_UI_ROUTE_OBSERVE
#include "activity_ui_route_observe.h"
#define AUI_ROUTE(a,e,n,r,route,ok,capture,reason) activity_ui_route_record(a,e,n,r,route,ok,capture,reason)
#else
#define AUI_ROUTE(a,e,n,r,route,ok,capture,reason) ((void)0)
#endif
static int activity_snapshot_same(const ActivitySourceSnapshot *a,const ActivitySourceSnapshot *b) {
    if(!a->count || a->count!=b->count)return 0;
#ifdef GF2VR_ACTIVITY_PRIMARY_UI
    if((a->primary.canvas!=0)!=(b->primary.canvas!=0))return 0;
    if(a->primary.canvas && (a->primary.canvas!=b->primary.canvas || a->primary.plane!=b->primary.plane ||
       !activity_entry_same(&a->primary.entry,&b->primary.entry) || !activity_role_same(&a->primary.role,&b->primary.role)))return 0;
#endif
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
    if((a->dialogue.canvas!=0)!=(b->dialogue.canvas!=0))return 0;
    if(a->dialogue.canvas && (a->dialogue.canvas!=b->dialogue.canvas || a->dialogue.plane!=b->dialogue.plane ||
       !activity_entry_same(&a->dialogue.entry,&b->dialogue.entry) || !activity_role_same(&a->dialogue.role,&b->dialogue.role)))return 0;
#endif
    for(unsigned i=0;i<a->count;++i)if(!activity_entry_same(&a->entries[i],&b->entries[i]) ||
        !activity_role_same(&a->roles[i],&b->roles[i]))return 0;
    return 1;
}
#ifdef GF2VR_ACTIVITY_UI_LIFETIME
static int activity_world_snapshot_same(const ActivitySourceSnapshot *a,const ActivitySourceSnapshot *b) {
    if(!a->count || a->count!=b->count)return 0;
    for(unsigned i=0;i<a->count;++i)if(!activity_entry_same(&a->entries[i],&b->entries[i]) ||
        !activity_role_same(&a->roles[i],&b->roles[i]))return 0;
    return 1;
}
static int activity_ui_source_same(const ActivityPrimarySource *a,const ActivityPrimarySource *b) {
    if((a->canvas!=0)!=(b->canvas!=0))return 0;
    return !a->canvas || (a->canvas==b->canvas && a->plane==b->plane &&
        activity_entry_same(&a->entry,&b->entry) && activity_role_same(&a->role,&b->role));
}
static int activity_ui_snapshot_same(const ActivitySourceSnapshot *a,const ActivitySourceSnapshot *b) {
    return activity_ui_source_same(&a->primary,&b->primary) &&
        activity_ui_source_same(&a->dialogue,&b->dialogue);
}
static void activity_ui_move(ActivitySourceSnapshot *out,ActivitySourceSnapshot *from) {
    out->primary=from->primary;from->primary=(ActivityPrimarySource){0};
    out->dialogue=from->dialogue;from->dialogue=(ActivityPrimarySource){0};
}
static void activity_ui_request_change(ActivitySourceGroup *a) {
    a->ui_change=1;
    GF2VR_ACTIVITY_UI_INVALIDATE(a);
}
#endif
static int activity_source_collect(void *context) {
    ActivitySourceGroup *a=context;
    if(!a || a->busy)return 0;
    /* Called only AFTER all pool eye/data/CPU/GPU borrowers drain. Pending is
       the newly rooted candidate, independent from the retiring active set. */
#ifdef GF2VR_ACTIVITY_UI_GENERATION_LEASE
    if(!GF2VR_ACTIVITY_UI_GENERATION_RELEASE(a))return 0;
#endif
    activity_snapshot_drop(a,&a->active);
#ifdef GF2VR_ACTIVITY_UI_LIFETIME
    activity_snapshot_drop(a,&a->ui_pending);a->ui_change=a->ui_pending_ready=0;
    GF2VR_ACTIVITY_UI_INVALIDATE(a);
#endif
    return 1;
}
static int activity_source_adopt(void *context,unsigned long long stamp) {
    ActivitySourceGroup *a=context;
    if(!a || a->busy || a->active.count)return 0;
    if(!stamp){activity_snapshot_drop(a,&a->pending);return 1;}
    if(stamp!=a->pending.stamp || !a->pending.count)return 0;
#ifdef GF2VR_ACTIVITY_UI_LIFETIME
    if(a->ui_clock==~0ULL)return 0;
    a->ui_serial=++a->ui_clock;
#endif
    a->active=a->pending;a->pending=(ActivitySourceSnapshot){0};return 1;
}
static int activity_source_current(ActivitySourceGroup *a,unsigned long long generation) {
    SourceGeneration *s=a?a->source:0;
    return s && generation && !s->cancelled && s->selection.eligible && s->source_root &&
        s->selection.generation==generation && a->active.count>1 &&
        s->identity.group_stamp==a->active.stamp &&
        s->selection.source==a->active.entries[0].identity.source;
}
static int activity_source_recheck(ActivitySourceGroup *a,unsigned long long generation) {
    if(!activity_source_current(a,generation) || a->busy)return 0;
    a->busy=1;int ok=1;
    for(unsigned i=0;i<a->active.count;++i) {
        SourceTopologyEntry e={0};PhaseRoleSnapshot r={0};
        if(!source_topology_entry(a->topology->api,a->active.entries[i].identity.source,&e)) {ok=0;break;}
        phase_role_read(a->binding,e.identity.source,phase_list_bind,&r);
        if(!activity_entry_same(&e,&a->active.entries[i]) || !activity_role_same(&r,&a->active.roles[i]) ||
           !activity_source_current(a,generation)){ok=0;break;}
    }
    #if defined(GF2VR_ACTIVITY_PRIMARY_UI) && !defined(GF2VR_ACTIVITY_UI_LIFETIME)
    if(ok && a->active.primary.canvas) {
        SourceTopologyEntry e={0};PhaseRoleSnapshot r={0};
        if(!source_topology_entry(a->topology->api,a->active.primary.entry.identity.source,&e))ok=0;
        else {phase_role_read(a->binding,e.identity.source,phase_list_bind,&r);
            ok=activity_entry_same(&e,&a->active.primary.entry) && activity_role_same(&r,&a->active.primary.role) && GF2VR_ACTIVITY_UI_CURRENT(a);}
    }
    #endif
#if defined(GF2VR_ACTIVITY_DIALOGUE_UI) && !defined(GF2VR_ACTIVITY_UI_LIFETIME)
    if(ok && a->active.dialogue.canvas) {
        SourceTopologyEntry e={0};PhaseRoleSnapshot r={0};
        if(!source_topology_entry(a->topology->api,a->active.dialogue.entry.identity.source,&e))ok=0;
        else {phase_role_read(a->binding,e.identity.source,phase_list_bind,&r);
            ok=activity_entry_same(&e,&a->active.dialogue.entry) && activity_role_same(&r,&a->active.dialogue.role) && GF2VR_ACTIVITY_UI_CURRENT(a);}
    }
#endif
    a->busy=0;return ok;
}

#ifdef GF2VR_GENERIC_SOURCE_ADAPTER
static int generic_source_active(ActivitySourceGroup *,void *,unsigned *);
#endif
#ifdef GF2VR_SKILL_SINGLE_SOURCE
/* Qualification comes from the complete preexisting world+UI route. It is
   evidence to compare, not a camera root or permission to revive old output.
   Only a newly rooted current input with unchanged full topology can use it. */
static void skill_source_qualify(ActivitySourceGroup *a,const SourceTopologyEntry *e,float units) {
    a->qualified=*e;a->qualified.identity.units_per_meter=units;a->qualified_valid=1;
}
static int skill_source_sole(ActivitySourceGroup *a,const SourceTopologyEntry *e,unsigned *index,float *units) {
    MainTestGrant *g=a->manual;SourceGeneration *s=a->source;
    SourceTopologyEntry current=*e;current.identity.units_per_meter=g->units_per_meter;
    if(!s || !a->qualified_valid || a->busy || !a->topology ||
       (g->state!=MTG_REQUESTED && g->state!=MTG_ACTIVE) || s->cancelled || a->topology->cancelled ||
       !(g->units_per_meter>0) || e->orthographic || e->display || e->has_target ||
       e->width<=0 || e->height<=0 || !((unsigned)e->mask&~0x20u) ||
       !activity_entry_same(&a->qualified,&current))return 0;
    a->busy=1;int ok=0;PhaseRoleSnapshot before={0},after={0};SourceTopologyEntry rechecked={0};
#ifdef GF2VR_GENERIC_SOURCE_ADAPTER
    unsigned require_active=rca_same(a->generic_unique,(RcaId){(unsigned long long)e->identity.source,e->identity.instance_id});
    unsigned active=0;if(require_active && (!generic_source_active(a,e->identity.source,&active) || !active))goto done;
#endif
    phase_role_read(a->binding,e->identity.source,phase_list_bind,&before);
    /* A standalone Base has no required referenced contributors. Nonempty
       stacks reject the whole route; the group owner must preserve every edge. */
    if(!before.attached || !before.complete || before.valid!=15 || before.render_type!=0 || before.output ||
       before.lists[0].count || before.lists[1].count)goto done;
    if(!source_topology_entry(a->topology->api,e->identity.source,&rechecked))goto done;
    rechecked.identity.units_per_meter=g->units_per_meter;
    phase_role_read(a->binding,e->identity.source,phase_list_bind,&after);
    if(!activity_entry_same(&current,&rechecked) || !activity_role_same(&before,&after) ||
       s->cancelled || a->topology->cancelled ||
       (g->state!=MTG_REQUESTED && g->state!=MTG_ACTIVE))goto done;
    /* Identity stays the same through UI-only disappearance. OFF/re-entry
       still goes through SourceGeneration adoption/new roots/real retirement. */
    #ifdef GF2VR_GENERIC_SOURCE_ADAPTER
    if(require_active && (!generic_source_active(a,e->identity.source,&active) || !active))goto done;
    #endif
    a->topology->group_stamp=0;*index=0;*units=g->units_per_meter;g->state=MTG_ACTIVE;ok=1;
done:
    a->busy=0;return ok;
}
#endif


#ifdef GF2VR_GENERIC_SOURCE_ADAPTER
/* Game adapter: only authoritative screen/world Base is a viewpoint candidate.
   Required perspective Overlays contribute without automatically being anchors.
   No standalone interpretation of camera count, names or instance order. */
static int generic_source_active(ActivitySourceGroup *a,void *camera,unsigned *active) {
    if(!a->active_attempted) {
        a->active_attempted=1;
        void *klass=render_class("UnityEngine.CoreModule.dll","UnityEngine","Behaviour");
        a->active_getter=eye_binding_method(klass,"get_isActiveAndEnabled",0,"System.Boolean",0,a->binding->refs.generic);
    }
    if(!a->active_getter)return 0;
    SourcePoseApi values={0};values.calls=a->binding->refs.settings.calls;unsigned char result=0;
    if(!source_pose_value(&values,a->active_getter,camera,a->binding->refs.settings.value_class[10],&result,1) || result>1)return 0;
    *active=result;return 1;
}
static int generic_source_recent(ActivitySourceGroup *a,const SourceTopologyEntry *entries,
    const unsigned *members,unsigned count,RcaPlan *out) {
    if(!count || count>8 || a->recent_clock==~0ULL)return 0;
    for(unsigned i=0;i<count;++i){const SourceTopologyEntry *e=&entries[members[i]];
        a->recent_input[i]=(RcaCamera){{(unsigned long long)e->identity.source,e->identity.instance_id},1,1,0,0};}
    /* Every member is preserved; the current Base is the only proved eligible
       viewport owner. Appearance of an Overlay must not steal the viewpoint. */
    *out=recent_camera_anchor_update_eligible(&a->recent,1,++a->recent_clock,a->recent_input,count,1,1u);
    for(unsigned i=0;i<8;++i)a->recent_input[i]=(RcaCamera){0};
    return out->status==RCA_PLAN && out->count==count &&
        out->anchor.object==(unsigned long long)entries[members[0]].identity.source;
}
static int generic_source_unique(ActivitySourceGroup *a,const SourceTopologyEntry *entries,
    unsigned count,unsigned intent,unsigned *index,float *units) {
    if(!a || !count || count>8 || a->busy || !a->source || !a->topology ||
       (intent!=MTG_REQUESTED && intent!=MTG_ACTIVE) || a->source->cancelled || a->topology->cancelled)return 0;
    MainTestGrant *g=a->manual;float checked;
    if(!(g->units_per_meter>0) || !eye_optics_float(g->units_per_meter,&checked))return 0;
    a->busy=1;int ok=0;unsigned base=8,bases=0;
    for(unsigned i=0;i<count;++i){
        const SourceTopologyEntry *e=&entries[i];phase_role_read(a->binding,e->identity.source,phase_list_bind,&a->input_roles[i]);
        const PhaseRoleSnapshot *r=&a->input_roles[i];
        if(!e->identity.source || !e->identity.instance_id || !e->identity.count || e->has_target || e->display ||
           e->width<=0 || e->height<=0 || e->width!=entries[0].width || e->height!=entries[0].height ||
           !r->attached || !r->complete || r->valid!=15 || r->output || r->lists[0].count || r->lists[1].count)goto done;
        for(unsigned j=0;j<i;++j)if(e->identity.source==entries[j].identity.source || e->identity.instance_id==entries[j].identity.instance_id)goto done;
        if(r->render_type==0) {
            unsigned active=0;
            if(e->orthographic || !((unsigned)e->mask&~0x20u) ||
               !generic_source_active(a,e->identity.source,&active) || !active)goto done;
            base=i;++bases;
        }
        else if(r->render_type!=2 && r->render_type!=3)goto done; /* Never omit an Overlay. */
    }
    if(bases!=1)goto done;
#ifdef GF2VR_SKILL_SINGLE_SOURCE
    /* A changed mask/extent/target on the same already qualified object cannot
       reuse a Camera.CopyFrom allocation made for the old configuration. */
    if(a->qualified_valid && a->qualified.identity.source==entries[base].identity.source) {
        SourceTopologyEntry q=entries[base];q.identity.units_per_meter=g->units_per_meter;
        if(!activity_entry_same(&a->qualified,&q))goto done;
    }
#endif
    for(unsigned i=0;i<count;++i){SourceTopologyEntry e={0};PhaseRoleSnapshot r={0};
        if(!source_topology_entry(a->topology->api,entries[i].identity.source,&e))goto done;
        phase_role_read(a->binding,e.identity.source,phase_list_bind,&r);
        if(!activity_entry_same(&entries[i],&e) || !activity_role_same(&a->input_roles[i],&r))goto done;}
    unsigned active=0;if(!generic_source_active(a,entries[base].identity.source,&active) || !active ||
        a->source->cancelled || a->topology->cancelled)goto done;
    RcaPlan plan={0};if(!generic_source_recent(a,entries,&base,1,&plan))goto done;
    a->topology->group_stamp=0;*index=base;*units=g->units_per_meter;
    main_test_revoke(g);g->identity=entries[base].identity;g->identity.units_per_meter=*units;g->state=MTG_ACTIVE;
#ifdef GF2VR_SKILL_SINGLE_SOURCE
    skill_source_qualify(a,&entries[base],*units);
#endif
    a->generic_unique=plan.anchor;ok=1;
done:
    AUI_ROUTE(a,entries,count,a->input_roles,2,ok,0,0);
    for(unsigned i=0;i<8;++i)a->input_roles[i]=(PhaseRoleSnapshot){0};
    a->busy=0;return ok;
}
#endif

static int activity_source_attribute(void *context,const SourceTopologyEntry *entries,
    unsigned count,unsigned *index,float *units) {
    ActivitySourceGroup *a=context;MainTestGrant *g=a?a->manual:0;
    if(!a || !g || a->busy || !a->binding || !a->binding->type_root ||
       !entries || !index || !units)return 0;
    unsigned intent=g->state;
    activity_snapshot_drop(a,&a->pending);
    /* Preserve the proved two-camera Main path. Larger inputs are typed first:
       seeded Main chronology must not select one member of a proved stack. */
#ifdef GF2VR_GENERIC_SOURCE_ADAPTER
    /* Supported nonstandard UI shapes bypass the old UI-mask cardinality
       assumption, but never bypass actual typed output ownership. */
    unsigned conventional_ui=0;
    for(unsigned i=0;i<count;++i)if((unsigned)entries[i].mask==0x20u)++conventional_ui;
    if(count==2 && conventional_ui!=1 && generic_source_unique(a,entries,count,intent,index,units)) {
        GF2VR_MAIN_POLICY_DETAIL(g,entries,count,0);return 1;
    }
#endif
    if(count==2 && main_test_attribute(g,entries,count,index,units)) {
        AUI_ROUTE(a,entries,count,0,1,1,0,0);
#ifdef GF2VR_SKILL_SINGLE_SOURCE
        if(*index<count)skill_source_qualify(a,&entries[*index],*units);
#endif
        return 1;
    }
#ifdef GF2VR_SKILL_SINGLE_SOURCE
    if(count==1) {
        int ok=skill_source_sole(a,&entries[0],index,units);
#ifdef GF2VR_GENERIC_SOURCE_ADAPTER
        /* Do not retry failed getters/required-stack validation on an already
           qualified current object. Generic cold/new objects are a new route. */
        if(!ok && (!a->qualified_valid || a->qualified.identity.source!=entries[0].identity.source))
            ok=generic_source_unique(a,entries,count,intent,index,units);
#endif
        if(!ok)main_test_revoke(g);
        AUI_ROUTE(a,entries,count,0,4,ok,0,0);
        GF2VR_MAIN_POLICY_RECORD(g,entries,count,ok?0:6);
        GF2VR_MAIN_POLICY_DETAIL(g,entries,count,ok?0:12);
        return ok;
    }
#endif
    if((intent!=MTG_REQUESTED && intent!=MTG_ACTIVE) || count<2 || count>8) {
        int accepted=main_test_attribute(g,entries,count,index,units);
#ifdef GF2VR_SKILL_SINGLE_SOURCE
        if(accepted && *index<count)skill_source_qualify(a,&entries[*index],*units);
#endif
        return accepted;
    }
    float checked;if(!(g->units_per_meter>0) || !eye_optics_float(g->units_per_meter,&checked))return 0;
    a->busy=1;int ok=0;ActivityStackPlan plan={0};
#ifdef GF2VR_ACTIVITY_UI_ROUTE_OBSERVE
    unsigned ui_capture=0; /* 0 not attempted,1 rejected,2 captured; telemetry only. */
#endif
    for(unsigned i=0;i<count;++i)phase_role_read(a->binding,entries[i].identity.source,
        phase_list_bind,&a->input_roles[i]);
    if(!activity_stack_plan(entries,a->input_roles,count,&plan)) {
#ifdef GF2VR_GENERIC_SOURCE_ADAPTER
        a->busy=0;
        if(generic_source_unique(a,entries,count,intent,index,units)) {
            GF2VR_MAIN_POLICY_DETAIL(g,entries,count,0);return 1;
        }
        a->busy=1;
        /* A failed unique-world check clears scratch; reload only the exact
           roles needed by the unchanged old stack/legacy fallback below. */
        for(unsigned i=0;i<count;++i)phase_role_read(a->binding,entries[i].identity.source,phase_list_bind,&a->input_roles[i]);
#endif
        unsigned stack=0;
        for(unsigned i=0;i<count;++i) {
            const PhaseRoleSnapshot *r=&a->input_roles[i];
            if(r->attached && r->complete && r->valid==15 &&
               (r->render_type==1 || (r->render_type==0 && (r->lists[0].count || r->lists[1].count))))stack=1;
        }
        for(unsigned i=0;i<8;++i)a->input_roles[i]=(PhaseRoleSnapshot){0};
        a->busy=0;
        if(stack){main_test_revoke(g);return 0;} /* No subset fallback. */
        if(count==2)return 0;
        int accepted=main_test_attribute(g,entries,count,index,units);
#ifdef GF2VR_SKILL_SINGLE_SOURCE
        if(accepted && *index<count)skill_source_qualify(a,&entries[*index],*units);
#endif
        return accepted;
    }
    if(a->topology->cancelled || a->source->cancelled)goto done;
    a->reading.count=plan.count;
    for(unsigned i=0;i<plan.count;++i) {
        unsigned n=plan.members[i];
        if(entries[n].orthographic)goto done; /* No guessed perspective conversion. */
        a->reading.entries[i]=entries[n];a->reading.roles[i]=a->input_roles[n];
        a->reading.roots[i]=a->binding->refs.settings.calls.pin(entries[n].identity.source,1);
        if(!a->reading.roots[i])goto done;
    }
#ifdef GF2VR_ACTIVITY_PRIMARY_UI
    if(plan.ui_count && plan.count<8) {
        /* The complete typed plan can include an additional CustomUI camera
           during dialogue. Choose only the unique already supported whole-UI
           contract, independently of total UI count or original array order.
           Ambiguity suppresses optional UI; the full world plan is unchanged. */
        unsigned u=8,matches=0;
        for(unsigned n=0;n<plan.ui_count;++n) {
            unsigned candidate=plan.ui[n];
            if(!entries[candidate].orthographic && entries[candidate].mask==0x20 &&
               a->input_roles[candidate].render_type==2 && a->input_roles[candidate].renderer==1) {
                u=candidate;++matches;
            }
        }
        if(matches==1)
#ifdef GF2VR_ACTIVITY_UI_ROUTE_OBSERVE
            ui_capture=GF2VR_ACTIVITY_UI_CAPTURE(a,&a->reading,&entries[u],&a->input_roles[u])?2u:1u;
#else
            (void)GF2VR_ACTIVITY_UI_CAPTURE(a,&a->reading,&entries[u],&a->input_roles[u]);
#endif
    }
#endif
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
    if(a->reading.primary.canvas && plan.count<=6) {
        unsigned u=8,matches=0;
        for(unsigned n=0;n<plan.ui_count;++n){unsigned candidate=plan.ui[n];
            if(entries[candidate].orthographic && entries[candidate].mask==0x400020 &&
               a->input_roles[candidate].render_type==2 && a->input_roles[candidate].renderer==1){u=candidate;++matches;}
        }
        if(matches==1)(void)GF2VR_ACTIVITY_DIALOGUE_CAPTURE(a,&a->reading,&entries[u],&a->input_roles[u]);
    }
#endif
#ifdef GF2VR_ACTIVITY_UI_DIALOGUE_OWNER_OBSERVE
    /* Read-only dialogue ownership snapshot. All original typed peers and
       topology are still rechecked after these additional diagnostic reads. */
    GF2VR_ACTIVITY_UI_DIALOGUE_OWNER(a,entries,count,a->input_roles);
#endif
    /* Recheck ALL typed roles after the last root/getter, then topology itself
       checks the original array and selected identity before returning. */
    for(unsigned i=0;i<count;++i) {
        PhaseRoleSnapshot r={0};phase_role_read(a->binding,entries[i].identity.source,phase_list_bind,&r);
        if(!activity_role_same(&r,&a->input_roles[i]))goto done;
    }
    if(a->topology->cancelled || a->source->cancelled)goto done;
    if(a->source->selection.eligible && !a->source->cancelled &&
       a->source->identity.units_per_meter==g->units_per_meter &&
#ifdef GF2VR_ACTIVITY_UI_LIFETIME
       activity_world_snapshot_same(&a->reading,&a->active)) {
        if(a->ui_change || !activity_ui_snapshot_same(&a->reading,&a->active)) {
            activity_snapshot_drop(a,&a->ui_pending);
            activity_ui_move(&a->ui_pending,&a->reading);a->ui_pending_ready=1;
            activity_ui_request_change(a);
        }
#else
       activity_snapshot_same(&a->reading,&a->active)) {
#endif
        a->topology->group_stamp=a->active.stamp;
    }else{
#ifdef GF2VR_ACTIVITY_UI_LIFETIME
        activity_snapshot_drop(a,&a->ui_pending);a->ui_change=a->ui_pending_ready=0;
#endif
        if(a->clock==~0ULL)goto done;
        a->reading.stamp=++a->clock;
        a->pending=a->reading;a->reading=(ActivitySourceSnapshot){0};
        a->topology->group_stamp=a->pending.stamp;
    }
#ifdef GF2VR_GENERIC_SOURCE_ADAPTER
    /* Existing accepted stack keeps its Base view and every Overlay. Recency
       observation is optional here if the new public getter is unsupported. */
    RcaPlan recent={0};(void)generic_source_recent(a,entries,plan.members,plan.count,&recent);
#endif
    *index=plan.base;*units=g->units_per_meter;main_test_revoke(g);g->state=MTG_ACTIVE;ok=1;
done:
    AUI_ROUTE(a,entries,count,a->input_roles,3,ok,ui_capture,plan.reason);
    activity_snapshot_drop(a,&a->reading);
    for(unsigned i=0;i<8;++i)a->input_roles[i]=(PhaseRoleSnapshot){0};
    a->busy=0;if(!ok)activity_snapshot_drop(a,&a->pending);return ok;
}
#endif
