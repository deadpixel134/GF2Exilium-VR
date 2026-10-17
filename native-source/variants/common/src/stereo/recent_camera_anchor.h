#ifndef GF2_RECENT_CAMERA_ANCHOR_H
#define GF2_RECENT_CAMERA_ANCHOR_H
/* Pure owner-thread policy; no managed pointers, getters, renders or XR state.
   Caller proves output coverage and supplies actual activation order when known.
   A Camera[] index is NOT activation_order. This is not a projection grant. */
#define RCA_CAPACITY 32u
typedef struct {unsigned long long object;int instance;} RcaId;
typedef struct {
    RcaId id;
    unsigned world_candidate; /* already excludes UI/offscreen/unsupported */
    unsigned contributes; /* current output evidence, not mask membership */
    unsigned activated; /* semantic activation/cut, NOT continuous pose motion */
    unsigned activation_order; /* trusted event order within THIS observation */
} RcaCamera;
typedef struct {
    RcaId id;
    unsigned present;
    unsigned long long observed;
    unsigned order;
} RcaHistory;
typedef struct {
    unsigned long long epoch,frame;
    RcaHistory history[RCA_CAPACITY];
    unsigned count;
    RcaId anchor;
} RecentCameraAnchor;
enum {RCA_INVALID,RCA_NEEDS_OUTPUT_PROOF,RCA_NO_WORLD,RCA_AMBIGUOUS,RCA_PLAN,RCA_NO_ANCHOR};
typedef struct {
    unsigned status,count,anchor_changed;
    RcaId anchor,contributors[RCA_CAPACITY];
} RcaPlan;
static int rca_same(RcaId a,RcaId b) {return a.object==b.object && a.instance==b.instance;}
static int rca_valid(RcaId a) {return a.object && a.instance;}
static RcaPlan recent_camera_anchor_update_eligible(RecentCameraAnchor *s,
    unsigned long long epoch,unsigned long long frame,const RcaCamera *c,
    unsigned count,unsigned complete_output_evidence,unsigned eligible_mask) {
    RcaPlan out={0};
    if(!s || !epoch || !frame || count>RCA_CAPACITY || (count && !c) || complete_output_evidence>1)return out;
    if(count<32 && (eligible_mask>>count))return out;
    if(s->epoch==epoch && frame<=s->frame)return out;
    for(unsigned i=0;i<count;++i) {
        if(!rca_valid(c[i].id) || c[i].world_candidate>1 || c[i].contributes>1 ||
           c[i].activated>1 || (c[i].contributes && !c[i].world_candidate) ||
           (c[i].activation_order && !c[i].activated))return out;
        if((eligible_mask&(1u<<i)) && (!c[i].world_candidate || !c[i].contributes))return out;
        for(unsigned j=0;j<i;++j)
            if(c[i].id.object==c[j].id.object || c[i].id.instance==c[j].id.instance)return out;
    }
    RecentCameraAnchor next=*s;
    if(next.epoch!=epoch)next=(RecentCameraAnchor){0};
    next.epoch=epoch;next.frame=frame;
    unsigned was_present[RCA_CAPACITY]={0};
    for(unsigned j=0;j<next.count;++j){was_present[j]=next.history[j].present;next.history[j].present=0;}
    unsigned long long newest=0;unsigned newest_order=0,ties=0;
    RcaId chosen={0};
    for(unsigned i=0;i<count;++i) {
        if(!c[i].world_candidate)continue;
        unsigned j=0;
        for(;j<next.count;++j)if(rca_same(next.history[j].id,c[i].id))break;
        if(j==next.count) {
            if(next.count==RCA_CAPACITY) {
                /* Absent identities can be discarded: returning after absence
                   is a new observed activation anyway. Protect all CURRENT
                   members, including ones not visited yet in this array. */
                for(j=0;j<next.count;++j) {
                    unsigned current=0;
                    for(unsigned k=0;k<count;++k)
                        if(c[k].world_candidate && rca_same(next.history[j].id,c[k].id)){current=1;break;}
                    if(!current)break;
                }
                if(j==next.count)return out;
                was_present[j]=0;
            }else ++next.count;
            next.history[j]=(RcaHistory){c[i].id,0,0,0};
        }
        RcaHistory *h=&next.history[j];
        if(!was_present[j] || c[i].activated){h->observed=frame;h->order=c[i].activation_order;}
        h->present=1;
        if(!c[i].contributes)continue;
        out.contributors[out.count++]=c[i].id;
        if(!(eligible_mask&(1u<<i)))continue; /* Contribution is not viewpoint eligibility. */
        if(h->observed>newest || (h->observed==newest && h->order>newest_order)) {
            newest=h->observed;newest_order=h->order;chosen=h->id;ties=1;
        }else if(h->observed==newest && h->order==newest_order)++ties;
    }
    out.status=RCA_NEEDS_OUTPUT_PROOF;
    if(complete_output_evidence) {
        if(!out.count)out.status=RCA_NO_WORLD;
        else if(!ties)out.status=RCA_NO_ANCHOR;
        else if(ties!=1)out.status=RCA_AMBIGUOUS;
        else {
            out.status=RCA_PLAN;out.anchor=chosen;
            out.anchor_changed=!rca_same(next.anchor,chosen);next.anchor=chosen;
        }
    }
    if(out.status!=RCA_PLAN) {
        next.anchor=(RcaId){0};out.anchor=(RcaId){0};out.count=0;
    }
    *s=next;return out;
}
/* Existing callers retain their original all-contributors eligible policy. */
static RcaPlan recent_camera_anchor_update(RecentCameraAnchor *s,unsigned long long epoch,
    unsigned long long frame,const RcaCamera *c,unsigned count,unsigned proof) {
    unsigned mask=0;
    if(c && count<=32)for(unsigned i=0;i<count;++i)
        if(c[i].world_candidate && c[i].contributes)mask|=1u<<i;
    return recent_camera_anchor_update_eligible(s,epoch,frame,c,count,proof,mask);
}
#endif
