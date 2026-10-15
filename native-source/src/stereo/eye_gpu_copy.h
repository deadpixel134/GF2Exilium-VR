#ifndef GF2_EYE_GPU_COPY_H
#define GF2_EYE_GPU_COPY_H
/* D3D11 render-owner-thread by default, including polling/disposal. The optional
   fenced completion API explicitly transfers completed COM retirement under
   an exclusive interval CAS; immediate-context work remains render-owned.
   Zero initialize.
   Caller owns exclusive, unmapped textures and a serialized immediate context.
   Submit only at a proved boundary AFTER this ticket's eye render commands have
   reached that context. CPU root return/Present alone is not such proof.
   XR destinations must already be acquired+waited and retained until drained.
   This backend proves copy completion, never that Unity rendered either eye. */
typedef struct {
    unsigned long long serial,generation,epoch;
} EyeGpuTicket;
/* Numeric snapshot only; no COM ownership crosses into XR configuration. */
typedef struct {unsigned width,height,format;} EyeGpuDescription;
/* Optional same-call numeric diagnostics. No extra queries or retained COM
   interfaces; valid_mask identifies descriptors actually reached. */
typedef struct {
    unsigned width,height,mips,array,format,samples,quality,usage,bind,cpu,misc;
} EyeGpuObservedTexture;
typedef struct {
    unsigned stage,eye,reject_mask,valid_mask,device_flags,result;
    EyeGpuObservedTexture textures[4]; /* source L/R, acquired XR L/R */
} EyeGpuObservation;
int eye_gpu_describe_observed(void *,void *,void *,EyeGpuDescription *,EyeGpuObservation *);
int eye_gpu_describe(void *device,void *left,void *right,EyeGpuDescription *out);
/* Read-only resource preflight before event issuance. Caller holds valid native
   interfaces on a multithread-capable device. No immediate-context calls, GPU
   commands or lifetime transfer. Submit repeats validation on render owner. */
int eye_gpu_preflight(void *device,void *left,void *right,void *dl,void *dr);
int eye_gpu_preflight_observed(void *,void *,void *,void *,void *,EyeGpuObservation *);
enum { EGC_IDLE,EGC_PENDING,EGC_DRAINED,EGC_FAULT };
#ifdef GF2VR_PERF_FENCE_TIMING
typedef struct {
    unsigned long long signal,first_pending,last_pending,first_complete;
    unsigned polls,pending;
} EyeGpuTiming;
#endif
typedef struct {
    void *device,*context,*query,*textures[4],*transfer;
    EyeGpuTicket ticket;
    unsigned thread,state,drain_only;
#ifdef GF2VR_PERF_FENCE_DELIVERY
    void *fence;
    unsigned fence_armed,fence_completed;
#ifdef GF2VR_BANK_FENCE_OBSERVE
    unsigned observe_pending;
#endif
#ifdef GF2VR_PERF_FENCE_TIMING
    EyeGpuTiming timing;
#endif
#endif
} EyeGpuCopy;
int eye_gpu_submit(EyeGpuCopy *,void *device,void *context,
                   void *left,void *right,void *destination_left,void *destination_right,
                   EyeGpuTicket ticket);
/* Query-only retirement after a separately proved engine command ordering
   boundary. No XR destinations, no copies, no presentable-output permission.
   It cannot drain commands that Unity has not yet submitted to this context. */
int eye_gpu_drain(EyeGpuCopy *,void *device,void *context,void *left,void *right,EyeGpuTicket);
int eye_gpu_poll(EyeGpuCopy *); /* state; FAULT retains everything in quarantine */
int eye_gpu_current(const EyeGpuCopy *,EyeGpuTicket current);
int eye_gpu_dispose(EyeGpuCopy *); /* refuses pending/fault; no Unity calls */
#ifdef GF2VR_PERF_FENCE_DELIVERY
/* Caller must exclusively own the published interval via its state CAS.
   These never invoke immediate-context methods; completion reads fence/device
   only, disposal releases completed COM borrows. Unsupported returns PENDING. */
int eye_gpu_poll_fenced(EyeGpuCopy *);
int eye_gpu_dispose_fenced(EyeGpuCopy *);
#endif
#endif
