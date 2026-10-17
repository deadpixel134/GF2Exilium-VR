#ifndef GF2_EYE_EVENT_COPY_H
#define GF2_EYE_EVENT_COPY_H
#include "eye_gpu_copy.h"
enum { EEC_IDLE,EEC_QUEUED,EEC_RUNNING,EEC_PENDING,EEC_DONE,EEC_FAULT };
#ifdef GF2VR_RENDER_OWNER_COPY
enum { EEC_PRIMED=6 }; /* no destination, GPU work or completion claim */
#endif
typedef struct {
    unsigned state,owner_thread,render_thread;
    int event_id,last_event_id,issued;
    unsigned drain_only;
#ifdef GF2VR_RENDER_OWNER_COPY
    unsigned native_dispatch;
#endif
    EyeGpuTicket ticket;
    void *refs[6]; /* device, immediate context, source pair, destination pair */
    EyeGpuCopy gpu;
#ifdef GF2VR_BANK_COPY_DELIVERY_OBSERVE
    EyeGpuTiming timing; /* numeric only; immutable with actual DONE */
#endif
} EyeEventCopy;
/* Owner thread only. Caller has proved same-context submission order and holds
   valid unmapped native sources + acquired/waited XR images. All six interfaces
   are retained BEFORE publishing QUEUED. Explicit order_ready is a prerequisite
   supplied by the adapter, not an observation manufactured by this module.
   IDs increase for this process-lived slot; never reuse/wrap them. */
int eye_event_prepare(EyeEventCopy *,int event_id,EyeGpuTicket,unsigned render_thread,
                     int order_ready,void *device,void *context,void *left,void *right,
                     void *destination_left,void *destination_right);
/* Failed/unpublished frame retirement: retain only device/context/source pair.
   Same authoritative engine-order prerequisite; never grants copy/presentation. */
int eye_event_prepare_drain(EyeEventCopy *,int event_id,EyeGpuTicket,unsigned render_thread,
                           int order_ready,void *device,void *context,void *left,void *right);
/* Owner calls the previously validated Unity callback+ID entry. Storage and
   callback code must remain alive even if the void enqueue never calls back.
   No automatic native hook, unverified address selection or timeout cancellation. */
int eye_event_issue(EyeEventCopy *,void (*issue)(void (*)(int),int),void (*callback)(int));
#ifdef GF2VR_RENDER_OWNER_COPY
/* Main primes immutable sources before Begin. After genuine XR Acquire/Wait,
   the backend's numeric COPY_ISSUED pin authorizes exact destinations to be
   posted once. Posting never executes GPU work or transfers acknowledgement
   ownership. A verified render-owner context consumes the posted payload. */
int eye_event_prime(EyeEventCopy *,int,EyeGpuTicket,unsigned,int,void *,void *,void *,void *);
int eye_event_post_native(EyeEventCopy *,EyeGpuTicket,void *,void *);
void eye_event_service_native(EyeEventCopy *,void *device,void *context);
/* Main reclaim only, while the backend lock excludes future XR posting.
   Refuses any posted/running/GPU-owned request; never invents a drain. */
int eye_event_cancel_prime(EyeEventCopy *);
#endif
/* From the actual matching Unity render event. No managed or XR calls. */
void eye_event_callback(EyeEventCopy *,int event_id);
/* On the same render owner at a later safe tick. Never waits or flushes. */
void eye_event_poll(EyeEventCopy *);
#ifdef GF2VR_PERF_FENCE_DELIVERY
/* Optional exclusive fenced completion, no immediate-context methods.
   Missing/unsupported marker stays pending for render-owner query fallback.
   Only backend GPU refs retire here; request refs still require owner take. */
void eye_event_fence_complete(EyeEventCopy *);
#endif
/* Owner-thread acknowledgement only. Returns the exact completed ticket.
   This result proves copy drain only. Caller reconciles its SourceRenderFrame
   serial before releasing that borrow; XR release remains XR-owner work. */
int eye_event_take(EyeEventCopy *,EyeGpuTicket *);
/* Separate acknowledgement prevents a query-only completion being mistaken
   for copied XR output. Both take functions reject the other request kind. */
int eye_event_take_drain(EyeEventCopy *,EyeGpuTicket *);
#endif
