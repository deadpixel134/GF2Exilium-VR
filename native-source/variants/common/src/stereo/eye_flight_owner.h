#ifndef GF2_EYE_FLIGHT_OWNER_H
#define GF2_EYE_FLIGHT_OWNER_H
#include "eye_gpu_copy.h"

/* Unconnected two-flight ownership core. No XR/Unity calls or resource release.
   All transitions take a nonblocking internal lock. BUSY means RETRY the same
   event, never drop a completion. Caller supplies immutable pose/native payloads
   per slot; reads here are numeric snapshots, not borrowed pointers. A slot is
   reusable only after take + actual owner-thread cleanup + retire_ack.
   This does not schedule xrWaitFrame or authorize concurrent BeginFrame calls. */
#ifdef GF2VR_CAPACITY_EXPERIMENT
#include "capacity_experiment.h"
enum { EFO_BUSY=-1,EFO_REJECTED=0,EFO_OK=1,EFO_CAPACITY=GF2VR_EXPERIMENT_N };
#else
enum { EFO_BUSY=-1,EFO_REJECTED=0,EFO_OK=1,EFO_CAPACITY=2 };
#endif
enum {
    EFF_RESERVED=1u, EFF_RENDER_ISSUED=2u, EFF_CPU_RETURNED=4u,
    EFF_RENDER_DRAINED=8u, EFF_XR_BEGUN=16u, EFF_COPY_ISSUED=32u,
    EFF_COPY_DRAINED=64u, EFF_XR_ENDED=128u, EFF_CANCELLED=256u,
    EFF_FAULT=512u, EFF_RECLAIMING=1024u, EFF_RENDER_SUBMITTED=2048u,
    EFF_DISPLAY_DETACHED=4096u, EFF_PRESENTATION_RETIRED=8192u,
    EFF_PRODUCER_RETAINED=16384u, EFF_PRODUCER_TRANSFERRED=32768u,
    EFF_PRIVATE_PRODUCER=65536u
};
enum { EFO_BEGIN_EVENT=1,EFO_END_EVENT=2,EFO_COPY_EVENT=3 };
typedef struct {unsigned long long frame,generation,epoch;} EyeFlightToken;
typedef struct {
    EyeFlightToken token;
    EyeGpuTicket ticket;
    long long display_time;
    unsigned flags,renderable,cpu_ok;
    unsigned acquired_mask,waited_mask,released_mask;
    int begin_id,end_id,copy_id;
} EyeFlightSnapshot;
typedef struct {
    unsigned locked;
    unsigned long long generation,epoch,last_epoch,last_frame,last_begun,last_serial;
    long long last_display_time;
    int last_event_id;
    EyeFlightSnapshot slots[EFO_CAPACITY];
} EyeFlightOwner;

/* Zero-initialize once, NEVER reset while any slot or callback is outstanding.
   Activation requires both slots empty; frame/root/event IDs survive re-entry.
   Time ordering is per reference epoch, not a guessed cross-session clock. */
int eye_flight_activate(EyeFlightOwner *,unsigned long long generation,unsigned long long epoch);
int eye_flight_stop(EyeFlightOwner *); /* revoke; retain all outstanding leases */
/* Sole XR offer-writer uses this before reserving its one Wait payload.
   Main may only retire slots, never offer behind this capacity check. */
int eye_flight_available(EyeFlightOwner *,unsigned *,unsigned long long *,unsigned long long *);
/* Call AFTER a successful serialized WaitFrame and stable pose capture.
   At most one predicted but not yet begun frame. API adapter must reserve its
   own wait/payload slot before calling WaitFrame; this core is not that adapter. */
int eye_flight_offer(EyeFlightOwner *,EyeFlightToken,long long display_time,int renderable,unsigned *slot);
int eye_flight_snapshot(EyeFlightOwner *,EyeFlightToken,EyeFlightSnapshot *);
int eye_flight_slot(EyeFlightOwner *,EyeFlightToken,unsigned *slot);
int eye_flight_reserve_render(EyeFlightOwner *,EyeFlightToken,unsigned long long root_serial,EyeFlightSnapshot *);
/* Main root admission atomically selects/reserves the oldest eligible pose.
   No unlocked enumeration of XR-owned slot payloads. */
int eye_flight_reserve_next_render(EyeFlightOwner *,unsigned long long root_serial,EyeFlightSnapshot *,unsigned *slot);
int eye_flight_issue_render(EyeFlightOwner *,EyeFlightToken,EyeGpuTicket);
int eye_flight_cancel_unissued(EyeFlightOwner *,EyeFlightToken,EyeGpuTicket);
int eye_flight_cpu_return(EyeFlightOwner *,EyeFlightToken,EyeGpuTicket,int success);
/* Only authoritative backend drain/disposal may call these, never a timer,
   root return or Present. Distinct render and deferred-copy completion. */
int eye_flight_render_drained(EyeFlightOwner *,EyeFlightToken,EyeGpuTicket);
/* Exact-ticket validated CPU command submission, not GPU completion. */
int eye_flight_render_submitted(EyeFlightOwner *,EyeFlightToken,EyeGpuTicket);
#ifdef GF2VR_PRODUCER_TRANSFER
/* Actual once-issued private tail only; not CPU success or GPU completion. */
int eye_flight_private_producer(EyeFlightOwner *,EyeFlightToken,EyeGpuTicket);
#endif
int eye_flight_copy_drained(EyeFlightOwner *,EyeFlightToken,EyeGpuTicket);
/* Read-only callback route: unique IDs remain immutable until retirement. */
int eye_flight_event(EyeFlightOwner *,int event_id,EyeFlightSnapshot *,unsigned *phase);
/* Called after ordered successful BeginFrame; rejects a second begun frame.
   The actual XR API adapter must check eligibility BEFORE making API calls. */
int eye_flight_can_begin(EyeFlightOwner *,EyeFlightToken);
int eye_flight_begun(EyeFlightOwner *,EyeFlightToken);
int eye_flight_acquired(EyeFlightOwner *,EyeFlightToken,unsigned eye_mask);
int eye_flight_waited(EyeFlightOwner *,EyeFlightToken,unsigned eye_mask);
int eye_flight_issue_copy(EyeFlightOwner *,EyeFlightToken,EyeGpuTicket);
int eye_flight_can_release(EyeFlightOwner *,EyeFlightToken,unsigned eye_mask);
int eye_flight_released(EyeFlightOwner *,EyeFlightToken,unsigned eye_mask);
/* publish=0 may end a cancelled frame while PRIVATE render is pending, only
   if no XR destination remains in use. publish=1 requires both fences and
   exact display time/current generation. No stale relabeling. */
int eye_flight_can_end(EyeFlightOwner *,EyeFlightToken,long long display_time,int publish);
int eye_flight_ended(EyeFlightOwner *,EyeFlightToken,long long display_time,int publish);
#ifdef GF2VR_ASYNC_PRESENTATION
/* Dedicated opt-in path: caller must prove the REAL display End excluded both
   writing chains. No GPU completion/release is implied. Original token, pose
   and display_time survive; existing end/cancel behavior remains unchanged. */
int eye_flight_can_detach(EyeFlightOwner *,EyeFlightToken,long long display_time);
int eye_flight_detached(EyeFlightOwner *,EyeFlightToken,long long display_time);
/* Pending producer path, distinct from CPU-returned image detachment. Caller
   must reserve its writing bank BEFORE the real End and attest that End excluded
   that bank. No acquired images/copy are allowed at this boundary. Admission
   and CPU preparation may finish later using the ORIGINAL token/pose/time.
   The adapter owns bounded expiry; neither this transition nor replay renews it.
   These APIs alone do not authorize an XR call or choose a swapchain bank. */
int eye_flight_can_retain_offer(EyeFlightOwner *,EyeFlightToken,long long display_time);
int eye_flight_retained_offer(EyeFlightOwner *,EyeFlightToken,long long display_time);
/* Expiry/revocation stops admission without inventing CPU/GPU completion.
   Main must cancel any reserved-but-unissued ticket or return an issued root. */
int eye_flight_cancel_retained_offer(EyeFlightOwner *,EyeFlightToken);
/* After real image release and both drains, acknowledge late presentation or
   intentional discard. A detached slot cannot take/reuse before this ack. */
int eye_flight_detached_dispose(EyeFlightOwner *,EyeFlightToken,int published);
int eye_flight_can_detached_dispose(EyeFlightOwner *,EyeFlightToken,int published);
#ifdef GF2VR_BANK_TRANSFER
/* XR has copied immutable presentation metadata and holds the real released
   physical bank. This frees producer cleanup, NOT successful display End. */
int eye_flight_transfer(EyeFlightOwner *,EyeFlightToken,EyeGpuTicket);
#endif
#endif
/* Sole XR owner, AFTER actual EndSession and exited wait worker. Terminates
   frame obligations only: no GPU/CPU completion, image release or fault reset.
   Requires prior stop and every acquired image genuinely released. */
int eye_flight_session_stopped(EyeFlightOwner *,EyeFlightToken terminal[EFO_CAPACITY]);
int eye_flight_fault(EyeFlightOwner *,EyeFlightToken); /* permanent quarantine */
int eye_flight_take(EyeFlightOwner *,EyeFlightToken,EyeFlightSnapshot *);
int eye_flight_retire_ack(EyeFlightOwner *,EyeFlightToken);
#endif
