#ifndef GF2_XR_PRESENTATION_CONFIG_H
#define GF2_XR_PRESENTATION_CONFIG_H
#include "xr_eye_config.h"
#include "xr_presentation_cache.h"
/* Reuse the original main->XR numeric configuration channel. Main still uses
   xr_eye_config_request/ready; no new shared pointers or mutable description.
   XR alone owns storage; numeric owner/backend/source cleanup remain separate.
   Dormant until the product scheduler switches all dependent paths together. */
static inline int xbc_close(XrEyeConfig *q,XrPresentationFlights *f,XrBankPresentation *cache,
                           PanelXr *p,EyeFlightOwner *owner) {
    if(!q || !f || !cache || !p || !owner)return 0;
    if(f->storage.session && !xpb_owner(&f->storage,p))return 0;
    unsigned state=__atomic_load_n(&q->state,__ATOMIC_ACQUIRE);
    if(state==XEC_WRITING || state==XEC_PROCESSING)return 0;
    if(!__atomic_compare_exchange_n(&q->state,&state,XEC_PROCESSING,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE))return 0;
    cache->valid=0;pbp_stop(&f->storage.leases);
    /* BUSY never opens configuration reuse; no GPU facts are implied by stop. */
    if(eye_flight_stop(owner)!=EFO_OK)goto pending;
#ifdef GF2VR_BANK_TRANSFER
    /* Transferred images are genuinely drained/released and XR-only. Numeric
       producer cleanup is independent; pending transfer still has images and
       must follow the existing drain path. Never forget receipts on resize. */
    for(unsigned i=0;i<PBP_BANKS;++i)if(f->receipts[i].state==2 &&
        xbt_finalize(f,p,f->receipts[i].proof.token,0,0)!=EFO_OK)goto pending;
    for(unsigned i=0;i<PBP_BANKS;++i)if(f->receipts[i].state)goto pending;
#endif
    for(unsigned i=0;i<PBP_BANKS;++i)if(f->images[i].token.frame || f->images[i].fault)goto pending;
    if(!xpb_close(&f->storage,p))goto pending;
    __atomic_store_n(&q->state,XEC_IDLE,__ATOMIC_RELEASE);return 1;
pending:
    __atomic_store_n(&q->state,XEC_RETIRING,__ATOMIC_RELEASE);return 0;
}
static inline void xbc_drive(XrEyeConfig *q,XrPresentationFlights *f,XrBankPresentation *cache,
                           PanelXr *p,EyeFlightOwner *owner,unsigned long long epoch) {
    if(!q || !f || !cache || !p || !owner)return;
    unsigned state=__atomic_load_n(&q->state,__ATOMIC_ACQUIRE);
    if(state==XEC_RETIRING){(void)xbc_close(q,f,cache,p,owner);return;}
    if(!epoch)return; /* temporary missing prediction is not retirement */
    if(state==XEC_READY || state==XEC_FAILED) {
        if(q->epoch!=epoch)(void)xbc_close(q,f,cache,p,owner);
        return;
    }
    unsigned expected=XEC_REQUEST;
    if(!__atomic_compare_exchange_n(&q->state,&expected,XEC_PROCESSING,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE))return;
    if(q->epoch!=epoch){__atomic_store_n(&q->state,XEC_IDLE,__ATOMIC_RELEASE);return;}
    int created=xpb_create(&f->storage,p,&q->description);
    __atomic_store_n(&q->state,created?XEC_READY:XEC_FAILED,__ATOMIC_RELEASE);
}
/* Match sole-XR bank capacity to the source owner's existing numeric contract.
   Never activate/reset numeric source/pose/native ownership from configuration.
   New generation is adopted only after ALL numeric leases truly retired. */
static inline int xbc_generation(XrPresentationFlights *f,PanelXr *p,EyeFlightOwner *owner) {
    if(!f || !owner || !xpb_owner(&f->storage,p) || !f->storage.ready || f->storage.closing)return EFO_REJECTED;
    unsigned free_slots;unsigned long long generation,epoch;
    int r=eye_flight_available(owner,&free_slots,&generation,&epoch);if(r!=EFO_OK)return r;
    PresentationBankPool *banks=&f->storage.leases;
    if(!generation || !epoch){pbp_stop(banks);return EFO_REJECTED;}
    if(!banks->stopped && banks->generation==generation && banks->epoch==epoch)return EFO_OK;
    /* First creation can occur after Begin/PRIVATE main admission requested
       the description. No XR bank has ever been exposed in this storage yet;
       adopting the current numeric identity does not reset a provider slot.
       Replacements still require genuine whole-pool retirement below. */
    if(!banks->generation && !banks->epoch) {
        for(unsigned i=0;i<PBP_BANKS;++i)if(f->images[i].token.frame || f->images[i].fault ||
            banks->banks[i].state!=PBP_FREE)return EFO_BUSY;
        return pbp_activate(banks,generation,epoch)?EFO_OK:EFO_BUSY;
    }
    pbp_stop(banks);
#ifdef GF2VR_BANK_GENERATION_RECOVERY
    /* A changed live numeric identity was activated only after EVERY previous
       numeric token retired. Begin may already have offered a token belonging
       to this new identity. That occupancy is not an old lease. Physical
       images/receipts/banks still must clear independently below. A stopped
       restart of the SAME identity keeps the original empty-pool guard. */
    if(banks->generation==generation && banks->epoch==epoch && free_slots!=EFO_CAPACITY)
        return EFO_BUSY;
#else
    if(free_slots!=EFO_CAPACITY)return EFO_BUSY;
#endif
    for(unsigned i=0;i<PBP_BANKS;++i)if(f->images[i].token.frame || f->images[i].fault)return EFO_BUSY;
#if defined(GF2VR_BANK_GENERATION_RECOVERY) && defined(GF2VR_BANK_TRANSFER)
    for(unsigned i=0;i<PBP_BANKS;++i)if(f->receipts[i].state)return EFO_BUSY;
#endif
    return pbp_activate(banks,generation,epoch)?EFO_OK:EFO_BUSY;
}
#endif
