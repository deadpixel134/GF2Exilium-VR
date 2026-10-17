#ifndef GF2_FOCUS_PROJECTION_RUNTIME_H
#define GF2_FOCUS_PROJECTION_RUNTIME_H
#include "focus_projection.h"
static struct {
    U64 frame,serial;
    float views[2][16],bases[2][16];
    unsigned ready,attempted,samples;
} focus_projection_state;
static void (*focus_command_original)(void *,const float *,const float *);
static void focus_projection_optics(U64 frame,const void *views,const void *bases) {
    focus_projection_state.frame=frame;
    focus_projection_state.serial=stereo_pipeline.render.serial;
    for(unsigned e=0;e<2;++e) for(unsigned i=0;i<16;++i) {
        focus_projection_state.views[e][i]=((const ManagedEyeMatrix *)views)[e].elements[i];
        focus_projection_state.bases[e][i]=((const ManagedEyeMatrix *)bases)[e].elements[i];
    }
}
static void focus_command_forward(void *command,const float *view,const float *projection) {
    DWORD saved=GetLastError();
    /* Short-circuit before touching owner-only state from another thread. No
       managed calls/references here; the original wrapper copies both matrices
       synchronously into command storage. No pointer to our stack is retained. */
    if(GetCurrentThreadId()==stereo_pipeline.owner_thread && focus_projection_state.ready &&
       stereo_pipeline.render.active && stereo_pipeline.render.injected &&
       stereo_submit_scope.depth==1 && stereo_submit_scope.context && !stereo_submit_scope.ambiguous &&
       stereo_submit_scope.serial==stereo_pipeline.render.serial &&
       stereo_source_frame.serial==stereo_pipeline.render.serial &&
       focus_projection_state.serial==stereo_source_frame.serial &&
       focus_projection_state.frame==stereo_source_frame.check.ticket.frame_id &&
       source_render_frame_current(&stereo_source_frame)) {
        float v[16],p[16],corrected[16];U64 read=0;
        if(ReadProcessMemory((HANDLE)(long long)-1,view,v,sizeof(v),&read) && read==sizeof(v) &&
           ReadProcessMemory((HANDLE)(long long)-1,projection,p,sizeof(p),&read) && read==sizeof(p)) {
            int eye=focus_projection_correct(focus_projection_state.views,focus_projection_state.bases,v,p,corrected);
            if(eye && source_render_frame_current(&stereo_source_frame)) {
                if(focus_projection_state.samples<16) {
                    ++focus_projection_state.samples;
                    event("focus_projection_corrected",((U64)eye<<56)|stereo_source_frame.serial,GetCurrentThreadId());
                }
                SetLastError(saved);focus_command_original(command,view,corrected);return;
            }
        }
    }
    SetLastError(saved);focus_command_original(command,view,projection);
}
static int focus_projection_install(void) {
    if(focus_projection_state.attempted) return focus_projection_state.ready;
    focus_projection_state.attempted=1;
    HINSTANCE game=GetModuleHandleW(L"GameAssembly.dll");
    void *(*resolve)(const char *)=(void *)GetProcAddress(game,"il2cpp_resolve_icall");
    if(!unity || !resolve) return 0;
    void *target=resolve("UnityEngine.Rendering.CommandBuffer::SetViewProjectionMatrices_Injected");
    static const unsigned char expected[]={0x48,0x89,0x5c,0x24,0x10,0x57,0x48,0x81,0xec,0xc0,0,0,0,
        0x49,0x8b,0xd8,0x48,0xc7,0x44,0x24,0x20,0,0,0,0,0x4c,0x8b,0xc1,0x48,0x8b,0xfa};
    if(target!=unity+0x9377b0 || !stereo_hook_bytes(target,expected,sizeof(expected))) return 0;
    if(MH_CreateHook(target,focus_command_forward,(void **)&focus_command_original)) return 0;
    /* Same process-pinned lifetime as existing hooks. Never free a published
       trampoline, including a failed enable. Inert callback always forwards. */
    if(MH_EnableHook(target)) return 0;
    focus_projection_state.ready=1;return 1;
}
#ifdef BOUNDARY_TEST
static const float *focus_test_pointer;
static float focus_test_matrix[16];
static DWORD focus_test_error;
static void focus_test_forward(void *command,const float *v,const float *p) {
    (void)command;(void)v;focus_test_pointer=p;focus_test_error=GetLastError();
    U64 read=0;(void)ReadProcessMemory((HANDLE)(long long)-1,p,focus_test_matrix,sizeof(focus_test_matrix),&read);
    SetLastError(456);
}
__declspec(dllexport) int focus_projection_runtime_test(void) {
    __typeof__(focus_projection_state) saved_state=focus_projection_state;
    __typeof__(stereo_submit_scope) saved_submit=stereo_submit_scope;
    CameraPipelineScope saved_pipeline=stereo_pipeline;
    SourceRenderFrame saved_frame=stereo_source_frame;
    void (*saved_original)(void *,const float *,const float *)=focus_command_original;
    int result=0;
#define FP_CHECK(x) do {if(!(x)){result=__LINE__;goto done;}}while(0)
    XrEyeMailbox mailbox={0};mailbox.epoch=2;mailbox.frame=3;
    ManagedEyes eyes={0};eyes.borrowed=eyes.ready=eyes.targets_ready=1;eyes.matrix_frame=3;
    SourceRenderSelection selection={0};selection.source=&eyes;selection.generation=4;selection.eligible=1;selection.units_per_meter=1;
    CameraDataOwner data={0};data.binding=(void *)&data;data.ready=1;data.generation=4;data.source_camera=&eyes;
    stereo_source_frame=(SourceRenderFrame){0};stereo_source_frame.data=&data;stereo_source_frame.selection=&selection;
    stereo_source_frame.borrowed=1;stereo_source_frame.serial=9;stereo_source_frame.epoch=2;stereo_source_frame.units=1;
    stereo_source_frame.check=(PoseRenderCheck){.mailbox=&mailbox,.ticket={2,3,4,5},.eyes=&eyes,.source_generation=&selection.generation};
    stereo_pipeline.owner_thread=GetCurrentThreadId();stereo_pipeline.render.active=1;stereo_pipeline.render.injected=1;stereo_pipeline.render.serial=9;
    stereo_submit_scope.depth=1;stereo_submit_scope.serial=9;stereo_submit_scope.context=&data;stereo_submit_scope.ambiguous=0;
    focus_projection_state.ready=1;focus_projection_state.serial=9;focus_projection_state.frame=3;
    focus_projection_state.samples=16;
    float v[2][16]={{0}},p[2][16]={{0}},j[16];
    for(unsigned e=0;e<2;++e) {
        v[e][0]=v[e][5]=v[e][10]=v[e][15]=1;v[e][12]=e?-.032f:.032f;
        p[e][0]=p[e][5]=1;p[e][8]=e?.07f:-.07f;p[e][9]=-.035f;p[e][10]=-1;p[e][11]=-1;p[e][14]=-.2f;
        for(unsigned i=0;i<16;++i){focus_projection_state.views[e][i]=v[e][i];focus_projection_state.bases[e][i]=p[e][i];}
    }
    for(unsigned i=0;i<16;++i) j[i]=p[0][i];j[8]=.0002f;j[9]=-.0001f;
    focus_command_original=focus_test_forward;
    SetLastError(123);focus_command_forward(&data,v[0],j);
    FP_CHECK(focus_test_pointer!=j && focus_test_matrix[8]==p[0][8]+j[8] && focus_test_error==123 && GetLastError()==456);
    /* Invalidate each owning boundary separately; original pointers preserved. */
    for(unsigned test=0;test<8;++test) {
        if(test==0) stereo_pipeline.owner_thread++;
        if(test==1) stereo_submit_scope.depth=2;
        if(test==2) stereo_pipeline.render.injected=0;
        if(test==3) focus_projection_state.serial=8;
        if(test==4) mailbox.epoch=0;
        if(test==5) selection.generation=7;
        if(test==6) data.retiring=1;
        if(test==7) focus_projection_state.ready=0;
        SetLastError(123);focus_command_forward(&data,v[0],j);
        FP_CHECK(focus_test_pointer==j && focus_test_error==123 && GetLastError()==456);
        stereo_pipeline.owner_thread=GetCurrentThreadId();stereo_submit_scope.depth=1;stereo_pipeline.render.injected=1;
        focus_projection_state.serial=9;mailbox.epoch=2;selection.generation=4;data.retiring=0;focus_projection_state.ready=1;
    }
    focus_command_forward(&data,(const float *)1,j);FP_CHECK(focus_test_pointer==j);
done:
    focus_projection_state=saved_state;stereo_submit_scope=saved_submit;stereo_pipeline=saved_pipeline;
    stereo_source_frame=saved_frame;focus_command_original=saved_original;
#undef FP_CHECK
    return result;
}
#endif
#endif
