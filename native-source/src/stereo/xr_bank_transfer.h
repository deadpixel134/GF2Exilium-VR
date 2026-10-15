#ifndef GF2_XR_BANK_TRANSFER_H
#define GF2_XR_BANK_TRANSFER_H
#ifdef BOUNDARY_TEST
static void (*xbt_test_before_ack)(EyeFlightOwner *);
#endif
/* Included after presentation flights. Sole XR owner, no borrowed producer
   pointers. Once transfer succeeds NO numeric/pose/backend read is permitted. */
static int xbt_valid(XrPresentationFlights *f,PanelXr *p,unsigned i,XrSpace space,
                    unsigned long long generation,unsigned long long epoch,
                    unsigned long long cancel) {
    if(!f || i>=PBP_BANKS || !xpb_owner(&f->storage,p) || !f->storage.ready ||
       f->storage.closing || f->storage.leases.stopped)return 0;
    XrBankReceipt *r=&f->receipts[i];PresentationBank *b=&f->storage.leases.banks[i];
    XrEyeOutput *o=&f->storage.outputs[i];unsigned long long now=GetTickCount64();
#ifdef GF2VR_NUMERIC_LOSS_OBSERVE
    unsigned nlo_reason=0;
#endif
    int valid=!NLO_TEST(101,!(r->state==2)) &&
        !NLO_TEST(102,!(b->state==PBP_READY)) &&
        !NLO_TEST(103,!(pbp_same(b->token,r->proof.token))) &&
        !NLO_TEST(104,!(b->ready_serial==r->proof.ticket.serial)) &&
        !NLO_TEST(105,!(b->prepared_ms==r->prepared_ms)) &&
        !NLO_TEST(106,!(r->session==p->session)) &&
        !NLO_TEST(107,!(space)) &&
        !NLO_TEST(108,!(r->space==space)) &&
        !NLO_TEST(109,!(r->proof.token.generation==generation)) &&
        !NLO_TEST(110,!(r->proof.token.epoch==epoch)) &&
        !NLO_TEST(111,!(r->cancel_clock==cancel)) &&
        !NLO_TEST(112,!(now>=r->prepared_ms)) &&
        !NLO_TEST(113,!(now-r->prepared_ms<250)) &&
        !NLO_TEST(114,!(o->chains[0]==r->chains[0])) &&
        !NLO_TEST(115,!(o->chains[1]==r->chains[1])) &&
        !NLO_TEST(116,!(o->width==r->width)) &&
        !NLO_TEST(117,!(o->height==r->height)) &&
        !NLO_TEST(118,!(o->state==XEO_IDLE)) &&
        !NLO_TEST(119,!(!o->acquired[0])) &&
        !NLO_TEST(120,!(!o->acquired[1])) &&
        !NLO_TEST(121,!(!o->waited[0])) &&
        !NLO_TEST(122,!(!o->waited[1]));
    if(!valid)NLO_SNAPSHOT(&f->storage,&r->proof,NLO_VALID,nlo_reason,EFO_REJECTED,0);
    return valid;
}
/* Retryable transfer; SDK calls and pose copy precede the locked exact-ticket
   acknowledgement. BUSY keeps proof/bank/API progress; never renews its age. */
static int xbt_transfer(XrPresentationFlights *f,PanelXr *p,EyeFlightOwner *owner,
        EyePosePool *poses,EyeFlightToken token,XrSpace space,
        const unsigned long long *generation,const unsigned long long *cancel_clock) {
    unsigned i;if(!space || !generation || !cancel_clock || !xpf_find(f,p,token,&i) ||
       !f->storage.ready || f->storage.closing || f->storage.leases.stopped)return EFO_REJECTED;
    XrBankReceipt *receipt=&f->receipts[i];XrEyeFlightImages *images=&f->images[i];
    XrEyeOutput *o=&f->storage.outputs[i];PresentationBank *bank=&f->storage.leases.banks[i];
    unsigned long long cancel=__atomic_load_n(cancel_clock,__ATOMIC_ACQUIRE),now=GetTickCount64();
    if(receipt->state==2)return xbt_valid(f,p,i,space,
        __atomic_load_n(generation,__ATOMIC_ACQUIRE),token.epoch,cancel)?EFO_OK:EFO_REJECTED;
    if(__atomic_load_n(generation,__ATOMIC_ACQUIRE)!=token.generation ||
       now<bank->prepared_ms || now-bank->prepared_ms>=250)return EFO_REJECTED;
    if(!receipt->state) {
        if(bank->state!=PBP_WRITING)return EFO_REJECTED;
        int r=xfi_complete(images,o,owner);if(r!=EFO_OK)return r;
        r=xfi_release(images,o,p,owner);if(r!=EFO_OK)return r;
        EyeFlightSnapshot proof;XrEyeInput pose;
        r=eye_pose_pool_read(poses,owner,token,&proof,&pose);if(r!=EFO_OK)return r;
        if(!images->detached || images->fault || images->actual_released!=3 ||
           o->state!=XEO_IDLE || o->acquired[0] || o->acquired[1] ||
           !xr_eye_matches(&pose,token.epoch,token.frame,proof.display_time) ||
           cancel!=__atomic_load_n(cancel_clock,__ATOMIC_ACQUIRE) ||
           __atomic_load_n(generation,__ATOMIC_ACQUIRE)!=token.generation ||
           !pbp_complete(&f->storage.leases,i,&proof))return EFO_REJECTED;
        *receipt=(XrBankReceipt){.state=1,.proof=proof,.pose=pose,.session=p->session,.space=space,
            .chains={o->chains[0],o->chains[1]},.width=o->width,.height=o->height,
            .prepared_ms=bank->prepared_ms,.cancel_clock=cancel};
    }
    if(receipt->cancel_clock!=__atomic_load_n(cancel_clock,__ATOMIC_ACQUIRE) ||
       receipt->space!=space || receipt->session!=p->session)return EFO_REJECTED;
    /* Cleanup eligibility BEFORE opening main reuse. Nothing can fail or call
       SDK between successful acknowledgement and local receipt publication. */
    if(images->fault || !images->detached || images->thread!=GetCurrentThreadId() ||
       o->owner_thread!=images->thread || !pbp_same(images->token,token) ||
       o->frame!=token.frame || o->epoch!=token.epoch || o->ticket.serial!=receipt->proof.ticket.serial ||
       o->chains[0]!=receipt->chains[0] || o->chains[1]!=receipt->chains[1] ||
       o->state!=XEO_IDLE || o->acquired[0] || o->acquired[1] || o->waited[0] || o->waited[1])return EFO_REJECTED;
#ifdef BOUNDARY_TEST
    if(xbt_test_before_ack)xbt_test_before_ack(owner);
#endif
#ifdef GF2VR_BANK_TRANSFER_OBSERVE
    bank_admission_observe_record(receipt->proof.ticket.serial,token.generation,token.epoch,
        29,token.frame,receipt->proof.flags,0);
#endif
    int r=eye_flight_transfer(owner,token,receipt->proof.ticket);
#ifdef GF2VR_BANK_TRANSFER_OBSERVE
    bank_admission_observe_record(receipt->proof.ticket.serial,token.generation,token.epoch,
        30,token.frame,receipt->proof.flags,r);
#endif
    if(r!=EFO_OK)return r;
    receipt->proof.flags|=EFF_PRODUCER_TRANSFERRED;
    o->frame=o->epoch=0;o->presentable=0;o->ticket=(EyeGpuTicket){0};
    *images=(XrEyeFlightImages){0};receipt->state=2;
    return EFO_OK;
}
static int xbt_projection(XrPresentationFlights *f,PanelXr *p,EyeFlightToken token,
        XrSpace space,const XrFrameEndInfo *end,unsigned long long generation,
        unsigned long long epoch,unsigned long long cancel,
        XrCompositionLayerProjection *layer,XrCompositionLayerProjectionView views[2]) {
    if(layer)*layer=(XrCompositionLayerProjection){0};
    if(views)views[0]=views[1]=(XrCompositionLayerProjectionView){0};
    unsigned i;if(!layer || !views || !end || end->type!=XR_TYPE_FRAME_END_INFO ||
       !xpf_find(f,p,token,&i) || !xbt_valid(f,p,i,space,generation,epoch,cancel))return EFO_REJECTED;
    XrBankReceipt *r=&f->receipts[i];
    if(end->displayTime<=r->proof.display_time){NLO_SNAPSHOT(&f->storage,&r->proof,NLO_PROJECTION,1,EFO_REJECTED,0);return EFO_REJECTED;}
    for(unsigned j=0;j<PBP_BANKS;++j)if(f->storage.leases.banks[j].state==PBP_HELD &&
               f->storage.leases.banks[j].token.frame>=token.frame){NLO_SNAPSHOT(&f->storage,&r->proof,NLO_PROJECTION,2,EFO_REJECTED,0);return EFO_REJECTED;}
    for(unsigned eye=0;eye<2;++eye)views[eye]=(XrCompositionLayerProjectionView){
        .type=XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW,.pose=r->pose.poses[eye],.fov=r->pose.fov[eye],
        .subImage={.swapchain=r->chains[eye],.imageRect.extent={(int)r->width,(int)r->height}}};
    *layer=(XrCompositionLayerProjection){.type=XR_TYPE_COMPOSITION_LAYER_PROJECTION,
        .space=r->space,.viewCount=2,.views=views};r->projected=1;return EFO_OK;
}
/* Actual later End (publication) or explicit invalidation (discard). Local
   disposition is separate from earlier producer transfer, never a numeric ack. */
static int xbt_finalize(XrPresentationFlights *f,PanelXr *p,EyeFlightToken token,int publish,int end_ok) {
    unsigned i;if(!xpf_find(f,p,token,&i) || f->receipts[i].state!=2 ||
        (publish && (!end_ok || !f->receipts[i].projected)))return EFO_REJECTED;
    EyeFlightSnapshot proof=f->receipts[i].proof;
    proof.flags|=EFF_PRESENTATION_RETIRED;
    if(!publish)proof.flags|=EFF_CANCELLED;
    int ok=publish && pbp_publish(&f->storage.leases,i,&proof,GetTickCount64());
#ifdef GF2VR_NUMERIC_LOSS_OBSERVE
    unsigned nlo_published=ok!=0;
#endif
#ifdef GF2VR_BANK_TRANSFER_OBSERVE
    if(ok && publish)bank_admission_observe_record(proof.ticket.serial,token.generation,token.epoch,
        31,token.frame,proof.flags,EFO_OK);
#endif
    if(!ok)ok=pbp_discard(&f->storage.leases,i,&proof);
    NLO_SNAPSHOT(&f->storage,&proof,NLO_FINALIZE,nlo_published?0:(publish?2:1),ok?EFO_OK:EFO_REJECTED,
       (publish!=0)|((end_ok!=0)<<1));
    if(ok)f->receipts[i]=(XrBankReceipt){0};return ok?EFO_OK:EFO_REJECTED;
}
#endif
