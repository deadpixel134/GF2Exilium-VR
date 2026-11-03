#ifndef GF2_HOME_SOURCE_POLICY_H
#define GF2_HOME_SOURCE_POLICY_H
/* The current game's Home helper supplies the viewpoint identity. This policy
   never selects by array order, renderer number, instance ID or ancestry name.
   Caller owns every current topology root and validates the helper twice. */
static int home_source_choose(const SourceTopologyEntry *entries,
    const PhaseRoleSnapshot *roles,unsigned count,void *camera,unsigned *index) {
    if(!entries || !roles || !camera || !index || count<2 || count>8)return 0;
    unsigned worlds=0,matches=0,selected=8;
    for(unsigned i=0;i<count;++i) {
        const SourceTopologyEntry *e=&entries[i];const PhaseRoleSnapshot *r=&roles[i];
        if(!e->identity.source || !e->identity.instance_id || !e->identity.count ||
           e->has_target || e->display || e->width<=0 || e->height<=0 ||
           e->width!=entries[0].width || e->height!=entries[0].height ||
           !r->attached || !r->complete || r->valid!=15 || r->output ||
           !r->lists[0].complete || !r->lists[1].complete ||
           r->lists[0].count || r->lists[1].count)return 0;
        for(unsigned j=0;j<i;++j)if(e->identity.source==entries[j].identity.source ||
            e->identity.instance_id==entries[j].identity.instance_id)return 0;
        if(r->render_type==0) {
            if(e->orthographic || !((unsigned)e->mask&~0x20u))return 0;
            ++worlds;
            if(e->identity.source==camera){selected=i;++matches;}
        }else if(r->render_type!=2 && r->render_type!=3)return 0;
    }
    if(worlds<2 || matches!=1)return 0;
    *index=selected;return 1;
}
#endif
