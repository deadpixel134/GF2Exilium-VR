#include "il2cpp_export.h"
#ifndef GF2_PRODUCER_ENGINE_ADAPTER_H
#define GF2_PRODUCER_ENGINE_ADAPTER_H
#include "producer_engine_transport.h"
/* No resource is borrowed by this ABI-safe callback, including if the engine
   interposition is absent or late. Module/trampoline lifetime is process-pinned. */
static void producer_engine_noop(int id) {(void)id;}
static struct {
    ProducerEngineTransport transport;
    PenFields api;
    void *fields[PEN_FIELDS],*target;
    void (*get)(void *,void *);
    void (*set)(void *,void *);
    void (*original)(void (*)(int),int);
    unsigned attempted,created,enabled,entries,reason;
} producer_engine;
static int producer_engine_get(void *context,unsigned id,void *out,size_t size) {
    (void)context;
    if(id>=PEN_FIELDS || size!=pen_size(id) || !out || !producer_engine.fields[id] || !producer_engine.get)return 0;
    producer_engine.get(producer_engine.fields[id],out);return 1;
}
static int producer_engine_set(void *context,unsigned id,const void *value,size_t size) {
    (void)context;
    if((id!=PEN_RESULT && id!=PEN_ACK) || size!=pen_size(id) || !value ||
       !producer_engine.fields[id] || !producer_engine.set)return 0;
    producer_engine.set(producer_engine.fields[id],(void *)value);return 1;
}
static int producer_engine_notify(void *context,int64_t serial,int operation,int camera) {
    (void)context;return producer_scope_notify(serial,operation,camera);
}
static void producer_engine_forward(void (*callback)(int),int id) {
    DWORD saved=GetLastError();
    if(callback==producer_engine_noop && id==PEN_EVENT_ID) {
        /* Exact protocol is synchronous and consumed, never queued as if it
           were a GPU notification. Other callbacks always forward unchanged. */
        if(__atomic_load_n(&producer_engine.transport.owner,__ATOMIC_ACQUIRE)==GetCurrentThreadId()) {
            ++producer_engine.entries;
            (void)pen_request(&producer_engine.transport,GetCurrentThreadId(),&producer_engine.api,
                              producer_engine_notify,0);
        }
        SetLastError(saved);return;
    }
    SetLastError(saved);
    if(producer_engine.original)producer_engine.original(callback,id);
}
static int producer_engine_hook_body(void *entry,void *body) {
    /* The product issuer guard owns the original 94d840 E9 thunk. Intercept
       its already-bound body; never patch the bytes that admit stereo work. */
    static const unsigned char thunk[]={0xe9,0x4b,0xcc,0xd2,0xff};
    static const unsigned char prefix[]={0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x8b,0xda,0x48,0x8b,0xf9};
    producer_engine.reason=3;
    if((uintptr_t)entry!=(uintptr_t)body+0x2d33b0u ||
       !stereo_hook_bytes(entry,thunk,sizeof(thunk)) ||
       !stereo_hook_bytes(body,prefix,sizeof(prefix)))return 0;
    producer_engine.target=body;
    producer_engine.reason=5;
    int status=MH_Initialize();if(status!=0 && status!=1)return 0;
    producer_engine.reason=6;
    if(MH_CreateHook(body,producer_engine_forward,(void **)&producer_engine.original))return 0;
    producer_engine.created=1;
    producer_engine.reason=7;
    if(MH_EnableHook(body)) {
        /* Published trampoline is kept alive; do not remove a possibly used hook. */
        (void)MH_DisableHook(body);return 0;
    }
    producer_engine.reason=8;
    if(!stereo_hook_bytes(entry,thunk,sizeof(thunk))) {
        (void)MH_DisableHook(body);return 0;
    }
    producer_engine.enabled=1;producer_engine.reason=0;return 1;
}
static int producer_engine_bind(void *klass,U64 owner) {
    if(producer_engine.attempted)return producer_engine.enabled;
    producer_engine.attempted=1;
    producer_engine.reason=1;
    if(!klass || !owner || owner!=GetCurrentThreadId() || !unity)return 0;
    HINSTANCE game=GetModuleHandleW(L"GameAssembly.dll");if(!game)return 0;
    void *(*resolve)(const char *)=(void *)il2cpp_export(game,"il2cpp_resolve_icall");
    void *(*field)(void *,const char *)=(void *)il2cpp_export(game,"il2cpp_class_get_field_from_name");
    void *(*type)(void *)=(void *)il2cpp_export(game,"il2cpp_field_get_type");
    unsigned (*flags)(void *)=(void *)il2cpp_export(game,"il2cpp_field_get_flags");
    producer_engine.get=(void *)il2cpp_export(game,"il2cpp_field_static_get_value");
    producer_engine.set=(void *)il2cpp_export(game,"il2cpp_field_static_set_value");
    producer_engine.reason=2;
    if(!resolve || !field || !type || !flags || !producer_engine.get || !producer_engine.set)return 0;
    const char *names[]={"engineSerial","engineOperation","engineCamera","engineNonce","engineResult","engineAck"};
    for(unsigned i=0;i<PEN_FIELDS;++i) {
        producer_engine.reason=0x100u+i;
        void *f=field(klass,names[i]);
        if(!f || !(flags(f)&0x10) || (flags(f)&0x60) ||
           !eye_binding_type(type(f),pen_size(i)==8?"System.Int64":"System.Int32"))return 0;
        producer_engine.fields[i]=f;
    }
    void *target=resolve("UnityEngine.GL::GLIssuePluginEvent");
    producer_engine.reason=3;
    if(target!=unity+0x94d840)return 0;
    producer_engine.api=(PenFields){0,producer_engine_get,producer_engine_set};
    producer_engine.reason=4;
    if(!pen_initialize(&producer_engine.transport,owner,(U64)producer_engine_noop))return 0;
    return producer_engine_hook_body(target,unity+0x67a490);
}
#ifdef BOUNDARY_TEST
static unsigned pen_test_forwarded;
static DWORD pen_test_error;
static void (*pen_test_pointer)(int);
static int pen_test_id;
static int64_t pen_test_values[PEN_FIELDS];
static void pen_test_get(void *field,void *out) {
    unsigned id=(unsigned)(uintptr_t)field-1;
    if(id<PEN_FIELDS)for(size_t i=0;i<pen_size(id);++i)((unsigned char *)out)[i]=((unsigned char *)&pen_test_values[id])[i];
    SetLastError(733);
}
static void pen_test_set(void *field,void *value) {
    unsigned id=(unsigned)(uintptr_t)field-1;
    if(id<PEN_FIELDS)for(size_t i=0;i<pen_size(id);++i)((unsigned char *)&pen_test_values[id])[i]=((unsigned char *)value)[i];
    SetLastError(734);
}
__attribute__((used,noinline)) static void pen_test_original(void (*callback)(int),int id) {
    ++pen_test_forwarded;pen_test_pointer=callback;pen_test_id=id;
    pen_test_error=GetLastError();SetLastError(721);
}
/* Same unconditional-jump entry shape as the bound engine thunk. Exercise
   actual MinHook relocation and original forwarding without loading Unity. */
__attribute__((used,naked,noinline)) static void pen_test_hook_site(
    void (*callback)(int) __attribute__((unused)),int id __attribute__((unused))) {
    __asm__("jmp pen_test_original");
}
__declspec(dllexport) int producer_engine_adapter_test(void) {
    __typeof__(producer_engine) old=producer_engine;
    __typeof__(producer_scope) old_scope=producer_scope;DWORD error=GetLastError();int line=0,hooked=0;
#define PEN_CHECK(x) do {if(!(x)){line=__LINE__;goto done;}}while(0)
    producer_engine=(__typeof__(producer_engine)){0};producer_engine.original=pen_test_original;
    pen_test_forwarded=0;SetLastError(720);
    producer_engine_forward(producer_scope_callback,0x1234);
    PEN_CHECK(pen_test_forwarded==1 && pen_test_pointer==producer_scope_callback &&
              pen_test_id==0x1234 && pen_test_error==720 && GetLastError()==721);
    SetLastError(722);producer_engine_forward(producer_engine_noop,PEN_EVENT_ID);
    PEN_CHECK(pen_test_forwarded==1 && GetLastError()==722 && !producer_engine.entries);
    producer_engine_forward(producer_engine_noop,123);
    PEN_CHECK(pen_test_forwarded==2 && pen_test_pointer==producer_engine_noop && pen_test_id==123);
    for(unsigned i=0;i<PEN_FIELDS;++i){producer_engine.fields[i]=(void *)(uintptr_t)(i+1);pen_test_values[i]=0;}
    producer_engine.get=pen_test_get;producer_engine.set=pen_test_set;
    producer_engine.api=(PenFields){0,producer_engine_get,producer_engine_set};
    PEN_CHECK(pen_initialize(&producer_engine.transport,GetCurrentThreadId(),(U64)producer_engine_noop));
    pen_test_values[PEN_SERIAL]=(int64_t)(uintptr_t)producer_engine_noop;
    pen_test_values[PEN_CAMERA]=PEN_MAGIC;pen_test_values[PEN_NONCE]=0x100000005LL;
    SetLastError(735);producer_engine_forward(producer_engine_noop,PEN_EVENT_ID);
    PEN_CHECK(GetLastError()==735 && producer_engine.entries==1 && producer_engine.transport.ready &&
              pen_test_values[PEN_ACK]==0x100000005LL && pen_test_values[PEN_RESULT]==PEN_MAGIC);
    producer_scope=(__typeof__(producer_scope)){0};
    PsnToken token={0x100000009ULL,11,13};
    PEN_CHECK(psn_arm(&producer_scope.native,GetCurrentThreadId(),token,17,19,100,200));
    producer_scope.active=producer_scope.count=1;producer_scope.rows[0].token=token;
    pen_test_values[PEN_SERIAL]=(int64_t)token.serial;pen_test_values[PEN_OPERATION]=PSN_BEGIN;
    pen_test_values[PEN_CAMERA]=100;++pen_test_values[PEN_NONCE];
    SetLastError(736);producer_engine_forward(producer_engine_noop,PEN_EVENT_ID);
    PEN_CHECK(GetLastError()==736 && producer_scope.native.started==1 &&
              pen_test_values[PEN_RESULT]==1 && pen_test_values[PEN_ACK]==pen_test_values[PEN_NONCE]);
    PEN_CHECK(psn_submit_enter(&producer_scope.native,GetCurrentThreadId(),token,17,19) &&
              psn_submit_return(&producer_scope.native,GetCurrentThreadId(),token,17,19,1));
    pen_test_values[PEN_OPERATION]=PSN_END;++pen_test_values[PEN_NONCE];
    producer_engine_forward(producer_engine_noop,PEN_EVENT_ID);
    PEN_CHECK(producer_scope.native.ended==1 && pen_test_values[PEN_RESULT]==1 &&
              pen_test_values[PEN_ACK]==pen_test_values[PEN_NONCE] && pen_test_forwarded==2);
    PEN_CHECK(!producer_scope.rows[0].issue_attempts && !producer_scope.rows[0].delivered);
    int mh=MH_Initialize();PEN_CHECK(mh==0 || mh==1);
    PEN_CHECK(!MH_CreateHook(pen_test_hook_site,producer_engine_forward,(void **)&producer_engine.original));
    PEN_CHECK(!MH_EnableHook(pen_test_hook_site));hooked=1;
    SetLastError(737);pen_test_hook_site(producer_scope_callback,421);
    PEN_CHECK(pen_test_forwarded==3 && pen_test_pointer==producer_scope_callback &&
              pen_test_id==421 && pen_test_error==737 && GetLastError()==721);
    pen_test_values[PEN_OPERATION]=PSN_FAULT;++pen_test_values[PEN_NONCE];
    SetLastError(738);pen_test_hook_site(producer_engine_noop,PEN_EVENT_ID);
    PEN_CHECK(GetLastError()==738 && pen_test_forwarded==3 &&
              producer_scope.native.failed_serial==token.serial &&
              pen_test_values[PEN_ACK]==pen_test_values[PEN_NONCE]);
done:
    if(hooked)(void)MH_DisableHook(pen_test_hook_site);
    producer_engine=old;producer_scope=old_scope;SetLastError(error);return line;
#undef PEN_CHECK
}
#endif
#endif
