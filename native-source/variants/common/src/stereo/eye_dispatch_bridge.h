#ifndef GF2_EYE_DISPATCH_BRIDGE_H
#define GF2_EYE_DISPATCH_BRIDGE_H
/*012 proves different slot targets at this exact pre-Draw return boundary.
  Production bridge has no sample/time cap and owns no borrowed resources. */
typedef U64 (*EyeDispatchPrepare)(U64,U64,U64,U64);
static EyeDispatchPrepare eye_dispatch_original;
static unsigned eye_dispatch_attempted,eye_dispatch_ready;
static DWORD eye_dispatch_thread;
static U64 eye_dispatch_child,eye_dispatch_context;
static U64 eye_dispatch_hook(U64 a,U64 b,U64 c,U64 d) {
    U64 caller=(U64)__builtin_return_address(0);
    U64 result=eye_dispatch_original(a,b,c,d);DWORD saved=GetLastError();
    if(caller==(U64)unity+0x4589a9 && GetCurrentThreadId()==eye_dispatch_thread &&
       a==eye_dispatch_child && panel_same_device((void *)b,(void *)eye_dispatch_context)) {
        /* Unity may pass a context wrapper. Match COM identity, then refresh
           the retained native context whose driver dispatch we observe. */
        if(!eye_target_hooks_refresh((void *)eye_dispatch_context)) {
            static unsigned reported;
            if(!reported){reported=1;event("eye_dispatch_refresh_failed",0,GetCurrentThreadId());}
        }
    }
    SetLastError(saved);return result;
}
static int eye_dispatch_install(U64 child,void *context) {
    if(eye_dispatch_attempted) return eye_dispatch_ready;
    eye_dispatch_attempted=1;
    U64 table=0,read=0;
    static const unsigned char prefix[]={0x48,0x89,0x5c,0x24,0x08,0x44,0x89,0x44,0x24,0x18,0x57,0x48,0x83,0xec,0x40};
    if(!unity || !child || !context ||
       !ReadProcessMemory((HANDLE)(long long)-1,(void *)child,&table,8,&read) || read!=8 ||
       table!=(U64)(unity+0x14babc8) || !stereo_hook_bytes(unity+0x460a60,prefix,sizeof(prefix))) return 0;
    eye_dispatch_thread=GetCurrentThreadId();eye_dispatch_child=child;eye_dispatch_context=(U64)context;
    if(MH_CreateHook(unity+0x460a60,(void *)eye_dispatch_hook,(void **)&eye_dispatch_original)) return 0;
    if(MH_EnableHook(unity+0x460a60)){(void)MH_DisableHook(unity+0x460a60);return 0;}
    eye_dispatch_ready=1;return 1;
}
#ifdef BOUNDARY_TEST
static U64 eye_dispatch_test_args[4];static unsigned eye_dispatch_test_error;
__declspec(noinline) static U64 eye_dispatch_fake(U64 a,U64 b,U64 c,U64 d) {
    if(GetLastError()!=193) eye_dispatch_test_error=1;
    eye_dispatch_test_args[0]=a;eye_dispatch_test_args[1]=b;eye_dispatch_test_args[2]=c;eye_dispatch_test_args[3]=d;
    SetLastError(391);return 0xfedcba9876543210ULL;
}
__declspec(dllexport) int eye_dispatch_bridge_test(void) {
    int status=MH_Initialize();if(status!=0 && status!=1) return __LINE__;
    if(MH_CreateHook((void *)eye_dispatch_fake,(void *)eye_dispatch_hook,(void **)&eye_dispatch_original) ||
       MH_EnableHook((void *)eye_dispatch_fake)) return __LINE__;
    EyeDispatchPrepare volatile call=eye_dispatch_fake;SetLastError(193);
    U64 result=call(11,22,33,44);unsigned error=GetLastError();
    int disabled=MH_DisableHook((void *)eye_dispatch_fake);
    if(disabled || result!=0xfedcba9876543210ULL || error!=391 || eye_dispatch_test_error) return __LINE__;
    for(unsigned i=0;i<4;++i) if(eye_dispatch_test_args[i]!=11*(i+1)) return __LINE__;
    return 0;
}
#endif
#endif
