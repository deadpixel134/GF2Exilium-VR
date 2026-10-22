#include "il2cpp_export.h"
/* Read-only managed camera metadata on the already attributed main callback.
   No raw managed layout, method-pointer ABI, camera mutation or render calls. */
#include "../bootstrap/runtime_logging_state.h"
typedef struct SourceApi {
    void *(*domain_get)(void);
    void **(*domain_get_assemblies)(void *,U64 *);
    void *(*assembly_get_image)(void *);
    const char *(*image_get_name)(void *);
    void *(*class_from_name)(void *,const char *,const char *);
    void *(*class_get_methods)(void *,void **);
    void *(*class_get_parent)(void *);
    const char *(*method_get_name)(void *);
    unsigned (*method_get_param_count)(void *);
    void *(*method_get_param)(void *,unsigned);
    void *(*method_get_return_type)(void *);
    unsigned (*method_get_flags)(void *,unsigned *);
    int (*type_get_type)(void *);
    void *(*runtime_invoke)(void *,void *,void **,void **);
    void *(*object_unbox)(void *);
    void *(*object_get_class)(void *);
    unsigned (*array_length)(void *);
    unsigned (*gchandle_new)(void *,BOOL);
    void (*gchandle_free)(unsigned);
    int (*string_length)(void *);
    const unsigned short *(*string_chars)(void *);
} SourceApi;
static SourceApi sa;
static void *source_camera,*source_getall,*source_getvalue;
static unsigned source_initialized,source_disabled,source_samples;
static U64 source_next;
static BOOL source_equal(const char *a,const char *b) {
    if(!a || !b) return 0;
    for(unsigned i=0;i<160;++i) { if(a[i]!=b[i]) return 0; if(!a[i]) return 1; }
    return 0;
}
static void *source_method(void *klass,const char *name,int argc,int parameter,int result,BOOL is_static) {
    for(unsigned parents=0;klass && parents<16;++parents,klass=sa.class_get_parent(klass)) {
        void *iterator=NULL,*method;
        for(unsigned i=0;i<2048 && (method=sa.class_get_methods(klass,&iterator));++i) {
            if(!source_equal(sa.method_get_name(method),name) || sa.method_get_param_count(method)!=(unsigned)argc) continue;
            unsigned impl=0,flags=sa.method_get_flags(method,&impl);
            if(((flags&16)!=0)!=is_static) continue;
            if(argc && sa.type_get_type(sa.method_get_param(method,0))!=parameter) continue;
            if(result && sa.type_get_type(sa.method_get_return_type(method))!=result) continue;
            return method;
        }
    }
    return NULL;
}
static void *source_invoke(void *method,void *object,void **args,BOOL *ok) {
    if(!method) { *ok=0; return NULL; }
    void *exception=NULL;
    void *value=sa.runtime_invoke(method,object,args,&exception);
    if(exception) { *ok=0; event("source_managed_exception",(U64)method,GetCurrentThreadId()); return NULL; }
    return value;
}
static void source_log(char *line,char *end) {
    if(!GF2VR_LOGGING_ALLOWED()) return;
    DWORD written; HANDLE f=CreateFileW(GF2VR_PROBE_LOG,FILE_APPEND_DATA,7,NULL,OPEN_ALWAYS,128,NULL);
    if(f!=INVALID_HANDLE_VALUE) { WriteFile(f,line,(DWORD)(end-line),&written,NULL); CloseHandle(f); }
}
static char *source_prefix(char *p,const char *kind) {
    p=literal(p,"{\"probe\":\"" GF2VR_OBSERVER_ID "\",\"event\":\"");p=literal(p,kind);
    p=literal(p,"\",\"pid\":");p=decimal(p,GetCurrentProcessId());
    p=literal(p,",\"tid\":");p=decimal(p,GetCurrentThreadId());
    p=literal(p,",\"sample\":");p=decimal(p,source_samples);
    p=literal(p,",\"tick_ms\":");return hexadecimal(p,GetTickCount64());
}
static char *source_string(char *p,void *object) {
    if(!object) return literal(p,"null");
    int n=sa.string_length(object); const unsigned short *chars=sa.string_chars(object);
    if(n<0 || n>4096 || !chars) return literal(p,"null");
    if(n>96) n=96;
    *p++='"';
    const char *hex="0123456789abcdef";
    for(int i=0;i<n;++i) { p=literal(p,"\\u"); for(int b=12;b>=0;b-=4) *p++=hex[(chars[i]>>b)&15]; }
    *p++='"';return p;
}
static BOOL source_bind(void) {
    HINSTANCE module=GetModuleHandleW(L"GameAssembly.dll");if(!module) return 0;
#define SOURCE_LOAD(n) sa.n=(void *)il2cpp_export(module,"il2cpp_" #n); if(!sa.n) { event("source_export_missing",__LINE__,GetCurrentThreadId());source_disabled=1;return 0; }
    SOURCE_LOAD(domain_get) SOURCE_LOAD(domain_get_assemblies) SOURCE_LOAD(assembly_get_image)
    SOURCE_LOAD(image_get_name) SOURCE_LOAD(class_from_name) SOURCE_LOAD(class_get_methods)
    SOURCE_LOAD(class_get_parent) SOURCE_LOAD(method_get_name) SOURCE_LOAD(method_get_param_count)
    SOURCE_LOAD(method_get_param) SOURCE_LOAD(method_get_return_type) SOURCE_LOAD(method_get_flags)
    SOURCE_LOAD(type_get_type) SOURCE_LOAD(runtime_invoke) SOURCE_LOAD(object_unbox)
    SOURCE_LOAD(object_get_class) SOURCE_LOAD(array_length) SOURCE_LOAD(gchandle_new)
    SOURCE_LOAD(gchandle_free) SOURCE_LOAD(string_length) SOURCE_LOAD(string_chars)
#undef SOURCE_LOAD
    U64 count=0;void *domain=sa.domain_get();if(!domain) return 0;
    void **assemblies=sa.domain_get_assemblies(domain,&count);
    if(!assemblies || !count || count>4096) return 0;
    void *core=NULL,*mscorlib=NULL;
    for(U64 i=0;i<count;++i) {
        void *image=sa.assembly_get_image(assemblies[i]);if(!image) continue;
        const char *name=sa.image_get_name(image);
        if(source_equal(name,"UnityEngine.CoreModule.dll")) core=image;
        if(source_equal(name,"mscorlib.dll")) mscorlib=image;
    }
    if(!core || !mscorlib) return 0;
    source_camera=sa.class_from_name(core,"UnityEngine","Camera");
    void *array=sa.class_from_name(mscorlib,"System","Array");
    source_getall=source_method(source_camera,"get_allCameras",0,0,29,1);
    source_getvalue=source_method(array,"GetValue",1,8,28,0);
    if(!source_getall || !source_getvalue) {
        event("source_method_missing",(source_getall?1:0)|(source_getvalue?2:0),GetCurrentThreadId());
        source_disabled=1;return 0;
    }
    event("source_api_bound",count,GetCurrentThreadId());return 1;
}
static void source_scalar(char **out,void *klass,void *object,const char *getter,const char *key,int type) {
    char *p=*out;p=literal(p,",\"");p=literal(p,key);p=literal(p,"\":");
    BOOL ok=1;void *method=source_method(klass,getter,0,0,type,0);
    void *boxed=source_invoke(method,object,NULL,&ok);
    if(ok && boxed) {
        const unsigned char *v=sa.object_unbox(boxed);
        if(v) { unsigned bits=v[0];if(type!=2) bits|=(unsigned)v[1]<<8|(unsigned)v[2]<<16|(unsigned)v[3]<<24; p=hexadecimal(p,bits); }
        else p=literal(p,"null");
    } else p=literal(p,"null");
    *out=p;
}
static void source_snapshot(void) {
    BOOL ok=1;void *array=source_invoke(source_getall,NULL,NULL,&ok);
    if(!ok || !array) { source_disabled=1;return; }
    unsigned root=sa.gchandle_new(array,1);if(!root) { source_disabled=1;return; }
    unsigned count=sa.array_length(array);
    event("source_camera_count",count,GetCurrentThreadId());
    if(count>32) {event("source_camera_limit",count,GetCurrentThreadId());sa.gchandle_free(root);return;}
    for(unsigned i=0;i<count;++i) {
        int index=(int)i;void *args[]={&index};
        void *camera=source_invoke(source_getvalue,array,args,&ok);if(!ok) break;if(!camera) continue;
        unsigned camera_root=sa.gchandle_new(camera,1);if(!camera_root) break;
        char line[2048],*p=source_prefix(line,"source_camera");
        p=literal(p,",\"index\":");p=decimal(p,i);
        p=literal(p,",\"object\":");p=hexadecimal(p,(U64)camera);
        void *klass=sa.object_get_class(camera);
        void *name=source_invoke(source_method(klass,"get_name",0,0,14,0),camera,NULL,&ok);
        p=literal(p,",\"name\":");p=source_string(p,ok?name:NULL);
        source_scalar(&p,klass,camera,"GetInstanceID","instance_id",8);
        source_scalar(&p,klass,camera,"get_orthographic","orthographic",2);
        source_scalar(&p,klass,camera,"get_cullingMask","culling_mask",8);
        source_scalar(&p,klass,camera,"get_depth","depth_f32_bits",12);
        source_scalar(&p,klass,camera,"get_fieldOfView","fov_f32_bits",12);
        source_scalar(&p,klass,camera,"get_pixelWidth","pixel_width",8);
        source_scalar(&p,klass,camera,"get_pixelHeight","pixel_height",8);
        source_scalar(&p,klass,camera,"get_targetDisplay","target_display",8);
        ok=1;void *target=source_invoke(source_method(klass,"get_targetTexture",0,0,18,0),camera,NULL,&ok);
        p=literal(p,",\"target_query_ok\":");p=literal(p,ok?"true":"false");
        p=literal(p,",\"target_object\":");p=hexadecimal(p,(U64)target);
        if(target && ok) {
            unsigned target_root=sa.gchandle_new(target,1);
            if(target_root) {
                void *tk=sa.object_get_class(target);
                source_scalar(&p,tk,target,"GetInstanceID","target_id",8);
                source_scalar(&p,tk,target,"get_width","target_width",8);
                source_scalar(&p,tk,target,"get_height","target_height",8);
                sa.gchandle_free(target_root);
            }
        }
        p=literal(p,"}\n");source_log(line,p);
#ifdef GF2VR_SOURCE_CAMERA_OBSERVE
        GF2VR_SOURCE_CAMERA_OBSERVE(klass,camera);
#endif
        sa.gchandle_free(camera_root);
    }
    sa.gchandle_free(root);
}
#ifdef BOUNDARY_TEST
__declspec(dllexport) void source_test_snapshot(const SourceApi *api,void *getall,void *getvalue) {
    for(unsigned i=0;i<sizeof(sa);++i) ((volatile unsigned char *)&sa)[i]=((const unsigned char *)api)[i];source_getall=getall;source_getvalue=getvalue;source_disabled=0;source_samples=1;source_snapshot();
}
#endif
static void source_main_tick(U64 n) {
    panel_main_tick(n);
    if(source_disabled || source_samples>=120 || GetCurrentThreadId()!=__atomic_load_n(&first_tid,__ATOMIC_ACQUIRE)) return;
    U64 now=GetTickCount64();if(now<source_next) return;source_next=now+1000;
    /* Only diagnostic sampling is capped. Panel/XR production is untouched. */
    ++source_samples;
    if(!source_initialized) source_initialized=source_bind();
    if(source_initialized) source_snapshot();
    if(source_samples==120) event("source_observation_complete",source_samples,GetCurrentThreadId());
}


