#include "il2cpp_export.h"
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
typedef int (*StereoReadFn)(void *ctx,const void *address,void *out,unsigned size);
typedef struct {void *game_base,*unity_base;U64 game_size,unity_size;StereoReadFn read;void *ctx;} StereoTargetHost;
static int stereo_target_range(void *base,U64 size,const void *address,unsigned count) {
    U64 b=(U64)base,p=(U64)address;
    return b && size && size<=~(U64)0-b && p>=b && p-b<size && count<=size-(p-b);
}
static int stereo_hook_targets_check(const StereoTargetHost *h,void *render,void *submit,unsigned char actual_prologue[32]) {
    static const unsigned char rb[32]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57,0x48,0x83,0xec,0x20,0x49,0x8b,0xe9,0x49,0x8b,0xf0,0x48,0x8b,0xda,0x48,0x8b,0xf9};
    unsigned char actual[32]={0},sb[8];
    if(actual_prologue)for(unsigned i=0;i<32;++i)actual_prologue[i]=0;
    if(render && render==submit)return 6;
    if(!h || !h->read || !stereo_target_range(h->game_base,h->game_size,render,32))return 1;
    if(!h->read(h->ctx,render,actual,32))return 2;
    if(actual_prologue)for(unsigned i=0;i<32;++i)actual_prologue[i]=actual[i];
    for(unsigned i=0;i<32;++i)if(actual[i]!=rb[i])return 2;
    if(!submit || !stereo_target_range(h->unity_base,h->unity_size,submit,8))return 3;
    if(!h->read(h->ctx,submit,sb,8) || sb[0]!=0x48 || sb[1]!=0x8b || sb[2]!=0x09 || sb[3]!=0xe9)return 4;
    unsigned bits=(unsigned)sb[4]|((unsigned)sb[5]<<8)|((unsigned)sb[6]<<16)|((unsigned)sb[7]<<24);
    long long rel=(bits&0x80000000u)?(long long)bits-0x100000000LL:(long long)bits;
    U64 next=(U64)submit+8,target;
    if(rel<0) {if(next<(U64)(-rel))return 5;target=next-(U64)(-rel);}
    else {if(next>~(U64)0-(U64)rel)return 5;target=next+(U64)rel;}
    if(!stereo_target_range(h->unity_base,h->unity_size,(void *)target,1))return 5;
    return 0;
}
static int stereo_target_read(void *ctx,const void *address,void *out,unsigned size) {
    U64 read=0;(void)ctx;
    return ReadProcessMemory((HANDLE)(long long)-1,address,out,size,&read) && read==size;
}
static U64 stereo_target_image_size(void *base) {
    unsigned pe=0,signature=0,size=0;U64 b=(U64)base;
    if(!b || b>~(U64)0-0x1100 || !stereo_target_read(0,(void *)(b+0x3c),&pe,4) || pe>0x1000 ||
       !stereo_target_read(0,(void *)(b+pe),&signature,4) || signature!=0x4550 ||
       !stereo_target_read(0,(void *)(b+pe+24+56),&size,4) || size<pe+24+60)return 0;
    return size;
}
/* Owner tick only, after root observer binding/install. Metadata-selected code
   and the engine-resolved submit thunk must both satisfy the recorded contract. */
static void *stereo_hooks_method(void **render,void **submit) {
    static unsigned reported;
    if(!render || !submit)return 0;
    *render=0;*submit=0;
    HINSTANCE game=GetModuleHandleW(L"GameAssembly.dll");if(!game || !unity) return 0;
    unsigned char (*generic)(void *)=(void *)il2cpp_export(game,"il2cpp_method_is_generic");
    void *(*resolve)(const char *)=(void *)il2cpp_export(game,"il2cpp_resolve_icall");
    void *klass=render_class("Unity.RenderPipelines.Universal.Runtime.dll","UnityEngine.Rendering.Universal","UniversalRenderPipeline");
    const char *params[]={"UnityEngine.Rendering.ScriptableRenderContext","UnityEngine.Camera[]"};
    if(!klass || !generic || !resolve) return 0;
    void *method=eye_binding_method_args(klass,"Render",2,params,"System.Void",0,generic);
    U64 code=0;
    if(!method || !stereo_target_read(0,method,&code,8))return 0;
    void *r=(void *)code,*s=resolve("UnityEngine.Rendering.ScriptableRenderContext::Submit_Internal_Injected");
    StereoTargetHost host={game,unity,stereo_target_image_size(game),stereo_target_image_size(unity),stereo_target_read,0};
    unsigned char actual[32];int reason=stereo_hook_targets_check(&host,r,s,actual);
    if(!(reported&(1u<<reason))) {
        reported|=1u<<reason;
        if(!reason) {
            event("stereo_target_render_rva",(U64)r-(U64)game,GetCurrentThreadId());
            event("stereo_target_submit_rva",(U64)s-(U64)unity,GetCurrentThreadId());
        } else if(reason==2) {
            const char *names[]={"stereo_target_render_prologue","stereo_target_render_prologue_8","stereo_target_render_prologue_16","stereo_target_render_prologue_24"};
            for(unsigned i=0;i<4;++i) {
                U64 value=0;for(unsigned j=0;j<8;++j)value|=(U64)actual[i*8+j]<<(j*8);
                event(names[i],value,GetCurrentThreadId());
            }
        } else {
            const char *names[]={"","stereo_target_render_range","","stereo_target_submit_resolve","stereo_target_submit_shape","stereo_target_submit_jump","stereo_target_same"};
            event(names[reason],reason,GetCurrentThreadId());
        }
    }
    if(reason)return 0;
    *render=r;*submit=s;
    return method;
}
#endif
