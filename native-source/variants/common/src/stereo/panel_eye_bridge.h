#ifndef GF2_PANEL_EYE_BRIDGE_H
#define GF2_PANEL_EYE_BRIDGE_H
#include "xr_eye_runtime.h"
#include "initial_pose_key.h"
static XrEyeRuntime stereo_eye_runtime;
#ifdef GF2VR_AUTO_STEREO
/* Runtime constant; mutable only in the local assertion DLL. The main and XR
   owners retain independent intent/focus state, no cross-thread source writes. */
static unsigned stereo_auto_enabled=
#ifdef BOUNDARY_TEST
    0;
#else
    1;
#endif
static unsigned stereo_auto_xr_focused;
#endif
#ifdef GF2VR_INITIAL_CALIBRATION
static InitialPoseKey stereo_initial_key;
#ifdef BOUNDARY_TEST
static void (*stereo_initial_test_input)(int *,int *);
#endif
#endif
#ifdef BOUNDARY_TEST
static PFN_xrGetInstanceProcAddr stereo_eye_test_get;
#endif
static void stereo_eye_open(XrInstance instance,XrSession session,void *module) {
#ifdef GF2VR_AUTO_STEREO
    stereo_auto_xr_focused=0;
#endif
    PFN_xrGetInstanceProcAddr get=0;
#ifdef BOUNDARY_TEST
    get=stereo_eye_test_get;
#endif
    if(!get && module) get=(PFN_xrGetInstanceProcAddr)GetProcAddress(module,"xrGetInstanceProcAddr");
    if(!xr_eye_runtime_open(&stereo_eye_runtime,get,instance,session))
        event("eye_input_unavailable",0,GetCurrentThreadId());
#ifdef GF2VR_INITIAL_CALIBRATION
    else {
        stereo_eye_runtime.owner.require_calibration=1;
#ifdef GF2VR_AUTO_STEREO
        stereo_eye_runtime.owner.level_initial_reference=stereo_auto_enabled;
#endif
        stereo_initial_key=(InitialPoseKey){0};
        event("initial_pose_confirmation_required",stereo_eye_runtime.owner.epoch,GetCurrentThreadId());
    }
#endif
}
/* Actual current-frame callback shared by the legacy loop and ordered driver.
   F9 is sampled here, never on the Wait worker or during future prediction. */
static int stereo_eye_frame_prepare(void *context,unsigned long long frame,const XrFrameState *state,
        unsigned predicted_attempted,const XrEyeInput *prediction,XrEyeInput *out) {
    (void)context;
    if(!out)return 0;
    *out=(XrEyeInput){0};
#ifdef GF2VR_INITIAL_CALIBRATION
    DWORD saved=GetLastError();
    HINSTANCE user=GetModuleHandleW(L"user32.dll");
    short (WINAPI *key)(int)=user?(void *)GetProcAddress(user,"GetAsyncKeyState"):0;
    void *(WINAPI *foreground)(void)=user?(void *)GetProcAddress(user,"GetForegroundWindow"):0;
    DWORD (WINAPI *process)(void *,DWORD *)=user?(void *)GetProcAddress(user,"GetWindowThreadProcessId"):0;
    DWORD pid=0;void *window=foreground?foreground():0;
    if(window && process) process(window,&pid);
    int focused=key && pid==GetCurrentProcessId();
    int down=focused && ((unsigned short)key(0x78)&0x8000u);
#ifdef BOUNDARY_TEST
    if(stereo_initial_test_input) stereo_initial_test_input(&focused,&down);
#endif
    int requested=initial_pose_key_update(&stereo_initial_key,focused,down) &&
        xr_eye_owner_confirm_initial(&stereo_eye_runtime.owner);
#ifdef GF2VR_AUTO_STEREO
    /* Bootstrap only, at actual current-frame preparation. Prediction never
       chooses neutral. A failed auto attempt may retry next eligible frame;
       an established reference can never be overwritten by this operation. */
    int auto_requested=!requested && stereo_auto_enabled && focused && stereo_auto_xr_focused &&
        !predicted_attempted && state && state->shouldRender &&
        xr_eye_owner_confirm_initial(&stereo_eye_runtime.owner);
#endif
    int captured=predicted_attempted?
        xr_eye_runtime_publish_prediction(&stereo_eye_runtime,prediction,frame,state):
        xr_eye_runtime_capture(&stereo_eye_runtime,frame,state);
    if(requested) event(captured?"initial_pose_confirmed":"initial_pose_confirmation_failed",
                       stereo_eye_runtime.owner.epoch,GetCurrentThreadId());
#ifdef GF2VR_AUTO_STEREO
    else if(auto_requested && captured)
        event("initial_pose_auto_confirmed",stereo_eye_runtime.owner.epoch,GetCurrentThreadId());
#endif
    SetLastError(saved);
#else
    int captured=predicted_attempted?
        xr_eye_runtime_publish_prediction(&stereo_eye_runtime,prediction,frame,state):
        xr_eye_runtime_capture(&stereo_eye_runtime,frame,state);
    (void)xr_eye_owner_confirm_initial;(void)initial_pose_key_update;
#endif
    return captured && xr_eye_mailbox_read(&stereo_eye_runtime.mailbox,out);
}
static void stereo_eye_frame(unsigned long long frame,const XrFrameState *state) {
    XrEyeInput current;
    (void)stereo_eye_frame_prepare(0,frame,state,0,0,&current);
}
static void stereo_eye_change(const XrEventDataReferenceSpaceChangePending *change) {
    (void)xr_eye_runtime_change(&stereo_eye_runtime,change);
}
#define GF2VR_EYE_OPEN(instance,session,module) stereo_eye_open(instance,session,module)
#define GF2VR_EYE_FRAME(number,frame) stereo_eye_frame(number,frame)
#define GF2VR_EYE_CHANGE(change) stereo_eye_change(change)
#define GF2VR_EYE_INVALIDATE() xr_eye_mailbox_invalidate(&stereo_eye_runtime.mailbox)
#define GF2VR_EYE_CLOSE() xr_eye_runtime_close(&stereo_eye_runtime)
#endif
