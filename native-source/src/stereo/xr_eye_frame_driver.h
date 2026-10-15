#ifndef GF2_XR_EYE_FRAME_DRIVER_H
#define GF2_XR_EYE_FRAME_DRIVER_H
#include "xr_ordered_frames.h"
#include "eye_pose_pool.h"
#include "xr_eye_runtime.h"
#ifdef GF2VR_BANK_END_OBSERVE
#include "end_boundary_observe.h"
#endif
#ifdef GF2VR_BANK_ADMISSION_OBSERVE
#include "bank_fence_observe.h"
#endif
/* Sole XR frame-call owner. Caller supplies current-frame F9/mailbox policy;
   it runs once only AFTER actual Begin. Future capture does not sample input.
   Sole driver offers poses; main only reserves/retire. Images remain owned by
   the separate flight-image adapter; caller releases/acks them BEFORE End.
   No numeric GPU proof, frame-count timer, quality change or recenter here. */
typedef int (*XrCurrentPose)(void *,unsigned long long,const XrFrameState *,
                           unsigned,const XrEyeInput *,XrEyeInput *);
typedef struct {
    XrOrderedFrames api;
    EyeFlightOwner *owner;EyePosePool *poses;XrEyeRuntime *eyes;
    unsigned thread,stopping,fault,predicted_attempted,offered,prepared,begun,end_called,end_publish;
#ifdef GF2VR_ASYNC_PRESENTATION
    unsigned requested_display_only,current_display_only;
#ifdef GF2VR_ASYNC_READMIT
    unsigned requested_single_writer;
#endif
#endif
    unsigned long long requested,predicted,current,requested_generation,requested_epoch;
    EyeFlightToken pending_token,current_token;
    XrFrameState prediction,current_state;
    /* Exact API result survives numeric BUSY/ack and remains available to
       composition/input consumers after current-frame retirement. */
    XrResult begin_result,end_result;
    XrEyeInput predicted_pose,current_pose;
#ifdef GF2VR_BANK_CAPACITY_READMIT
    unsigned long long capacity_readmits;
#endif
#ifdef GF2VR_READY_RECEIPT_READMIT
    unsigned long long ready_readmits;
#ifdef GF2VR_INDEPENDENT_READY_CAPACITY
    unsigned long long ready_writing_readmits;
#endif

#endif
} XrEyeFrameDriver;
static int xfd_owner(XrEyeFrameDriver *d) {
    return d && d->thread==GetCurrentThreadId() && d->owner && d->poses && d->eyes;
}
/* Stop intent survives a busy numeric lock; no successful XR call is repeated. */
static int xfd_stop_tick(XrEyeFrameDriver *d) {
    return d->stopping?eye_flight_stop(d->owner):EFO_OK;
}
static int xfd_open(XrEyeFrameDriver *d,EyeFlightOwner *owner,EyePosePool *poses,XrEyeRuntime *eyes,
        XrSession session,PFN_xrWaitFrame wait,PFN_xrBeginFrame begin,PFN_xrEndFrame end) {
    if(!d || !owner || !poses || !eyes || d->thread || d->requested || d->current || d->predicted)return EFO_REJECTED;
    if(!xr_ordered_frames_open(&d->api,session,wait,begin,end))return EFO_REJECTED;
    d->owner=owner;d->poses=poses;d->eyes=eyes;d->thread=GetCurrentThreadId();
    d->stopping=d->fault=0;return EFO_OK;
}
static int xfd_request_mode(XrEyeFrameDriver *d,unsigned long long id,int independent_display) {
    if(!xfd_owner(d) || d->stopping || d->fault || d->requested || d->predicted ||
       (d->current && !d->begun))return EFO_REJECTED;
    unsigned available;unsigned long long generation,epoch;
    int r=eye_flight_available(d->owner,&available,&generation,&epoch);if(r!=EFO_OK)return r;
    if(generation && epoch && !available && !independent_display)return EFO_BUSY;
    /* Fixed current+prediction storage is reserved before the worker may Wait.
       Numeric capacity cannot shrink: this sole XR writer creates all poses. */
    if(!xr_ordered_frames_request(&d->api,id,1))return EFO_REJECTED;
#ifdef GF2VR_ASYNC_PRESENTATION
    d->requested_display_only=(independent_display==2 && available<EFO_CAPACITY) ||
                             (independent_display && generation && epoch && !available);
#ifdef GF2VR_ASYNC_READMIT
    d->requested_single_writer=independent_display==2;
#endif
#endif
    d->requested=id;d->requested_generation=generation;d->requested_epoch=epoch;return EFO_OK;
}
static int xfd_request(XrEyeFrameDriver *d,unsigned long long id) {
    return xfd_request_mode(d,id,0);
}
#ifdef GF2VR_ASYNC_PRESENTATION
/* Full production pool must not stop the display clock. This frame receives
   no numeric producer slot, cannot reserve/render, but balances genuine
   Wait/Begin/End and still runs the once-only current input/reference callback. */
static inline int xfd_request_display(XrEyeFrameDriver *d,unsigned long long id) {
    return xfd_request_mode(d,id,1);
}
/* The connected image adapter owns only one writing lease. A spare numeric
   slot is NOT another bank: prefetch during Begin must not offer work that
   cannot attach/retain at End. Keep real display/input calls independent while
   any producer remains outstanding, including main-thread retirement. The
   generic two-slot driver above remains available to other adapters. */
static inline int xfd_request_single_writer_display(XrEyeFrameDriver *d,unsigned long long id) {
    return xfd_request_mode(d,id,2);
}
#endif
static int xfd_offer(XrEyeFrameDriver *d,EyeFlightToken token,const XrFrameState *state,
                     const XrEyeInput *pose,EyeFlightToken *out) {
    if(!token.generation || !token.epoch || !pose || !pose->valid ||
       !xr_eye_matches(pose,token.epoch,token.frame,state->predictedDisplayTime))return EFO_REJECTED;
    unsigned slot;
#ifdef GF2VR_BANK_ADMISSION_OBSERVE
    bank_admission_observe_record(0,token.generation,token.epoch,25,token.frame,0,0);
#endif
    int r=eye_pose_pool_offer(d->poses,d->owner,token,state->predictedDisplayTime,1,pose,&slot);
#ifdef GF2VR_BANK_ADMISSION_OBSERVE
    bank_admission_observe_record(0,token.generation,token.epoch,26,token.frame,0,r);
#endif
    if(r==EFO_OK)*out=token;return r;
}
#ifdef GF2VR_ASYNC_READMIT
/* A prefetch-time suppression is not a lifetime claim. Reconsider only BEFORE
   Begin, after genuine prior retirement left the whole single-writer pool free.
   Keep the exact captured prediction; never recapture pose or repeat XR calls.
   Main can only retire; this XR owner is the sole offer writer, so free capacity
   cannot shrink between this check and its ordinary pose-pool offer. */
static int xfd_readmit_prediction(XrEyeFrameDriver *d) {
    if(!xfd_owner(d))return EFO_REJECTED;
    XrEyeOwner *o=&d->eyes->owner;
    if(!d->requested_single_writer || !d->requested_display_only || !d->predicted ||
       d->pending_token.frame || d->stopping || d->fault || !d->prediction.shouldRender ||
       !d->predicted_pose.valid || o->changing || o->retiring || o->calibration_request ||
       o->epoch!=d->requested_epoch || o->head_reference_epoch!=o->epoch)
        return EFO_OK;
    unsigned available;unsigned long long generation,epoch;
    int r=eye_flight_available(d->owner,&available,&generation,&epoch);
    if(r!=EFO_OK)return r;
    if(available==EFO_CAPACITY && generation && epoch &&
       generation==d->requested_generation && epoch==d->requested_epoch &&
       xr_eye_matches(&d->predicted_pose,epoch,d->predicted,d->prediction.predictedDisplayTime)) {
        d->requested_display_only=0;d->offered=0;
    }
    return EFO_OK;
}
#endif
static int xfd_take(XrEyeFrameDriver *d) {
    if(!xfd_owner(d) || d->fault || (!d->requested && !d->predicted))return EFO_REJECTED;
    if(!d->predicted) {
        unsigned long long id;XrResult result;XrFrameState state;
        if(!xr_ordered_frames_take(&d->api,&id,&state,&result))return EFO_BUSY;
        d->requested=0;
        if(XR_FAILED(result)) {d->fault=d->stopping=1;(void)eye_flight_stop(d->owner);return EFO_REJECTED;}
        d->predicted=id;d->prediction=state;d->offered=d->predicted_attempted=0;
#ifdef GF2VR_ASYNC_PRESENTATION
        if(d->requested_display_only)d->offered=1; /* deliberately no producer offer */
#endif
        d->predicted_pose=(XrEyeInput){0};d->pending_token=(EyeFlightToken){0};
        if(result!=XR_SUCCESS)d->stopping=1;
        XrEyeOwner *o=&d->eyes->owner;
        if(!d->stopping && state.shouldRender && o->epoch && !o->changing && !o->retiring &&
           !o->calibration_request && o->head_reference_epoch==o->epoch) {
            d->predicted_attempted=1;
            (void)xr_eye_runtime_predict(d->eyes,id,&state,&d->predicted_pose);
        }
    }
    int stopped=xfd_stop_tick(d);if(stopped!=EFO_OK)return stopped;
#ifdef GF2VR_ASYNC_READMIT
    int readmitted=xfd_readmit_prediction(d);if(readmitted!=EFO_OK)return readmitted;
#endif
    if(!d->offered && d->predicted_pose.valid && !d->stopping) {
        EyeFlightToken t={d->predicted,d->requested_generation,d->requested_epoch};
        int r=xfd_offer(d,t,&d->prediction,&d->predicted_pose,&d->pending_token);
        if(r==EFO_BUSY)return r;
        d->offered=1; /* rejection is this-frame fallback, not pose recapture */
    }
    return EFO_OK;
}
static int xfd_begin(XrEyeFrameDriver *d,XrCurrentPose prepare,void *context) {
    if(!xfd_owner(d) || d->fault || !prepare)return EFO_REJECTED;
    if(!d->current) {
        if(!d->predicted)return EFO_REJECTED;
        if(d->predicted_pose.valid && !d->offered && !d->stopping)return EFO_BUSY;
        if(d->pending_token.frame) {
            int r=eye_flight_can_begin(d->owner,d->pending_token);if(r!=EFO_OK)return r;
        }
        XrResult result;
        if(!xr_ordered_frames_begin(&d->api,d->predicted,&result))return EFO_REJECTED;
        d->begin_result=result;d->end_result=XR_ERROR_CALL_ORDER_INVALID;
        if(XR_FAILED(result)){d->fault=d->stopping=1;(void)eye_flight_stop(d->owner);return EFO_REJECTED;}
        d->current=d->predicted;d->current_state=d->prediction;d->current_token=d->pending_token;
#ifdef GF2VR_ASYNC_PRESENTATION
        d->current_display_only=d->requested_display_only;d->requested_display_only=0;
#endif
        d->predicted=0;d->pending_token=(EyeFlightToken){0};
        d->prepared=d->begun=d->end_called=0;
        if(result!=XR_SUCCESS)d->stopping=1;
    } else if(d->begun || d->end_called)return EFO_REJECTED;
    int stopped=xfd_stop_tick(d);if(stopped!=EFO_OK)return stopped;
    if(!d->prepared) {
        d->current_pose=(XrEyeInput){0};
        int ready=0;
        if(!d->stopping && xr_ordered_frames_visual(&d->api,d->current))
            ready=prepare(context,d->current,&d->current_state,d->predicted_attempted,
                          &d->predicted_pose,&d->current_pose);
        if(!ready) {
            d->current_pose=(XrEyeInput){0};
            xr_eye_mailbox_invalidate(&d->eyes->mailbox);
        }
        d->prepared=1;
    }
    if(!d->current_token.frame && d->current_pose.valid && !d->stopping
#ifdef GF2VR_ASYNC_PRESENTATION
       && !d->current_display_only
#endif
       ) {
        EyeFlightToken t={d->current,d->requested_generation,d->requested_epoch};
        int r=xfd_offer(d,t,&d->current_state,&d->current_pose,&d->current_token);
        if(r==EFO_BUSY)return r;
    }
    if(d->current_token.frame) {
        int r=eye_flight_begun(d->owner,d->current_token);if(r!=EFO_OK)return r;
    }
    d->begun=1;return EFO_OK;
}
static int xfd_end_mode(XrEyeFrameDriver *d,const XrFrameEndInfo *end,int publish) {
    if(!xfd_owner(d) || d->fault || !d->current || !d->begun || !end ||
       end->type!=XR_TYPE_FRAME_END_INFO || end->displayTime!=d->current_state.predictedDisplayTime)return EFO_REJECTED;
    if(!d->end_called) {
        if(d->current_token.frame) {
            int r;
#ifdef GF2VR_ASYNC_PRESENTATION
            if(publish==2)r=eye_flight_can_detach(d->owner,d->current_token,end->displayTime);
            else if(publish==3)r=eye_flight_can_retain_offer(d->owner,d->current_token,end->displayTime);
            else
#endif
                r=eye_flight_can_end(d->owner,d->current_token,end->displayTime,publish);
            if(r!=EFO_OK){
#ifdef GF2VR_BANK_END_OBSERVE
                ebo_busy(0,r);
#endif
                return r;
            }
        } else if(publish)return EFO_REJECTED;
        XrResult result;
        if(!xr_ordered_frames_end(&d->api,d->current,end,&result))return EFO_REJECTED;
        d->end_result=result;
        if(XR_FAILED(result)){d->fault=d->stopping=1;(void)eye_flight_stop(d->owner);return EFO_REJECTED;}
        d->end_called=1;
#ifdef GF2VR_ASYNC_PRESENTATION
        d->end_publish=publish>=2?(unsigned)publish:(unsigned)(publish && result==XR_SUCCESS);
#else
        d->end_publish=publish && result==XR_SUCCESS;
#endif
        if(result!=XR_SUCCESS)d->stopping=1;
    }
    int stopped=xfd_stop_tick(d);if(stopped!=EFO_OK){
#ifdef GF2VR_BANK_END_OBSERVE
        ebo_busy(1,stopped);
#endif
        return stopped;
    }
    if(d->current_token.frame) {
        int r;
#ifdef GF2VR_ASYNC_PRESENTATION
        if(d->end_publish==2)r=eye_flight_detached(d->owner,d->current_token,d->current_state.predictedDisplayTime);
        else if(d->end_publish==3)r=eye_flight_retained_offer(d->owner,d->current_token,d->current_state.predictedDisplayTime);
        else
#endif
            r=eye_flight_ended(d->owner,d->current_token,d->current_state.predictedDisplayTime,d->end_publish);
        if(r!=EFO_OK){
#ifdef GF2VR_BANK_END_OBSERVE
            ebo_busy(1,r);
#endif
            return r; /* no repeated API End while numeric lock is busy */
        }
    }
    d->current=0;d->begun=d->prepared=d->end_called=0;
#ifdef GF2VR_ASYNC_PRESENTATION
    d->current_display_only=0;
#endif
    d->current_token=(EyeFlightToken){0};d->current_pose=(XrEyeInput){0};
    xr_eye_mailbox_invalidate(&d->eyes->mailbox);return EFO_OK;
}
static int xfd_end(XrEyeFrameDriver *d,const XrFrameEndInfo *end,int publish) {
    if(publish!=0 && publish!=1)return EFO_REJECTED;
    if(d && d->end_called && d->end_publish>=2)return EFO_REJECTED;
    return xfd_end_mode(d,end,publish);
}
#ifdef GF2VR_ASYNC_PRESENTATION
#include "xr_display_exclusion.h"
#include "xr_eye_flight_images.h"
/* Actual ordered End, never an invented terminal flag. Output handles/thread/
   session and (when attached) exact writing ticket are checked before API End.
   A separate frame can start after this while the retained image lease drains. */
static inline int xfd_end_detached(XrEyeFrameDriver *d,const XrFrameEndInfo *end,const XrEyeOutput *o) {
    if(!xfd_owner(d) || !d->current_token.frame || !o || !o->enabled ||
       o->owner_thread!=d->thread || o->session!=d->api.prefetch.session ||
       (d->end_called && d->end_publish!=2) ||
       !xr_display_excludes(end,o->chains[0],o->chains[1]))return EFO_REJECTED;
    if(o->frame) {
        if(o->frame!=d->current_token.frame || o->epoch!=d->current_token.epoch ||
           o->ticket.generation!=d->current_token.generation)return EFO_REJECTED;
        EyeFlightSnapshot s;int r=eye_flight_snapshot(d->owner,d->current_token,&s);
        if(r!=EFO_OK)return r;
        if(!o->ticket.serial || o->ticket.serial!=s.ticket.serial ||
           o->ticket.generation!=s.ticket.generation || o->ticket.epoch!=s.ticket.epoch)return EFO_REJECTED;
    } else if(o->acquired[0] || o->acquired[1] ||
              __atomic_load_n(&o->state,__ATOMIC_ACQUIRE)!=XEO_IDLE)return EFO_REJECTED;
    return xfd_end_mode(d,end,2);
}
/* Pending-offer End requires an already attached immutable bank reservation,
   including its original token when no main ticket exists yet. Later admission
   may race this call, but no image/claim/copy can race on the sole XR owner. */
static inline int xfd_end_retained_offer(XrEyeFrameDriver *d,const XrFrameEndInfo *end,
                                       const XrEyeFlightImages *f,const XrEyeOutput *o) {
    if(!xfd_owner(d) || !d->current_token.frame || !f || !f->pending_offer || f->fault ||
       !xfi_same(f->token,d->current_token) || f->thread!=d->thread || !f->prepared_tick ||
       !o || !o->enabled || o->frame!=f->token.frame || o->epoch!=f->token.epoch ||
       o->ticket.generation!=f->token.generation || o->ticket.epoch!=f->token.epoch ||
       o->owner_thread!=d->thread || o->session!=d->api.prefetch.session ||
       o->chains[0]!=f->writing_chains[0] || o->chains[1]!=f->writing_chains[1] ||
       o->acquired[0] || o->acquired[1] || f->actual_released ||
       __atomic_load_n(&o->state,__ATOMIC_ACQUIRE)!=XEO_IDLE ||
       (d->end_called && d->end_publish!=3) ||
       !xr_display_excludes(end,f->writing_chains[0],f->writing_chains[1]))return EFO_REJECTED;
    EyeFlightSnapshot s;int r=eye_flight_snapshot(d->owner,f->token,&s);
    if(r!=EFO_OK)return r;
    if(o->ticket.serial && (o->ticket.serial!=s.ticket.serial ||
       o->ticket.generation!=s.ticket.generation || o->ticket.epoch!=s.ticket.epoch))return EFO_REJECTED;
    /* Zero serial is the pre-admission bank identity, not a GPU ticket. Main
       may have reserved since xfi_sync; adoption is required before Acquire. */
    return xfd_end_mode(d,end,3);
}
#include "xr_presentation_flights.h"
#ifdef GF2VR_BANK_READMIT_OBSERVE
/* Sole XR owner, numeric payload only; no new owner/GPU query or wait. */
__declspec(dllimport) int __stdcall QueryPerformanceCounter(long long *);
#ifdef BOUNDARY_TEST
static unsigned bank_readmit_test_observe;
#endif
static void xfd_bank_readmit_note(XrEyeFrameDriver *d,
#ifdef GF2VR_XR_READMIT_CAUSE_OBSERVE
                                  XrPresentationFlights *f,
#endif
                                  unsigned reason,
                                 unsigned long long value,int result) {
    static unsigned notes;
#ifdef BOUNDARY_TEST
    if(!bank_readmit_test_observe)return;
#endif
    if(!bank_fence_observe_started() || !d->requested_display_only || !d->requested_generation ||
       !d->predicted_pose.valid || notes>=96)return;
    ++notes;
    DWORD saved=GetLastError();long long at=0;
    if(QueryPerformanceCounter(&at)) {
        BankFenceObservation row={0};row.phase=32;row.generation=d->requested_generation;
        row.epoch=d->requested_epoch;row.serial=d->predicted;row.value=value;
        row.drain_only=reason;row.tried=(unsigned)result;
        row.thread=GetCurrentThreadId();row.qpc=(unsigned long long)at;
#ifdef GF2VR_XR_READMIT_CAUSE_OBSERVE
        /* Diagnostic only. Already-owned XR scalars; no owner query or lease read.
           phase32 hr[] are packed bank states/free/writing/valid-bit+ready counts,
           never HRESULT or completion authority. */
        if(f && f->storage.thread==d->thread && f->storage.session==d->api.prefetch.session) {
            row.hr[3]=0x80000000u;
            for(unsigned i=0;i<PBP_BANKS;++i) {
                unsigned state=f->storage.leases.banks[i].state;
                row.hr[0]|=(state<=PBP_DRAINING?state:15u)<<(4*i);
                row.hr[1]+=state==PBP_FREE;row.hr[2]+=state==PBP_WRITING;
                row.hr[3]+=state==PBP_READY;
            }
        }
#endif
        bank_fence_observe_record(&row);
    }
    SetLastError(saved);
}
#ifdef GF2VR_XR_READMIT_CAUSE_OBSERVE
#define BANK_READMIT_NOTE(reason,value,result) xfd_bank_readmit_note(d,f,reason,value,result)
#else
#define BANK_READMIT_NOTE(reason,value,result) xfd_bank_readmit_note(d,reason,value,result)
#endif
#else
#define BANK_READMIT_NOTE(reason,value,result) ((void)0)
#endif
#ifdef GF2VR_NUMERIC_LOSS_OBSERVE
#define BANK_READMIT_LEGACY(reason,value,result) BANK_READMIT_NOTE(reason,value,result)
/* Preserve legacy expansion by spelling its active implementation explicitly. */
#undef BANK_READMIT_LEGACY
#ifdef GF2VR_BANK_READMIT_OBSERVE
#ifdef GF2VR_XR_READMIT_CAUSE_OBSERVE
#define BANK_READMIT_LEGACY(reason,value,result) xfd_bank_readmit_note(d,f,reason,value,result)
#else
#define BANK_READMIT_LEGACY(reason,value,result) xfd_bank_readmit_note(d,reason,value,result)
#endif
#else
#define BANK_READMIT_LEGACY(reason,value,result) ((void)0)
#endif
#undef BANK_READMIT_NOTE
#define BANK_READMIT_NOTE(reason,value,result) do { BANK_READMIT_LEGACY(reason,value,result); \
 NLO_DRIVER(d,NLO_BANK,reason,result,(unsigned)(value)); } while(0)
#endif
#if defined(GF2VR_BANK_READMIT) || (defined(BOUNDARY_TEST) && defined(GF2VR_BANK_TRANSFER))
/* A full pool at prefetch is only a scheduling snapshot. Before actual Begin,
   reconsider it after genuine retirement. READY/DRAINING is not room.
   XR alone owns the banks/offers. Main can only retire numeric slots. */
static int xfd_readmit_bank_prediction(XrEyeFrameDriver *d,XrPresentationFlights *f,
                                     PanelXr *p,unsigned long long live_generation) {
    if(!xfd_owner(d))return EFO_REJECTED;
#ifdef GF2VR_BANK_READMIT
    XrEyeOwner *o=&d->eyes->owner;
#ifdef GF2VR_NUMERIC_LOSS_OBSERVE
    unsigned nlo_reason=0;
#endif
    if(NLO_TEST(101,!d->requested_display_only) ||
       NLO_TEST(102,!d->predicted) ||
       NLO_TEST(103,d->current) ||
       NLO_TEST(104,d->begun) ||
       NLO_TEST(105,d->pending_token.frame) ||
       NLO_TEST(106,!d->offered) ||
       NLO_TEST(107,d->stopping) ||
       NLO_TEST(108,d->fault) ||
       NLO_TEST(109,!d->prediction.shouldRender) ||
       NLO_TEST(110,o->changing) ||
       NLO_TEST(111,o->retiring) ||
       NLO_TEST(112,o->calibration_request) ||
       NLO_TEST(113,o->epoch!=d->requested_epoch) ||
       NLO_TEST(114,o->head_reference_epoch!=o->epoch) ||
       NLO_TEST(115,!live_generation) ||
       NLO_TEST(116,live_generation!=d->requested_generation) ||
       NLO_TEST(117,!xr_eye_matches(&d->predicted_pose,d->requested_epoch,d->predicted,
                       d->prediction.predictedDisplayTime)) ||
       NLO_TEST(118,!f) ||
       NLO_TEST(119,!xpb_owner(&f->storage,p)) ||
       NLO_TEST(120,f->storage.thread!=d->thread) ||
       NLO_TEST(121,f->storage.session!=d->api.prefetch.session) ||
       NLO_TEST(122,!f->storage.ready) ||
       NLO_TEST(123,f->storage.closing) ||
       NLO_TEST(124,f->storage.creation_failed) ||
       NLO_TEST(125,f->storage.leases.stopped) ||
       NLO_TEST(126,f->storage.leases.generation!=d->requested_generation) ||
       NLO_TEST(127,f->storage.leases.epoch!=d->requested_epoch) ||
       NLO_TEST(128,d->predicted<=f->storage.leases.last_frame)) {
        BANK_READMIT_NOTE(2,d->predicted,EFO_OK);
        NLO_DRIVER(d,NLO_BANK,nlo_reason,EFO_OK,0);return EFO_OK;
    }
    unsigned rooms=0,held=0;
#ifdef GF2VR_BANK_CAPACITY_READMIT
    unsigned completed_held=0;
#endif
#ifdef GF2VR_BANK_PARTIAL_READMIT
    unsigned writers=0;EyeFlightToken retained={0};
#ifdef GF2VR_CAPACITY_EXPERIMENT
    EyeFlightToken retained_tokens[EFO_CAPACITY];
#endif
#endif
    for(unsigned i=0;i<PBP_BANKS;++i) {
        unsigned state=f->storage.leases.banks[i].state;
        if(state==PBP_HELD){
            ++held;
#ifdef GF2VR_BANK_CAPACITY_READMIT
            /* HELD is published only after genuine GPU completion, Release
               and successful End. Finalization clears its transfer receipt.
               Never look up this old numeric/pose/native token after transfer;
               main may already be retiring it. Only inspect XR-owned storage. */
            PresentationBank *bank=&f->storage.leases.banks[i];
            XrEyeOutput *output=&f->storage.outputs[i];
            if(bank->ready_serial && bank->token.frame && bank->token.frame<d->predicted &&
               bank->token.generation==d->requested_generation &&
               bank->token.epoch==d->requested_epoch && output->state==XEO_IDLE &&
               !output->acquired[0] && !output->acquired[1] &&
               !output->waited[0] && !output->waited[1] &&
               !f->images[i].token.frame && !f->receipts[i].state)++completed_held;
#endif
            continue;
        }
#ifdef GF2VR_BANK_PARTIAL_READMIT
        if(state==PBP_WRITING) {
            retained=f->storage.leases.banks[i].token;
#ifdef GF2VR_CAPACITY_EXPERIMENT
            if(++writers>=EFO_CAPACITY || !pbp_same(retained,f->images[i].token) ||
#else
            if(++writers>1 || !pbp_same(retained,f->images[i].token) ||
#endif
               retained.generation!=d->requested_generation || retained.epoch!=d->requested_epoch ||
               retained.frame>=d->predicted || f->receipts[i].state){NLO_DRIVER(d,NLO_BANK,9,EFO_OK,writers);return EFO_OK;}
#ifdef GF2VR_CAPACITY_EXPERIMENT
            retained_tokens[writers-1]=retained;
#endif
            continue; /* never inspect or modify the existing writer's GPU storage */
        }
#endif
        if(state!=PBP_FREE){BANK_READMIT_NOTE(3,((unsigned long long)i<<32)|state,EFO_OK);return EFO_OK;}
        XrEyeOutput *output=&f->storage.outputs[i];
        if(output->state!=XEO_IDLE || output->acquired[0] || output->acquired[1] ||
           output->waited[0] || output->waited[1] || f->images[i].token.frame ||
           f->receipts[i].state){BANK_READMIT_NOTE(4,i,EFO_OK);return EFO_OK;}
        ++rooms;
    }
#ifdef GF2VR_BANK_PARTIAL_READMIT
    if(rooms<PBP_MAX_WRITERS-writers || held>1){BANK_READMIT_NOTE(5,((unsigned long long)held<<32)|rooms,EFO_OK);return EFO_OK;}
#else
    if(rooms<PBP_MAX_WRITERS || held>1){BANK_READMIT_NOTE(5,((unsigned long long)held<<32)|rooms,EFO_OK);return EFO_OK;}
#endif
    unsigned available;unsigned long long generation,epoch;
    int r=eye_flight_available(d->owner,&available,&generation,&epoch);
    if(r!=EFO_OK){BANK_READMIT_NOTE(6,0,r);return r;}
#ifdef GF2VR_BANK_CAPACITY_READMIT
    unsigned needed=EFO_CAPACITY
#ifdef GF2VR_BANK_PARTIAL_READMIT
       -writers
#endif
       ;
    /* A published pair can outlive main's native/source cleanup. One free
       numeric slot plus independent FREE writing storage suffices for ONE
       producer. The sole XR offer writer cannot lose capacity to another
       offer; main only frees it. xfd_offer still rechecks under the owner lock.
       Leave the existing retained-WRITING admission contract unchanged. */
    if(needed==EFO_CAPACITY && available==1 && held==1 && completed_held==1)needed=1;
#ifdef GF2VR_CAPACITY_EXPERIMENT
    if(!available || available>EFO_CAPACITY || generation!=d->requested_generation ||
#else
    if(available!=needed || generation!=d->requested_generation ||
#endif
#else
    if(available!=EFO_CAPACITY
#ifdef GF2VR_BANK_PARTIAL_READMIT
       -writers
#endif
       || generation!=d->requested_generation ||
#endif
       epoch!=d->requested_epoch){BANK_READMIT_NOTE(7,available,EFO_OK);return EFO_OK;}
#ifdef GF2VR_BANK_PARTIAL_READMIT
    if(writers) {
#ifdef GF2VR_CAPACITY_EXPERIMENT
        for(unsigned writer=0;writer<writers;++writer) {
        EyeFlightSnapshot prior;r=eye_flight_snapshot(d->owner,retained_tokens[writer],&prior);
#else
        EyeFlightSnapshot prior;r=eye_flight_snapshot(d->owner,retained,&prior);
#endif
        if(r==EFO_BUSY){NLO_DRIVER(d,NLO_BANK,10,r,0);return r;}
        unsigned required=EFF_XR_ENDED|EFF_DISPLAY_DETACHED|EFF_PRODUCER_RETAINED;
        if(r!=EFO_OK || !prior.renderable || (prior.flags&required)!=required ||
           prior.flags&(EFF_CANCELLED|EFF_FAULT|EFF_RECLAIMING|EFF_PRESENTATION_RETIRED|EFF_PRODUCER_TRANSFERRED)){NLO_DRIVER(d,NLO_BANK,11,EFO_OK,prior.flags);return EFO_OK;}
#ifdef GF2VR_CAPACITY_EXPERIMENT
        }
#endif
        /* Main cannot take this detached writer. Only this XR owner can retire
           or transfer its bank. One new offer therefore fills the numeric pool
           through Begin/prefetch, preventing a third unbacked producer. */
    }
#endif
    EyeFlightToken token={d->predicted,generation,epoch};
    r=xfd_offer(d,token,&d->prediction,&d->predicted_pose,&d->pending_token);
    BANK_READMIT_NOTE(8,token.frame,r);
#ifdef GF2VR_BANK_CAPACITY_READMIT
    if(r==EFO_OK){
        d->requested_display_only=0;
        if(needed==1 && held==1 && completed_held==1
#ifdef GF2VR_BANK_PARTIAL_READMIT
           && !writers
#endif
           )++d->capacity_readmits;
    }
#else
    if(r==EFO_OK)d->requested_display_only=0;
#endif
    return r==EFO_REJECTED?EFO_OK:r; /* unchanged display fallback on rejection */
#else
    (void)f;(void)p;(void)live_generation;return EFO_OK;
#endif
}
#endif
/* Three-bank adapter: retain THIS offer only after validating that the actual
   display list excludes EVERY unfinished writing bank, not just this frame's.
   Delegates once-only API End and retained numeric ack to the existing driver.
   Dormant until selected scheduler/configuration integration is complete. */
static inline int xfd_end_bank_offer(XrEyeFrameDriver *d,XrPresentationFlights *banks,
                                   PanelXr *panel,const XrFrameEndInfo *end) {
    unsigned index;
    if(!xfd_owner(d) || !banks || !xpf_find(banks,panel,d->current_token,&index) ||
       banks->storage.thread!=d->thread || banks->storage.session!=d->api.prefetch.session ||
       !banks->storage.ready || banks->storage.closing || banks->storage.leases.stopped ||
       banks->storage.leases.banks[index].state!=PBP_WRITING)return EFO_REJECTED;
    for(unsigned i=0;i<PBP_BANKS;++i) {
        unsigned state=banks->storage.leases.banks[i].state;
        if((state==PBP_WRITING || state==PBP_DRAINING) &&
           !xr_display_excludes(end,banks->storage.outputs[i].chains[0],
                                  banks->storage.outputs[i].chains[1]))return EFO_REJECTED;
    }
    return xfd_end_retained_offer(d,end,&banks->images[index],&banks->storage.outputs[index]);
}
#endif
/* Stop blocks new Wait, retains the worker and all frame/payload obligations.
   Caller may balance pending/current frames with zero layers and drain image
   adapter. A hard error is quarantine, NOT synthetic Begin/End/reset proof. */
static int xfd_close(XrEyeFrameDriver *d) {
    if(!xfd_owner(d))return EFO_REJECTED;
    d->stopping=1;int stopped=xfd_stop_tick(d);
    int closed=xr_ordered_frames_close(&d->api);
    if(stopped!=EFO_OK)return stopped;
    if(!closed)return EFO_BUSY;
    d->requested=0; /* close proves an unentered queued Wait was cancelled */
    if(d->predicted || d->current)return EFO_BUSY;
    d->owner=0;d->poses=0;d->eyes=0;d->thread=0;return EFO_OK;
}
/* Caller attests actual EndSession. No frame API may be called after this.
   API-error quarantine may end here; numeric GPU/image faults never do. */
static int xfd_session_stopped(XrEyeFrameDriver *d,EyeFlightToken terminal[EFO_CAPACITY]) {
    if(!xfd_owner(d))return EFO_REJECTED;
    d->stopping=1;int r=xfd_stop_tick(d);if(r!=EFO_OK)return r;
    if(!xr_ordered_frames_session_stopped(&d->api))return EFO_BUSY;
    r=eye_flight_session_stopped(d->owner,terminal);if(r!=EFO_OK)return r;
    d->requested=d->predicted=d->current=0;
    d->pending_token=d->current_token=(EyeFlightToken){0};
    d->predicted_pose=d->current_pose=(XrEyeInput){0};
    d->begun=d->prepared=d->end_called=d->fault=0;
#ifdef GF2VR_ASYNC_PRESENTATION
    d->requested_display_only=d->current_display_only=0;
#ifdef GF2VR_ASYNC_READMIT
    d->requested_single_writer=0;
#endif
#endif
    xr_eye_mailbox_invalidate(&d->eyes->mailbox);
    return EFO_OK;
}
#endif
