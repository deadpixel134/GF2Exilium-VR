#ifndef GF2_ROOT_INTERVAL_OBSERVE_H
#define GF2_ROOT_INTERVAL_OBSERVE_H
#ifdef GF2VR_ROOT_INTERVAL_TIMING_OBSERVE
#if !defined(GF2VR_NUMERIC_RESIDENCY) || !defined(GF2VR_NUMERIC_LOSS_OBSERVE)
#error Root interval timing requires the existing immutable numeric loss ring
#endif
#include "numeric_residency.h"
/* Observation only: PID + generation/epoch/serial joins immutable native tags
   to numeric/main frames. Native frame0 does not claim a live source slot.
   flags stores the ACTUAL writer TID, not deferred persistence's logging TID.
   No current/snapshot/engine/XR/GPU call, timeout or completion grant. */
enum { RIT_BOUNDARY=12,RIT_RENDER_ENTER=1,RIT_RENDER_RETURN,RIT_SUBMIT_ENTER,
    RIT_SUBMIT_RETURN,RIT_FINISH_ENTER,RIT_FINISH_RETURN,RIT_BEGIN_ENTER,
    RIT_BEGIN_RETURN,RIT_TAIL_ENTER,RIT_TAIL_RETURN,RIT_GPU_DRAIN,RIT_NATIVE_CLEAN };
typedef struct {unsigned long long owner,frame,generation,epoch,serial;} RootIntervalTag;
static inline void rit_record(RootIntervalTag tag,unsigned phase,int result) {
    if(!tag.owner || !tag.generation || !tag.epoch || !tag.serial)return;
    unsigned saved=GetLastError();unsigned thread=GetCurrentThreadId();
    numeric_residency_loss(tag.owner,tag.frame,tag.generation,tag.epoch,tag.serial,
        RIT_BOUNDARY,phase,result,thread);
    SetLastError(saved);
}
static inline void rit_render_call(RootIntervalTag tag,
    void (*original)(void *,void *,void *,void *),void *pipeline,void *context,void *cameras,void *method) {
    unsigned incoming=GetLastError();rit_record(tag,RIT_RENDER_ENTER,1);SetLastError(incoming);
    original(pipeline,context,cameras,method);
    unsigned outgoing=GetLastError();rit_record(tag,RIT_RENDER_RETURN,1);SetLastError(outgoing);
}
static inline void rit_submit_call(RootIntervalTag tag,void (*original)(void *),void *storage) {
    unsigned incoming=GetLastError();rit_record(tag,RIT_SUBMIT_ENTER,1);SetLastError(incoming);
    original(storage);
    unsigned outgoing=GetLastError();rit_record(tag,RIT_SUBMIT_RETURN,1);SetLastError(outgoing);
}
#endif
#endif
