#ifndef GF2_EYE_FLIGHT_BACKEND_H
#define GF2_EYE_FLIGHT_BACKEND_H
#include "eye_flight_owner.h"
#include "eye_render_interval.h"
#include "eye_event_copy.h"
/* Process-lived two-flight graphics adapter. Main owner prepares/acknowledges;
   actual render callbacks use fixed per-slot helpers, never a global active
   slot. XR posts waited destinations only after the selected route's exact
   private-render coverage and ordered submission/drain proof.
   Main/XR metadata lock is nonblocking: BUSY means retry unchanged. Rendering
   never takes that lock, calls XR/Unity, or releases a managed camera borrow.
   Backend -> numeric owner is the only lock order. No timer/Flush policy. */
typedef struct {
    EyeFlightSnapshot frame;
    void *refs[4]; /* owned device/context/private native pair */
    EyeRenderInterval render;
    EyeEventCopy copy;
    EyeIntervalResult render_result;
    EyeIntervalResult submitted_result; /* validated CPU submission; no GPU release */
    unsigned render_submitted;
#ifdef GF2VR_PRODUCER_TRANSFER
    unsigned producer_end_issued,producer_ack_pending;
#endif
    unsigned occupied,render_taken,render_ack,copy_requested,copy_started,copy_taken,copy_ack,fault_pending;
    unsigned long long child_queue; /* immutable before helper publication */
    void *destinations[2]; /* protected by numeric COPY_ISSUED until real drain */
} EyeFlightNativeSlot;
typedef struct {
    unsigned locked,owner_thread,render_thread;
    EyeFlightNativeSlot slots[EFO_CAPACITY];
#ifdef GF2VR_XR_SUBMISSION_ACK
    unsigned long long xr_submission_acks; /* sole XR writer; submission != drain */
#endif
#ifdef GF2VR_MAIN_TAIL_COMPLETION
    unsigned long long main_tail_copy_acks; /* main adds, XR atomically exchanges */
#endif
} EyeFlightBackend;
int eye_flight_backend_prepare(EyeFlightBackend *,EyeFlightOwner *,unsigned slot,EyeFlightToken,
    unsigned render_thread,int order_ready,void *device,void *context,void *left,void *right,unsigned format);
/* Production preparation freezes the already verified native child identity.
   It is independent of the main-thread currently selected root/slot. */
int eye_flight_backend_prepare_on_queue(EyeFlightBackend *,EyeFlightOwner *,unsigned slot,EyeFlightToken,
    unsigned render_thread,unsigned long long child_queue,int order_ready,
    void *device,void *context,void *left,void *right,unsigned format);
int eye_flight_backend_begin(EyeFlightBackend *,EyeFlightOwner *,unsigned slot,
    void (*issue)(void (*)(int),int),void (*callback)(int));
int eye_flight_backend_cancel_unissued(EyeFlightBackend *,EyeFlightOwner *,unsigned slot);
int eye_flight_backend_end(EyeFlightBackend *,unsigned slot,int order_ready,
    void (*issue)(void (*)(int),int),void (*callback)(int));
#ifdef GF2VR_PRODUCER_TRANSFER
int eye_flight_backend_producer_end(EyeFlightBackend *,EyeFlightOwner *,unsigned,
    int ordered,void (*issue)(void (*)(int),int),void (*callback)(int));
#endif
/* CPU scope returned; cancellation still needs the already issued tail/drain. */
int eye_flight_backend_cpu_return(EyeFlightBackend *,EyeFlightOwner *,unsigned slot,int success);
#ifdef GF2VR_XR_SUBMISSION_ACK
/* XR may publish an already released immutable render submission before main
   reconciliation. Backend -> numeric lock only; no native context, take, GPU
   completion, reference release, or image API. BUSY retains the same flight. */
int eye_flight_backend_publish_submission(EyeFlightBackend *,EyeFlightOwner *,EyeFlightToken);
#endif
/* XR caller must have genuinely acquired AND waited both exact frame images.
   This freezes their numeric lease BEFORE publishing native pointers. */
int eye_flight_backend_offer_copy(EyeFlightBackend *,EyeFlightOwner *,EyeFlightToken,void *,void *);
void eye_flight_backend_callback(EyeFlightBackend *,int event_id);
void eye_flight_backend_callback_on_queue(EyeFlightBackend *,int event_id,unsigned long long child_queue);
void eye_flight_backend_poll(EyeFlightBackend *);
#ifdef GF2VR_RENDER_OWNER_COPY
/* Existing selected-chain pre-Present render owner only. Same captured native
   device/context; no metadata lock, main/XR API, wait, Flush or extra event.
   Only an XR-posted exact request can execute; main still owns take/ack. */
void eye_flight_backend_service_copies(EyeFlightBackend *,void *device,void *context);
#endif
/* Main-thread only: take backend results exactly once; BUSY acknowledgements
   remain retained. May enqueue a deferred copy at the approved Unity event. */
int eye_flight_backend_reconcile(EyeFlightBackend *,EyeFlightOwner *,
    void (*issue)(void (*)(int),int),void (*callback)(int));
#ifdef GF2VR_MAIN_TAIL_COMPLETION
/* Same main owner, after the original root has returned. Nonblocking actual
   fence/DONE take and numeric ack only. Never issue a deferred copy, call the
   immediate context, or retire numeric/native/source/pose/image ownership. */
int eye_flight_backend_reconcile_completed(EyeFlightBackend *,EyeFlightOwner *);
#endif
/* After numeric take, before managed-borrow cleanup and pose-pool retire_ack.
   Does not itself mark the numeric flight reusable or release engine objects. */
int eye_flight_backend_retire(EyeFlightBackend *,EyeFlightOwner *,EyeFlightToken);
#endif

#ifdef GF2VR_EVENT_NOTICE_HANDOFF
/* Sole-XR caller validates its frame/session owner. Notice only schedules an
   actual fenced recheck + one-shot numeric drain ack, never native take/retire. */
int eye_flight_backend_notice(EyeFlightBackend *,EyeFlightOwner *,EyeGpuTicket,unsigned drain_only,unsigned xr_thread,unsigned *new_ack);
int eye_flight_backend_retirement_ready(EyeFlightBackend *,EyeFlightToken); /* main only */
#endif
