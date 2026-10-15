typedef void *(*RenderInvoke)(void *,void *,void **,void **);
#ifndef GF2VR_RENDER_SOURCE_SNAPSHOT
#define GF2VR_RENDER_SOURCE_SNAPSHOT() source_snapshot()
#endif
#ifndef GF2VR_RENDER_SAMPLE_LIMIT
#define GF2VR_RENDER_SAMPLE_LIMIT 120
#endif
#ifndef GF2VR_RENDER_OBSERVATION_MS
#define GF2VR_RENDER_OBSERVATION_MS 180000
#endif
static RenderInvoke render_original;
static void *render_loop;
static U64 render_calls,render_next,render_deadline;
static unsigned render_samples,render_guard,render_attempts;
static BOOL render_installed,render_failed;
static char *(*render_type_name)(void *);
static void (*render_free)(void *);

static char *render_ascii(char *p,const char *s) {
    if(!s) return literal(p,"null");
    *p++='"';
    const char *hex="0123456789abcdef";
    for(unsigned i=0;i<240 && s[i];++i) {
        unsigned char c=(unsigned char)s[i];
        if(c=='"' || c=='\\') { *p++='\\'; *p++=(char)c; }
        else if(c<32 || c>=127) { p=literal(p,"\\u00");*p++=hex[c>>4];*p++=hex[c&15]; }
        else *p++=(char)c;
    }
    *p++='"';return p;
}
static void render_signature(void *m,const char *owner) {
    /* Binding runs only on the accepted main callback before installation. */
    static char line[16384];char *p=source_prefix(line,"render_method");
    p=literal(p,",\"owner\":");p=render_ascii(p,owner);
    p=literal(p,",\"name\":");p=render_ascii(p,sa.method_get_name(m));
    unsigned impl=0;
    p=literal(p,",\"flags\":");p=hexadecimal(p,sa.method_get_flags(m,&impl));
    p=literal(p,",\"method\":");p=hexadecimal(p,(U64)m);
    p=literal(p,",\"parameters\":[");
    unsigned count=sa.method_get_param_count(m);
    for(unsigned i=0;i<count && i<8;++i) {
        if(i) *p++=',';
        void *type=sa.method_get_param(m,i);
        char *name=render_type_name(type);p=render_ascii(p,name);if(name) render_free(name);
    }
    p=literal(p,"],\"return_type\":");
    char *name=render_type_name(sa.method_get_return_type(m));p=render_ascii(p,name);if(name) render_free(name);
    p=literal(p,"}\n");source_log(line,p);
#ifdef GF2VR_METHOD_OBSERVE
    GF2VR_METHOD_OBSERVE(m,owner);
#endif
}
static void *render_class(const char *image_name,const char *ns,const char *name) {
    U64 count=0;void **assemblies=sa.domain_get_assemblies(sa.domain_get(),&count);
    if(!assemblies || count>4096) return NULL;
    for(U64 i=0;i<count;++i) {
        void *image=sa.assembly_get_image(assemblies[i]);
        if(image && source_equal(sa.image_get_name(image),image_name)) return sa.class_from_name(image,ns,name);
    }
    return NULL;
}
static BOOL render_bind(void) {
    if(!source_bind()) return 0;
    HINSTANCE game=GetModuleHandleW(L"GameAssembly.dll");
    render_type_name=(void *)GetProcAddress(game,"il2cpp_type_get_name");
    render_free=(void *)GetProcAddress(game,"il2cpp_free");
    if(!render_type_name || !render_free) return 0;
    void *klass=render_class("UnityEngine.CoreModule.dll","UnityEngine.Rendering","RenderPipelineManager");
    if(!klass) return 0;
    void *iterator=NULL,*m;unsigned found=0;
    for(unsigned i=0;i<2048 && (m=sa.class_get_methods(klass,&iterator));++i) {
        const char *name=sa.method_get_name(m);
        if(source_equal(name,"DoRenderLoop_Internal")) {
            render_signature(m,"RenderPipelineManager");
            unsigned impl=0;
            if((sa.method_get_flags(m,&impl)&16) && sa.method_get_param_count(m)==2 &&
               sa.type_get_type(sa.method_get_return_type(m))==1) { render_loop=m;++found; }
        } else if(source_equal(name,"BeginCameraRendering") || source_equal(name,"EndCameraRendering") ||
                  source_equal(name,"add_beginCameraRendering") || source_equal(name,"add_endCameraRendering"))
            render_signature(m,"RenderPipelineManager");
    }
    /* Metadata presence is logged only; no event subscriptions or camera-data access. */
    void *data=render_class("Unity.RenderPipelines.Universal.Runtime.dll","UnityEngine.Rendering.Universal","UniversalAdditionalCameraData");
    event("render_additional_camera_class",data?1:0,GetCurrentThreadId());
    return found==1;
}
static void *render_invoke(void *method,void *object,void **args,void **exception) {
    DWORD incoming=GetLastError();
    RenderInvoke forward=render_original;
    BOOL observe=method==render_loop && GetCurrentThreadId()==__atomic_load_n(&first_tid,__ATOMIC_ACQUIRE) && !render_guard;
    BOOL sample=0;U64 call=0;
    if(observe) {
        render_guard=1;call=++render_calls;
        U64 now=GetTickCount64();
        sample=now>=render_next && now<render_deadline && render_samples<GF2VR_RENDER_SAMPLE_LIMIT;
        if(sample) {
            render_next=now+1000;++render_samples;
            event("render_loop_enter",call,GetCurrentThreadId());
            source_samples=render_samples*2-1;
            GF2VR_RENDER_SOURCE_SNAPSHOT();
        }
    }
#ifdef GF2VR_RENDER_SCOPE_BEGIN
    if(observe) GF2VR_RENDER_SCOPE_BEGIN(call,sample);
#endif
    SetLastError(incoming);
    void *result=forward(method,object,args,exception);
    DWORD outgoing=GetLastError();
#ifdef GF2VR_RENDER_SCOPE_END
    if(observe) GF2VR_RENDER_SCOPE_END(exception && *exception);
#endif
    if(observe) {
        if(sample) {
            event("render_loop_return",call,GetCurrentThreadId());
            event("render_loop_exception",exception && *exception?1:0,GetCurrentThreadId());
            source_samples=render_samples*2;
#ifdef GF2VR_RENDER_AFTER
            if(!exception || !*exception) GF2VR_RENDER_AFTER();
#endif
            /* Do not make further engine calls on a failed render invocation. */
            if(!exception || !*exception) GF2VR_RENDER_SOURCE_SNAPSHOT();
        }
        render_guard=0;
    }
    SetLastError(outgoing);return result;
}
static BOOL render_install(RenderInvoke *slot,RenderInvoke expected,void *method) {
    if(!expected || !method || expected==render_invoke) return 0;
    render_original=expected;render_loop=method;
    return __atomic_compare_exchange_n(slot,&expected,render_invoke,0,__ATOMIC_RELEASE,__ATOMIC_RELAXED);
}
static void render_main_tick(U64 n) {
    panel_main_tick(n);
    (void)source_main_tick; /* Reuse typed metadata helpers; do not run old diagnostic. */
    if(render_installed || render_failed || n<2 || !unity ||
       GetCurrentThreadId()!=__atomic_load_n(&first_tid,__ATOMIC_ACQUIRE)) return;
    U64 now=GetTickCount64();if(now<render_next) return;render_next=now+1000;
#ifndef GF2VR_RENDER_BIND_PERSISTENT
    if(++render_attempts>30) { render_failed=1;event("render_binding_timeout",0,GetCurrentThreadId());return; }
#else
    if(render_attempts!=~0u) ++render_attempts;
#endif
    if(!render_bind()) { event("render_binding_unavailable",render_attempts,GetCurrentThreadId());return; }
    const unsigned char signature[7]={0x48,0x8b,0x90,0xd0,0x09,0x00,0x00};
    for(unsigned i=0;i<7;++i) if(unity[0x745e0a+i]!=signature[i]) { render_failed=1;event("render_signature_mismatch",0,GetCurrentThreadId());return; }
    void *cache=__atomic_load_n((void **)(unity+0x17daca0),__ATOMIC_ACQUIRE);
    if(!cache || *(void **)((unsigned char *)cache+0x9d0)!=render_loop) {
        render_failed=1;event("render_cache_identity_mismatch",0,GetCurrentThreadId());return;
    }
    render_deadline=now+GF2VR_RENDER_OBSERVATION_MS;render_next=0;
    render_installed=render_install((RenderInvoke *)(unity+0x18524d8),sa.runtime_invoke,render_loop);
    render_failed=!render_installed;
    event(render_installed?"render_invoke_slot_installed":"render_invoke_foreign_owner",0,GetCurrentThreadId());
    /* Module is already process-pinned by accepted Bootstrap; keep forwarding after cap. */
}
#ifdef BOUNDARY_TEST
__declspec(dllexport) BOOL render_test_install(RenderInvoke *slot,RenderInvoke expected,void *method) {
    render_deadline=0;render_samples=0;render_guard=0;render_calls=0;
    first_tid=GetCurrentThreadId();return render_install(slot,expected,method);
}
__declspec(dllexport) U64 render_test_calls(void) { return render_calls; }
#endif
