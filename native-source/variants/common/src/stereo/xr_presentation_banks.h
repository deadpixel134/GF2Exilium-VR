#ifndef GF2_XR_PRESENTATION_BANKS_H
#define GF2_XR_PRESENTATION_BANKS_H
#include "presentation_bank_pool.h"
/* Included after xr_eye_output.h. Sole XR-owner physical storage, dormant until
   per-token flight adapters are connected. Each output has independent grant,
   acquired/waited images and ticket. Never enable its legacy spare-bank route.
   No Begin/End, GPU completion, source disposal or physical-reference changes. */
typedef struct {
    XrEyeOutput outputs[PBP_BANKS];
    PresentationBankPool leases;
    XrSession session;
    unsigned thread,ready,creation_failed,closing;
} XrPresentationBanks;
static inline int xpb_owner(const XrPresentationBanks *b,const PanelXr *p) {
    return b && p && b->session && b->session==p->session &&
        b->thread==GetCurrentThreadId();
}
static inline int xpb_create(XrPresentationBanks *b,PanelXr *p,const EyeGpuDescription *d) {
    if(!b || !p || !d || !p->session || b->session || b->ready || b->closing)return 0;
    /* Validate all storage before publishing any configuration. */
    for(unsigned i=0;i<PBP_BANKS;++i) {
        XrEyeOutput *o=&b->outputs[i];
        if(o->enabled || o->session || o->chains[0] || o->chains[1] ||
           o->state!=XEO_IDLE || o->acquired[0] || o->acquired[1] ||
           o->waited[0] || o->waited[1] || b->leases.banks[i].state!=PBP_FREE)return 0;
#ifdef GF2VR_MAIN_TRANSITION_HOLD
        if(o->hold_enabled || o->spare_chains[0] || o->spare_chains[1])return 0;
#endif
    }
    XrEyeOutput check={0};
    if(!xr_eye_output_configure(&check,d))return 0;
    b->session=p->session;b->thread=GetCurrentThreadId();
    for(unsigned i=0;i<PBP_BANKS;++i) {
        XrEyeOutput *o=&b->outputs[i];
        if(!xr_eye_output_configure(o,d)) {b->creation_failed=1;return 0;}
        o->enabled=1;
        if(!xr_eye_output_create(o,p)) {b->creation_failed=1;return 0;}
        /* All six destination handles must be distinct. Retain partial handles
           on any failure; cleanup never forgets or double-destroys an alias. */
        for(unsigned j=0;j<=i;++j)for(unsigned k=0;k<2;++k)
            for(unsigned n=0;n<2;++n)if((j<i || k<n) &&
                b->outputs[j].chains[k]==o->chains[n]) {
                b->creation_failed=1;return 0;
            }
    }
    b->ready=1;return 1;
}
/* Storage access only, no lease/admission claim. Per-token adapter must reserve
   before original End and check exact token before each use. */
static inline XrEyeOutput *xpb_output(XrPresentationBanks *b,PanelXr *p,unsigned i) {
    if(!xpb_owner(b,p) || !b->ready || b->closing || i>=PBP_BANKS)return 0;
    return &b->outputs[i];
}
/* Caller first stops publication, cancels/drains through the exact flight owner
   and discards every lease. No legacy reclaim: it may cancel an unclaimed grant
   without accounting for a flight. Validate ALL banks before destroying ANY.
   Only failed initial creation permits fault-state cleanup without GPU facts:
   no output was ever exposed or acquired then. Destroy failures retain handles
   and retry progress; actual GPU/API faults after exposure remain quarantined. */
static inline int xpb_close(XrPresentationBanks *b,PanelXr *p) {
    if(!b || !p)return 0;
    if(!b->session)return !b->ready;
    if(!xpb_owner(b,p))return 0;
    b->closing=1;b->ready=0;
    for(unsigned i=0;i<PBP_BANKS;++i) {
        XrEyeOutput *o=&b->outputs[i];
        unsigned state=__atomic_load_n(&o->state,__ATOMIC_ACQUIRE);
        if(b->leases.banks[i].state!=PBP_FREE ||
           (state!=XEO_IDLE && !(b->creation_failed && state==XEO_FAULT)) ||
           o->acquired[0] || o->acquired[1] || o->waited[0] || o->waited[1])return 0;
    }
    for(unsigned i=0;i<PBP_BANKS;++i)for(unsigned k=0;k<2;++k) {
        XrSwapchain chain=b->outputs[i].chains[k];if(!chain)continue;
        if(p->api.DestroySwapchain(chain)!=XR_SUCCESS)return 0;
        /* Alias from a broken creation contract is one handle, not two. */
        for(unsigned j=0;j<PBP_BANKS;++j)for(unsigned n=0;n<2;++n)
            if(b->outputs[j].chains[n]==chain)b->outputs[j].chains[n]=XR_NULL_HANDLE;
    }
    /* Keep the monotonic frame clock through configuration recreation. */
    unsigned long long last=b->leases.last_frame;
    *b=(XrPresentationBanks){0};b->leases.last_frame=last;return 1;
}
#endif
