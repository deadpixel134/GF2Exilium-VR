#ifndef GF2_MAIN_TEST_GRANT_H
#define GF2_MAIN_TEST_GRANT_H
#include "source_topology.h"
#ifndef GF2VR_MAIN_POLICY_RECORD
#define GF2VR_MAIN_POLICY_RECORD(g,e,n,r) ((void)(r))
#endif
#ifndef GF2VR_MAIN_HANDOFF_RECORD
#define GF2VR_MAIN_HANDOFF_RECORD(old_id,new_id) ((void)0)
#endif
#ifndef GF2VR_MAIN_POLICY_DETAIL
#define GF2VR_MAIN_POLICY_DETAIL(g,e,n,r) ((void)0)
#define GF2VR_MAIN_POLICY_STEP(r) ((void)0)
#endif
/* TD-MAIN-TEST-001 ONLY. User asserts current-main scope explicitly; these
   numeric guards identify an unambiguous candidate within that scope, never
   infer which scene is main. No character/name or historical pointer selector. */
enum { MTG_OFF,MTG_REQUESTED,MTG_ACTIVE };
typedef struct {
    unsigned state,key_released;
    float units_per_meter; /* explicit trial calibration; not measured scale */
    SourceGenerationCandidate identity; /* compare only, no dereference */
    /* Opt-in TD-MAIN-PHASE-001 trial. Snapshot identities are compare-only. */
    unsigned handoff_enabled,world_count;
    unsigned flat_tracking,tracking_focused;
    SourceGenerationCandidate worlds[31],ui_identity;
} MainTestGrant;
static void main_test_revoke_impl(MainTestGrant *g) {
    g->state=MTG_OFF;g->identity=(SourceGenerationCandidate){0};
    g->world_count=0;g->ui_identity=(SourceGenerationCandidate){0};
}
#ifndef GF2VR_MAIN_REVOKE_TRACE
#define GF2VR_MAIN_REVOKE_TRACE(g,line) ((void)0)
#endif
#define main_test_revoke(g) (GF2VR_MAIN_REVOKE_TRACE((g),__LINE__),main_test_revoke_impl(g))
/* Rising edge only after a release in the focused game. Holding F8 across
   startup/focus changes cannot activate. Revoke does not reset the key latch. */
static void main_test_key(MainTestGrant *g,int focused,int down) {
    g->tracking_focused=focused!=0;
    if(!focused) {main_test_revoke(g);g->key_released=0;return;}
    if(!down) {g->key_released=1;return;}
    if(!g->key_released) return;
    g->key_released=0;
    if(g->state!=MTG_OFF) {
        if(g->flat_tracking)g->state=MTG_OFF; /* tracking is not a grant */
        else main_test_revoke(g);
    }
    else g->state=MTG_REQUESTED;
}
/* A newcomer means newly observed in this active sequence, never highest
   instance ID or last array slot. No historical pointer is dereferenced. */
static int main_test_handoff(MainTestGrant *g,const SourceTopologyEntry *entries,
    unsigned count,unsigned *index,float *units) {
    unsigned worlds=0,ui=0,ui_index=0,selected=32,newcomers=0,new_index=32;
    unsigned world_indices[31];float checked;
#ifdef GF2VR_MULTICAMERA_OBSERVE
    unsigned detail=1;
#endif
    unsigned seeded=g->flat_tracking ? g->world_count!=0 : g->state!=MTG_REQUESTED;
    if(count<2 || count>32 || !(g->units_per_meter>0) ||
       !eye_optics_float(g->units_per_meter,&checked))goto reject;
    for(unsigned i=0;i<count;++i){
        const SourceTopologyEntry *e=&entries[i];
        GF2VR_MAIN_POLICY_STEP(2);
        if(e->has_target || e->display || e->width<=0 || e->height<=0 ||
           e->width!=entries[0].width || e->height!=entries[0].height ||
           !e->identity.source || !e->identity.instance_id || !e->identity.count ||
           e->identity.count>8)goto reject;
        for(unsigned j=0;j<i;++j)
            if(e->identity.source==entries[j].identity.source ||
               e->identity.instance_id==entries[j].identity.instance_id){GF2VR_MAIN_POLICY_STEP(3);goto reject;}
        if((unsigned)e->mask==0x20u){++ui;ui_index=i;}
        else if(!e->orthographic && ((unsigned)e->mask&~0x20u)){
            if(worlds==31)goto reject;
            world_indices[worlds++]=i;
        }else {GF2VR_MAIN_POLICY_STEP(4);goto reject;}
    }
    GF2VR_MAIN_POLICY_STEP(5);
    if(ui!=1 || !worlds)goto reject;
    SourceGenerationCandidate ui_now=entries[ui_index].identity;
    ui_now.units_per_meter=g->units_per_meter;
    if(!seeded){
        GF2VR_MAIN_POLICY_STEP(6);
        if(worlds!=1)goto reject; /* No observed chronology at initial entry. */
        selected=world_indices[0];
    }else{
        GF2VR_MAIN_POLICY_STEP(7);
        if(!g->world_count || g->world_count>31 ||
           !source_generation_same(&g->ui_identity,&ui_now))goto reject;
        for(unsigned i=0;i<worlds;++i){
            unsigned n=world_indices[i];SourceGenerationCandidate c=entries[n].identity;
            c.units_per_meter=g->units_per_meter;
            unsigned known=0;
            for(unsigned j=0;j<g->world_count;++j){
                if(source_generation_same(&g->worlds[j],&c)){known=1;break;}
                /* Same object changing ancestry is not a new camera. */
                if(g->worlds[j].source==c.source || g->worlds[j].instance_id==c.instance_id){GF2VR_MAIN_POLICY_STEP(8);goto reject;}
            }
            if(!known){++newcomers;new_index=n;}
            if(source_generation_same(&g->identity,&c))selected=n;
        }
        GF2VR_MAIN_POLICY_STEP(9);
        if(newcomers>1)goto reject;
        if(newcomers==1)selected=new_index;
        GF2VR_MAIN_POLICY_STEP(10);
        if(selected==32)goto reject; /* Do not fall back to a retained old world. */
    }
    if(!seeded)g->world_count=0;
    if(!seeded || newcomers==1){
        GF2VR_MAIN_POLICY_STEP(11);
        if(g->world_count==31)goto reject;
        g->worlds[g->world_count]=entries[selected].identity;
        g->worlds[g->world_count++].units_per_meter=g->units_per_meter;
    }
    g->ui_identity=ui_now;
    if(seeded && newcomers==1)
        GF2VR_MAIN_HANDOFF_RECORD(g->identity.instance_id,entries[selected].identity.instance_id);
    g->identity=entries[selected].identity;g->identity.units_per_meter=g->units_per_meter;
    GF2VR_MAIN_POLICY_RECORD(g,entries,count,0);
    GF2VR_MAIN_POLICY_DETAIL(g,entries,count,0);
    if(g->state!=MTG_OFF)g->state=MTG_ACTIVE;
    *index=selected;*units=g->units_per_meter;return 1;
reject:
    GF2VR_MAIN_POLICY_RECORD(g,entries,count,5);
    GF2VR_MAIN_POLICY_DETAIL(g,entries,count,detail);
    main_test_revoke(g);return 0;
}
/* Used only by the rooted read-only topology path. Its output must never be
   fed to source_generation_update: a successful observation is NOT a grant. */
static int main_test_track(void *context,const SourceTopologyEntry *entries,
    unsigned count,unsigned *index,float *units) {
    MainTestGrant *g=context;
    if(!g || !g->flat_tracking || !g->handoff_enabled || !g->tracking_focused ||
       g->state!=MTG_OFF || !entries || !index || !units)return 0;
    return main_test_handoff(g,entries,count,index,units);
}
static int main_test_attribute(void *context,const SourceTopologyEntry *entries,
    unsigned count,unsigned *index,float *units) {
    MainTestGrant *g=context;float checked;
    if(!g || !entries || !index || !units) return 0;
    if(g->state!=MTG_REQUESTED && g->state!=MTG_ACTIVE) return 0;
    if(g->handoff_enabled)return main_test_handoff(g,entries,count,index,units);
    /* Observed main input is one world plus independent layer-5 UI, both
       screen/display0. Require that complete shape for this narrow trial. */
    unsigned reason=1;
    if(count!=2 || !(g->units_per_meter>0) || !eye_optics_float(g->units_per_meter,&checked)) goto reject;
    unsigned worlds=0,ui=0,selected=0;
    reason=2;
    for(unsigned i=0;i<count;++i) {
        const SourceTopologyEntry *e=&entries[i];
        if(e->has_target || e->display!=0 || e->width<=0 || e->height<=0 ||
           !e->identity.source || !e->identity.instance_id || !e->identity.count) goto reject;
        if((unsigned)e->mask==0x20u) ++ui;
        else if(!e->orthographic && ((unsigned)e->mask&~0x20u)) {++worlds;selected=i;}
        else goto reject;
    }
    reason=3;
    if(worlds!=1 || ui!=1 || entries[0].width!=entries[1].width || entries[0].height!=entries[1].height) goto reject;
    SourceGenerationCandidate current=entries[selected].identity;
    current.units_per_meter=g->units_per_meter;
    reason=4;
    if(g->state==MTG_ACTIVE && !source_generation_same(&g->identity,&current)) goto reject;
    GF2VR_MAIN_POLICY_RECORD(g,entries,count,0);
    g->identity=current;g->state=MTG_ACTIVE;*index=selected;*units=g->units_per_meter;return 1;
reject:
    GF2VR_MAIN_POLICY_RECORD(g,entries,count,reason);
    main_test_revoke(g);return 0;
}
#endif
