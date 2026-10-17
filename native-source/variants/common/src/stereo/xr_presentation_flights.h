#ifndef GF2_XR_PRESENTATION_FLIGHTS_H
#define GF2_XR_PRESENTATION_FLIGHTS_H
#include "xr_presentation_banks.h"
#include "xr_eye_flight_images.h"
#ifdef GF2VR_BANK_TRANSFER
#include "eye_pose_pool.h"
typedef struct {
    unsigned state,projected; /* 0 absent,1 copied/ack pending,2 transferred */
    EyeFlightSnapshot proof;
    XrEyeInput pose;
    XrSession session;XrSpace space;
    XrSwapchain chains[2];
    unsigned width,height;
    unsigned long long prepared_ms,cancel_clock;
} XrBankReceipt;
#endif
#ifndef GF2VR_ASYNC_PRESENTATION
#error Presentation flights require the detached original-token contract
#endif
/* Sole XR owner. Dormant: not selected by the production driver. Native/source
   slots remain separately owned; bank index is NOT a numeric/native slot index. */
typedef struct {
    XrPresentationBanks storage;
    XrEyeFlightImages images[PBP_BANKS];
#ifdef GF2VR_BANK_TRANSFER
    XrBankReceipt receipts[PBP_BANKS];
#endif
} XrPresentationFlights;
#ifdef GF2VR_BANK_TRANSFER
static int xbt_finalize(XrPresentationFlights *,PanelXr *,EyeFlightToken,int,int);
#endif
static inline int xpf_find(XrPresentationFlights *f,PanelXr *p,EyeFlightToken t,unsigned *index) {
    if(!f || !index || !xpb_owner(&f->storage,p))return 0;
    for(unsigned i=0;i<PBP_BANKS;++i)
        if(f->storage.leases.banks[i].state!=PBP_FREE &&
           f->storage.leases.banks[i].state!=PBP_HELD &&
           pbp_same(f->storage.leases.banks[i].token,t)) {*index=i;return 1;}
    return 0;
}
/* Reserve BEFORE original excluded-layer End, no images acquired yet. BUSY or
   failed preparation retains its bank/clock for exact cancellation/retry. */
static inline int xpf_prepare(XrPresentationFlights *f,PanelXr *p,EyeFlightOwner *owner,
                             EyeFlightToken t) {
    if(!f || !owner || !xpb_owner(&f->storage,p) || !f->storage.ready ||
       f->storage.closing || f->storage.leases.stopped)return EFO_REJECTED;
    unsigned i;
    if(!xpf_find(f,p,t,&i)) {
        EyeFlightSnapshot s;int r=eye_flight_snapshot(owner,t,&s);if(r!=EFO_OK)return r;
        if(!(s.flags&EFF_XR_BEGUN) || s.flags&(EFF_XR_ENDED|EFF_CANCELLED|EFF_FAULT|EFF_RECLAIMING) ||
           !s.renderable || s.acquired_mask || s.waited_mask || s.released_mask ||
           (s.flags&EFF_COPY_ISSUED))return EFO_REJECTED;
        if(!pbp_reserve(&f->storage.leases,t,GetTickCount64(),&i))return EFO_REJECTED;
        /* Storage was fully created before admission. Bind the local exact
           lease NOW so a BUSY second owner snapshot cannot orphan a bank when
           source/focus loss cancels the offer before prepare is retried. */
        XrEyeOutput *o=&f->storage.outputs[i];XrEyeFlightImages *images=&f->images[i];
        if(images->token.frame || o->state!=XEO_IDLE || o->acquired[0] || o->acquired[1])return EFO_REJECTED;
        images->token=t;images->thread=f->storage.thread;images->pending_offer=1;
        images->writing_chains[0]=o->chains[0];images->writing_chains[1]=o->chains[1];
        images->prepared_tick=f->storage.leases.banks[i].prepared_ms;
        o->frame=t.frame;o->epoch=t.epoch;o->presentable=0;
        o->ticket=s.ticket.serial?s.ticket:(EyeGpuTicket){0,t.generation,t.epoch};
    }
    XrEyeOutput *o=&f->storage.outputs[i];
    int r=xfi_prepare_offer(&f->images[i],o,p,owner,t);
    if(r==EFO_OK)f->images[i].prepared_tick=f->storage.leases.banks[i].prepared_ms;
    return r;
}
static inline int xpf_copy_tick(XrPresentationFlights *f,PanelXr *p,EyeFlightOwner *owner,
                             EyeFlightBackend *backend,EyeFlightToken t) {
    unsigned i;if(!xpf_find(f,p,t,&i) || !f->storage.ready || f->storage.closing ||
        f->storage.leases.stopped || f->storage.leases.banks[i].state!=PBP_WRITING)return EFO_REJECTED;
    unsigned long long now=GetTickCount64(),start=f->storage.leases.banks[i].prepared_ms;
    if(now<start || now-start>=250)return EFO_REJECTED; /* caller cancels/drains, never frees here */
    return xfi_copy_tick(&f->images[i],&f->storage.outputs[i],p,owner,backend,t);
}
/* Dormant scheduling primitive: service already-End-attested producers while
   the display owner waits for its next frame. Never acquire an unended offer:
   the existing pending-offer End/cleanup contract remains unchanged. Caller
   supplies current focused navigation/source identities (zero blocks work).
   Bounded to one pass; no Sleep, frame API, publication, release or input work.
   BUSY is pending progress, not a reason to delay the display frame. */
static inline int xpf_copy_service(XrPresentationFlights *f,PanelXr *p,
        EyeFlightOwner *owner,EyeFlightBackend *backend,
        unsigned long long generation,unsigned long long epoch) {
    if(!f || !owner || !generation || !epoch || !xpb_owner(&f->storage,p) ||
       !f->storage.ready || f->storage.closing || f->storage.leases.stopped)
        return EFO_REJECTED;
    int pending=0;
    for(unsigned i=0;i<PBP_BANKS;++i) {
        XrEyeFlightImages *images=&f->images[i];EyeFlightToken t=images->token;
        if(f->storage.leases.banks[i].state!=PBP_WRITING || !t.frame ||
           t.generation!=generation || t.epoch!=epoch)continue;
        EyeFlightSnapshot s;int r=eye_flight_snapshot(owner,t,&s);
        if(r==EFO_BUSY){pending=1;continue;}
        if(r!=EFO_OK || images->fault || (s.flags&EFF_FAULT))return EFO_REJECTED;
        if(!(s.flags&EFF_DISPLAY_DETACHED) ||
           s.flags&(EFF_CANCELLED|EFF_RECLAIMING|EFF_PRESENTATION_RETIRED))continue;
        r=xpf_copy_tick(f,p,owner,backend,t);
        if(images->fault)return EFO_REJECTED;
        if(r==EFO_BUSY)pending=1;
        /* Ordinary REJECTED means not ready/expired; caller retains/drains the
           original lease. No synthetic completion or cleanup here. */
    }
    return pending?EFO_BUSY:EFO_OK;
}
/* Candidate chain may be submitted only when it is not currently being written
   or drained. Release alone never makes another active writer submit-safe. */
static inline int xpf_chain_allowed(XrPresentationFlights *f,PanelXr *p,XrSwapchain chain) {
    if(!chain || !f || !xpb_owner(&f->storage,p) || !f->storage.ready ||
       f->storage.closing || f->storage.leases.stopped)return 0;
    for(unsigned i=0;i<PBP_BANKS;++i) {
        unsigned s=f->storage.leases.banks[i].state;
        if((s==PBP_WRITING || s==PBP_DRAINING) &&
           (chain==f->storage.outputs[i].chains[0] || chain==f->storage.outputs[i].chains[1]))return 0;
    }
    return 1;
}
static inline int xpf_projection(XrPresentationFlights *f,PanelXr *p,EyeFlightOwner *owner,
        EyeFlightToken t,const XrEyeInput *input,XrSpace space,const XrFrameEndInfo *end,
        const unsigned long long *generation,XrCompositionLayerProjection *layer,
        XrCompositionLayerProjectionView views[2]) {
    if(layer)*layer=(XrCompositionLayerProjection){0};
    if(views)views[0]=views[1]=(XrCompositionLayerProjectionView){0};
    unsigned i;if(!layer || !views || !xpf_find(f,p,t,&i) || !f->storage.ready ||
       f->storage.closing || f->storage.leases.stopped ||
       f->storage.leases.banks[i].state!=PBP_WRITING)return EFO_REJECTED;
    unsigned long long now=GetTickCount64(),start=f->storage.leases.banks[i].prepared_ms;
    if(now<start || now-start>=250)return EFO_REJECTED;
    for(unsigned j=0;j<PBP_BANKS;++j)if(f->storage.leases.banks[j].state==PBP_HELD &&
        f->storage.leases.banks[j].token.frame>=t.frame)return EFO_REJECTED;
    EyeFlightSnapshot s;int r=eye_flight_snapshot(owner,t,&s);if(r!=EFO_OK)return r;
    r=xfi_detached_projection(&f->images[i],&f->storage.outputs[i],p,owner,input,space,end,
                              generation,layer,views);
    if(r!=EFO_OK)return r;
    /* Actual release facts were reconciled by xfi_release. No second snapshot
       after constructing the layer: a BUSY there would strand projected=1.
       CPU/ticket/drain fields are the original immutable completed snapshot. */
    s.released_mask=f->images[i].actual_released;
    if(!pbp_complete(&f->storage.leases,i,&s)) {
        *layer=(XrCompositionLayerProjection){0};return EFO_REJECTED;
    }
    return EFO_OK; /* NOT End success or cache publication */
}
/* Finalize AFTER actual later End. Snapshot BEFORE disposition: main can take
   the numeric slot immediately after that ack. No post-disposition owner read.
   Failed/BUSY numeric ack retains bank and API progress; repeated End forbidden
   by the caller's display-frame owner, not performed in this module. */
static inline int xpf_finalize(XrPresentationFlights *f,PanelXr *p,EyeFlightOwner *owner,
                              EyeFlightToken t,int published,int actual_end_success) {
    unsigned i;if(!xpf_find(f,p,t,&i))return EFO_REJECTED;
#ifdef GF2VR_BANK_TRANSFER
    if(f->receipts[i].state==2)return xbt_finalize(f,p,t,published,actual_end_success);
#endif
    if(published && (f->storage.leases.banks[i].state!=PBP_READY || !actual_end_success))return EFO_REJECTED;
    XrEyeFlightImages *images=&f->images[i];XrEyeOutput *o=&f->storage.outputs[i];
    int r=xfi_sync(images,o,owner);if(r!=EFO_OK)return r;
    r=xfi_release(images,o,p,owner);if(r!=EFO_OK)return r;
    EyeFlightSnapshot s;r=eye_flight_snapshot(owner,t,&s);if(r!=EFO_OK)return r;
    r=xfi_detached_finalize(images,o,owner,published,actual_end_success);if(r!=EFO_OK)return r;
#ifdef GF2VR_BANK_TRANSFER
    f->receipts[i]=(XrBankReceipt){0}; /* pending ack cancelled: legacy drain disposition */
#endif
    /* Successful numeric disposition attests this single terminal flag. This
       local copy is NOT used to invent CPU/GPU drain or image-release facts. */
    s.flags|=EFF_PRESENTATION_RETIRED;
    if(published && pbp_publish(&f->storage.leases,i,&s,GetTickCount64()))return EFO_OK;
    /* End may succeed just as age expires; discard rather than refresh age. */
    return pbp_discard(&f->storage.leases,i,&s)?EFO_OK:EFO_REJECTED;
}
#ifdef GF2VR_BANK_TRANSFER
#include "xr_bank_transfer.h"
#endif
#endif
