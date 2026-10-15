#ifndef GF2_STEREO_HOOKS_H
#define GF2_STEREO_HOOKS_H
__declspec(dllimport) BOOL WINAPI ReadProcessMemory(HANDLE,const void *,void *,U64,U64 *);
int WINAPI MH_Initialize(void);
int WINAPI MH_CreateHook(void *,void *,void **);
int WINAPI MH_EnableHook(void *);
int WINAPI MH_DisableHook(void *);
typedef struct {unsigned attempted,ready,created[2],rollback_pending;void *targets[2];} StereoHooks;
/* Never remove/free a published trampoline: a suspended caller may still use
   it. The bootstrap module is process-pinned. Failed startup leaves forwarding
   adapters inert and preserves the original root slot's forwarding contract. */
static int stereo_hooks_install(StereoHooks *h,void *render,void *submit,void *render_hook,void *submit_hook,
    void **render_original,void **submit_original) {
    if(!h || h->attempted || !render || !submit || render==submit || !render_hook || !submit_hook ||
       !render_original || !submit_original) return 0;
    h->attempted=1;h->targets[0]=render;h->targets[1]=submit;
    int status=MH_Initialize();if(status!=0 && status!=1) return 0;
    if(MH_CreateHook(render,render_hook,render_original)) return 0;
    h->created[0]=1;
    if(MH_CreateHook(submit,submit_hook,submit_original)) goto failed;
    h->created[1]=1;
    /* Submission adapter available before any selected wrapper is intercepted.
       Do not apply a shared global MinHook queue or enable foreign hooks. */
    if(MH_EnableHook(submit) || MH_EnableHook(render)) goto failed;
    h->ready=1;return 1;
failed:
    for(unsigned i=0;i<2;++i) if(h->created[i]) {
        int disabled=MH_DisableHook(h->targets[i]);
        if(disabled!=0 && disabled!=6) h->rollback_pending=1;
    }
    return 0;
}
static int stereo_hook_bytes(void *target,const unsigned char *expected,unsigned count) {
    unsigned char bytes[32];U64 read=0;
    if(count>32 || !ReadProcessMemory((HANDLE)(long long)-1,target,bytes,count,&read) || read!=count) return 0;
    for(unsigned i=0;i<count;++i) if(bytes[i]!=expected[i]) return 0;
    return 1;
}
/* Owner tick only, after root observer binding/install. No hard-coded MethodInfo
   identity or guessed class: exact metadata and the recorded wrapper bytes agree. */
static void *stereo_hooks_method(void **render,void **submit) {
    HINSTANCE game=GetModuleHandleW(L"GameAssembly.dll");if(!game || !unity) return 0;
    unsigned char (*generic)(void *)=(void *)GetProcAddress(game,"il2cpp_method_is_generic");
    void *klass=render_class("Unity.RenderPipelines.Universal.Runtime.dll","UnityEngine.Rendering.Universal","UniversalRenderPipeline");
    const char *params[]={"UnityEngine.Rendering.ScriptableRenderContext","UnityEngine.Camera[]"};
    if(!klass || !generic) return 0;
    void *method=eye_binding_method_args(klass,"Render",2,params,"System.Void",0,generic);
    *render=(unsigned char *)game+0x620f40;*submit=unity+0x986090;
    U64 code=0,read=0;
    static const unsigned char rb[32]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57,0x48,0x83,0xec,0x20,0x49,0x8b,0xe9,0x49,0x8b,0xf0,0x48,0x8b,0xda,0x48,0x8b,0xf9};
    static const unsigned char sb[8]={0x48,0x8b,0x09,0xe9,0x88,0x6b,0xdc,0xff};
    if(!method || !ReadProcessMemory((HANDLE)(long long)-1,method,&code,8,&read) || read!=8 || code!=(U64)*render ||
       !stereo_hook_bytes(*render,rb,32) || !stereo_hook_bytes(*submit,sb,8)) return 0;
    return method;
}
#endif
