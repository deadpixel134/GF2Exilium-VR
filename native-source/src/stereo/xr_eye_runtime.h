#ifndef GF2_XR_EYE_RUNTIME_H
#define GF2_XR_EYE_RUNTIME_H
#include "xr_eye_binding.h"
#include "xr_eye_mailbox.h"
/* Integration entry points for the EXISTING XR worker. No second frame loop.
   Zero-initialize once and preserve the object across session recreation.
   Unity reads only mailbox; other members are XR-worker-owned. */
typedef struct {
    XrEyeBinding binding;XrEyeOwner owner;XrEyeMailbox mailbox;
    /* XR-thread publication clock, separate from the latest located future
       prediction. Reset only when a new physical XR session opens. */
    unsigned long long published_frame;
} XrEyeRuntime;
static int xr_eye_runtime_close(XrEyeRuntime *r) {
    xr_eye_mailbox_invalidate(&r->mailbox);
    return xr_eye_binding_close(&r->binding,&r->owner);
}
static int xr_eye_runtime_open(XrEyeRuntime *r,PFN_xrGetInstanceProcAddr get,
                               XrInstance instance,XrSession session) {
    /* Do not reset an active reference on an accidental duplicate open. */
    if(r->owner.space || r->owner.session || r->binding.math_module) return 0;
    r->published_frame=0;
    xr_eye_mailbox_invalidate(&r->mailbox);
    if(!xr_eye_binding_open(&r->binding,get,instance) ||
       !xr_eye_owner_open(&r->owner,&r->binding.api,session)) {
        xr_eye_runtime_close(r);return 0;
    }
    return 1;
}
/* Locate a future prediction into caller-owned immutable storage. Never touch
   the current-frame mailbox or consume/reseed an initial F9 confirmation.
   A real reference-space transition must be processed by the current-frame
   route, not silently completed ahead of that frame. Sole XR thread only. */
static int xr_eye_runtime_predict(XrEyeRuntime *r,unsigned long long frame_id,
                                 const XrFrameState *frame,XrEyeInput *out) {
    if(!out)return 0;
    *out=(XrEyeInput){0};
    if(!r || !r->owner.epoch || r->owner.changing || r->owner.retiring ||
       r->owner.calibration_request ||
       r->owner.head_reference_epoch!=r->owner.epoch)return 0;
    return xr_eye_owner_capture(&r->owner,frame_id,frame,out);
}
/* Publish exactly once when the predicted frame actually becomes current.
   Invalid/future/stale publication must fail closed for the current mailbox.
   No LocateViews/LocateSpace or input/navigation sampling is repeated here. */
static int xr_eye_runtime_publish_prediction(XrEyeRuntime *r,const XrEyeInput *packet,
                                            unsigned long long current_frame,
                                            const XrFrameState *frame) {
    if(!r)return 0;
    xr_eye_mailbox_invalidate(&r->mailbox);
    if(!frame || frame->type!=XR_TYPE_FRAME_STATE || !frame->shouldRender ||
       !current_frame || current_frame<=r->published_frame ||
       current_frame!=r->owner.last_frame || r->owner.changing || r->owner.retiring ||
       r->owner.head_reference_epoch!=r->owner.epoch ||
       !xr_eye_matches(packet,r->owner.epoch,current_frame,frame->predictedDisplayTime))return 0;
    if(!xr_eye_mailbox_publish(&r->mailbox,packet))return 0;
    r->published_frame=current_frame;return 1;
}
static int xr_eye_runtime_capture(XrEyeRuntime *r,unsigned long long frame_id,const XrFrameState *frame) {
    /* Invalidate before LocateViews, so a blocked/failed query cannot expose
       the prior frame while the producer is working on the new prediction. */
    if(!r)return 0;
    (void)xr_eye_runtime_predict; /* future-prediction adapter, disabled in legacy loop */
    xr_eye_mailbox_invalidate(&r->mailbox);
    XrEyeInput packet={0};
    if(!xr_eye_owner_capture(&r->owner,frame_id,frame,&packet)) return 0;
    return xr_eye_runtime_publish_prediction(r,&packet,frame_id,frame);
}
static int xr_eye_runtime_change(XrEyeRuntime *r,const XrEventDataReferenceSpaceChangePending *event) {
    if(!xr_eye_owner_change(&r->owner,event)) return 0;
    xr_eye_mailbox_invalidate(&r->mailbox);return 1;
}
#endif
