#ifndef GF2_XR_FRAME_PREFETCH_H
#define GF2_XR_FRAME_PREFETCH_H
#include <openxr/openxr.h>
/* One WAIT producer, not a second XR frame loop. The existing XR owner alone
   polls events, locates poses, acquires images and calls BeginFrame/EndFrame.
   No Unity, graphics, input, pose-reference or swapchain access on this worker.
   Unconnected until the full two-flight adapter is validated. */
enum { XFP_CLOSED,XFP_IDLE,XFP_QUEUED,XFP_RUNNING,XFP_READY,XFP_FAULT };
typedef struct {
    PFN_xrWaitFrame wait_frame;
    XrSession session;
    /* OS handles stay opaque at the renderer's public ABI boundary. */
    void *wake,*thread;
    unsigned owner_thread,state,stopping;
    unsigned long long last_requested,last_begun,requested,pending_frame,open_frame;
    XrFrameState packet;
    XrResult result;
} XrFramePrefetch;
/* Zero once. last_requested survives close/reopen to reject old frame IDs. */
int xr_frame_prefetch_open(XrFramePrefetch *,XrSession,PFN_xrWaitFrame);
/* Existing XR owner only. Call before initial Wait or AFTER acknowledging the
   previous successful BeginFrame. Never also call WaitFrame on that owner. */
int xr_frame_prefetch_request(XrFramePrefetch *,unsigned long long frame);
/* Returns 1 only for a completed call (including error). Output is cleared on
   pending/refusal. XR_SUCCESS owns a real waited frame: balance Begin/End or
   actual EndSession before disposal. shouldRender=false still must balance. */
int xr_frame_prefetch_take(XrFramePrefetch *,unsigned long long *frame,XrFrameState *,XrResult *);
int xr_frame_prefetch_can_begin(XrFramePrefetch *,unsigned long long frame);
int xr_frame_prefetch_begun(XrFramePrefetch *,unsigned long long frame);
int xr_frame_prefetch_ended(XrFramePrefetch *,unsigned long long frame);
/* Nonblocking shutdown. Refuses while WaitFrame is executing, a successful
   wait is unconsumed, or Begin/End is unbalanced. Never timeout-destroy session.
   Caller retains session/loader/module until success; stop does not cancel XR. */
int xr_frame_prefetch_close(XrFramePrefetch *);
/* Stop scheduling and attest worker exit, retaining every packet/handle.
   Required BEFORE EndSession; it must not be used to unblock an in-call Wait. */
int xr_frame_prefetch_quiesce(XrFramePrefetch *);
/* Only AFTER actual EndSession reset frame-call state AND the wait worker has
   exited. Explicit owner acknowledgement, not a timeout/force-close escape. */
int xr_frame_prefetch_session_stopped(XrFramePrefetch *);
#endif
