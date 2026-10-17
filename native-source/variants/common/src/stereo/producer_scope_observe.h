#ifndef GF2_PRODUCER_SCOPE_OBSERVE_H
#define GF2_PRODUCER_SCOPE_OBSERVE_H
#ifdef GF2VR_PRODUCER_FIRST_FAULT_OBSERVE
#include "producer_failure_trace.h"
static ProducerFailureTrace producer_failure_trace;
static void producer_scope_failure_trace(uint64_t serial,const char *file,unsigned line) {
    DWORD saved=GetLastError();
    pft_record(&producer_failure_trace,serial,GetCurrentThreadId(),pft_source(file),line);
    SetLastError(saved);
}
#define PSN_FAILURE_TRACE(serial,file,line) producer_scope_failure_trace((serial),(file),(line))
#endif
#include "producer_scope_native.h"
#include "producer_pending.h"
#include "producer_image.h"
#include "producer_bridge_invoke.h"
#include "composition_camera_plan_binding.h"
#ifdef GF2VR_PRODUCER_PREPARE_OWNERSHIP
#include "producer_prepare_lifetime.h"
static ProducerPrepareLifetime producer_prepare;
#endif
#ifdef GF2VR_PRODUCER_PINVOKE_OBSERVE
#include "producer_native_transport.h"
static ProducerNativeTransport producer_transport;
#endif
#ifdef GF2VR_PRODUCER_INSTALL_OBSERVE
#include "producer_install_diagnostic.h"
static void producer_scope_install_failure(void);
#endif
__declspec(dllimport) BOOL WINAPI QueryPerformanceCounter(long long *);
/* Eight actual roots, independent process-lived rows/callback storage. No
   XR/GPU resource/fence operations, extra Submit or product owner decisions.
   Issued is not delivered; delivered is not GPU completion or final pixels. */
#define PSO_CAPACITY 8
#ifdef GF2VR_PRODUCER_TAIL_ORDER_OBSERVE
#include "producer_tail_order_observe.h"
#endif
#ifdef GF2VR_PRODUCER_BORROW_RETURN_FIX
#include "producer_borrow_return.h"
#endif
typedef struct {
    PsnToken token;
    U64 frame,context,child,issued_at,delivered_at,returned_at;
    unsigned render_thread,delivered,callback_thread,callback_queue_ok,cpu_done,cpu_ok;
    unsigned native_result,managed_result,reported,issue_attempts;
#ifdef GF2VR_PRODUCER_TRANSFER
    unsigned sample;
#endif
    unsigned started,ended,tail_attempted,begin_order,end_order,tail_facts,notifications;
    U64 normal_submits;
#ifdef GF2VR_PRODUCER_FIRST_EVENT_OBSERVE
    U64 raw_notify,raw_submit_enter,raw_submit_return,first_submit_context,first_submit_child;
    unsigned first_notify_operation,first_submit_flags;
#endif
#ifdef GF2VR_PRODUCER_FIRST_FAULT_OBSERVE
    unsigned fault_reported;
#endif
#ifdef GF2VR_PRODUCER_TAIL_ORDER_OBSERVE
    ProducerTailOrderObservation tail_order;
#endif
#ifdef GF2VR_PRODUCER_BORROW_RETURN_FIX
    ProducerBorrowReturn borrow_return;
#endif
} ProducerScopeObservation;
static struct {
    unsigned attempted,ready,disabled,count,active,stage;
    CompositionImageApi image_api;
    CompositionImage image;
    CompositionCameraApi camera_api;
    ProducerBridgeInvoker invoker;
    ProducerScopeNative native;
    ProducerScopeObservation rows[PSO_CAPACITY];
} producer_scope;
static U64 producer_scope_thread(void) {return GetCurrentThreadId();}
static int producer_scope_read_value(uintptr_t address,void *out,size_t size) {
    U64 got=0;
    return address && address<=~(U64)0-size &&
        ReadProcessMemory((HANDLE)(long long)-1,(void *)address,out,size,&got) && got==size;
}
static int producer_scope_read_pending(void *owner,uint64_t address,void *out,size_t size) {
    (void)owner;return producer_scope_read_value((uintptr_t)address,out,size);
}
static unsigned producer_scope_find(void **out) {
    *out=0;void *domain=sa.domain_get();if(!domain)return 2;
    U64 count=0;void **assemblies=sa.domain_get_assemblies(domain,&count);
    if(!assemblies || count>4096)return 2;
    unsigned matches=0;
    for(U64 i=0;i<count;++i) {
        void *image=sa.assembly_get_image(assemblies[i]);
        if(image && source_equal(sa.image_get_name(image),"GF2.Performance.ProducerBridge.dll")) {
            ++matches;*out=image;if(matches>1)return matches;
        }
    }return matches;
}
static int producer_scope_absent(void *owner) {(void)owner;void *image=0;return producer_scope_find(&image)==0;}
static void *producer_scope_resolve(void *owner) {
    (void)owner;void *image=0;return producer_scope_find(&image)==1?
        sa.class_from_name(image,"GF2.Performance","ProducerScopeBridge"):0;
}
static void producer_scope_stage(unsigned stage,unsigned detail) {
    DWORD saved=GetLastError();producer_scope.stage=stage;
#ifdef GF2VR_PRODUCER_TRANSFER
    if(stage==7 && producer_scope.count>=PSO_CAPACITY){SetLastError(saved);return;}
#endif
    event("producer_scope_prepare",((U64)stage<<32)|detail,GetCurrentThreadId());SetLastError(saved);
}
static void producer_scope_callback(int id) {
    DWORD saved=GetLastError();
    unsigned index=(unsigned)id-0x65310000u;
    if(index<PSO_CAPACITY) {
        ProducerScopeObservation *r=&producer_scope.rows[index];
        if(r->token.serial && !__atomic_load_n(&r->delivered,__ATOMIC_ACQUIRE)) {
            U64 child=(U64)draw_runtime_graphics();long long stamp=0;
            r->callback_thread=GetCurrentThreadId();r->callback_queue_ok=child==r->child && child!=0;
            if(QueryPerformanceCounter(&stamp))r->delivered_at=(U64)stamp;
            __atomic_store_n(&r->delivered,1,__ATOMIC_RELEASE);
        }
    }SetLastError(saved);
}
static int producer_scope_issue(void *owner,PsnToken token) {
    ProducerScopeObservation *r=owner;
    if(!r || !psn_same(r->token,token) || r->issue_attempts || !stereo_interval_entry())return PSN_NOT_QUEUED;
    unsigned index=(unsigned)(r-producer_scope.rows);if(index>=PSO_CAPACITY)return PSN_NOT_QUEUED;
    ++r->issue_attempts;long long stamp=0;
    if(QueryPerformanceCounter(&stamp))r->issued_at=(U64)stamp;
#ifdef GF2VR_PRODUCER_TRANSFER
    unsigned slot=stereo_two.root_slot;
    if(slot>=EFO_CAPACITY || stereo_two.root_serial!=token.serial)return PSN_NOT_QUEUED;
    int result=eye_flight_backend_producer_end(&stereo_two.backend,&stereo_two.owner,slot,1,
        stereo_two_issue,stereo_two_callback);
    if(result==EFO_OK)return PSN_QUEUED;
    return stereo_two.backend.slots[slot].producer_end_issued?PSN_UNCERTAIN:PSN_NOT_QUEUED;
#else
    stereo_two_issue(producer_scope_callback,(int)(0x65310000u+index));
    /* Existing ordered issuer called once. Only a later actual callback proves
       delivery. This probe result never changes backend end_issued/fresh. */
    return PSN_QUEUED;
#endif
}
#ifdef GF2VR_PRODUCER_TAIL_ORDER_OBSERVE
static int producer_scope_tail_ordered(ProducerScopeObservation *r,DrawQueueIdentity *now,
                                      const DrawQueueIdentity *entry) {
    ProducerTailOrderObservation *o=&r->tail_order;
    *o=(ProducerTailOrderObservation){.observed=1,.depth=stereo_submit_scope.depth,
        .submit_depth=stereo_submit_scope.submit_depth,.ambiguous=stereo_submit_scope.ambiguous};
    /* Original operands and order, no forced identity/getter/issuer calls. */
    int ordered=stereo_submit_scope.depth==1 && !stereo_submit_scope.submit_depth &&
        !stereo_submit_scope.ambiguous && PTO_RESULT(o,identity,draw_runtime_identity(now)) &&
        PTO_RESULT(o,queue,(o->expected=*entry,draw_queue_same(o->expected,*now))) &&
        PTO_RESULT(o,issuer,stereo_interval_entry());
    o->actual=*now;return ordered;
}
static void producer_scope_tail_log(ProducerScopeObservation *r) {
#ifdef GF2VR_NUMERIC_RESIDENCY
    (void)r;return; /* Keep original ordering guard, disable only legacy IO. */
#endif
#line 154
    ProducerTailOrderObservation *o=&r->tail_order;
    if(!o->observed || o->reported)return;
    o->reported=1;
    char line[1600],*p=source_prefix(line,"producer_tail_order_boundary");
#define PTO_WRITE(k,v) p=literal(p,",\"" k "\":");p=decimal(p,(U64)(v))
    PTO_WRITE("serial",r->token.serial);PTO_WRITE("generation",r->token.generation);
    PTO_WRITE("epoch",r->token.epoch);PTO_WRITE("frame",r->frame);
    PTO_WRITE("depth",o->depth);PTO_WRITE("submit_depth",o->submit_depth);PTO_WRITE("ambiguous",o->ambiguous);
    PTO_WRITE("first_guard",pto_first(o));
    PTO_WRITE("identity_reached",o->identity_reached);PTO_WRITE("identity_result",o->identity_result);
    PTO_WRITE("queue_reached",o->queue_reached);PTO_WRITE("queue_result",o->queue_result);
    PTO_WRITE("issuer_reached",o->issuer_reached);PTO_WRITE("issuer_result",o->issuer_result);
    PTO_WRITE("expected_graphics",o->expected.graphics);PTO_WRITE("expected_queue",o->expected.queue);
    PTO_WRITE("expected_child",o->expected.child);PTO_WRITE("expected_mode",o->expected.mode);
    PTO_WRITE("expected_dispatch",o->expected.dispatch_state);
    PTO_WRITE("actual_graphics",o->actual.graphics);PTO_WRITE("actual_queue",o->actual.queue);
    PTO_WRITE("actual_child",o->actual.child);PTO_WRITE("actual_mode",o->actual.mode);
    PTO_WRITE("actual_dispatch",o->actual.dispatch_state);
#undef PTO_WRITE
    p=literal(p,"}\n");source_log(line,p);
}
#endif
#ifdef GF2VR_PRODUCER_BORROW_RETURN_FIX
static int producer_scope_tail_execute(ProducerScopeObservation *r,int camera) {
    PsnBarrierFacts facts={0};DrawQueueIdentity now={0};
    if(stereo_live && stereo_two.root_slot<EFO_CAPACITY) {
        SourceRenderFlight *s=&stereo_live->flights.slots[stereo_two.root_slot];
        facts.token=r->token;facts.context=(U64)stereo_submit_scope.context;
        facts.current=stereo_pipeline.render.active && stereo_pipeline.render.serial==r->token.serial &&
            stereo_two.root_serial==r->token.serial && s->token.generation==r->token.generation &&
            s->token.epoch==r->token.epoch && s->token.frame==r->frame && stereo_two_frame_current(s);
        facts.ordered=producer_scope_tail_ordered(r,&now,&stereo_two.slots[stereo_two.root_slot].entry);
        facts.child=now.child;
        facts.empty_pending=psn_pending_empty(facts.context,producer_scope_read_pending,0);
        r->tail_facts=facts.current|(facts.ordered<<1)|(facts.empty_pending<<2)|
            ((unsigned)(facts.context==r->context)<<3)|((unsigned)(facts.child==r->child)<<4);
    }
    return psn_notify(&producer_scope.native,GetCurrentThreadId(),r->token.serial,
        PSN_TAIL,camera,&facts,producer_scope_issue,r);
}
static void producer_scope_after_nested(unsigned selected) {
    if(GetCurrentThreadId()!=stereo_pipeline.owner_thread || !producer_scope.active ||
       producer_scope.active>PSO_CAPACITY)return;
    DWORD saved=GetLastError();
    ProducerScopeObservation *r=&producer_scope.rows[producer_scope.active-1];int camera=0;
    if(!r->borrow_return.pending || stereo_submit_scope.depth!=1) {
        if(!stereo_submit_scope.depth)pbr_cancel(&r->borrow_return);
        SetLastError(saved);return;
    }
    unsigned valid=!selected && stereo_submit_scope.serial==r->token.serial &&
        stereo_pipeline.render.active && stereo_pipeline.render.serial==r->token.serial &&
        psn_same(r->token,producer_scope.native.token) &&
        psn_current(&producer_scope.native,GetCurrentThreadId(),r->token.serial);
    if(pbr_take(&r->borrow_return,r->token,(U64)stereo_submit_scope.context,
        stereo_submit_scope.depth,stereo_submit_scope.submit_depth,stereo_submit_scope.ambiguous,valid,&camera))
        (void)producer_scope_tail_execute(r,camera);
    SetLastError(saved);
}
static void producer_scope_borrow_log(ProducerScopeObservation *r) {
#ifdef GF2VR_NUMERIC_RESIDENCY
    (void)r;return; /* Keep original ordering guard, disable only legacy IO. */
#endif
#line 213
    ProducerBorrowReturn *b=&r->borrow_return;
    if(!b->request_depth || b->reported)return;
    b->reported=1;char line[640],*p=source_prefix(line,"producer_borrow_return_result");
#define PBR_WRITE(k,v) p=literal(p,",\"" k "\":");p=decimal(p,(U64)(v))
    PBR_WRITE("serial",r->token.serial);PBR_WRITE("generation",r->token.generation);
    PBR_WRITE("epoch",r->token.epoch);PBR_WRITE("frame",r->frame);
    PBR_WRITE("request_depth",b->request_depth);PBR_WRITE("pending",b->pending);
    PBR_WRITE("consumed",b->consumed);PBR_WRITE("cancelled",b->cancelled);
    PBR_WRITE("ordered_observed",r->tail_order.observed);PBR_WRITE("ordered_depth",r->tail_order.depth);
    PBR_WRITE("first_guard",pto_first(&r->tail_order));
#undef PBR_WRITE
    p=literal(p,"}\n");source_log(line,p);
}
#endif
static int producer_scope_notify(long long serial,int operation,int camera) {
    DWORD saved=GetLastError();int result=0;
#ifdef GF2VR_ACTIVITY_CANVAS_SCOPE
    if(!primary_ui_canvas_before_notify((U64)serial,operation,camera)) {
        psn_fail(&producer_scope.native,(U64)serial);SetLastError(saved);return 0;
    }
#endif
#ifdef GF2VR_PRODUCER_FIRST_EVENT_OBSERVE
    if(GetCurrentThreadId()==stereo_pipeline.owner_thread && producer_scope.active && producer_scope.active<=PSO_CAPACITY) {
        ProducerScopeObservation *raw=&producer_scope.rows[producer_scope.active-1];
        if(!raw->raw_notify)raw->first_notify_operation=(unsigned)operation;
        ++raw->raw_notify;
    }
#endif
    if(psn_current(&producer_scope.native,GetCurrentThreadId(),(U64)serial)) {
        PsnBarrierFacts facts={0};ProducerScopeObservation *r=0;
        if(producer_scope.active && producer_scope.active<=PSO_CAPACITY)r=&producer_scope.rows[producer_scope.active-1];
        if(r) {
            ++r->notifications;
            unsigned eye=camera==producer_scope.native.cameras[0]?1u:camera==producer_scope.native.cameras[1]?2u:0;
            if(operation==PSN_BEGIN)r->begin_order=r->begin_order*4+eye;
            if(operation==PSN_END)r->end_order=r->end_order*4+eye;
        }
#ifdef GF2VR_PRODUCER_BORROW_RETURN_FIX
        if(operation==PSN_TAIL && r) {
            ProducerScopeNative *s=&producer_scope.native;
            if(r->borrow_return.pending || r->borrow_return.request_depth) {
                psn_fail(s,(U64)serial);SetLastError(saved);return 0;
            }
            int complete=psn_same(r->token,s->token) && camera && camera!=s->cameras[0] &&
                camera!=s->cameras[1] && !s->open && s->started==3 && s->ended==3 &&
                !s->submit_depth && !s->tail_attempted;
            if(pbr_defer(&r->borrow_return,r->token,(U64)stereo_submit_scope.context,camera,
                stereo_submit_scope.depth,stereo_submit_scope.submit_depth,stereo_submit_scope.ambiguous,complete)) {
                /* Managed tail rejection remains harmless fallback. Only the
                   exact outer borrow's later depth1 return may attempt it. */
                SetLastError(saved);return 0;
            }
            result=producer_scope_tail_execute(r,camera);SetLastError(saved);return result;
        }
#else
        if(operation==PSN_TAIL && r && stereo_live && stereo_two.root_slot<EFO_CAPACITY) {
            DrawQueueIdentity now={0};SourceRenderFlight *s=&stereo_live->flights.slots[stereo_two.root_slot];
            facts.token=r->token;facts.context=(U64)stereo_submit_scope.context;
            facts.current=stereo_pipeline.render.active && stereo_pipeline.render.serial==r->token.serial &&
                stereo_two.root_serial==r->token.serial && s->token.generation==r->token.generation &&
                s->token.epoch==r->token.epoch && s->token.frame==r->frame && stereo_two_frame_current(s);
#ifdef GF2VR_PRODUCER_TAIL_ORDER_OBSERVE
            facts.ordered=producer_scope_tail_ordered(r,&now,&stereo_two.slots[stereo_two.root_slot].entry);
#else
            facts.ordered=stereo_submit_scope.depth==1 && !stereo_submit_scope.submit_depth &&
                !stereo_submit_scope.ambiguous && draw_runtime_identity(&now) &&
                draw_queue_same(stereo_two.slots[stereo_two.root_slot].entry,now) && stereo_interval_entry();
#endif
            facts.child=now.child;
            facts.empty_pending=psn_pending_empty(facts.context,producer_scope_read_pending,0);
            r->tail_facts=facts.current|(facts.ordered<<1)|(facts.empty_pending<<2)|
                ((unsigned)(facts.context==r->context)<<3)|((unsigned)(facts.child==r->child)<<4);
        }
#endif
        result=psn_notify(&producer_scope.native,GetCurrentThreadId(),(U64)serial,operation,camera,
            operation==PSN_TAIL?&facts:0,producer_scope_issue,r);
#ifdef GF2VR_ACTIVITY_CANVAS_SCOPE
        result=primary_ui_canvas_after_notify((U64)serial,operation,camera,result);
        if(!result && operation==PSN_MEMBER_BEGIN)psn_fail(&producer_scope.native,(U64)serial);
#endif
    }SetLastError(saved);return result;
}
#ifdef GF2VR_PRODUCER_PINVOKE_OBSERVE
/* One owned primitive ABI, exported only by this diagnostic macro. No
   delegate conversion, MethodInfo ABI, shared icall registry or resources. */
__declspec(dllexport) int gf2vr_producer_notify(long long serial,int operation,int camera) {
    DWORD saved=GetLastError();int result=0;U64 thread=GetCurrentThreadId();
    if(operation==0)result=pnt_handshake(&producer_transport,thread,(U64)serial,camera);
    else if(pnt_forward(&producer_transport,thread))result=producer_scope_notify(serial,operation,camera);
    SetLastError(saved);return result;
}
#ifdef BOUNDARY_TEST
__declspec(dllexport) int producer_pinvoke_transport_test(void) {
    DWORD saved_error=GetLastError();ProducerNativeTransport saved=producer_transport;
    int line=0;U64 thread=GetCurrentThreadId();
#define PNT_CHECK(x) do {if(!(x)){line=__LINE__;goto done;}}while(0)
    producer_transport=(ProducerNativeTransport){0};
    PNT_CHECK(pnt_initialize(&producer_transport,thread,0x100000005ULL));
    SetLastError(703);PNT_CHECK(gf2vr_producer_notify(5,0,PNT_MAGIC)==0 && GetLastError()==703);
    PNT_CHECK(gf2vr_producer_notify(0x100000005LL,0,PNT_MAGIC)==0);
    producer_transport=(ProducerNativeTransport){0};
    PNT_CHECK(pnt_initialize(&producer_transport,thread,0x100000005ULL));
    SetLastError(704);PNT_CHECK(gf2vr_producer_notify(0x100000005LL,0,PNT_MAGIC)==PNT_MAGIC && GetLastError()==704);
    PNT_CHECK(gf2vr_producer_notify(0x100000005LL,0,PNT_MAGIC)==0);
    PNT_CHECK(gf2vr_producer_notify(0x100000005LL,PSN_BEGIN,100)==0 && GetLastError()==704);
done:
    producer_transport=saved;SetLastError(saved_error);return line;
#undef PNT_CHECK
}
#endif
#endif
#ifdef GF2VR_PRODUCER_ENGINE_OBSERVE
#include "producer_engine_adapter.h"
#endif
#ifdef GF2VR_PRODUCER_FIRST_EVENT_OBSERVE
#include "producer_first_event_observe.h"
#endif
static int producer_scope_setup(void) {
    if(producer_scope.attempted)return producer_scope.ready;
    producer_scope.attempted=1;
    if(!composition_image_bind(&producer_scope.image_api) || !composition_camera_bind(&producer_scope.camera_api)) {
        producer_scope_stage(1,0);return 0;
    }
    producer_scope.image_api.image_absent=producer_scope_absent;
    producer_scope.image_api.resolve_class=producer_scope_resolve;
    if(!producer_image_load(&producer_scope.image_api,&producer_scope.image,gf2_producer_bridge_package,
       sizeof(gf2_producer_bridge_package),1)) {producer_scope_stage(2,0);return 0;}
    HINSTANCE game=GetModuleHandleW(L"GameAssembly.dll");
    unsigned char (*generic)(void *)=game?(void *)GetProcAddress(game,"il2cpp_method_is_generic"):0;
    unsigned char (*inflated)(void *)=game?(void *)GetProcAddress(game,"il2cpp_method_is_inflated"):0;
    void *(*declaring)(void *)=game?(void *)GetProcAddress(game,"il2cpp_method_get_class"):0;
    int (*size)(void *,unsigned *)=game?(void *)GetProcAddress(game,"il2cpp_class_value_size"):0;
    void *context=render_class("UnityEngine.CoreModule.dll","UnityEngine.Rendering","ScriptableRenderContext");
    unsigned alignment=0;
    if(!size || !context || size(context,&alignment)!=(int)sizeof(uintptr_t) ||
       !alignment || alignment>_Alignof(uintptr_t) ||
       !pbi_bind(&producer_scope.invoker.methods,producer_scope.image.bridge_class,1,generic,inflated,declaring)) {
        producer_scope_stage(3,0);return 0;
    }
    ProducerBridgeInvoker *a=&producer_scope.invoker;
    a->owner=GetCurrentThreadId();a->integer=producer_scope.camera_api.int_class;
    a->camera_class=producer_scope.camera_api.camera_class;a->camera_array_class=producer_scope.camera_api.array_class;
    a->thread=producer_scope_thread;a->object_class=sa.object_get_class;a->array_length=sa.array_length;
    a->invoke=sa.runtime_invoke;a->unbox=sa.object_unbox;a->pin=sa.gchandle_new;a->unpin=sa.gchandle_free;
    a->read=producer_scope_read_value;
#ifdef GF2VR_PRODUCER_PINVOKE_OBSERVE
    if(!pnt_initialize(&producer_transport,a->owner,(U64)producer_scope_notify)) {
        producer_scope_stage(9,0);return 0;
    }
#endif
#ifdef GF2VR_PRODUCER_ENGINE_OBSERVE
    if(!producer_engine_bind(producer_scope.image.bridge_class,a->owner)) {
        producer_scope_stage(10,producer_engine.reason);return 0;
    }
    uintptr_t install_callback=(uintptr_t)producer_engine_noop;
#else
    uintptr_t install_callback=(uintptr_t)producer_scope_notify;
#endif
    int result=0;int called=pbi_invoke(a,PBI_INSTALL,0,install_callback,0,0,0,0,0,&result);
    producer_scope_stage(4,(called?1u:0u)|((unsigned)(result==1)<<1));
    producer_scope.ready=called && result==1;
#ifdef GF2VR_PRODUCER_ENGINE_OBSERVE
    if(!producer_engine.transport.ready)producer_scope.ready=0;
    event("producer_engine_contract",((U64)producer_engine.entries<<32)|
        (producer_engine.transport.ready?1u:0u),GetCurrentThreadId());
#endif
#ifdef GF2VR_PRODUCER_FIRST_EVENT_OBSERVE
    if(producer_scope.ready)producer_first_event_bind();
#endif
    return producer_scope.ready;
}
#ifdef GF2VR_PRODUCER_INSTALL_OBSERVE
static void producer_scope_install_failure(void) {
    DWORD saved=GetLastError();HINSTANCE game=GetModuleHandleW(L"GameAssembly.dll");
    unsigned char (*generic)(void *)=game?(void *)GetProcAddress(game,"il2cpp_method_is_generic"):0;
    unsigned char (*inflated)(void *)=game?(void *)GetProcAddress(game,"il2cpp_method_is_inflated"):0;
    void *(*declaring)(void *)=game?(void *)GetProcAddress(game,"il2cpp_method_get_class"):0;
    int (*length)(void *)=game?(void *)GetProcAddress(game,"il2cpp_string_length"):0;
    const uint16_t *(*chars)(void *)=game?(void *)GetProcAddress(game,"il2cpp_string_chars"):0;
    ProducerInstallMethods methods={0};ProducerInstallDiagnostic d={0};
    int bound=pid_bind(&methods,producer_scope.image.bridge_class,generic,inflated,declaring);
    void *string_class=render_class("mscorlib.dll","System","String");
    if(bound)pid_capture(&producer_scope.invoker,methods,string_class,length,chars,&d);
    char line[3000],*p=source_prefix(line,"producer_install_diagnostic");
#define PID_WRITE(k,v) p=literal(p,",\"" k "\":");p=decimal(p,(U64)(v))
    PID_WRITE("methods_bound",bound);PID_WRITE("progress_read",d.progress_read);PID_WRITE("progress",d.progress);
    PID_WRITE("text_read",d.text_read);PID_WRITE("text_length",d.length);PID_WRITE("truncated",d.truncated);
    p=literal(p,",\"utf16\":[");
    for(unsigned i=0;i<d.copied;++i){if(i)p=literal(p,",");p=decimal(p,d.units[i]);}
    p=literal(p,"]}\n");source_log(line,p);
#undef PID_WRITE
    SetLastError(saved);
}
#endif
/* Cold setup is scheduled at the root entry BEFORE any private reservation.
   A later Arm can use ready metadata only. Pending callbacks retain the same
   process-lived storage; stopping observations never releases product work. */
#ifdef GF2VR_PRODUCER_PREPARE_OWNERSHIP
static int producer_prepare_quiescent(void) {
    if(GetCurrentThreadId()!=stereo_pipeline.owner_thread)return 0;
    /* enabled is the production route selector, initialized to one; it says
       nothing about a borrow. Prepare only before the private source owner
       opens, while the main owner cannot activate new flights reentrantly. */
    if(stereo_pipeline.render.active || stereo_live || stereo_start_owner.thread ||
       stereo_start.busy || stereo_two.root_serial || stereo_source_frame.serial ||
       stereo_submit_scope.depth || stereo_submit_scope.submit_depth)return 0;
    /* The flight owner is shared with XR. Read its numeric state under the
       existing nonblocking lock; BUSY or any retained flight blocks setup.
       Inactive generation plus unopened main source prevents a new offer. */
    unsigned free_slots=0;U64 generation=0,epoch=0;
    return eye_flight_available(&stereo_two.owner,&free_slots,&generation,&epoch)==EFO_OK &&
        free_slots==EFO_CAPACITY && !generation && !epoch;
}
static int producer_prepare_install(void *context) {
    (void)context;
    event("producer_prepare_begin",0,GetCurrentThreadId());
    int okay=producer_scope_setup();
    if(!okay && producer_scope.stage==4)producer_scope_install_failure();
    event("producer_prepare_ready",okay!=0,GetCurrentThreadId());return okay;
}
static int producer_prepare_detach(void *context) {
    (void)context;int result=0;
    int called=pbi_invoke(&producer_scope.invoker,PBI_DETACH,0,0,0,0,0,0,0,&result);
    producer_scope_stage(8,(called?1u:0u)|((unsigned)(result==1)<<1));
    return called && result==1;
}
static const ProducerPrepareOps producer_prepare_ops={0,producer_prepare_install,producer_prepare_detach};
static void producer_scope_preflight(void) {
    DWORD error=GetLastError();U64 thread=GetCurrentThreadId();
    if(thread!=stereo_pipeline.owner_thread)return;
    if(!__atomic_load_n(&producer_prepare.owner,__ATOMIC_ACQUIRE))
        (void)pps_init(&producer_prepare,thread);
    if(!producer_prepare.attempted && producer_prepare_quiescent()) {
        if(!pps_prepare(&producer_prepare,thread,1,&producer_prepare_ops))producer_scope.disabled=1;
    }
    SetLastError(error);
}
static void producer_scope_stop(void) {
    DWORD error=GetLastError();
    (void)pps_stop(&producer_prepare,GetCurrentThreadId(),&producer_prepare_ops);
    producer_scope.disabled=1;producer_scope.ready=0;
    SetLastError(error);
}
#endif
static int producer_scope_camera_id(void *camera,int *id) {
    *id=0;void *exception=0;
    void *box=sa.runtime_invoke(producer_scope.camera_api.get_instance_id,camera,0,&exception);
    if(exception || !box)return 0;
    unsigned root=sa.gchandle_new(box,1);if(!root)return 0;
    int okay=0;
    if(sa.object_get_class(box)==producer_scope.camera_api.int_class) {
        void *value=sa.object_unbox(box);if(value)okay=producer_scope_read_value((uintptr_t)value,id,sizeof(*id)) && *id;
    }sa.gchandle_free(root);return okay;
}
static void producer_scope_arm(void *context) {
    DWORD saved=GetLastError();
    if(producer_scope.disabled || producer_scope.active ||
#ifndef GF2VR_PRODUCER_TRANSFER
       producer_scope.count>=PSO_CAPACITY ||
#endif
       !context || !stereo_live || stereo_two.root_slot>=EFO_CAPACITY || stereo_submit_scope.depth ||
       GetCurrentThreadId()!=stereo_pipeline.owner_thread || !stereo_two.original_cameras)return;
#ifdef GF2VR_PRODUCER_PREPARE_OWNERSHIP
    if(!producer_prepare.ready || producer_prepare.stopped)return;
#else
    if(!producer_scope_setup()) {
#ifdef GF2VR_PRODUCER_INSTALL_OBSERVE
        // Only the newly observed cold Install boundary; once per process.
        if(producer_scope.stage==4)producer_scope_install_failure();
#endif
        producer_scope.disabled=1;SetLastError(saved);return;
    }
#endif
    unsigned index=stereo_two.root_slot;SourceRenderFlight *s=&stereo_live->flights.slots[index];
    PsnToken token={stereo_two.root_serial,s->token.generation,s->token.epoch};
    int left=0,right=0;
#ifdef GF2VR_PRODUCER_PREPARE_OWNERSHIP
    unsigned admission=0;
    if(!stereo_two_frame_current(s))admission=1;
    else if(!producer_scope_camera_id(s->scope.left,&left))admission=2;
    else if(!producer_scope_camera_id(s->scope.right,&right))admission=3;
    if(admission) {
#ifdef GF2VR_PRODUCER_TRANSFER
        /* A transient source/owner contention is root fallback, not the end
           of the process-pinned metadata/subscription lifetime. */
        SetLastError(saved);return;
#else
        producer_scope_stage(5,admission);producer_scope_stop();SetLastError(saved);return;
#endif
    }
#else
    if(!stereo_two_frame_current(s) || !producer_scope_camera_id(s->scope.left,&left) ||
       !producer_scope_camera_id(s->scope.right,&right)) {
        producer_scope_stage(5,0);producer_scope.disabled=1;SetLastError(saved);return;
    }
#endif
#ifdef GF2VR_PRODUCER_TRANSFER
    /* Product callback IDs/storage belong to the backend flight, never this
       main-only reusable scalar row. Sampling does not limit admission. */
    ProducerScopeObservation *r=&producer_scope.rows[0];
    *r=(ProducerScopeObservation){0};r->sample=producer_scope.count<PSO_CAPACITY;
#else
    ProducerScopeObservation *r=&producer_scope.rows[producer_scope.count];
#endif
    r->token=token;r->frame=s->token.frame;r->context=(U64)context;
    r->child=stereo_two.slots[index].entry.child;r->render_thread=panel_render_tid;
#ifdef GF2VR_PRODUCER_FIRST_FAULT_OBSERVE
    (void)pft_bind(&producer_failure_trace,producer_scope.count,token.serial);
#endif
    if(!psn_arm(&producer_scope.native,GetCurrentThreadId(),token,r->context,r->child,left,right)) {
#ifdef GF2VR_PRODUCER_PREPARE_OWNERSHIP
        producer_scope_stage(6,0);producer_scope_stop();SetLastError(saved);return;
#else
        producer_scope_stage(6,0);producer_scope.disabled=1;SetLastError(saved);return;
#endif
    }
#ifdef GF2VR_PRODUCER_PREPARE_OWNERSHIP
    if(!pps_activate(&producer_prepare,GetCurrentThreadId())) {
        (void)psn_finish(&producer_scope.native,GetCurrentThreadId(),token.serial,0);
        producer_scope_stage(6,1);producer_scope_stop();SetLastError(saved);return;
    }
#endif
#ifdef GF2VR_PRODUCER_TRANSFER
    if(producer_scope.count<PSO_CAPACITY)++producer_scope.count;
    producer_scope.active=1;
#else
    ++producer_scope.count;producer_scope.active=producer_scope.count;
#endif
    int result=0;int called=pbi_invoke(&producer_scope.invoker,PBI_ARM,token.serial,0,(uintptr_t)context,
        s->scope.left,s->scope.right,stereo_two.original_cameras,0,&result);
    producer_scope_stage(7,(called?1u:0u)|((unsigned)(result==1)<<1));
#ifdef GF2VR_TOUCH_PRIVATE
    if(called && result==1)touch_p2_draw(s);
#endif
#line 544
#ifdef GF2VR_ACTIVITY_CANVAS_SCOPE
    if(called && result==1 && !primary_ui_canvas_prepare(token.serial))psn_fail(&producer_scope.native,token.serial);
#endif
#ifdef GF2VR_UI_DRAW_PREPARE
    if(called && result==1)GF2VR_UI_DRAW_PREPARE(token.serial);
#endif
#ifdef GF2VR_PRODUCER_FIRST_EVENT_OBSERVE
    if(called && result==1)producer_first_event_snapshot(r,1);
#endif
    if(!called || result!=1) {
        /* Even an exception may have followed a partial Arm. Try owner Finish
           once; never assume null/exception means no managed side effect. */
        int ignored=0;(void)pbi_invoke(&producer_scope.invoker,PBI_FINISH,token.serial,0,0,0,0,0,0,&ignored);
        r->native_result=psn_finish(&producer_scope.native,GetCurrentThreadId(),token.serial,0);
        r->cpu_done=1;producer_scope.active=0;producer_scope.disabled=1;
#ifdef GF2VR_PRODUCER_PREPARE_OWNERSHIP
        (void)pps_finish(&producer_prepare,GetCurrentThreadId(),1,&producer_prepare_ops);
        producer_scope.ready=0;
#endif
    }
    SetLastError(saved);
}
static void producer_scope_submit(int enter,void *context,int valid) {
    if(!producer_scope.active)return;
    ProducerScopeNative *s=&producer_scope.native;
#ifdef GF2VR_PRODUCER_FIRST_EVENT_OBSERVE
    ProducerScopeObservation *raw=producer_scope.active<=PSO_CAPACITY?&producer_scope.rows[producer_scope.active-1]:0;
    unsigned first=raw && enter && !raw->raw_submit_enter;
    if(raw) {
        if(enter)++raw->raw_submit_enter;else ++raw->raw_submit_return;
        if(first) {
            raw->first_submit_context=(U64)context;
            raw->first_submit_flags=(!valid?1u:0u)|((unsigned)((U64)context!=s->context)<<1)|
                ((unsigned)(__atomic_load_n(&s->failed_serial,__ATOMIC_ACQUIRE)>=s->token.serial)<<3)|
                ((unsigned)(s->submit_depth!=0)<<4);
        }
    }
#endif
    if(!valid) {psn_fail(s,s->token.serial);return;}
    DrawQueueIdentity now={0};U64 child=draw_runtime_identity(&now)?now.child:0;
#ifdef GF2VR_PRODUCER_FIRST_EVENT_OBSERVE
    if(first) {
        raw->first_submit_child=child;
        raw->first_submit_flags|=(unsigned)(child!=s->child)<<2;
    }
#endif
    if(enter)(void)psn_submit_enter(s,GetCurrentThreadId(),s->token,(U64)context,child);
    else (void)psn_submit_return(s,GetCurrentThreadId(),s->token,(U64)context,child,1);
#ifdef GF2VR_PRODUCER_FIRST_EVENT_OBSERVE
    if(first && __atomic_load_n(&s->failed_serial,__ATOMIC_ACQUIRE)>=s->token.serial)
        raw->first_submit_flags|=32;
#endif
}
static void producer_scope_foreign(void) {
    /* No foreign thread reads root/row/camera/context payload. */
    U64 serial=__atomic_load_n(&producer_scope.native.active,__ATOMIC_ACQUIRE);
    if(serial)psn_fail(&producer_scope.native,serial);
}
#ifdef GF2VR_PRODUCER_FIRST_FAULT_OBSERVE
static void producer_scope_foreign_trace(const char *file,unsigned line) {
    /* Identical foreign invalidation, with its caller site attached to the
       actual CAS winner. Atomic active ticket is the only root fact read. */
    U64 serial=__atomic_load_n(&producer_scope.native.active,__ATOMIC_ACQUIRE);
    if(serial)psn_fail_traced(&producer_scope.native,serial,file,line);
}
#endif
static void producer_scope_finish(U64 serial,int failed) {
#ifdef GF2VR_ACTIVITY_CANVAS_SCOPE
    DWORD canvas_saved=GetLastError();
    if(!primary_ui_canvas_finish(serial))failed=1;
    SetLastError(canvas_saved);
#endif
#ifdef GF2VR_UI_DRAW_RETURN
    GF2VR_UI_DRAW_RETURN(serial);
#endif
#ifdef GF2VR_PRODUCER_PREPARE_OWNERSHIP
    if(!producer_scope.active) {
#ifndef GF2VR_PRODUCER_TRANSFER
        if(failed && pps_owner(&producer_prepare,GetCurrentThreadId()) && producer_prepare.ready)
            producer_scope_stop();
#endif
        return;
    }
#endif
    if(!producer_scope.active || producer_scope.active>PSO_CAPACITY)return;
    ProducerScopeObservation *r=&producer_scope.rows[producer_scope.active-1];if(r->token.serial!=serial)return;
#ifdef GF2VR_PRODUCER_BORROW_RETURN_FIX
    pbr_cancel(&r->borrow_return);
#endif
    DWORD saved=GetLastError();int result=0;
    int cpu=!failed && !stereo_submit_scope.depth && !stereo_submit_scope.submit_depth &&
        !stereo_submit_scope.ambiguous && stereo_submit_scope.returned_serial==serial;
#ifdef GF2VR_PRODUCER_FIRST_EVENT_OBSERVE
    producer_first_event_snapshot(r,2);
#endif
    int called=pbi_invoke(&producer_scope.invoker,PBI_FINISH,serial,0,0,0,0,0,cpu,&result);
    r->managed_result=(unsigned)result;r->cpu_ok=cpu!=0;r->native_result=
        psn_finish(&producer_scope.native,GetCurrentThreadId(),serial,called && result>0 && cpu);
    r->started=producer_scope.native.started;r->ended=producer_scope.native.ended;
    r->tail_attempted=producer_scope.native.tail_attempted;r->normal_submits=producer_scope.native.submit_sequence;
    long long stamp=0;if(QueryPerformanceCounter(&stamp))r->returned_at=(U64)stamp;
    r->cpu_done=1;producer_scope.active=0;
#ifdef GF2VR_PRODUCER_TRANSFER
    if(stereo_two.root_slot<EFO_CAPACITY &&
       stereo_two.backend.slots[stereo_two.root_slot].producer_end_issued &&
       !(r->native_result&1))stereo_two.slots[stereo_two.root_slot].producer_failed=1;
    /* Finish always clears a normal owner scope, including a failed root.
       Unknown invocation/cleanup still stops the bridge, not the legacy mode. */
    if(!called)producer_scope.disabled=1;
#else
    if(!called || result==0)producer_scope.disabled=1;
#endif
#ifdef GF2VR_PRODUCER_PREPARE_OWNERSHIP
    (void)pps_finish(&producer_prepare,GetCurrentThreadId(),
        producer_scope.disabled
#ifndef GF2VR_PRODUCER_TRANSFER
        || producer_scope.count>=PSO_CAPACITY
#endif
        ,&producer_prepare_ops);
    if(producer_prepare.stopped){producer_scope.disabled=1;producer_scope.ready=0;}
#else
    if(producer_scope.disabled || producer_scope.count>=PSO_CAPACITY) {
        int detached=0;int call=pbi_invoke(&producer_scope.invoker,PBI_DETACH,0,0,0,0,0,0,0,&detached);
        producer_scope_stage(8,(call?1u:0u)|((unsigned)(detached==1)<<1));
    }
#endif
    SetLastError(saved);
}
static void producer_scope_tick(void) {
    if(GetCurrentThreadId()!=stereo_pipeline.owner_thread || stereo_pipeline.render.active)return;
    DWORD saved=GetLastError();
#ifdef GF2VR_PRODUCER_TRANSFER
    if(!producer_scope.rows[0].sample){SetLastError(saved);return;}
    ProducerScopeObservation *sample=&producer_scope.rows[0];
    if(sample->cpu_done && !sample->reported && stereo_live) {
        unsigned slot=EFO_CAPACITY;
        EyeFlightToken token={sample->frame,sample->token.generation,sample->token.epoch};
        EyeFlightSnapshot numeric={0};int snap=eye_flight_snapshot(&stereo_two.owner,token,&numeric);
        unsigned end=0,state=ERI_IDLE;
        if(eye_flight_slot(&stereo_two.owner,token,&slot)==EFO_OK && slot<EFO_CAPACITY) {
            end=stereo_two.backend.slots[slot].producer_end_issued;
            state=__atomic_load_n(&stereo_two.backend.slots[slot].render.state,__ATOMIC_ACQUIRE);
        }
        sample->reported=1;char line[900],*p=source_prefix(line,"producer_transfer_root");
#define PTR_WRITE(k,v) p=literal(p,",\"" k "\":");p=decimal(p,(U64)(v))
        PTR_WRITE("serial",sample->token.serial);PTR_WRITE("generation",token.generation);
        PTR_WRITE("epoch",token.epoch);PTR_WRITE("frame",token.frame);
        PTR_WRITE("tail_facts",sample->tail_facts);PTR_WRITE("native_scope",sample->native_result);
        PTR_WRITE("managed_scope",sample->managed_result);PTR_WRITE("whole_cpu_root",sample->cpu_ok);
        PTR_WRITE("producer_end_issued",end);PTR_WRITE("render_state",state);
        PTR_WRITE("snapshot_ok",snap==EFO_OK);PTR_WRITE("flags",numeric.flags);PTR_WRITE("cpu_ok",numeric.cpu_ok);
#undef PTR_WRITE
        p=literal(p,"}\n");source_log(line,p);
    }
    SetLastError(saved);return;
#endif
    for(unsigned i=0;i<producer_scope.count;++i) {
        ProducerScopeObservation *r=&producer_scope.rows[i];
        unsigned delivered=__atomic_load_n(&r->delivered,__ATOMIC_ACQUIRE);
#ifdef GF2VR_PRODUCER_BORROW_RETURN_FIX
        if(r->cpu_done)producer_scope_borrow_log(r);
#endif
#ifdef GF2VR_PRODUCER_TAIL_ORDER_OBSERVE
        if(r->cpu_done)producer_scope_tail_log(r);
#endif
#ifdef GF2VR_PRODUCER_FIRST_FAULT_OBSERVE
        /* Record on a later normal owner tick too: the CAS-winning foreign
           thread may have been preempted before publishing its trace word. */
        U64 fault=pft_read(&producer_failure_trace,r->token.serial);
        if(r->cpu_done && fault && !r->fault_reported) {
            r->fault_reported=1;
            char fault_line[640],*q=source_prefix(fault_line,"producer_first_fault_result");
            q=literal(q,",\"serial\":");q=decimal(q,r->token.serial);
            q=literal(q,",\"generation\":");q=decimal(q,r->token.generation);
            q=literal(q,",\"epoch\":");q=decimal(q,r->token.epoch);
            q=literal(q,",\"fault_thread\":");q=decimal(q,fault>>32);
            q=literal(q,",\"fault_source\":");q=decimal(q,(fault>>16)&65535);
            q=literal(q,",\"fault_line\":");q=decimal(q,fault&65535);
            q=literal(q,"}\n");source_log(fault_line,q);
        }
#endif
        if(!r->cpu_done || r->reported==(delivered?2u:1u))continue;
        r->reported=delivered?2:1;
        char line[1800],*p=source_prefix(line,"producer_scope_result");
#define PSO_WRITE(k,v) p=literal(p,",\"" k "\":");p=decimal(p,(U64)(v))
        PSO_WRITE("serial",r->token.serial);PSO_WRITE("generation",r->token.generation);PSO_WRITE("epoch",r->token.epoch);
        PSO_WRITE("frame",r->frame);PSO_WRITE("context",r->context);PSO_WRITE("child",r->child);
        PSO_WRITE("cpu_ok",r->cpu_ok);PSO_WRITE("managed",r->managed_result);PSO_WRITE("native",r->native_result);
        PSO_WRITE("issued",r->issue_attempts);PSO_WRITE("delivered",delivered);PSO_WRITE("queue_ok",delivered?r->callback_queue_ok:0);
        PSO_WRITE("callback_thread",delivered?r->callback_thread:0);PSO_WRITE("expected_render_thread",r->render_thread);
        PSO_WRITE("issued_qpc",r->issued_at);PSO_WRITE("delivered_qpc",delivered?r->delivered_at:0);PSO_WRITE("returned_qpc",r->returned_at);
        PSO_WRITE("started",r->started);PSO_WRITE("ended",r->ended);PSO_WRITE("tail_attempted",r->tail_attempted);
        PSO_WRITE("begin_order",r->begin_order);PSO_WRITE("end_order",r->end_order);PSO_WRITE("tail_facts",r->tail_facts);
        PSO_WRITE("notifications",r->notifications);PSO_WRITE("normal_submits",r->normal_submits);
#undef PSO_WRITE
        p=literal(p,"}\n");source_log(line,p);
    }SetLastError(saved);
}
#endif
