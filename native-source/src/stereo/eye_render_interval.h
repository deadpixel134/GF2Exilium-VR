#ifndef GF2_EYE_RENDER_INTERVAL_H
#define GF2_EYE_RENDER_INTERVAL_H
#include "eye_target_hooks.h"
/* Process-lived storage. Main owner prepares and issues two ordered events;
   render owner observes commands, drains the GPU and releases the watch.
   Queue/child validation belongs to the game adapter, before each callback.
   Missing callbacks/order or device failure quarantine, never timeout-release. */
enum { ERI_IDLE,ERI_PREPARING,ERI_QUEUED,ERI_RUNNING,ERI_ACTIVE,ERI_PENDING,ERI_DONE,ERI_FAULT };
/* Diagnostic provenance only, published with ERI_FAULT; never grants recovery. */
enum { ERIF_NONE,ERIF_END_WATCH,ERIF_GPU_SUBMIT,ERIF_REVERSED,
       ERIF_ABANDONED,ERIF_GPU_DISPOSE,ERIF_WATCH_RELEASE,ERIF_GPU_POLL };
typedef struct {
    unsigned state,owner_thread,render_thread,begin_issued,end_issued,abandoned;
    int begin_id,end_id,last_id;
    EyeGpuTicket ticket;
    void *refs[6];
    void *initial_views[2]; /* Prepared before publication; retained through drain. */
    EyeTargetWatch watch;
    EyeGpuCopy gpu;
#if defined(GF2VR_BANK_GPU_OBSERVE) || defined(GF2VR_BANK_EYE_WRITE_OBSERVE)
    EyeGpuDescription gpu_description;
#endif
#ifdef GF2VR_PERF_FENCE_TIMING
    EyeGpuTiming timing; /* Published with DONE; not completion authority. */
#endif
    unsigned attached,draw_mask,clear_mask,copy_allowed,copied,fault_reason;
} EyeRenderInterval;
typedef struct {EyeGpuTicket ticket;unsigned draw_mask,clear_mask,copied;} EyeIntervalResult;
int eye_interval_prepare(EyeRenderInterval *,int begin_id,int end_id,EyeGpuTicket,
                         unsigned render_thread,int order_ready,void *device,void *context,void *left,void *right);
/* Two-flight private render: initialize typed private targets and measure draw
   coverage without any acquired XR destination. Tail drains source work only.
   Caller retains the source pair through its subsequent deferred-copy fence;
   taking this interval alone is NOT permission to return that managed borrow. */
int eye_interval_prepare_initialized(EyeRenderInterval *,int,int,EyeGpuTicket,
    unsigned,int,void *,void *,void *,void *,unsigned typed_format);
/* Optional already acquired/waited destinations, retained before publication.
   Tail copies only when BOTH private eyes have a direct draw after a clear in
   this interval. This proves command coverage, not visible stereo/VFX quality. */
int eye_interval_prepare_copy(EyeRenderInterval *,int begin_id,int end_id,EyeGpuTicket,
                              unsigned render_thread,int order_ready,void *device,void *context,
                              void *left,void *right,void *destination_left,void *destination_right);
/* Owned private-eye baseline at the ordered begin, before any producer draws.
   typed_format is the already validated Unity/XR interpretation. */
int eye_interval_prepare_initialized_copy(EyeRenderInterval *,int,int,EyeGpuTicket,
    unsigned,int,void *,void *,void *,void *,void *,void *,unsigned typed_format);
/* Cancellation is legal only BEFORE either event was published. */
int eye_interval_cancel(EyeRenderInterval *,EyeGpuTicket);
int eye_interval_issue_begin(EyeRenderInterval *,void (*issue)(void (*)(int),int),void (*callback)(int));
int eye_interval_issue_end(EyeRenderInterval *,EyeGpuTicket,int order_ready,
                          void (*issue)(void (*)(int),int),void (*callback)(int));
/* Same authoritative tail, but retired source/output cannot authorize copies. */
int eye_interval_issue_retire(EyeRenderInterval *,EyeGpuTicket,int order_ready,
                             void (*issue)(void (*)(int),int),void (*callback)(int));
void eye_interval_callback(EyeRenderInterval *,int id);
/* CPU command-order evidence only, NOT GPU completion or release authority. */
int eye_interval_submitted(const EyeRenderInterval *,EyeIntervalResult *);
/* Abandon suppresses observations on the render owner; it does not prove GPU
   completion or permit any release, including after an uninjected root. */
void eye_interval_abandon(EyeRenderInterval *,EyeGpuTicket);
void eye_interval_poll(EyeRenderInterval *);
/* Completion-owner notification after actual drain, BEFORE publishing DONE to the
   main owner. It may return an XR lease, but must not release engine objects or
   reenter interval ownership. Result pointer is callback-stack-only. */
void eye_interval_poll_notify(EyeRenderInterval *,void (*notify)(const EyeIntervalResult *));
#ifdef GF2VR_PERF_FENCE_DELIVERY
/* Main/XR/render may claim PENDING exclusively. No GPU commands/GetData, no
   engine calls; a matched completed fence drains both eyes before one notify.
   Missing fence retains the render-owner query fallback. No timer release. */
void eye_interval_fence_notify(EyeRenderInterval *,void (*notify)(const EyeIntervalResult *));
#endif
/* Successful selected Present tail, render owner only. Submit buffered work
   once and observe for at most2ms/64 polls. Expiry retains the pending lease. */
void eye_interval_service_notify(EyeRenderInterval *,void (*notify)(const EyeIntervalResult *));
int eye_interval_take(EyeRenderInterval *,EyeIntervalResult *);
#endif
