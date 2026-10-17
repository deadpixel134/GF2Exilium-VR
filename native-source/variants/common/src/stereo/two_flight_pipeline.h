#if defined(GF2VR_NATIVE_TARGET_CACHE) && !defined(GF2VR_NATIVE_BOUNDARY_OBSERVE)
#error Native target cache requires the typed native boundary adapter
#endif
/* Target adapter included after native description/order services. The actual
   root/wrapper/Submit/main/render tick call sites below dispatch here for a
   dual live owner. No XR calls, input sampling or physical-reference resets. */
#ifdef GF2VR_MAIN_HANDOFF_OBSERVE
#include "main_production_observe.h"
#endif
#ifdef GF2VR_ASYNC_VALIDITY_OBSERVE
static unsigned validity_records;
static void async_validity_record(EyeFlightToken token,U64 serial,int native_ok,
    unsigned native_reason,U64 native_elapsed,unsigned current_reason,int current_result,unsigned current_flags) {

    if(validity_records<128) {
        ++validity_records;
        /* Main owner only, AFTER managed calls; original predicate unchanged.
           Separate cap, no callback log and no added snapshot/GPU/XR query. */
        DWORD saved=GetLastError();char line[1024],*p=source_prefix(line,"async_native_validity");
        p=literal(p,",\"flight\":");p=hexadecimal(p,token.frame);
        p=literal(p,",\"serial\":");p=hexadecimal(p,serial);
        p=literal(p,",\"native_ok\":");p=decimal(p,native_ok!=0);
        p=literal(p,",\"native_reason\":");p=decimal(p,native_reason);
        p=literal(p,",\"native_ms\":");p=decimal(p,native_elapsed);
        p=literal(p,",\"current_reason\":");p=decimal(p,current_reason);
        p=literal(p,",\"snapshot_result\":");p=hexadecimal(p,(U64)(long long)current_result);
        p=literal(p,",\"flags\":");p=hexadecimal(p,current_flags);
        p=literal(p,"}\n");source_log(line,p);SetLastError(saved);
    }
}
#ifdef BOUNDARY_TEST
__declspec(dllexport) int async_validity_record_test(void) {
    unsigned saved_count=validity_records;DWORD saved_error=GetLastError();
    validity_records=127;SetLastError(773);
    async_validity_record((EyeFlightToken){11,3,9},17,0,51,16,2,EFO_BUSY,0);
    async_validity_record((EyeFlightToken){12,3,9},18,1,0,8,0,EFO_OK,0);
    int ok=validity_records==128 && GetLastError()==773;
    validity_records=saved_count;SetLastError(saved_error);return ok?0:__LINE__;
}
#endif
#endif
static int stereo_two_frame_current(void *context) {
    SourceRenderFlight *s=context;
#ifdef GF2VR_MAIN_HANDOFF_OBSERVE
    unsigned reason=0;int result=EFO_OK;unsigned index;DrawQueueIdentity now={0};
    if(!stereo_live || stereo_live->pair_count!=EFO_CAPACITY || !stereo_source.enabled || !stereo_output.enabled)reason=1;
    else if(!source_render_flight_current(s))reason=2;
    else if(!__atomic_load_n(&panel_device_ready,__ATOMIC_ACQUIRE))reason=3;
    else if(__atomic_load_n(&panel_device_changed,__ATOMIC_ACQUIRE))reason=4;
    else if((result=eye_flight_slot(&stereo_two.owner,s->token,&index))!=EFO_OK)reason=5;
    else if(!stereo_two.slots[index].ready)reason=6;
    else if(!draw_runtime_identity(&now))reason=7;
    else if(!draw_queue_same(stereo_two.slots[index].entry,now))reason=8;
    if(s && reason && !s->observed_bridge_reason) {
        s->observed_bridge_reason=reason;s->observed_bridge_result=result;
    }
    return reason==0;
#else
    if(!stereo_live || stereo_live->pair_count!=EFO_CAPACITY || !stereo_source.enabled ||
       !stereo_output.enabled || !source_render_flight_current(s) ||
       !__atomic_load_n(&panel_device_ready,__ATOMIC_ACQUIRE) ||
       __atomic_load_n(&panel_device_changed,__ATOMIC_ACQUIRE))return 0;
    unsigned index;DrawQueueIdentity now={0};
    if(eye_flight_slot(&stereo_two.owner,s->token,&index)!=EFO_OK)return 0;
    return stereo_two.slots[index].ready && draw_runtime_identity(&now) &&
        draw_queue_same(stereo_two.slots[index].entry,now);
#endif
}
static void stereo_two_callback(int id) {
    DWORD saved=GetLastError();
    if(GetCurrentThreadId()==panel_render_tid) {
        U64 child=(U64)draw_runtime_graphics();
        for(unsigned i=0;i<EFO_CAPACITY;++i) {
            EyeRenderInterval *r=&stereo_two.backend.slots[i].render;
            if(__atomic_load_n(&r->state,__ATOMIC_ACQUIRE)==ERI_QUEUED &&
               id==r->begin_id && __atomic_load_n(&r->begin_issued,__ATOMIC_ACQUIRE) &&
               !__atomic_load_n(&r->abandoned,__ATOMIC_ACQUIRE) && child &&
               child==__atomic_load_n(&stereo_two.backend.slots[i].child_queue,__ATOMIC_ACQUIRE)) {
                static unsigned installed;
                if(!installed) {
                    installed=1;(void)eye_target_hooks_install(r->refs[1]);
                    (void)eye_dispatch_install(child,r->refs[1]);
                }
            }
        }
        eye_flight_backend_callback_on_queue(&stereo_two.backend,id,child);
    }
    SetLastError(saved);
}
static void stereo_two_issue(void (*callback)(int),int id) {
    ((void (*)(void (*)(int),int))(unity+0x94d840))(callback,id);
}
#ifdef GF2VR_ACTIVITY_UI_LIFETIME
static int stereo_two_ui_drained(void *context) {
    (void)context;
    if(stereo_two.root_serial || stereo_two.root_slot!=EFO_CAPACITY)return 0;
    for(unsigned i=0;i<EFO_CAPACITY;++i)
        if(stereo_two.slots[i].admitted.token.frame || stereo_two.backend.slots[i].occupied)return 0;
    return 1;
}
#endif
static int stereo_two_arm(void) {
    if(!stereo_live || stereo_live->pair_count!=EFO_CAPACITY || !stereo_two.enabled ||
       GetCurrentThreadId()!=stereo_live->thread || !stereo_pipeline.render.active ||
       stereo_prepare_attempted)return 0;
    /* Set BEFORE invoking any managed preparation; reentrant renders remain
       original-only and cannot reserve a second flight in the same root. */
    stereo_prepare_attempted=1;
#ifdef GF2VR_TWO_FLIGHT_ROOT_OBSERVE
    ++two_root_note.arm_attempts;
#endif
    /* A contended previous tail must be queued before another private-eye
       Begin; otherwise the coverage/watch intervals would overlap. */
    for(unsigned i=0;i<EFO_CAPACITY;++i)if(stereo_two.slots[i].end_pending) {
#ifdef GF2VR_TWO_FLIGHT_ROOT_OBSERVE
        ++two_root_note.end_pending_block;
#endif
        return 0;
    }
#ifdef GF2VR_ACTIVITY_UI_LIFETIME
    if(!source_live_ui_transition(stereo_live,stereo_two_ui_drained,0))return 0;
#endif
    EyeFlightSnapshot f;unsigned index;
#ifdef GF2VR_BANK_ADMISSION_OBSERVE
    bank_admission_observe_record(stereo_pipeline.render.serial,0,0,27,0,0,0);
#endif
    int reserved=eye_flight_reserve_next_render(&stereo_two.owner,stereo_pipeline.render.serial,&f,&index);
#ifdef GF2VR_BANK_ADMISSION_OBSERVE
    bank_admission_observe_record(stereo_pipeline.render.serial,f.token.generation,f.token.epoch,
        28,f.token.frame,f.flags,reserved);
#endif
    if(reserved!=EFO_OK) {
#ifdef GF2VR_TWO_FLIGHT_ROOT_OBSERVE
        if(reserved==EFO_BUSY)++two_root_note.reserve_busy;
        else ++two_root_note.reserve_none;
#endif
        return 0;
    }
#ifdef GF2VR_TWO_FLIGHT_ROOT_OBSERVE
    ++two_root_note.reserve_ok;two_root_reserved(index,f.token.frame);
#endif
    stereo_two.root_slot=index;stereo_two.root_serial=f.ticket.serial;
    stereo_two.slots[index].admitted=f;
#if defined(GF2VR_MAIN_HANDOFF_OBSERVE) || defined(GF2VR_NATIVE_BOUNDARY_OBSERVE)
    /* Reset evidence only after a real new reservation; no current() call. */
    SourceRenderFlight *observed=&stereo_live->flights.slots[index];
    observed->observed_current_reason=observed->observed_current_flags=0;
    observed->observed_current_result=EFO_OK;
    #ifdef GF2VR_MAIN_HANDOFF_OBSERVE
    observed->observed_bridge_reason=0;observed->observed_bridge_result=EFO_OK;
    #endif
    observed->frame.observed_reason=observed->frame.check.observed_reason=0;
#ifdef GF2VR_MAIN_HANDOFF_OBSERVE
    observed->scope.render.observed_injection=0;
#endif
#endif
    async_throughput_record(1,f.token,f.ticket.serial,f.flags,reserved,stereo_pipeline.render.serial);
#ifdef GF2VR_TWO_FLIGHT_ARM_OBSERVE
    stereo_two.slots[index].arm_stage=1;
    two_arm_nested_stage=0;
#endif
#ifdef GF2VR_NATIVE_BOUNDARY_OBSERVE
    native_boundary_begin();
#endif
    if(!source_live_prepare_slot(stereo_live,index,GetCurrentThreadId())) {
#ifdef GF2VR_NATIVE_BOUNDARY_OBSERVE
        native_boundary_note.active=0;
#endif
        NLO_SNAPSHOT(&stereo_two.owner,&f,NLO_ARM,2,EFO_REJECTED,0);
#ifdef GF2VR_TWO_FLIGHT_ARM_OBSERVE
        stereo_two.slots[index].arm_stage=2;
        stereo_two.slots[index].arm_nested_stage=two_arm_nested_stage;
#endif
        return 0;
    }
#ifdef GF2VR_TWO_FLIGHT_ARM_OBSERVE
    two_arm_nested_stage=0;
#endif
#ifdef GF2VR_NATIVE_BOUNDARY_OBSERVE
    native_boundary_phase(0);
#endif
    int armed=source_live_arm_flight(stereo_live,&stereo_two.owner,&stereo_two.poses,f.token,GetCurrentThreadId());
#ifdef GF2VR_NATIVE_BOUNDARY_OBSERVE
    native_boundary_phase(1);
#endif
    if(armed!=EFO_OK) {
#ifdef GF2VR_NATIVE_BOUNDARY_OBSERVE
        native_boundary_note.active=0;
#endif
        NLO_SNAPSHOT(&stereo_two.owner,&f,NLO_ARM,3,EFO_REJECTED,0);
#ifdef GF2VR_TWO_FLIGHT_ARM_OBSERVE
        stereo_two.slots[index].arm_stage=3;stereo_two.slots[index].arm_result=armed;
        stereo_two.slots[index].arm_nested_stage=two_arm_nested_stage;
#endif
        return 0;
    }
    SourceRenderFlight *s=&stereo_live->flights.slots[index];
    /* Without fully prepared native/order state its existing arm cannot inject. */
    s->scope.current=stereo_two_frame_current;s->scope.check_owner=s;
    EyeNativeTargets native={0};DrawQueueIdentity entry={0};EyeGpuDescription description={0};
    if(!stereo_interval_entry()) {
#ifdef GF2VR_NATIVE_BOUNDARY_OBSERVE
        native_boundary_note.active=0;
#endif
        NLO_SNAPSHOT(&stereo_two.owner,&f,NLO_ARM,4,EFO_REJECTED,0);
#ifdef GF2VR_TWO_FLIGHT_ARM_OBSERVE
        stereo_two.slots[index].arm_stage=4;
#endif
        return 0;
    }
    if(!draw_runtime_identity(&entry)) {
#ifdef GF2VR_NATIVE_BOUNDARY_OBSERVE
        native_boundary_note.active=0;
#endif
        NLO_SNAPSHOT(&stereo_two.owner,&f,NLO_ARM,5,EFO_REJECTED,0);
#ifdef GF2VR_TWO_FLIGHT_ARM_OBSERVE
        stereo_two.slots[index].arm_stage=5;
#endif
        return 0;
    }
#ifdef GF2VR_NATIVE_BOUNDARY_OBSERVE
    native_boundary_phase(2);
    unsigned native_boundary_reason=0;
#ifdef GF2VR_NATIVE_TARGET_CACHE
    unsigned native_cache_hit=0;
    int native_ok=eye_native_cache_read(&stereo_live->pool.native[index],
        &stereo_live->pool.preparation[index],&stereo_live->native,f.token.frame,
        source_render_flight_current,s,&eye_native_refs,&native,&native_boundary_reason,&native_cache_hit);
    native_boundary_note.cache_hit=native_cache_hit;
    static unsigned native_cache_samples;
    if(native_ok && native_cache_samples<12) {
        ++native_cache_samples;native_boundary_note.cache_sample=1;
    }
#else
    int native_ok=eye_native_read_detail(&stereo_live->native,s->frame.eyes,f.token.frame,
        source_render_flight_current,s,&native,&native_boundary_reason);
#endif
    native_boundary_phase(3);
    native_boundary_record(f.token.frame,f.token.generation,f.token.epoch,f.ticket.serial,
        native_ok,native_boundary_reason,s->observed_current_reason,s->observed_current_result,
        s->observed_current_flags,s->frame.observed_reason,s->frame.check.observed_reason);
#elif defined(GF2VR_TWO_FLIGHT_NATIVE_OBSERVE)
    unsigned native_reason=0;
#ifdef GF2VR_ASYNC_VALIDITY_OBSERVE
    s->observed_current_reason=s->observed_current_flags=0;s->observed_current_result=EFO_OK;
    U64 native_start=GetTickCount64();
#endif
    int native_ok=eye_native_read_detail(&stereo_live->native,s->frame.eyes,f.token.frame,
        source_render_flight_current,s,&native,&native_reason);
#ifdef GF2VR_ASYNC_VALIDITY_OBSERVE
    U64 native_elapsed=GetTickCount64()-native_start;
    async_validity_record(f.token,f.ticket.serial,native_ok,native_reason,native_elapsed,
        s->observed_current_reason,s->observed_current_result,s->observed_current_flags);
#endif
#else
    int native_ok=eye_native_read(&stereo_live->native,s->frame.eyes,f.token.frame,
        source_render_flight_current,s,&native);
#endif
    if(!native_ok) {
        NLO_SNAPSHOT(&stereo_two.owner,&f,NLO_ARM,6,EFO_REJECTED,0);
#ifdef GF2VR_TWO_FLIGHT_ARM_OBSERVE
        stereo_two.slots[index].arm_stage=6;
#ifdef GF2VR_TWO_FLIGHT_NATIVE_OBSERVE
        stereo_two.slots[index].arm_native_reason=native_reason;
#endif
#endif
        return 0;
    }
    if(!source_render_flight_current(s)) {
        NLO_SNAPSHOT(&stereo_two.owner,&f,NLO_ARM,7,EFO_REJECTED,0);
#ifdef GF2VR_TWO_FLIGHT_ARM_OBSERVE
        stereo_two.slots[index].arm_stage=7;
#endif
        return 0;
    }
    if(!stereo_describe(panel_device,native.resource[0],native.resource[1],&description)) {
        NLO_SNAPSHOT(&stereo_two.owner,&f,NLO_ARM,8,EFO_REJECTED,0);
#ifdef GF2VR_TWO_FLIGHT_ARM_OBSERVE
        stereo_two.slots[index].arm_stage=8;
#endif
        return 0;
    }
    if(!eye_native_format(&native,&description.format)) {
        NLO_SNAPSHOT(&stereo_two.owner,&f,NLO_ARM,9,EFO_REJECTED,0);
#ifdef GF2VR_TWO_FLIGHT_ARM_OBSERVE
        stereo_two.slots[index].arm_stage=9;
#endif
        return 0;
    }
    if(!xr_eye_config_request(&stereo_xr_config,&description,f.token.epoch)) {
        NLO_SNAPSHOT(&stereo_two.owner,&f,NLO_ARM,10,EFO_REJECTED,0);
#ifdef GF2VR_TWO_FLIGHT_ARM_OBSERVE
        stereo_two.slots[index].arm_stage=10;
#endif
        return 0;
    }
    if(__atomic_load_n(&panel_device_changed,__ATOMIC_ACQUIRE)) {
        NLO_SNAPSHOT(&stereo_two.owner,&f,NLO_ARM,11,EFO_REJECTED,0);
#ifdef GF2VR_TWO_FLIGHT_ARM_OBSERVE
        stereo_two.slots[index].arm_stage=11;
#endif
        return 0;
    }
    DrawQueueIdentity after={0};
    if(!draw_runtime_identity(&after)) {
        NLO_SNAPSHOT(&stereo_two.owner,&f,NLO_ARM,12,EFO_REJECTED,0);
#ifdef GF2VR_TWO_FLIGHT_ARM_OBSERVE
        stereo_two.slots[index].arm_stage=12;
#endif
        return 0;
    }
    if(!draw_queue_same(entry,after)) {
        NLO_SNAPSHOT(&stereo_two.owner,&f,NLO_ARM,13,EFO_REJECTED,0);
#ifdef GF2VR_TWO_FLIGHT_ARM_OBSERVE
        stereo_two.slots[index].arm_stage=13;
#endif
        return 0;
    }
    int prepared=eye_flight_backend_prepare_on_queue(&stereo_two.backend,&stereo_two.owner,index,f.token,
        panel_render_tid,entry.child,1,panel_device,panel_context,
        native.resource[0],native.resource[1],description.format);
    if(prepared!=EFO_OK) {
        NLO_SNAPSHOT(&stereo_two.owner,&f,NLO_ARM,14,EFO_REJECTED,0);
#ifdef GF2VR_TWO_FLIGHT_ARM_OBSERVE
        stereo_two.slots[index].arm_stage=14;stereo_two.slots[index].arm_result=prepared;
#endif
        return 0;
    }
    stereo_two.slots[index].entry=entry;stereo_two.slots[index].description=description;
    stereo_two.slots[index].ready=1;
#ifdef GF2VR_BANK_CPU_OBSERVE
    /* Existing successful preparation boundary, no extra managed/GPU query. */
    bank_admission_observe_record(f.ticket.serial,f.token.generation,f.token.epoch,
        40,f.token.frame,f.flags,EFO_OK);
#endif
    async_throughput_record(2,f.token,f.ticket.serial,0,prepared,0);
#ifdef GF2VR_TWO_FLIGHT_ROOT_OBSERVE
    ++two_root_note.arm_ready;
#endif
#ifdef GF2VR_TWO_FLIGHT_ARM_OBSERVE
    stereo_two.slots[index].arm_stage=15;
#endif
    /* Existing Submit correlation remains per REAL root and validates both
       ends of the borrowed original Render/Submit call, not GPU freshness. */
    stereo_draw_order.serial=f.ticket.serial;stereo_draw_order.entry=entry;stereo_draw_order.ready=1;
    return 1;
}
static int stereo_two_before_original(void *pipeline,void *context,void *cameras,void *method) {
    (void)context;
    if(!stereo_live || stereo_live->pair_count!=EFO_CAPACITY || stereo_two.root_slot>=EFO_CAPACITY)return 0;
    SourceRenderFlight *s=&stereo_live->flights.slots[stereo_two.root_slot];
    return s->scope.render.active && s->scope.armed && s->scope.render.injected &&
        cameras==s->scope.render.batch.array && pipeline==s->scope.pipeline && method==s->scope.method;
}
static void stereo_two_root_finished(U64 serial,int failed) {
#ifdef GF2VR_PRODUCER_TRANSFER
    if(stereo_two.root_slot<EFO_CAPACITY && stereo_two.slots[stereo_two.root_slot].producer_failed)failed=1;
#endif
    if(stereo_two.root_serial!=serial || stereo_two.root_slot>=EFO_CAPACITY)return;
    unsigned index=stereo_two.root_slot;SourceRenderFlight *s=&stereo_live->flights.slots[index];
    EyeFlightToken token=stereo_two.slots[index].admitted.token;
    EyeGpuTicket ticket=stereo_two.slots[index].admitted.ticket;
    unsigned injected=s->scope.render.injected;
#ifdef GF2VR_MAIN_HANDOFF_OBSERVE
    MainProductionNote observed={0};
    observed.current_reason=s->observed_current_reason;observed.current_flags=s->observed_current_flags;
    observed.current_result=s->observed_current_result;observed.frame_reason=s->frame.observed_reason;
    observed.pose_reason=s->frame.check.observed_reason;observed.bridge_reason=s->observed_bridge_reason;
    observed.bridge_result=s->observed_bridge_result;
    observed.scope=s->scope.render.active|(s->scope.armed<<1)|(s->scope.render.attempted<<2)|
        (s->scope.render.injected<<3)|(s->scope.render.returned<<4);
    observed.injection=s->scope.render.observed_injection;
    observed.arm=stereo_two.slots[index].arm_stage;observed.nested=stereo_two.slots[index].arm_nested_stage;
    observed.arm_result=stereo_two.slots[index].arm_result;
    observed.native_reason=stereo_two.slots[index].arm_native_reason;
#endif
#ifdef GF2VR_ROOT_INTERVAL_TIMING_OBSERVE
    RootIntervalTag timing_tag={(U64)&stereo_two.owner,token.frame,token.generation,token.epoch,ticket.serial};
    rit_record(timing_tag,RIT_FINISH_ENTER,!failed);
#endif
    int cpu=source_render_flight_finish(s,GetCurrentThreadId(),serial,!failed);
#ifdef GF2VR_ROOT_INTERVAL_TIMING_OBSERVE
    rit_record(timing_tag,RIT_FINISH_RETURN,cpu!=0);
#endif
    stereo_two.slots[index].root_finished=1;stereo_two.slots[index].cpu_ok=cpu!=0;
    EyeFlightNativeSlot *native=&stereo_two.backend.slots[index];
#ifdef GF2VR_MAIN_HANDOFF_OBSERVE
    main_production_record(token,ticket.serial,1,&observed,cpu,failed!=0,0,0);
#endif
#ifdef GF2VR_TWO_FLIGHT_ROOT_OBSERVE
    ++two_root_note.root_finished;
    U64 root_now=GetTickCount64();
    if(two_root_flight[index]==token.frame && root_now>=two_root_reserved_at[index])
        two_root_note.reserve_to_root_ms+=root_now-two_root_reserved_at[index];
    if(__atomic_load_n(&native->render.begin_issued,__ATOMIC_ACQUIRE))++two_root_note.render_issued;
    else ++two_root_note.preissue_failed;
#endif
#ifdef GF2VR_TWO_FLIGHT_ARM_OBSERVE
    if(!__atomic_load_n(&native->render.begin_issued,__ATOMIC_ACQUIRE)) {
        U64 detail=((U64)stereo_two.slots[index].arm_nested_stage<<16)|
            ((U64)(native->occupied!=0)<<3)|((U64)(injected!=0)<<2)|
            ((U64)(stereo_two.slots[index].ready!=0)<<1)|(failed!=0);
#ifdef GF2VR_TWO_FLIGHT_NATIVE_OBSERVE
        detail|=(U64)stereo_two.slots[index].arm_native_reason<<32;
#endif
        two_observe_record(1,"two_flight_arm_boundary",token,token.frame,ticket.serial,
            stereo_two.slots[index].arm_stage,stereo_two.slots[index].arm_result,0,detail,1);
    }
#endif
    if(!native->occupied) {
        NLO_SNAPSHOT(&stereo_two.owner,&stereo_two.slots[index].admitted,NLO_ROOT,1,EFO_REJECTED,
          (failed!=0)|((injected!=0)<<1)|((stereo_two.slots[index].ready!=0)<<2));
        /* No event was prepared/issued. Uninjected managed cleanup remains the
           real SourceRenderFrame finish operation, never a fictitious fence. */
        stereo_two.slots[index].cpu_pending=eye_flight_cancel_unissued(&stereo_two.owner,token,ticket)!=EFO_OK;
    } else if(!__atomic_load_n(&native->render.begin_issued,__ATOMIC_ACQUIRE)) {
        NLO_SNAPSHOT(&stereo_two.owner,&stereo_two.slots[index].admitted,NLO_ROOT,2,EFO_REJECTED,
          (failed!=0)|((injected!=0)<<1)|((stereo_two.slots[index].ready!=0)<<2));
        stereo_two.slots[index].cpu_pending=eye_flight_backend_cancel_unissued(&stereo_two.backend,
            &stereo_two.owner,index)!=EFO_OK;
    } else {
        stereo_two.slots[index].cpu_pending=eye_flight_backend_cpu_return(&stereo_two.backend,
            &stereo_two.owner,index,cpu)!=EFO_OK;
        DrawQueueIdentity after={0};
        int ordered=!failed && injected && stereo_draw_current(serial,&after) && stereo_interval_entry();
        int tail=eye_flight_backend_end(&stereo_two.backend,index,ordered,stereo_two_issue,stereo_two_callback);
        if(tail==EFO_BUSY && ordered)stereo_two.slots[index].end_pending=1;
        else if(tail!=EFO_OK)
            {native->fault_pending=1;(void)eye_flight_fault(&stereo_two.owner,token);} /* retain, never guess drain */
    }
    stereo_two.root_serial=0;stereo_two.root_slot=EFO_CAPACITY;
    async_throughput_record(3,token,ticket.serial,0,cpu,stereo_two.slots[index].cpu_pending);
#ifdef GF2VR_BANK_CPU_OBSERVE
    /* Original root/borrow finish and existing ordered tail have returned.
       CPU return is not render-GPU completion or permission to release. */
    bank_admission_observe_record(ticket.serial,token.generation,token.epoch,
        41,token.frame,stereo_two.slots[index].cpu_pending,cpu!=0);
#endif
}
#ifdef GF2VR_MAIN_TAIL_RETIREMENT
static unsigned stereo_two_retire_active;
static U64 stereo_two_tail_retired;
static void stereo_two_retire_ready(int from_tail);
#endif
#ifdef GF2VR_MAIN_TAIL_COMPLETION
static void stereo_two_completion_tail(void) {
    /* Called only after SourceRenderFrame finish, camera_pipeline_end and root
       bookkeeping on the same original main thread. The previous slot may have
       drained during that work. No full owner_tick or deferred command issue;
       an optional retire-only pass below separately requires quiescence. */
    DWORD saved=GetLastError();
    (void)eye_flight_backend_reconcile_completed(&stereo_two.backend,&stereo_two.owner);
#ifdef GF2VR_MAIN_TAIL_RETIREMENT
    U64 before=stereo_two_tail_retired;stereo_two_retire_ready(1);
    if(stereo_two_tail_retired!=before){static U64 next_report;static unsigned reports;
        U64 now=GetTickCount64();if(reports<40&&now>=next_report){next_report=now+2000;++reports;
            event("main_tail_source_retired",stereo_two_tail_retired,GetCurrentThreadId());}}
#endif
    SetLastError(saved);
}
#endif
static void stereo_two_owner_tick(void) {
    if(!stereo_live || stereo_live->pair_count!=EFO_CAPACITY || GetCurrentThreadId()!=stereo_live->thread)return;
#ifdef GF2VR_COPY_GPU_OBSERVE
    stereo_copy_gpu_observe_tick();
#endif
#ifdef GF2VR_COPY_PRESENT_OBSERVE
    stereo_copy_present_observe_tick();
#endif
#ifdef GF2VR_BANK_EYE_WRITE_OBSERVE
    stereo_bank_eye_write_observe_tick();
#endif
#ifdef GF2VR_BANK_GPU_OBSERVE
    stereo_bank_gpu_observe_tick();
#endif
#ifdef GF2VR_BANK_FENCE_OBSERVE
    stereo_bank_fence_observe_tick();
#endif
    if(!stereo_source.enabled || !stereo_live->generation.selection.eligible)
        (void)eye_flight_stop(&stereo_two.owner);
    for(unsigned i=0;i<EFO_CAPACITY;++i) {
        EyeFlightSnapshot admitted=stereo_two.slots[i].admitted;
        if(!admitted.token.frame)continue;
        if(admitted.token.generation!=stereo_live->generation.selection.generation)
            (void)eye_flight_stop(&stereo_two.owner);
        EyeFlightNativeSlot *b=&stereo_two.backend.slots[i];
        if(stereo_two.slots[i].end_pending) {
            /* Lock contention is not a GPU/ordering failure. Retain the exact
               tail and retry on main, still against the captured real queue.
               Extra queued work is conservative fence coverage, never early
               completion or permission to reuse the source pair. */
            DrawQueueIdentity now={0};
            int ordered=stereo_interval_entry() &&
                !__atomic_load_n(&panel_device_changed,__ATOMIC_ACQUIRE) &&
                draw_runtime_identity(&now) && draw_queue_same(stereo_two.slots[i].entry,now);
            int tail=ordered?eye_flight_backend_end(&stereo_two.backend,i,1,stereo_two_issue,stereo_two_callback):EFO_REJECTED;
            if(tail==EFO_OK)stereo_two.slots[i].end_pending=0;
            else if(tail!=EFO_BUSY) {b->fault_pending=1;(void)eye_flight_fault(&stereo_two.owner,admitted.token);}
        }
        if(stereo_two.slots[i].cpu_pending) {
            int result;
#ifdef GF2VR_CPU_RETURN_RETRY_FIX
            /* Main take resets interval.begin_issued after genuine GPU drain.
               CPU acknowledgment may still be pending on numeric-lock BUSY.
               Use this exact flight's monotonic issue fact, not helper storage
               whose lifetime already ended. BUSY retains the same CPU result. */
            EyeFlightSnapshot cpu_frame;
            result=eye_flight_snapshot(&stereo_two.owner,admitted.token,&cpu_frame);
            if(result==EFO_OK) {
                if(b->occupied && (cpu_frame.flags&EFF_RENDER_ISSUED))
                    result=eye_flight_backend_cpu_return(&stereo_two.backend,&stereo_two.owner,i,stereo_two.slots[i].cpu_ok);
                else if(b->occupied)result=eye_flight_backend_cancel_unissued(&stereo_two.backend,&stereo_two.owner,i);
                else result=eye_flight_cancel_unissued(&stereo_two.owner,admitted.token,admitted.ticket);
            }
#else
            if(b->occupied && __atomic_load_n(&b->render.begin_issued,__ATOMIC_ACQUIRE))
                result=eye_flight_backend_cpu_return(&stereo_two.backend,&stereo_two.owner,i,stereo_two.slots[i].cpu_ok);
            else if(b->occupied)result=eye_flight_backend_cancel_unissued(&stereo_two.backend,&stereo_two.owner,i);
            else result=eye_flight_cancel_unissued(&stereo_two.owner,admitted.token,admitted.ticket);
#endif
            if(result==EFO_OK)stereo_two.slots[i].cpu_pending=0;
        }
        /* Deferred copy is issued only on the same verified native queue.
           A changed device/queue retains claimed image ownership in quarantine. */
        if(b->occupied && __atomic_load_n(&b->copy_requested,__ATOMIC_ACQUIRE) && !b->copy_started) {
            DrawQueueIdentity now={0};
            if(!stereo_interval_entry() || __atomic_load_n(&panel_device_changed,__ATOMIC_ACQUIRE) ||
               !draw_runtime_identity(&now) || !draw_queue_same(stereo_two.slots[i].entry,now))
                {b->fault_pending=1;(void)eye_flight_fault(&stereo_two.owner,admitted.token);}
        }
    }
    if(stereo_two.backend.owner_thread)
        (void)eye_flight_backend_reconcile(&stereo_two.backend,&stereo_two.owner,stereo_two_issue,stereo_two_callback);
#ifdef GF2VR_MAIN_TAIL_RETIREMENT
    stereo_two_retire_ready(0);
#else
    for(unsigned i=0;i<EFO_CAPACITY;++i) {
        EyeFlightToken t=stereo_two.slots[i].admitted.token;EyeFlightSnapshot n;
#ifdef GF2VR_MAIN_HANDOFF_OBSERVE
        EyeFlightNativeSlot *observed_native=&stereo_two.backend.slots[i];
        if(t.frame && observed_native->render_taken && main_production_render_seen[i]!=t.frame) {
            main_production_render_seen[i]=t.frame;
            main_production_record(t,stereo_two.slots[i].admitted.ticket.serial,2,0,
                stereo_two.slots[i].cpu_ok,0,observed_native->render_result.draw_mask,
                observed_native->render_result.clear_mask);
        }
#endif
#ifdef GF2VR_TWO_FLIGHT_OBSERVE
        EyeFlightNativeSlot *note=&stereo_two.backend.slots[i];
        if(t.frame && note->render_taken && two_observe_main_frame[i]!=t.frame) {
            two_observe_main_frame[i]=t.frame;
            unsigned masks=(note->render_result.draw_mask<<8)|note->render_result.clear_mask;
            two_observe_record(1,"two_flight_source_boundary",t,t.frame,
                stereo_two.slots[i].admitted.ticket.serial,7,EFO_OK,0,masks,masks!=0x303);
        }
#endif
        if(!t.frame || eye_flight_snapshot(&stereo_two.owner,t,&n)!=EFO_OK)continue;
#ifdef GF2VR_ASYNC_THROUGHPUT_OBSERVE
        EyeFlightNativeSlot *timing=&stereo_two.backend.slots[i];
        async_throughput_record(4,t,n.ticket.serial,n.flags,EFO_OK,
            timing->render_submitted|((U64)timing->render_ack<<1)|
            ((U64)timing->copy_started<<2)|((U64)timing->copy_ack<<3));
#endif
        two_root_snapshot(i,t,&n);
#ifdef GF2VR_EVENT_NOTICE_HANDOFF
        /* Early numeric readiness does not transfer main native/source lifetime.
           Before first reclaim, finish main take. Reclaim retries after native
           retire/source-release BUSY must still follow the original path. */
        if(
#ifdef BOUNDARY_TEST
           !event_notice_test_skip_retirement_guard &&
#endif
           !(n.flags&EFF_RECLAIMING) && stereo_two.backend.slots[i].occupied &&
           eye_flight_backend_retirement_ready(&stereo_two.backend,t)!=EFO_OK)continue;
#endif
#ifdef GF2VR_NATIVE_TARGET_CACHE
        EyeFlightNativeSlot *cache_native=&stereo_two.backend.slots[i];
        SourceRenderFlight *cache_source=&stereo_live->flights.slots[i];
        if(eye_native_cache_failed_output(cache_native->render_taken,cache_native->render_result.draw_mask,
            cache_native->render_result.clear_mask,
            stereo_two.slots[i].root_finished && cache_source->frame.injected,stereo_two.slots[i].cpu_ok))
            eye_native_cache_invalidate(&stereo_live->pool.native[i]);
#endif
        if(!(n.flags&EFF_RECLAIMING) && eye_flight_take(&stereo_two.owner,t,&n)!=EFO_OK)continue;
        if(stereo_two.backend.slots[i].occupied &&
           eye_flight_backend_retire(&stereo_two.backend,&stereo_two.owner,t)!=EFO_OK)continue;
        if(source_live_release_flight(stereo_live,&stereo_two.owner,t,GetCurrentThreadId())!=EFO_OK)continue;
#ifdef GF2VR_BANK_ADMISSION_OBSERVE
        bank_admission_observe_record(n.ticket.serial,t.generation,t.epoch,23,t.frame,n.flags,0);
#endif
        int retired=eye_pose_pool_retire(&stereo_two.poses,&stereo_two.owner,t);
#ifdef GF2VR_BANK_ADMISSION_OBSERVE
        bank_admission_observe_record(n.ticket.serial,t.generation,t.epoch,24,t.frame,n.flags,retired);
#endif
        if(retired!=EFO_OK)continue;
        async_throughput_record(5,t,n.ticket.serial,n.flags,EFO_OK,0);
        two_root_retired(i,t.frame);
        __builtin_memset(&stereo_two.slots[i],0,sizeof(stereo_two.slots[i]));
    }
#endif
}
#ifdef GF2VR_MAIN_TAIL_RETIREMENT
/* Same main retirement sequence as owner-entry. Tail may only enter after the
   whole CPU root/context borrow/bookkeeping is closed. No prepare/issue/XR work. */
static void stereo_two_retire_ready(int from_tail) {
    if(!stereo_live||stereo_live->pair_count!=EFO_CAPACITY||GetCurrentThreadId()!=stereo_live->thread)return;
    if(stereo_two_retire_active)return;
    if(from_tail&&(stereo_live->busy||stereo_live->retiring||stereo_pipeline.render.active||
       stereo_submit_scope.context||stereo_submit_scope.depth||stereo_submit_scope.submit_depth||
       stereo_two.root_serial||stereo_two.root_slot!=EFO_CAPACITY))return;
    DWORD saved=GetLastError();stereo_two_retire_active=1;
    for(unsigned i=0;i<EFO_CAPACITY;++i) {
        EyeFlightToken t=stereo_two.slots[i].admitted.token;EyeFlightSnapshot n;
#ifdef GF2VR_MAIN_HANDOFF_OBSERVE
        EyeFlightNativeSlot *observed_native=&stereo_two.backend.slots[i];
        if(t.frame && observed_native->render_taken && main_production_render_seen[i]!=t.frame) {
            main_production_render_seen[i]=t.frame;
            main_production_record(t,stereo_two.slots[i].admitted.ticket.serial,2,0,
                stereo_two.slots[i].cpu_ok,0,observed_native->render_result.draw_mask,
                observed_native->render_result.clear_mask);
        }
#endif
#ifdef GF2VR_TWO_FLIGHT_OBSERVE
        EyeFlightNativeSlot *note=&stereo_two.backend.slots[i];
        if(t.frame && note->render_taken && two_observe_main_frame[i]!=t.frame) {
            two_observe_main_frame[i]=t.frame;
            unsigned masks=(note->render_result.draw_mask<<8)|note->render_result.clear_mask;
            two_observe_record(1,"two_flight_source_boundary",t,t.frame,
                stereo_two.slots[i].admitted.ticket.serial,7,EFO_OK,0,masks,masks!=0x303);
        }
#endif
        if(!t.frame || eye_flight_snapshot(&stereo_two.owner,t,&n)!=EFO_OK)continue;
#ifdef GF2VR_ASYNC_THROUGHPUT_OBSERVE
        EyeFlightNativeSlot *timing=&stereo_two.backend.slots[i];
        async_throughput_record(4,t,n.ticket.serial,n.flags,EFO_OK,
            timing->render_submitted|((U64)timing->render_ack<<1)|
            ((U64)timing->copy_started<<2)|((U64)timing->copy_ack<<3));
#endif
        two_root_snapshot(i,t,&n);
        if(from_tail){SourceRenderFlight *source=&stereo_live->flights.slots[i];
            if(source->occupied&&(source->scope.render.active||source->scope.armed||source->frame.busy||
               (source->frame.serial&&!source->frame.root_done)))continue;}
#ifdef GF2VR_EVENT_NOTICE_HANDOFF
        /* Early numeric readiness does not transfer main native/source lifetime.
           Before first reclaim, finish main take. Reclaim retries after native
           retire/source-release BUSY must still follow the original path. */
        if(
#ifdef BOUNDARY_TEST
           !event_notice_test_skip_retirement_guard &&
#endif
           !(n.flags&EFF_RECLAIMING) && stereo_two.backend.slots[i].occupied &&
           eye_flight_backend_retirement_ready(&stereo_two.backend,t)!=EFO_OK)continue;
#endif
#ifdef GF2VR_NATIVE_TARGET_CACHE
        EyeFlightNativeSlot *cache_native=&stereo_two.backend.slots[i];
        SourceRenderFlight *cache_source=&stereo_live->flights.slots[i];
        if(eye_native_cache_failed_output(cache_native->render_taken,cache_native->render_result.draw_mask,
            cache_native->render_result.clear_mask,
            stereo_two.slots[i].root_finished && cache_source->frame.injected,stereo_two.slots[i].cpu_ok))
            eye_native_cache_invalidate(&stereo_live->pool.native[i]);
#endif
        if(!(n.flags&EFF_RECLAIMING) && eye_flight_take(&stereo_two.owner,t,&n)!=EFO_OK)continue;
        if(stereo_two.backend.slots[i].occupied &&
           eye_flight_backend_retire(&stereo_two.backend,&stereo_two.owner,t)!=EFO_OK)continue;
        if(source_live_release_flight(stereo_live,&stereo_two.owner,t,GetCurrentThreadId())!=EFO_OK)continue;
#ifdef GF2VR_BANK_ADMISSION_OBSERVE
        bank_admission_observe_record(n.ticket.serial,t.generation,t.epoch,23,t.frame,n.flags,0);
#endif
        int retired=eye_pose_pool_retire(&stereo_two.poses,&stereo_two.owner,t);
#ifdef GF2VR_BANK_ADMISSION_OBSERVE
        bank_admission_observe_record(n.ticket.serial,t.generation,t.epoch,24,t.frame,n.flags,retired);
#endif
        if(retired!=EFO_OK)continue;
        async_throughput_record(5,t,n.ticket.serial,n.flags,EFO_OK,0);
        if(from_tail)++stereo_two_tail_retired;
        two_root_retired(i,t.frame);
        __builtin_memset(&stereo_two.slots[i],0,sizeof(stereo_two.slots[i]));
    }
    stereo_two_retire_active=0;SetLastError(saved);
}
#endif
static void stereo_two_render_tick(void) {
#ifdef GF2VR_RENDER_OWNER_COPY
    if(GetCurrentThreadId()==panel_render_tid &&
       __atomic_load_n(&panel_device_ready,__ATOMIC_ACQUIRE) &&
       !__atomic_load_n(&panel_device_changed,__ATOMIC_ACQUIRE))
        eye_flight_backend_service_copies(&stereo_two.backend,panel_device,panel_context);
#endif
    if(GetCurrentThreadId()==panel_render_tid)eye_flight_backend_poll(&stereo_two.backend);
}
