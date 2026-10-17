#ifndef GF2_ACTIVITY_UI_ROUTE_OBSERVE_H
#define GF2_ACTIVITY_UI_ROUTE_OBSERVE_H
/* Main owner only. Copy existing rooted input/typed snapshots; identities are
   compare/log-only. No getter, pin, Canvas call or production return value. */
typedef struct {
    unsigned route,accepted,count,capture,reason,eligible,active_count,pending_count,reading_count;
    U64 generation,group_stamp,active_stamp,pending_stamp,active_canvas,pending_canvas,reading_canvas;
    SourceTopologyEntry entries[8];PhaseRoleSnapshot roles[8];unsigned roles_present;
} ActivityUiRouteNote;
/* One invocation can report generic2 then sole4 (or generic2 then stack3).
   Each route remembers its own last semantic state; alternating unchanged
   branches must not exhaust the shared64-record budget before Activity. */
static ActivityUiRouteNote aui_route_previous[4];
static unsigned aui_route_valid,aui_route_records,aui_route_busy;
static int aui_route_role_same(const PhaseRoleSnapshot *a,const PhaseRoleSnapshot *b) {
    if(a->attached!=b->attached || a->complete!=b->complete || a->valid!=b->valid ||
       a->render_type!=b->render_type || a->clear_depth!=b->clear_depth ||
       a->renderer!=b->renderer || a->output!=b->output)return 0;
    for(unsigned l=0;l<2;++l) {
        if(a->lists[l].present!=b->lists[l].present || a->lists[l].complete!=b->lists[l].complete ||
           a->lists[l].count!=b->lists[l].count)return 0;
        /* Failed typed reads can contain an oversized reported count. */
        for(unsigned i=0;i<a->lists[l].count && i<8;++i)
            if(a->lists[l].cameras[i]!=b->lists[l].cameras[i])return 0;
    }return 1;
}
static void activity_ui_route_record(ActivitySourceGroup *a,const SourceTopologyEntry *entries,
    unsigned count,const PhaseRoleSnapshot *roles,unsigned route,int accepted,unsigned capture,unsigned reason) {
    if(!a || !a->source || a->source->owner_thread!=GetCurrentThreadId() ||
       !entries || !count || count>8 || route<1 || route>4 || aui_route_busy || aui_route_records>=64)return;
    aui_route_busy=1;DWORD saved=GetLastError();
    /* One guarded main owner; avoid large hook-stack allocations / CRT probes. */
    static ActivityUiRouteNote note;volatile unsigned char *clear=(void *)&note;
    for(unsigned i=0;i<sizeof(note);++i)clear[i]=0;
    note.route=route;note.accepted=accepted!=0;note.count=count;note.capture=capture;note.reason=reason;
    note.eligible=a->source->selection.eligible;note.generation=a->source->selection.generation;
    note.group_stamp=a->topology?a->topology->group_stamp:0;
    note.active_count=a->active.count;note.pending_count=a->pending.count;note.reading_count=a->reading.count;
    note.active_stamp=a->active.stamp;note.pending_stamp=a->pending.stamp;
    note.active_canvas=(U64)a->active.primary.canvas;note.pending_canvas=(U64)a->pending.primary.canvas;
    note.reading_canvas=(U64)a->reading.primary.canvas;note.roles_present=roles!=0;
    /* Copy snapshots, never retain a managed owner or inspect native memory.
       Deduplicate semantic fields rather than unspecified structure padding. */
    for(unsigned i=0;i<count;++i) {
        note.entries[i]=entries[i];if(roles)note.roles[i]=roles[i];
    }
    unsigned index=route-1,bit=1u<<index;
    const ActivityUiRouteNote *old=&aui_route_previous[index];
    int same=(aui_route_valid&bit) && old->route==note.route && old->accepted==note.accepted &&
        old->count==note.count && old->capture==note.capture && old->reason==note.reason &&
        old->eligible==note.eligible && old->generation==note.generation && old->group_stamp==note.group_stamp &&
        old->active_count==note.active_count && old->pending_count==note.pending_count && old->reading_count==note.reading_count &&
        old->active_stamp==note.active_stamp && old->pending_stamp==note.pending_stamp &&
        old->active_canvas==note.active_canvas && old->pending_canvas==note.pending_canvas &&
        old->reading_canvas==note.reading_canvas && old->roles_present==note.roles_present;
    for(unsigned i=0;same && i<count;++i)
        if(!activity_entry_same(&note.entries[i],&old->entries[i]) ||
           (roles && !aui_route_role_same(&note.roles[i],&old->roles[i])))same=0;
    if(same){SetLastError(saved);aui_route_busy=0;return;}
    aui_route_previous[index]=note;aui_route_valid|=bit;unsigned record=++aui_route_records;
    static char line[2300];char *p=source_prefix(line,"activity_ui_route");
#define AUI_NUM(name,value) do{p=literal(p,",\"" name "\":");p=decimal(p,(value));}while(0)
#define AUI_HEX(name,value) do{p=literal(p,",\"" name "\":");p=hexadecimal(p,(U64)(value));}while(0)
    AUI_NUM("record",record);AUI_NUM("route",route);AUI_NUM("accepted",accepted!=0);
    AUI_NUM("count",count);AUI_NUM("roles_present",roles!=0);AUI_NUM("capture",capture);AUI_NUM("plan_reason",reason);
    AUI_HEX("before_generation",note.generation);AUI_NUM("before_eligible",note.eligible);
    AUI_HEX("group_stamp",note.group_stamp);AUI_NUM("active_count",note.active_count);
    AUI_NUM("pending_count",note.pending_count);AUI_NUM("reading_count",note.reading_count);
    AUI_HEX("active_stamp",note.active_stamp);AUI_HEX("pending_stamp",note.pending_stamp);
    AUI_HEX("active_canvas",note.active_canvas);AUI_HEX("pending_canvas",note.pending_canvas);AUI_HEX("reading_canvas",note.reading_canvas);
    p=literal(p,"}\n");source_log(line,p);
    for(unsigned i=0;i<count;++i) {
        const SourceTopologyEntry *e=&note.entries[i];const PhaseRoleSnapshot *r=&note.roles[i];
        p=source_prefix(line,"activity_ui_route_camera");AUI_NUM("record",record);AUI_NUM("index",i);
        AUI_HEX("camera",e->identity.source);AUI_HEX("id",(unsigned)e->identity.instance_id);
        AUI_NUM("orthographic",e->orthographic);AUI_HEX("mask",(unsigned)e->mask);
        AUI_NUM("width",e->width);AUI_NUM("height",e->height);AUI_NUM("display",e->display);AUI_NUM("has_target",e->has_target);
        AUI_NUM("roles_present",note.roles_present);AUI_NUM("attached",r->attached);AUI_NUM("complete",r->complete);
        AUI_NUM("valid",r->valid);AUI_NUM("render_type",r->render_type);AUI_NUM("renderer",r->renderer);AUI_NUM("output",r->output);
        for(unsigned l=0;l<2;++l) {
            p=literal(p,",\"list");p=decimal(p,l);p=literal(p,"\":{\"present\":");p=decimal(p,r->lists[l].present);
            p=literal(p,",\"complete\":");p=decimal(p,r->lists[l].complete);p=literal(p,",\"count\":");p=decimal(p,r->lists[l].count);
            p=literal(p,",\"cameras\":[");
            for(unsigned j=0;j<r->lists[l].count && j<8;++j){if(j)p=literal(p,",");p=hexadecimal(p,r->lists[l].cameras[j]);}
            p=literal(p,"]}");
        }
        p=literal(p,"}\n");source_log(line,p);
    }
#undef AUI_NUM
#undef AUI_HEX
    SetLastError(saved);aui_route_busy=0;
}
#endif
