#ifndef GF2_XR_EYE_CONFIG_H
#define GF2_XR_EYE_CONFIG_H
/* One Unity producer, one XR consumer. Payload contains numbers only and is
   written ONLY by Unity, after IDLE -> WRITING. XR reads it only after claiming
   REQUEST. READY payload remains immutable until XR closes the old output.
   No locks, Unity calls, waits, or native pointers cross this channel. */
enum { XEC_IDLE,XEC_WRITING,XEC_REQUEST,XEC_PROCESSING,XEC_READY,XEC_FAILED,XEC_RETIRING };
typedef struct {
    unsigned state;
    EyeGpuDescription description;
    unsigned long long epoch;
} XrEyeConfig;
static int xr_eye_config_same(const EyeGpuDescription *a,const EyeGpuDescription *b) {
    return a->width==b->width && a->height==b->height && a->format==b->format;
}
/* Unity only, including reads of its own immutable request. READY means chains
   exist, not acquired images, submitted eye commands, or GPU completion. */
static int xr_eye_config_ready(XrEyeConfig *q,const EyeGpuDescription *d,unsigned long long epoch) {
    return q && d && epoch && __atomic_load_n(&q->state,__ATOMIC_ACQUIRE)==XEC_READY &&
        q->epoch==epoch && xr_eye_config_same(&q->description,d) &&
        __atomic_load_n(&q->state,__ATOMIC_ACQUIRE)==XEC_READY;
}
static int xr_eye_config_request(XrEyeConfig *q,const EyeGpuDescription *d,unsigned long long epoch) {
    if(!q || !d || !epoch || !d->width || !d->height || !d->format) return 0;
    unsigned expected=XEC_IDLE;
    if(__atomic_compare_exchange_n(&q->state,&expected,XEC_WRITING,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE)) {
        q->description=*d;q->epoch=epoch;
        __atomic_store_n(&q->state,XEC_REQUEST,__ATOMIC_RELEASE);return 0;
    }
    if((expected==XEC_READY || expected==XEC_FAILED) &&
       (q->epoch!=epoch || !xr_eye_config_same(&q->description,d))) {
        /* Retire the old immutable payload first. Do not overwrite a request
           or handles which XR/GPU may still own. Next Unity frame resubmits
           its then-current description after XR acknowledges IDLE. */
        (void)__atomic_compare_exchange_n(&q->state,&expected,XEC_RETIRING,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE);
        return 0;
    }
    return xr_eye_config_ready(q,d,epoch);
}
/* XR only. A pending writer is never cancelled underneath payload publication.
   Caller retries close; it must not destroy the parent XR session meanwhile. */
static int xr_eye_config_close(XrEyeConfig *q,XrEyeOutput *o,PanelXr *p) {
    unsigned state=__atomic_load_n(&q->state,__ATOMIC_ACQUIRE);
    if(state==XEC_WRITING || state==XEC_PROCESSING) return 0;
    if(!__atomic_compare_exchange_n(&q->state,&state,XEC_PROCESSING,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE)) return 0;
    o->enabled=0;
    if(!xr_eye_output_close(o,p)) {
        /* Retain request/handles and retry retirement, including same epoch.
           This state never authorizes injection or a new configuration. */
        __atomic_store_n(&q->state,XEC_RETIRING,__ATOMIC_RELEASE);return 0;
    }
    *o=(XrEyeOutput){0};
    __atomic_store_n(&q->state,XEC_IDLE,__ATOMIC_RELEASE);return 1;
}
static void xr_eye_config_drive(XrEyeConfig *q,XrEyeOutput *o,PanelXr *p,unsigned long long epoch) {
    unsigned state=__atomic_load_n(&q->state,__ATOMIC_ACQUIRE);
    if(state==XEC_RETIRING) {(void)xr_eye_config_close(q,o,p);return;}
    if(!epoch) return;
    if(state==XEC_READY || state==XEC_FAILED) {
        /* Only XR can reset these states, so payload is stable for this read. */
        if(q->epoch!=epoch) (void)xr_eye_config_close(q,o,p);
        return;
    }
    unsigned expected=XEC_REQUEST;
    if(!__atomic_compare_exchange_n(&q->state,&expected,XEC_PROCESSING,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE)) return;
    if(q->epoch!=epoch) {
        __atomic_store_n(&q->state,XEC_IDLE,__ATOMIC_RELEASE);return;
    }
    int ok=xr_eye_output_configure(o,&q->description);
    if(ok) {o->enabled=1;ok=xr_eye_output_create(o,p);}
    if(!ok) o->enabled=0;
    __atomic_store_n(&q->state,ok?XEC_READY:XEC_FAILED,__ATOMIC_RELEASE);
}
#endif
