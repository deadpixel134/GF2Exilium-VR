#ifndef GF2_XR_READY_READMIT_H
#define GF2_XR_READY_READMIT_H
#if !defined(GF2VR_BANK_READMIT) || !defined(GF2VR_BANK_TRANSFER)
#error READY readmission requires the existing bank readmit and immutable transfer route
#endif
/* Sole XR owner, before actual Begin only. READY is protected presentation
   storage, never reusable writing capacity. Reserve one ordinary numeric offer
   only when a different physical FREE bank can back it. Existing HELD/WRITING
   readmission remains unchanged; no SDK calls, pose recapture or numeric reads
   of transferred receipts. Main can only free capacity, not consume it. */
static int xfd_ready_readmit_impl(XrEyeFrameDriver *d,XrPresentationFlights *f,
        PanelXr *p,unsigned long long live_generation,const unsigned long long *cancel_clock) {
    if(!xfd_owner(d))return EFO_REJECTED;
    XrEyeOwner *o=&d->eyes->owner;
#ifdef GF2VR_NUMERIC_LOSS_OBSERVE
    unsigned nlo_reason=0;
#endif
    if(NLO_TEST(101,!p) ||
       NLO_TEST(102,!cancel_clock) ||
       NLO_TEST(103,!d->requested_display_only) ||
       NLO_TEST(104,!d->predicted) ||
       NLO_TEST(105,d->current) ||
       NLO_TEST(106,d->begun) ||
       NLO_TEST(107,d->pending_token.frame) ||
       NLO_TEST(108,!d->offered) ||
       NLO_TEST(109,d->stopping) ||
       NLO_TEST(110,d->fault) ||
       NLO_TEST(111,!d->prediction.shouldRender) ||
       NLO_TEST(112,o->changing) ||
       NLO_TEST(113,o->retiring) ||
       NLO_TEST(114,o->calibration_request) ||
       NLO_TEST(115,o->session!=p->session) ||
       NLO_TEST(116,!o->space) ||
       NLO_TEST(117,o->epoch!=d->requested_epoch) ||
       NLO_TEST(118,o->head_reference_epoch!=o->epoch) ||
       NLO_TEST(119,!live_generation) ||
       NLO_TEST(120,live_generation!=d->requested_generation) ||
       NLO_TEST(121,!xr_eye_matches(&d->predicted_pose,d->requested_epoch,d->predicted,d->prediction.predictedDisplayTime)) ||
       NLO_TEST(122,!f) ||
       NLO_TEST(123,!xpb_owner(&f->storage,p)) ||
       NLO_TEST(124,f->storage.thread!=d->thread) ||
       NLO_TEST(125,f->storage.session!=d->api.prefetch.session) ||
       NLO_TEST(126,!f->storage.ready) ||
       NLO_TEST(127,f->storage.closing) ||
       NLO_TEST(128,f->storage.creation_failed) ||
       NLO_TEST(129,f->storage.leases.stopped) ||
       NLO_TEST(130,f->storage.leases.generation!=d->requested_generation) ||
       NLO_TEST(131,f->storage.leases.epoch!=d->requested_epoch) ||
       NLO_TEST(132,d->predicted<=f->storage.leases.last_frame)){NLO_DRIVER(d,NLO_READY,nlo_reason,EFO_OK,0);return EFO_OK;}
#if defined(GF2VR_MULTI_READY_READMIT) && (!defined(GF2VR_INDEPENDENT_READY_CAPACITY) || !defined(GF2VR_CAPACITY_EXPERIMENT))
#error Multiple protected READY readmission requires the existing independent N3 storage contract
#endif
    unsigned ready=0,held=0,rooms=0;
#ifdef GF2VR_INDEPENDENT_READY_CAPACITY
    unsigned writers=0;EyeFlightToken retained={0};
#ifdef GF2VR_CAPACITY_EXPERIMENT
    EyeFlightToken retained_tokens[EFO_CAPACITY];
#endif
#endif

    unsigned long long cancel=__atomic_load_n(cancel_clock,__ATOMIC_ACQUIRE);
    for(unsigned i=0;i<PBP_BANKS;++i) {
        PresentationBank *bank=&f->storage.leases.banks[i];XrEyeOutput *output=&f->storage.outputs[i];
#ifdef GF2VR_INDEPENDENT_READY_CAPACITY
        if(bank->state==PBP_WRITING) {
            retained=bank->token;
#ifdef GF2VR_CAPACITY_EXPERIMENT
            if(++writers>=EFO_CAPACITY || !pbp_same(retained,f->images[i].token) || f->receipts[i].state ||
#else
            if(++writers>1 || !pbp_same(retained,f->images[i].token) || f->receipts[i].state ||
#endif
               retained.generation!=live_generation || retained.epoch!=o->epoch ||
               retained.frame>=d->predicted){NLO_DRIVER(d,NLO_READY,3,EFO_OK,writers);return EFO_OK;}
#ifdef GF2VR_CAPACITY_EXPERIMENT
            retained_tokens[writers-1]=retained;
#endif
            continue; /* Existing writer's GPU resources and leases stay untouched. */
        }
#endif
        if(output->state!=XEO_IDLE || output->acquired[0] || output->acquired[1] ||
           output->waited[0] || output->waited[1] || f->images[i].token.frame){NLO_DRIVER(d,NLO_READY,4,EFO_OK,i);return EFO_OK;}
        if(bank->state==PBP_READY) {
            XrBankReceipt *receipt=&f->receipts[i];
            unsigned complete=EFF_PRODUCER_TRANSFERRED|EFF_CPU_RETURNED|EFF_RENDER_DRAINED|EFF_COPY_DRAINED;
#ifdef GF2VR_MULTI_READY_READMIT
            /* Each READY remains immutable and is checked; it is never a writer. */
            ++ready;
            if(!xbt_valid(f,p,i,o->space,live_generation,o->epoch,cancel) ||
#else
            if(++ready!=1 || !xbt_valid(f,p,i,o->space,live_generation,o->epoch,cancel) ||
#endif
               !receipt->proof.cpu_ok || (receipt->proof.flags&complete)!=complete ||
               (receipt->proof.flags&(EFF_CANCELLED|EFF_FAULT|EFF_RECLAIMING|EFF_PRESENTATION_RETIRED)) ||
               receipt->proof.token.frame>=d->predicted ||
               receipt->proof.display_time>=d->prediction.predictedDisplayTime){
#ifdef GF2VR_MULTI_READY_READMIT
                NLO_DRIVER(d,NLO_READY,5,EFO_OK,ready); /* invalid proof, not count refusal */
#else
                NLO_DRIVER(d,NLO_READY,ready>1?6:5,EFO_OK,ready);
#endif
                return EFO_OK;}
        } else if(bank->state==PBP_HELD) {
            if(++held>1 || f->receipts[i].state || !bank->ready_serial || !bank->token.frame ||
               bank->token.frame>=d->predicted || bank->token.generation!=live_generation ||
               bank->token.epoch!=o->epoch){NLO_DRIVER(d,NLO_READY,7,EFO_OK,held);return EFO_OK;}
        } else if(bank->state==PBP_FREE) {
            if(f->receipts[i].state || bank->token.frame || bank->ready_serial){NLO_DRIVER(d,NLO_READY,8,EFO_OK,i);return EFO_OK;}
            ++rooms;
        } else {NLO_DRIVER(d,NLO_READY,9,EFO_OK,bank->state);return EFO_OK;} /* WRITING/DRAINING keep the existing guarded route */
    }
#ifdef GF2VR_MULTI_READY_READMIT
    if(!ready || !rooms){NLO_DRIVER(d,NLO_READY,10,EFO_OK,(ready<<16)|rooms);return EFO_OK;}
#else
    if(ready!=1 || !rooms){NLO_DRIVER(d,NLO_READY,10,EFO_OK,(ready<<16)|rooms);return EFO_OK;} /* occupied=one READY < max two writers */
#endif
    unsigned available;unsigned long long generation,epoch;
    int r=eye_flight_available(d->owner,&available,&generation,&epoch);
    if(r!=EFO_OK){NLO_DRIVER(d,NLO_READY,11,r,0);return r;}
    if(!available || generation!=d->requested_generation || epoch!=d->requested_epoch ||
       cancel!=__atomic_load_n(cancel_clock,__ATOMIC_ACQUIRE)){NLO_DRIVER(d,NLO_READY,12,EFO_OK,available);return EFO_OK;}
#ifdef GF2VR_INDEPENDENT_READY_CAPACITY
    if(writers) {
#ifndef GF2VR_CAPACITY_EXPERIMENT
        if(available!=1)return EFO_OK;
#endif
#ifdef GF2VR_CAPACITY_EXPERIMENT
        for(unsigned writer=0;writer<writers;++writer) {
        EyeFlightSnapshot prior;r=eye_flight_snapshot(d->owner,retained_tokens[writer],&prior);
#else
        EyeFlightSnapshot prior;r=eye_flight_snapshot(d->owner,retained,&prior);
#endif
        if(r==EFO_BUSY){NLO_DRIVER(d,NLO_READY,13,r,0);return r;}
        unsigned required=EFF_XR_ENDED|EFF_DISPLAY_DETACHED|EFF_PRODUCER_RETAINED;
        if(r!=EFO_OK || !prior.renderable || (prior.flags&required)!=required ||
           prior.flags&(EFF_CANCELLED|EFF_FAULT|EFF_RECLAIMING|EFF_PRESENTATION_RETIRED|EFF_PRODUCER_TRANSFERRED)){NLO_DRIVER(d,NLO_READY,14,EFO_OK,prior.flags);return EFO_OK;}
#ifdef GF2VR_CAPACITY_EXPERIMENT
        }
#endif
        if(cancel!=__atomic_load_n(cancel_clock,__ATOMIC_ACQUIRE)){NLO_DRIVER(d,NLO_READY,15,EFO_OK,0);return EFO_OK;}
    }
#endif
    EyeFlightToken token={d->predicted,generation,epoch};
    r=xfd_offer(d,token,&d->prediction,&d->predicted_pose,&d->pending_token);
    NLO_DRIVER(d,NLO_READY,16,r,0);
    if(r==EFO_OK){d->requested_display_only=0;++d->ready_readmits;
#ifdef GF2VR_INDEPENDENT_READY_CAPACITY
        if(writers)++d->ready_writing_readmits;
#endif
    }
    return r==EFO_REJECTED?EFO_OK:r;
}
static int xfd_readmit_ready_prediction(XrEyeFrameDriver *d,XrPresentationFlights *f,
        PanelXr *p,unsigned long long generation,const unsigned long long *cancel_clock) {
    DWORD saved=GetLastError();int r=xfd_ready_readmit_impl(d,f,p,generation,cancel_clock);
    SetLastError(saved);return r;
}
#endif
