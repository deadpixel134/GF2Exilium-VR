#ifndef GF2_ACTIVITY_STACK_PLAN_H
#define GF2_ACTIVITY_STACK_PLAN_H
/* Complete current-input contract, not an immersive admission grant. Numeric
   camera identities are correlated only while the topology owns every root.
   The Base is the rendering owner, not an invented "newest" viewpoint anchor.
   Unsupported/incomplete peers reject the whole plan; never omit a camera. */
enum { ASP_OK,ASP_INPUT,ASP_CONFIG,ASP_ROLE,ASP_BASE,ASP_LIST,ASP_EDGE,ASP_UNREFERENCED };
typedef struct {
    unsigned reason,base,count,members[8],ui_count,ui[8];
} ActivityStackPlan;
static int activity_stack_plan(const SourceTopologyEntry *entries,const PhaseRoleSnapshot *roles,
    unsigned count,ActivityStackPlan *out) {
    if(!out)return 0;
    *out=(ActivityStackPlan){0};out->reason=ASP_INPUT;out->base=8;
    if(!entries || !roles || !count || count>8)return 0;
    unsigned bases=0,overlays=0;
    for(unsigned i=0;i<count;++i) {
        const SourceTopologyEntry *e=&entries[i];const PhaseRoleSnapshot *r=&roles[i];
        out->reason=ASP_CONFIG;
        if(!e->identity.source || !e->identity.instance_id || !e->identity.count ||
           e->has_target || e->display || e->width<=0 || e->height<=0 ||
           e->width!=entries[0].width || e->height!=entries[0].height)return 0;
        for(unsigned j=0;j<i;++j)if(e->identity.source==entries[j].identity.source ||
            e->identity.instance_id==entries[j].identity.instance_id)return 0;
        out->reason=ASP_ROLE;
        if(!r->attached || !r->complete || r->valid!=15 || r->output)return 0;
        if(r->render_type==0){++bases;out->base=i;}
        else if(r->render_type==1)++overlays;
        else if(r->render_type==2 || r->render_type==3)out->ui[out->ui_count++]=i;
        else return 0;
        out->reason=ASP_LIST;
        for(unsigned l=0;l<2;++l)if(!r->lists[l].complete || r->lists[l].count>7 ||
            (!r->lists[l].present && r->lists[l].count) || (r->render_type!=0 && r->lists[l].count))return 0;
    }
    out->reason=ASP_BASE;if(bases!=1 || !overlays)return 0;
    out->members[0]=out->base;out->count=1;
    const PhaseRoleSnapshot *b=&roles[out->base];
    out->reason=ASP_LIST;
    if(!b->lists[0].present || !b->lists[1].present ||
       b->lists[0].count!=overlays || b->lists[1].count!=overlays)return 0;
    unsigned seen=1u<<out->base;
    out->reason=ASP_EDGE;
    for(unsigned n=0;n<overlays;++n) {
        unsigned long long object=b->lists[0].cameras[n];
        if(!object || object!=b->lists[1].cameras[n])return 0;
        unsigned i=0;for(;i<count;++i)if(object==(unsigned long long)entries[i].identity.source)break;
        if(i==count || roles[i].render_type!=1 || (seen&(1u<<i)))return 0;
        seen|=1u<<i;out->members[out->count++]=i;
    }
    out->reason=ASP_UNREFERENCED;
    for(unsigned i=0;i<count;++i)if(roles[i].render_type==1 && !(seen&(1u<<i)))return 0;
    out->reason=ASP_OK;return 1;
}
#endif
