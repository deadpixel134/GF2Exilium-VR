#ifndef GF2_BANK_PRESENTATION_RUNTIME_H
#define GF2_BANK_PRESENTATION_RUNTIME_H
#ifdef GF2VR_BANK_TRANSFER
#ifdef BOUNDARY_TEST
static unsigned stereo_bank_transfer_enabled;
#else
static unsigned stereo_bank_transfer_enabled=1;
#endif
/* Snapshot/pose reads stop at the transfer boundary; receipts remain XR-local. */
static int stereo_bank_transfer_service(PanelXr *p) {
    if(!stereo_bank_transfer_enabled)return EFO_OK;
    XrEyeOwner *o=&stereo_eye_runtime.owner;
    if(!stereo_hold_focused || o->changing || o->retiring || !o->epoch ||
       o->head_reference_epoch!=o->epoch)return EFO_OK;
    for(unsigned i=0;i<PBP_BANKS;++i) {
        EyeFlightToken t=stereo_banks.images[i].token;if(!t.frame)continue;
        int r=xbt_transfer(&stereo_banks,p,&stereo_two.owner,&stereo_two.poses,t,o->space,
            &stereo_publication_generation,&stereo_completed_cancel_clock);
        if(stereo_banks.images[i].fault)return EFO_REJECTED;
        (void)r; /* pending drains/BUSY/expired remain owned for retry or discard */
    }
    return EFO_OK;
}
#endif
#if defined(GF2VR_BANK_COPY_AFTER_END) && !defined(GF2VR_BANK_COPY_SERVICE)
#error After-End service requires the guarded sole-XR copy service
#endif
/* Included by the selected integration after the input/composition helpers.
   All bank/image mutations stay on the sole XR owner; main retains source and
   native ownership. No GPU wait is part of ordinary display-frame progress. */
static int stereo_bank_orphan(EyeFlightToken t,const EyeFlightSnapshot *before) {
    if(before->flags&EFF_RESERVED)return EFO_OK;
    int r;EyeFlightSnapshot s;
    do {r=eye_flight_take(&stereo_two.owner,t,&s);if(r==EFO_BUSY)Sleep(1);} while(r==EFO_BUSY);
    if(r!=EFO_OK)return r;
    do {r=eye_pose_pool_retire(&stereo_two.poses,&stereo_two.owner,t);if(r==EFO_BUSY)Sleep(1);} while(r==EFO_BUSY);
    return r;
}
#ifdef GF2VR_BANK_COPY_SERVICE
/* Next-Wait progress on the SAME XR owner. No Begin/End, completion wait or
   input/pose work here. Original unended offers remain strictly unacquired. */
static int stereo_bank_copy_service(PanelXr *p) {
    XrEyeFrameDriver *d=&stereo_two_xr;XrEyeOwner *o=&stereo_eye_runtime.owner;
    if(!xfd_owner(d) || d->api.prefetch.session!=p->session)return EFO_REJECTED;
    if(d->stopping || d->fault || !stereo_hold_focused || o->changing || o->retiring ||
       !o->epoch || o->head_reference_epoch!=o->epoch || !stereo_banks.storage.ready ||
       stereo_banks.storage.closing || stereo_banks.storage.leases.stopped)return EFO_OK;
    U64 generation=__atomic_load_n(&stereo_publication_generation,__ATOMIC_ACQUIRE);
    if(!generation)return EFO_OK;
#ifdef GF2VR_EVENT_NOTICE_HANDOFF
    stereo_event_notice_service(p);
#endif
    int r=xpf_copy_service(&stereo_banks,p,&stereo_two.owner,&stereo_two.backend,generation,o->epoch);
    if(r==EFO_REJECTED){d->fault=d->stopping=1;return r;}
#ifdef GF2VR_BANK_TRANSFER
    if(stereo_bank_transfer_service(p)==EFO_REJECTED)return EFO_REJECTED;
#endif
#ifdef BOUNDARY_TEST
    if(__atomic_load_n(&bank_service_test_gate,__ATOMIC_ACQUIRE)) {
        for(unsigned i=0;i<PBP_BANKS;++i) {
            EyeFlightToken t=stereo_banks.images[i].token;EyeFlightSnapshot s;
            if(t.frame && eye_flight_snapshot(&stereo_two.owner,t,&s)==EFO_OK &&
               (s.flags&EFF_COPY_ISSUED) && !(s.flags&EFF_RENDER_DRAINED))
                __atomic_store_n(&bank_service_test_posted,1,__ATOMIC_RELEASE);
        }
    }
#endif
    return EFO_OK; /* BUSY keeps progress, never delays display for GPU completion */
}
#endif
/* Original End/EndSession ack only. This never clears numeric/native owners.
   A pending offer cannot have acquired images before its original End. */
static int stereo_bank_unretained(unsigned i) {
    if(i>=PBP_BANKS)return EFO_REJECTED;
    XrEyeFlightImages *f=&stereo_banks.images[i];XrEyeOutput *o=&stereo_banks.storage.outputs[i];
    unsigned state=stereo_banks.storage.leases.banks[i].state;
    if(!f->token.frame || f->detached || f->fault || f->actual_released ||
       (state!=PBP_WRITING && state!=PBP_DRAINING) || o->waited[0] || o->waited[1] ||
       o->chains[0]!=f->writing_chains[0] || o->chains[1]!=f->writing_chains[1])return EFO_REJECTED;
    int r=xfi_retire_mode(f,o,1);
    if(r==EFO_OK)stereo_banks.storage.leases.banks[i]=(PresentationBank){0};
    return r;
}
static int stereo_bank_drain(PanelXr *p,int force) {
    if(force){stereo_bank_cache.valid=0;pbp_stop(&stereo_banks.storage.leases);}
    int pending=0;
    for(unsigned i=0;i<PBP_BANKS;++i) {
#ifdef GF2VR_BANK_TRANSFER
        if(stereo_banks.receipts[i].state==2) {
            XrBankReceipt *receipt=&stereo_banks.receipts[i];EyeFlightToken token=receipt->proof.token;
#ifdef GF2VR_NUMERIC_LOSS_OBSERVE
            unsigned nlo_reason=0;
#endif
            if(NLO_TEST(1,force) || NLO_TEST(2,!stereo_hold_focused) || NLO_TEST(3,stereo_eye_runtime.owner.changing) ||
                NLO_TEST(4,stereo_eye_runtime.owner.retiring) ||
                NLO_TEST(5,stereo_eye_runtime.owner.head_reference_epoch!=token.epoch) ||
                NLO_TEST(6,(stereo_bank_cache.token.epoch==token.epoch && stereo_bank_cache.token.frame>=token.frame)) ||
                NLO_TEST(7,!xbt_valid(&stereo_banks,p,i,stereo_eye_runtime.owner.space,
                    __atomic_load_n(&stereo_publication_generation,__ATOMIC_ACQUIRE),
                    stereo_eye_runtime.owner.epoch,
                    __atomic_load_n(&stereo_completed_cancel_clock,__ATOMIC_ACQUIRE)))) {
                NLO_SNAPSHOT(&stereo_banks.storage,&receipt->proof,NLO_DISCARD,nlo_reason,EFO_OK,0);
                if(xbt_finalize(&stereo_banks,p,token,0,0)!=EFO_OK)return EFO_REJECTED;
            }
            continue;
        }
#endif
        XrEyeFlightImages *f=&stereo_banks.images[i];EyeFlightToken t=f->token;
        if(!t.frame)continue;
        EyeFlightSnapshot s;int r=eye_flight_snapshot(&stereo_two.owner,t,&s);
        if(r==EFO_BUSY){pending=1;continue;}
        if(r!=EFO_OK || f->fault || (s.flags&EFF_FAULT))return EFO_REJECTED;
        if(!(s.flags&EFF_DISPLAY_DETACHED)){if(force)pending=1;continue;}
        U64 now=GetTickCount64(),start=stereo_banks.storage.leases.banks[i].prepared_ms;
        int cancel=force || stereo_banks.storage.leases.stopped || (s.flags&EFF_CANCELLED) ||
            !stereo_hold_focused || __atomic_load_n(&stereo_publication_generation,__ATOMIC_ACQUIRE)!=t.generation ||
            stereo_eye_runtime.owner.epoch!=t.epoch || stereo_eye_runtime.owner.head_reference_epoch!=t.epoch ||
            stereo_eye_runtime.owner.changing || stereo_eye_runtime.owner.retiring || now<start || now-start>=250 ||
            (stereo_bank_cache.token.epoch==t.epoch && stereo_bank_cache.token.frame>=t.frame);
        if(!cancel)continue;
        r=eye_flight_cancel_retained_offer(&stereo_two.owner,t);
        if(r==EFO_BUSY){pending=1;continue;}if(r!=EFO_OK)return r;
        r=xpf_finalize(&stereo_banks,p,&stereo_two.owner,t,0,0);
        if(r!=EFO_OK){if(f->fault)return EFO_REJECTED;pending=1;continue;}
        r=stereo_bank_orphan(t,&s);if(r!=EFO_OK)return r;
    }
    return pending?EFO_BUSY:EFO_OK;
}
static XrResult stereo_bank_end(PanelXr *p,const XrFrameEndInfo *original,int visible) {
    XrEyeFrameDriver *d=&stereo_two_xr;
    if(!xfd_owner(d) || d->api.prefetch.session!=p->session || !original)return XR_ERROR_CALL_ORDER_INVALID;
    XrFrameEndInfo end=*original;
    if(!xr_ordered_frames_visual(&d->api,d->current)){end.layerCount=0;end.layers=0;visible=0;}
    XrEyeInput current={0};if(visible)(void)xr_eye_mailbox_read(&stereo_eye_runtime.mailbox,&current);
    int immersive=visible && stereo_hold_focused && current.valid &&
        current.reference_epoch==stereo_eye_runtime.owner.epoch &&
        current.reference_epoch==stereo_eye_runtime.owner.head_reference_epoch &&
        !stereo_eye_runtime.owner.changing && !stereo_eye_runtime.owner.retiring;
    if(!immersive)stereo_bank_cache.valid=0;
    if(stereo_bank_drain(p,0)==EFO_REJECTED){d->fault=d->stopping=1;return XR_ERROR_RUNTIME_FAILURE;}
    EyeFlightToken fresh_token={0};XrEyeInput captured={0};int r;
#ifdef GF2VR_BANK_THROUGHPUT_OBSERVE
    U64 fresh_serial=0;
#endif
    for(unsigned i=0;i<PBP_BANKS;++i) {
#ifdef GF2VR_BANK_TRANSFER
        if(stereo_bank_transfer_enabled) {
            XrBankReceipt *receipt=&stereo_banks.receipts[i];
            if(immersive && xbt_valid(&stereo_banks,p,i,stereo_eye_runtime.owner.space,
                __atomic_load_n(&stereo_publication_generation,__ATOMIC_ACQUIRE),current.reference_epoch,
                __atomic_load_n(&stereo_completed_cancel_clock,__ATOMIC_ACQUIRE)) &&
                receipt->proof.token.frame>fresh_token.frame)fresh_token=receipt->proof.token;
        }
#endif
        XrEyeFlightImages *f=&stereo_banks.images[i];if(!f->token.frame)continue;
        EyeFlightSnapshot s;r=eye_flight_snapshot(&stereo_two.owner,f->token,&s);
        if(r==EFO_BUSY)continue;
        if(r!=EFO_OK || f->fault || (s.flags&EFF_FAULT)){d->fault=d->stopping=1;return XR_ERROR_RUNTIME_FAILURE;}
        if(!(s.flags&EFF_DISPLAY_DETACHED) || (s.flags&EFF_CANCELLED))continue;
        (void)xpf_copy_tick(&stereo_banks,p,&stereo_two.owner,&stereo_two.backend,f->token);
        if(f->fault){d->fault=d->stopping=1;return XR_ERROR_RUNTIME_FAILURE;}
#ifdef GF2VR_BANK_TRANSFER
        if(stereo_bank_transfer_enabled) {
            EyeFlightToken candidate=f->token;
            if(immersive && xbt_transfer(&stereo_banks,p,&stereo_two.owner,&stereo_two.poses,candidate,
                stereo_eye_runtime.owner.space,&stereo_publication_generation,
                &stereo_completed_cancel_clock)==EFO_OK && candidate.frame>fresh_token.frame)
                fresh_token=candidate;
            continue; /* numeric snapshot may now already be reclaimed */
        }
#endif
        r=eye_flight_snapshot(&stereo_two.owner,f->token,&s);
#ifdef GF2VR_BANK_THROUGHPUT_OBSERVE
        if(r==EFO_OK)async_throughput_record(7,f->token,s.ticket.serial,s.flags,r,d->current);
#endif
        unsigned complete=EFF_CPU_RETURNED|EFF_RENDER_DRAINED|EFF_COPY_ISSUED|EFF_COPY_DRAINED;
        if(r==EFO_OK && immersive && s.cpu_ok && !(s.flags&(EFF_CANCELLED|EFF_FAULT)) &&
           (s.flags&complete)==complete && f->token.frame>fresh_token.frame) {
            fresh_token=f->token;
#ifdef GF2VR_BANK_THROUGHPUT_OBSERVE
            fresh_serial=s.ticket.serial;
#endif
        }
    }
    XrCompositionLayerProjection projection={0};XrCompositionLayerProjectionView views[2]={0};
    int fresh=0,projected=0;U64 cancel=__atomic_load_n(&stereo_completed_cancel_clock,__ATOMIC_ACQUIRE);
    if(immersive && end.layerCount<=1 && (!end.layerCount || end.layers)) {
#ifdef GF2VR_BANK_TRANSFER
        if(stereo_bank_transfer_enabled) {
            if(fresh_token.frame)fresh=xbt_projection(&stereo_banks,p,fresh_token,
                stereo_eye_runtime.owner.space,&end,
                __atomic_load_n(&stereo_publication_generation,__ATOMIC_ACQUIRE),current.reference_epoch,cancel,
                &projection,views)==EFO_OK;
            if(fresh)for(unsigned i=0;i<2;++i)views[i].fov=stereo_unity_texture_fov(views[i].fov);
        } else
#endif
        {
        EyeFlightSnapshot exact;
        if(fresh_token.frame && eye_pose_pool_read(&stereo_two.poses,&stereo_two.owner,fresh_token,&exact,&captured)==EFO_OK) {
            fresh=xpf_projection(&stereo_banks,p,&stereo_two.owner,fresh_token,&captured,
                stereo_eye_runtime.owner.space,&end,&stereo_publication_generation,&projection,views)==EFO_OK;
            if(fresh)for(unsigned i=0;i<2;++i)views[i].fov=stereo_unity_texture_fov(views[i].fov);
        }
        }
        projected=fresh || xpc_layer(&stereo_bank_cache,&stereo_banks,p,&current,stereo_eye_runtime.owner.space,
            &end,&stereo_publication_generation,&stereo_completed_cancel_clock,&projection,views);
    }
    XrFrameEndInfo combined=end;const XrCompositionLayerBaseHeader *layers[2];
    XrCompositionLayerQuad auxiliary={0};
    if(projected) {
        layers[0]=(const XrCompositionLayerBaseHeader *)&projection;combined.layerCount=1;combined.layers=layers;
        if(end.layerCount) {
            if(!stereo_aux.ready){layers[1]=end.layers[0];combined.layerCount=2;}
            else if(end.layers[0]->type==XR_TYPE_COMPOSITION_LAYER_QUAD && aux_visible(&stereo_aux)) {
                aux_quad(&stereo_aux,(const XrCompositionLayerQuad *)end.layers[0],&auxiliary);
                layers[1]=(const XrCompositionLayerBaseHeader *)&auxiliary;combined.layerCount=2;
            }
        }
    }
    EyeFlightToken token=d->current_token;unsigned index=0;int attached=0;
    if(immersive && token.frame && __atomic_load_n(&stereo_publication_generation,__ATOMIC_ACQUIRE)==token.generation) {
        do {r=xpf_prepare(&stereo_banks,p,&stereo_two.owner,token);if(r==EFO_BUSY)Sleep(1);} while(r==EFO_BUSY);
    }
    int bound=token.frame && xpf_find(&stereo_banks,p,token,&index);
    if(bound) {
        do {r=eye_flight_can_retain_offer(&stereo_two.owner,token,end.displayTime);if(r==EFO_BUSY)Sleep(1);} while(r==EFO_BUSY);
        attached=r==EFO_OK && !d->stopping && !stereo_banks.storage.leases.stopped;
    }
    /* Release callbacks and preparation can reenter source cancellation. Never
       grant an old source/reference/cancel stamp a fresh submitted projection. */
    if(projected && (cancel!=__atomic_load_n(&stereo_completed_cancel_clock,__ATOMIC_ACQUIRE) ||
       !stereo_hold_focused || __atomic_load_n(&stereo_publication_generation,__ATOMIC_ACQUIRE)!=
       (fresh?fresh_token.generation:stereo_bank_cache.token.generation))) {combined=end;projected=0;}
    XrCompositionLayerQuad cursor={0};const XrCompositionLayerBaseHeader *ui_layers[3];int cursor_submitted=0;
    int cursor_ready=0;
#ifdef GF2VR_ACTIVITY_DIALOGUE_RAY_OBSERVE
    EyeFlightToken observed=fresh?fresh_token:stereo_bank_cache.token;
    dialogue_ray_observation=(DialogueRayObservation){.enabled=projected && observed.generation!=0,
        .stage=10,.frame=d->current,.generation=observed.generation,.epoch=observed.epoch,.displayed=observed.frame,
        .pose_frame=current.frame_id,.pose_time=(U64)current.display_time,.end_time=(U64)end.displayTime,
        .bank_flags=(projected!=0)|((combined.layerCount==1)<<1)|((!stereo_aux.shown)<<2)|
            ((current.head_valid!=0)<<3)|((xr_eye_matches(&current,current.reference_epoch,current.frame_id,end.displayTime)!=0)<<4)};
#endif
    if(combined.layerCount<=2 && visible){
#ifdef GF2VR_ACTIVITY_DIALOGUE_RAY
        if(projected && combined.layerCount==1 && !stereo_aux.shown && current.head_valid &&
           xr_eye_matches(&current,current.reference_epoch,current.frame_id,end.displayTime)){
            EyeFlightToken displayed=fresh?fresh_token:stereo_bank_cache.token;DialogueRaySurface surface;
#ifdef GF2VR_ACTIVITY_DIALOGUE_RAY_OBSERVE
            dialogue_ray_observation.stage=11;
#endif
            if(dialogue_ray_read(&dialogue_ray_mailbox,displayed.generation,displayed.epoch,displayed.frame,
                GetTickCount64(),&surface))
                cursor_ready=controller_runtime_prepare_dialogue(p,stereo_hold_focused,&surface,&current.head,&cursor);
        }else
#endif
        cursor_ready=controller_runtime_prepare(p,&combined,stereo_hold_focused,projected?1u:0u,&cursor);
    }
    if(cursor_ready) {
        for(unsigned i=0;i<combined.layerCount;++i)ui_layers[i]=combined.layers[i];
        ui_layers[combined.layerCount]=(const XrCompositionLayerBaseHeader *)&cursor;
        combined.layers=ui_layers;++combined.layerCount;cursor_submitted=1;
    }
    if(projected) {
        unsigned shown=stereo_bank_cache.index;EyeFlightToken shown_token=stereo_bank_cache.token;
        int mapped=1;if(fresh){shown_token=fresh_token;mapped=xpf_find(&stereo_banks,p,fresh_token,&shown);}
        U64 now=GetTickCount64();
        int valid=mapped && shown<PBP_BANKS && stereo_hold_focused &&
            cancel==__atomic_load_n(&stereo_completed_cancel_clock,__ATOMIC_ACQUIRE) &&
            __atomic_load_n(&stereo_publication_generation,__ATOMIC_ACQUIRE)==shown_token.generation &&
            current.reference_epoch==shown_token.epoch && stereo_eye_runtime.owner.epoch==shown_token.epoch &&
            stereo_eye_runtime.owner.head_reference_epoch==shown_token.epoch &&
            !stereo_eye_runtime.owner.changing && !stereo_eye_runtime.owner.retiring;
#ifdef GF2VR_HELD_FRESH_CLOCK
        if(valid && !fresh)valid=pbp_held_age(&stereo_banks.storage.leases.banks[shown],now) &&
            stereo_banks.storage.leases.banks[shown].held_ms==stereo_bank_cache.tick;
        else
#endif
        if(valid){U64 prepared=stereo_banks.storage.leases.banks[shown].prepared_ms;
            valid=now>=prepared && now-prepared<250;}
        if(!valid){combined=end;projected=cursor_submitted=0;stereo_bank_cache.valid=0;}
    }
    for(unsigned i=0;i<PBP_BANKS;++i) {
        unsigned state=stereo_banks.storage.leases.banks[i].state;
        if((state==PBP_WRITING || state==PBP_DRAINING) &&
           !xr_display_excludes(&combined,stereo_banks.storage.outputs[i].chains[0],stereo_banks.storage.outputs[i].chains[1]))
            return XR_ERROR_RUNTIME_FAILURE;
    }
#ifdef GF2VR_BANK_END_OBSERVE
    ebo_pose_context(&current,views,projected);
    EboU64 observe_start=ebo_stamp();
#endif
    U64 runtime_start=GetTickCount64();
    do {
#ifdef GF2VR_BANK_END_OBSERVE
        if(ebo_active())++end_boundary_observation.attempts;
#endif
        r=attached?xfd_end_bank_offer(d,&stereo_banks,p,&combined):xfd_end(d,&combined,0);
        /* Cancellation before actual End is ordinary fail-open. Numeric ack
           after a real retained End MUST stay on that same once-only route. */
        if(r==EFO_REJECTED && attached && !d->end_called && !d->fault) {
            int allowed=eye_flight_can_retain_offer(&stereo_two.owner,token,end.displayTime);
            if(allowed==EFO_REJECTED){attached=0;r=EFO_BUSY;}
        }
        if(r==EFO_BUSY)Sleep(1);
    } while(r==EFO_BUSY);
#ifdef GF2VR_BANK_END_OBSERVE
    ebo_driver(observe_start,ebo_stamp());
#endif
    XrResult result=r==EFO_OK?d->end_result:XR_ERROR_RUNTIME_FAILURE;
    stereo_pacing_runtime_end_ms+=GetTickCount64()-runtime_start;
#ifdef GF2VR_ACTIVITY_DIALOGUE_RAY
    if(stereo_controller.mode_identity==2 && (!projected || !stereo_hold_focused ||
       cancel!=__atomic_load_n(&stereo_completed_cancel_clock,__ATOMIC_ACQUIRE) ||
       stereo_eye_runtime.owner.changing || stereo_eye_runtime.owner.retiring ||
       stereo_eye_runtime.owner.epoch!=current.reference_epoch ||
       stereo_eye_runtime.owner.head_reference_epoch!=current.reference_epoch))cursor_submitted=0;
#endif
    controller_runtime_end(result,cursor_submitted);
#ifdef GF2VR_ACTIVITY_DIALOGUE_RAY_OBSERVE
    dialogue_ray_observe_flush();
#endif
    if(result==XR_SUCCESS && cursor_submitted)
        world_drag_arbitrate(&stereo_aux.drag.frame,
#ifdef GF2VR_CONTROLLER_HAND_ROLES
            stereo_panel_role_hand(),
#else
            0,
#endif
            1,stereo_aux.pointer.hand?0u:1u,1,1);
    U64 generation=__atomic_load_n(&stereo_publication_generation,__ATOMIC_ACQUIRE);
    stereo_world_drag_end(stereo_aux.drag.frame,result==XR_SUCCESS && projected && immersive &&
        generation==(fresh?fresh_token.generation:stereo_bank_cache.token.generation),current.reference_epoch,generation);
#ifdef GF2VR_THUMBSTICK_NAVIGATION
    stereo_navigation_end(&stereo_aux.navigation.frame,result==XR_SUCCESS && projected && immersive &&
        generation==(fresh?fresh_token.generation:stereo_bank_cache.token.generation),
        current.reference_epoch,generation,stereo_drag.owner<WD_NONE ||
        (cursor_submitted && (stereo_aux.pointer.trigger_down || stereo_aux.pointer.face_down)));
#endif
    if(r!=EFO_OK){stereo_bank_cache.valid=0;return result;}
    if(bound) {
        if(attached) {
            do {r=xfi_sync(&stereo_banks.images[index],&stereo_banks.storage.outputs[index],&stereo_two.owner);
                if(r==EFO_BUSY)Sleep(1);} while(r==EFO_BUSY);
        } else r=stereo_bank_unretained(index);
        if(r!=EFO_OK)return XR_ERROR_RUNTIME_FAILURE;
    }
    if(token.frame && !attached) {
        EyeFlightSnapshot s;do {r=eye_flight_snapshot(&stereo_two.owner,token,&s);if(r==EFO_BUSY)Sleep(1);} while(r==EFO_BUSY);
        if(r==EFO_OK && stereo_bank_orphan(token,&s)!=EFO_OK)return XR_ERROR_RUNTIME_FAILURE;
    }
    if(fresh) {
#ifdef GF2VR_NUMERIC_LOSS_OBSERVE
        unsigned nlo_reason=0;
#endif
        int publish=!NLO_TEST(1,result!=XR_SUCCESS) && !NLO_TEST(2,!projected) &&
           !NLO_TEST(3,cancel!=__atomic_load_n(&stereo_completed_cancel_clock,__ATOMIC_ACQUIRE));
#ifdef GF2VR_BANK_TRANSFER
        if(stereo_bank_transfer_enabled) {
            unsigned receipt_index;
            publish=publish && !NLO_TEST(4,!stereo_hold_focused) && !NLO_TEST(5,stereo_eye_runtime.owner.changing) &&
                !NLO_TEST(6,stereo_eye_runtime.owner.retiring) &&
                !NLO_TEST(7,stereo_eye_runtime.owner.head_reference_epoch!=fresh_token.epoch) &&
                !NLO_TEST(8,!xpf_find(&stereo_banks,p,fresh_token,&receipt_index)) &&
                !NLO_TEST(9,!xbt_valid(&stereo_banks,p,receipt_index,stereo_eye_runtime.owner.space,
                    __atomic_load_n(&stereo_publication_generation,__ATOMIC_ACQUIRE),
                    stereo_eye_runtime.owner.epoch,
                    __atomic_load_n(&stereo_completed_cancel_clock,__ATOMIC_ACQUIRE)));
        }
#endif
#ifdef GF2VR_BANK_THROUGHPUT_OBSERVE
        /* Save only existing immutable IDs; numeric slot may retire after ack. */
        async_throughput_record(8,fresh_token,fresh_serial,0,result,publish);
#endif
        NLO_RECORD(&stereo_two.owner,fresh_token,0,NLO_END,nlo_reason,publish,(result==XR_SUCCESS)|((projected!=0)<<1));
        do {r=xpf_finalize(&stereo_banks,p,&stereo_two.owner,fresh_token,publish,publish);
            if(r==EFO_BUSY)Sleep(1);} while(r==EFO_BUSY);
        if(r==EFO_REJECTED) {
            /* Actual End may race cancellation. Discard remains drain-gated. */
            do {r=eye_flight_cancel_retained_offer(&stereo_two.owner,fresh_token);if(r==EFO_BUSY)Sleep(1);} while(r==EFO_BUSY);
            if(r==EFO_OK)do {r=xpf_finalize(&stereo_banks,p,&stereo_two.owner,fresh_token,0,0);
                if(r==EFO_BUSY)Sleep(1);} while(r==EFO_BUSY);
        }
        if(r!=EFO_OK)return XR_ERROR_RUNTIME_FAILURE;
#ifdef GF2VR_BANK_THROUGHPUT_OBSERVE
        async_throughput_record(9,fresh_token,fresh_serial,0,r,publish);
#endif
        if(publish)(void)xpc_capture(&stereo_bank_cache,&stereo_banks,p,fresh_token,&projection,
            &stereo_completed_cancel_clock,cancel,result);
        if(publish)++stereo_pacing_fresh;
        if(publish)NLO_RECORD(&stereo_banks.storage,fresh_token,0,NLO_PUBLISH,0,EFO_OK,0);
#ifdef GF2VR_BANK_END_OBSERVE
        if(publish)ebo_arm();
#endif
    }
    if(result!=XR_SUCCESS)stereo_bank_cache.valid=0;
#ifdef GF2VR_BANK_COPY_AFTER_END
    /* A render submission can become visible while the actual End executes,
       AFTER the earlier copy scan. Service the now-detached banks once on the
       same XR owner, without waiting for completion or inventing a frame call.
       The helper retains focus/source/reference/stop and original-age guards. */
    if(result==XR_SUCCESS && stereo_bank_copy_service(p)==EFO_REJECTED)
        return XR_ERROR_RUNTIME_FAILURE;
#endif
    stereo_aux.last_projection=projected && XR_SUCCEEDED(result);
    static unsigned prior=~0u;
    unsigned mode=projected?(combined.layerCount>=2 && combined.layers[1]->type==XR_TYPE_COMPOSITION_LAYER_QUAD?2u:1u):0u;
    if(mode!=prior){event("composite_panel_mode",mode,GetCurrentThreadId());prior=mode;}
    if(fresh && projected && result==XR_SUCCESS) {
        static U64 count;if(++count<=8 || count%100==0)event("stereo_projection_end_frame",fresh_token.frame,GetCurrentThreadId());
    }
    return result;
}
static int stereo_bank_close(PanelXr *p) {
    XrEyeFrameDriver *d=&stereo_two_xr;
    if(eye_flight_stop(&stereo_two.owner)!=EFO_OK)return 0;
    int drain=stereo_bank_drain(p,1);if(drain==EFO_REJECTED)return 0;
    if(!d->thread)return drain==EFO_OK;
    int r=xfd_close(d);if(r==EFO_OK)return drain==EFO_OK;
    if(d->fault || r==EFO_REJECTED)return 0;
    if(d->requested)(void)xfd_take(d);
    if(!d->current && d->predicted)(void)xfd_begin(d,stereo_two_current_pose,(void *)1);
    if(d->current && !d->begun)(void)xfd_begin(d,stereo_two_current_pose,(void *)1);
    if(d->current && d->begun) {
        XrFrameEndInfo end={.type=XR_TYPE_FRAME_END_INFO,.displayTime=d->current_state.predictedDisplayTime,
            .environmentBlendMode=XR_ENVIRONMENT_BLEND_MODE_OPAQUE};
        (void)stereo_bank_end(p,&end,0);
    }
    return 0;
}
static int stereo_bank_stop_session(PanelXr *p) {
    stereo_frame_abort_input();XrEyeFrameDriver *d=&stereo_two_xr;
    if(!d->thread)return stereo_bank_drain(p,1)==EFO_OK;
    if(d->thread!=GetCurrentThreadId() || d->api.prefetch.session!=p->session)return 0;
    d->stopping=1;int r;
    do {r=eye_flight_stop(&stereo_two.owner);if(r==EFO_BUSY)Sleep(1);} while(r==EFO_BUSY);
    if(r!=EFO_OK)return 0;
    stereo_bank_cache.valid=0;pbp_stop(&stereo_banks.storage.leases);
    while(!xr_frame_prefetch_quiesce(&d->api.prefetch))Sleep(1);
    for(unsigned i=0;i<PBP_BANKS;++i) {
        XrEyeFlightImages *f=&stereo_banks.images[i];if(!f->token.frame)continue;
        do {r=xfi_release(f,&stereo_banks.storage.outputs[i],p,&stereo_two.owner);
            if(r==EFO_BUSY || (r==EFO_REJECTED && !f->fault))Sleep(1);
        } while(r==EFO_BUSY || (r==EFO_REJECTED && !f->fault));
        if(r!=EFO_OK)return 0;
    }
    /* Unended originals retain local identity until actual EndSession. They
       have no acquired images; source/native completion is separately owned. */
    do {
        r=stereo_bank_drain(p,1);if(r==EFO_REJECTED)return 0;
        int detached=0;for(unsigned i=0;i<PBP_BANKS;++i)if(stereo_banks.images[i].detached)detached=1;
        if(!detached)return 1;
        Sleep(1);
    } while(1);
}
static int stereo_bank_session_stopped(PanelXr *p) {
    XrEyeFrameDriver *d=&stereo_two_xr;
    if(!d->thread)return stereo_output_xr_close(p);
    if(p->running || d->api.prefetch.session!=p->session)return 0;
    EyeFlightToken terminal[EFO_CAPACITY];int r;
    do {r=xfd_session_stopped(d,terminal);if(r==EFO_BUSY)Sleep(1);} while(r==EFO_BUSY);
    if(r!=EFO_OK)return 0;
    for(unsigned i=0;i<PBP_BANKS;++i) {
        if(!stereo_banks.images[i].token.frame)continue;
        if(stereo_banks.images[i].detached)return 0; /* stop must truly drain it */
        if(stereo_bank_unretained(i)!=EFO_OK)return 0;
    }
    for(unsigned i=0;i<EFO_CAPACITY;++i) {
        EyeFlightToken t=terminal[i];if(!t.frame)continue;EyeFlightSnapshot s;
        do {r=eye_flight_snapshot(&stereo_two.owner,t,&s);if(r==EFO_BUSY)Sleep(1);} while(r==EFO_BUSY);
        if(r==EFO_REJECTED)continue;
        if(r!=EFO_OK || stereo_bank_orphan(t,&s)!=EFO_OK)return 0;
    }
    return stereo_output_xr_close(p);
}
#endif
