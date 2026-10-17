#ifndef GF2_EYE_TARGET_WATCH_H
#define GF2_EYE_TARGET_WATCH_H
#include "eye_gpu_copy.h"
/* Render-owner interval, bound to retained COM identities of private eyes.
   Observations are command evidence, not pixel-content or GPU completion. */
enum { ETW_IDLE,ETW_ACTIVE,ETW_ENDED,ETW_BUSY };
typedef struct {
    unsigned state,thread,draw_mask,clear_mask;
    unsigned observe,draw_calls,clear_calls,predicated;
    unsigned raw_draw_calls,empty_draw_calls;
    EyeGpuTicket ticket;
    void *context,*identity[2];
} EyeTargetWatch;
/* Publish storage to foreign-thread callbacks only after first begin. Thread
   then stays immutable; only that owner reads/writes the remaining fields. */
int eye_target_watch_begin(EyeTargetWatch *,void *context,void *left,void *right,EyeGpuTicket);
/* Call only at the corresponding original D3D11 API boundary. Zero-size draw,
   predication, foreign context/thread and unrelated RTVs do not contribute. */
void eye_target_watch_draw(EyeTargetWatch *,void *context,unsigned nonempty);
void eye_target_watch_clear(EyeTargetWatch *,void *context,void *view);
int eye_target_watch_end(EyeTargetWatch *,EyeGpuTicket,unsigned *draw_mask,unsigned *clear_mask);
/* Same render owner, after authoritative matching GPU drain. End alone cannot
   release retained native resources; no timeout/overwrite/force-free route. */
int eye_target_watch_release(EyeTargetWatch *,EyeGpuTicket,int gpu_drained);
#ifdef GF2VR_PERF_FENCE_DELIVERY
/* Exclusive interval CAS owner, only after hooks detach + ETW_ENDED and actual
   matching fence drain. Pure completed-COM retirement, no context/Unity call. */
int eye_target_watch_release_completed(EyeTargetWatch *,EyeGpuTicket);
#endif
#endif
