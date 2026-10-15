#ifndef GF2_XR_ORDERED_FRAMES_H
#define GF2_XR_ORDERED_FRAMES_H
#include "xr_frame_prefetch.h"
/* Actual Begin/End adapter around the wait-only worker. Sole XR-owner thread
   calls every method. Predictions may be taken before previous End, but never
   begun then. No pose/input/swapchain access here. The caller must reserve its
   finite payload capacity BEFORE requesting WaitFrame, and keep the session
   and API loader alive until close succeeds. Not yet bound to panel_xr.h. */
typedef struct {
    XrFramePrefetch prefetch;
    PFN_xrBeginFrame begin;
    PFN_xrEndFrame end;
    unsigned stopping,quarantined;
    unsigned long long pending_id,open_id;
    XrFrameState pending,open;
    XrResult pending_result;
} XrOrderedFrames;
int xr_ordered_frames_open(XrOrderedFrames *,XrSession,PFN_xrWaitFrame,PFN_xrBeginFrame,PFN_xrEndFrame);
int xr_ordered_frames_request(XrOrderedFrames *,unsigned long long frame,int payload_capacity_reserved);
int xr_ordered_frames_take(XrOrderedFrames *,unsigned long long *,XrFrameState *,XrResult *);
/* Returns 1 iff API was called, result always supplied/cleared. Success-class
   non-XR_SUCCESS results retain/balance obligations but disallow visual work. */
int xr_ordered_frames_begin(XrOrderedFrames *,unsigned long long,XrResult *);
int xr_ordered_frames_end(XrOrderedFrames *,unsigned long long,const XrFrameEndInfo *,XrResult *);
int xr_ordered_frames_visual(XrOrderedFrames *,unsigned long long);
/* Nonblocking. Stops new requests, permits balancing already waited frames
   with zero layers. Does not timeout-dispose an in-call worker or open frame. */
int xr_ordered_frames_close(XrOrderedFrames *);
/* Caller attests actual EndSession/platform frame-call reset, not mere stop.
   Worker must have exited before remaining obligations can be forgotten. */
int xr_ordered_frames_session_stopped(XrOrderedFrames *);
#endif
