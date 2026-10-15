#ifndef GF2_POSE_MATH_BINDING_H
#define GF2_POSE_MATH_BINDING_H
#include "pose_apply.h"
/* Main-thread-owned CRT reference, independent of the XR worker's reference.
   Owner lifetime must cover all callers; XR shutdown never closes this owner. */
__declspec(dllimport) BOOL WINAPI FreeLibrary(HINSTANCE);
typedef struct {HINSTANCE module;PoseMath api;unsigned borrowed;} PoseMathBinding;
static int pose_math_open(PoseMathBinding *b) {
    if(!b || b->module || b->borrowed) return 0;
    HINSTANCE module=0;if(!GetModuleHandleExW(0,L"ucrtbase.dll",&module)) return 0;
    PoseMath api={0};
    api.sin=(void *)GetProcAddress(module,"sin");api.cos=(void *)GetProcAddress(module,"cos");
    api.sqrt=(void *)GetProcAddress(module,"sqrt");api.atan2=(void *)GetProcAddress(module,"atan2");
    if(!api.sin || !api.cos || !api.sqrt || !api.atan2) {
        if(!FreeLibrary(module)) b->module=module;
        return 0;
    }
    b->module=module;b->api=api;return 1;
}
static int pose_math_close(PoseMathBinding *b) {
    if(!b || b->borrowed) return 0;
    if(b->module && !FreeLibrary(b->module)) return 0;
    *b=(PoseMathBinding){0};return 1;
}
static int pose_math_apply(PoseMathBinding *b,XrEyeMailbox *mailbox,const PoseSourceAnchor *anchor,
                            const ManagedEyeApi *api,ManagedEyes *eyes,float near_z,float far_z,
                            PoseAppliedTicket *ticket) {
    if(!ticket) return 0;
    *ticket=(PoseAppliedTicket){0};
    if(!b || !b->module || b->borrowed || !b->api.sin || !b->api.cos || !b->api.sqrt || !b->api.atan2) {
        if(eyes && !eyes->borrowed) eyes->matrices_ready=0;
        return 0;
    }
    b->borrowed=1;
    int ok=pose_apply(&b->api,mailbox,anchor,api,eyes,near_z,far_z,ticket);
    b->borrowed=0;return ok;
}
#endif
