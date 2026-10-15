#ifndef GF2_MAIN_MULTICAMERA_OBSERVE_H
#define GF2_MAIN_MULTICAMERA_OBSERVE_H
/* Main owner only. Every object identity is compare/log-only. The rooted
   topology reader already copied these values; no getters, pins or selection
   changes occur here. A budget limits telemetry only, never production. */
typedef struct {
    unsigned count,state,world_count,reason;
    SourceGenerationCandidate anchor,ui;
    SourceTopologyEntry entries[32];
} MulticameraPolicyNote;
static MulticameraPolicyNote multicamera_previous_policy;
static unsigned multicamera_policy_valid,multicamera_policy_records;
static void multicamera_copy(void *to,const void *from,unsigned n) {
    /* No CRT dependency; all calls use distinct owner/snapshot storage. */
    for(unsigned i=0;i<n;++i)
        ((volatile unsigned char *)to)[i]=((const volatile unsigned char *)from)[i];
}
static int multicamera_same_bytes(const void *a,const void *b,unsigned n) {
    for(unsigned i=0;i<n;++i)
        if(((const unsigned char *)a)[i]!=((const unsigned char *)b)[i])return 0;
    return 1;
}
static int multicamera_same_policy(const MulticameraPolicyNote *a,const MulticameraPolicyNote *b) {
    if(a->count!=b->count || a->state!=b->state || a->world_count!=b->world_count ||
       a->reason!=b->reason || !source_generation_same(&a->anchor,&b->anchor) ||
       !source_generation_same(&a->ui,&b->ui))return 0;
    for(unsigned i=0;i<a->count;++i) {
        const SourceTopologyEntry *x=&a->entries[i],*y=&b->entries[i];
        if(!source_generation_same(&x->identity,&y->identity) || x->orthographic!=y->orthographic ||
           x->display!=y->display || x->width!=y->width || x->height!=y->height ||
           x->mask!=y->mask || x->has_target!=y->has_target)return 0;
    }
    return 1;
}
static void multicamera_policy_record(void *opaque,const SourceTopologyEntry *entries,
                                      unsigned count,unsigned reason) {
    MainTestGrant *g=opaque;
    if(!g || !entries || count>32 || multicamera_policy_records>=128)return;
    /* Address-stable owner storage avoids a large hook stack allocation. */
    static MulticameraPolicyNote note;
    note.count=count;note.state=g->state;note.world_count=g->world_count;note.reason=reason;
    multicamera_copy(&note.anchor,&g->identity,sizeof(note.anchor));
    multicamera_copy(&note.ui,&g->ui_identity,sizeof(note.ui));
    for(unsigned i=0;i<count;++i)multicamera_copy(&note.entries[i],&entries[i],sizeof(entries[i]));
    if(multicamera_policy_valid && multicamera_same_policy(&note,&multicamera_previous_policy))return;
    multicamera_copy(&multicamera_previous_policy,&note,sizeof(note));multicamera_policy_valid=1;
    unsigned observation=++multicamera_policy_records;
    DWORD saved=GetLastError();char line[1800],*p=source_prefix(line,"multicamera_policy");
#define MC_NUM(name,value) do{p=literal(p,",\"" name "\":");p=decimal(p,(value));}while(0)
#define MC_HEX(name,value) do{p=literal(p,",\"" name "\":");p=hexadecimal(p,(U64)(value));}while(0)
    MC_NUM("observation",observation);MC_NUM("reason",reason);MC_NUM("count",count);
    MC_NUM("state",g->state);MC_NUM("known_worlds",g->world_count);
    MC_HEX("anchor_id",(unsigned)g->identity.instance_id);MC_HEX("ui_id",(unsigned)g->ui_identity.instance_id);
    p=literal(p,"}\n");source_log(line,p);
    for(unsigned i=0;i<count;++i) {
        const SourceTopologyEntry *e=&entries[i];p=source_prefix(line,"multicamera_camera");
        MC_NUM("observation",observation);MC_NUM("index",i);
        MC_HEX("object",e->identity.source);MC_HEX("id",(unsigned)e->identity.instance_id);
        MC_NUM("orthographic",e->orthographic);MC_NUM("display",e->display);
        MC_NUM("has_target",e->has_target);MC_NUM("width",e->width);MC_NUM("height",e->height);
        MC_HEX("mask",(unsigned)e->mask);MC_NUM("ancestry_count",e->identity.count);
        p=literal(p,"}\n");source_log(line,p);
        for(unsigned j=0;j<e->identity.count && j<8;++j) {
            p=source_prefix(line,"multicamera_ancestry");
            MC_NUM("observation",observation);MC_NUM("index",i);MC_NUM("depth",j);
            MC_HEX("object",e->identity.owners[j]);MC_HEX("id",(unsigned)e->identity.owner_ids[j]);
            p=literal(p,"}\n");source_log(line,p);
        }
    }
#ifdef GF2VR_ACTIVITY_STACK_OBSERVE
    /* The current topology owns every source root through this read-only call.
       Its telemetry never feeds the grant or changes retirement/admission. */
    if(reason && count>=3)activity_stack_observe(entries,count,observation);
#endif
    SetLastError(saved);
#undef MC_NUM
#undef MC_HEX
}
static void multicamera_source_record(const SourceGeneration *g,const MainTestGrant *manual,
                                      U64 serial,int admitted) {
    static U64 previous_generation;static unsigned previous_stage,previous_state,previous_result,valid,records;
    if(!g || !manual || records>=256)return;
    U64 generation=g->selection.eligible?g->selection.generation:0;
    unsigned stage=admitted?0:multicamera_admission_stage;
    if(valid && previous_generation==generation && previous_stage==stage &&
       previous_state==manual->state && previous_result==(unsigned)(admitted!=0))return;
    valid=1;previous_generation=generation;previous_stage=stage;
    previous_state=manual->state;previous_result=admitted!=0;++records;
    DWORD saved=GetLastError();char line[1200],*p=source_prefix(line,"multicamera_source");
#define MC_NUM(name,value) do{p=literal(p,",\"" name "\":");p=decimal(p,(value));}while(0)
#define MC_HEX(name,value) do{p=literal(p,",\"" name "\":");p=hexadecimal(p,(U64)(value));}while(0)
    MC_HEX("serial",serial);MC_HEX("generation",generation);
    MC_NUM("admitted",admitted!=0);MC_NUM("stage",stage);MC_NUM("state",manual->state);
    MC_NUM("replacement_pending",g->replacement_pending);MC_NUM("policy_observation",multicamera_policy_records);
    p=literal(p,"}\n");source_log(line,p);SetLastError(saved);
#undef MC_NUM
#undef MC_HEX
}
#ifdef BOUNDARY_TEST
#include "main_multicamera_observe_tests.h"
#endif
#endif
