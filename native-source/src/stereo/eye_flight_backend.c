#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include "eye_flight_backend.h"
#ifdef GF2VR_BANK_ADMISSION_OBSERVE
#include "bank_fence_observe.h"
#define BAO_ACK(s,phase) bank_admission_observe_record((s)->frame.ticket.serial,(s)->frame.ticket.generation,(s)->frame.ticket.epoch,phase,(s)->frame.token.frame,0,EFO_OK)
#else
#define BAO_ACK(s,phase) ((void)0)
#endif
#ifdef GF2VR_BANK_DISPATCH_OBSERVE
#include "bank_fence_observe.h"
#define BDO_NOTE(ticket,phase,value) bank_dispatch_observe_record((ticket).serial,(ticket).generation,(ticket).epoch,phase,value)
#else
#define BDO_NOTE(ticket,phase,value) ((void)0)
#endif
static int lock_backend(EyeFlightBackend *b) {
    unsigned expected=0;
    return __atomic_compare_exchange_n(&b->locked,&expected,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED);
}
static int unlock_backend(EyeFlightBackend *b,int result) {
    __atomic_store_n(&b->locked,0,__ATOMIC_RELEASE);return result;
}
static int same(EyeFlightToken a,EyeFlightToken c) {
    return a.frame && a.frame==c.frame && a.generation==c.generation && a.epoch==c.epoch;
}
static EyeFlightNativeSlot *find(EyeFlightBackend *b,EyeFlightToken t) {
    for(unsigned i=0;i<EFO_CAPACITY;++i)if(b->slots[i].occupied && same(b->slots[i].frame.token,t))return &b->slots[i];
    return 0;
}
static int owner(EyeFlightBackend *b) {return b && b->owner_thread==GetCurrentThreadId();}
#define LOCK(b) do {if(!(b))return EFO_REJECTED;if(!lock_backend(b))return EFO_BUSY;} while(0)
static int prepare(EyeFlightBackend *b,EyeFlightOwner *o,unsigned index,EyeFlightToken token,
    unsigned render_thread,unsigned long long child,int ordered,void *d,void *c,void *l,void *r,unsigned format) {
    if(!b || !o || index>=EFO_CAPACITY || !render_thread || !d || !c || !l || !r ||
       (b->owner_thread && !owner(b)))return EFO_REJECTED;
    LOCK(b);EyeFlightNativeSlot *s=&b->slots[index];EyeFlightSnapshot f;
    int result=eye_flight_snapshot(o,token,&f);
    if(result!=EFO_OK)return unlock_backend(b,result);
    unsigned ended=f.flags&EFF_XR_ENDED;
#ifdef GF2VR_ASYNC_PRESENTATION
    unsigned retained=EFF_XR_ENDED|EFF_DISPLAY_DETACHED|EFF_PRODUCER_RETAINED;
    if((f.flags&retained)==retained && !(f.flags&EFF_PRESENTATION_RETIRED))ended=0;
#endif
    if(s->occupied || !(f.flags&EFF_RESERVED) ||
       ended || f.flags&(EFF_RENDER_ISSUED|EFF_CANCELLED|EFF_FAULT|EFF_RECLAIMING|EFF_PRESENTATION_RETIRED) ||
       (b->render_thread && b->render_thread!=render_thread))return unlock_backend(b,EFO_REJECTED);
    /* The same numeric slot must own the same native bank. Under this task's
       allocation contract slots never move while occupied. */
    unsigned numeric_index;
    result=eye_flight_slot(o,token,&numeric_index);
    if(result!=EFO_OK)return unlock_backend(b,result);
    if(numeric_index!=index)return unlock_backend(b,EFO_REJECTED);
    /* Queue identity is published BEFORE helper state can route any callback.
       It is reset only after both helpers and all numeric leases drain. */
    __atomic_store_n(&s->child_queue,child,__ATOMIC_RELEASE);
    if(!eye_interval_prepare_initialized(&s->render,f.begin_id,f.end_id,f.ticket,
        render_thread,ordered,d,c,l,r,format))return unlock_backend(b,EFO_REJECTED);
    void *refs[]={d,c,l,r};
    for(unsigned i=0;i<4;++i){IUnknown_AddRef((IUnknown *)refs[i]);s->refs[i]=refs[i];}
#ifdef GF2VR_RENDER_OWNER_COPY
    if(child && !eye_event_prime(&s->copy,f.copy_id,f.ticket,render_thread,ordered,d,c,l,r)) {
        /* Begin has not been issued. Keep cancellation real and owner-local. */
        (void)eye_interval_cancel(&s->render,f.ticket);
        for(unsigned i=0;i<4;++i){IUnknown_Release((IUnknown *)s->refs[i]);s->refs[i]=0;}
        __atomic_store_n(&s->child_queue,0,__ATOMIC_RELEASE);
        return unlock_backend(b,EFO_REJECTED);
    }
#endif
    s->frame=f;s->occupied=1;b->owner_thread=GetCurrentThreadId();b->render_thread=render_thread;
    return unlock_backend(b,EFO_OK);
}
int eye_flight_backend_prepare(EyeFlightBackend *b,EyeFlightOwner *o,unsigned index,EyeFlightToken token,
    unsigned render_thread,int ordered,void *d,void *c,void *l,void *r,unsigned format) {
    return prepare(b,o,index,token,render_thread,0,ordered,d,c,l,r,format);
}
int eye_flight_backend_prepare_on_queue(EyeFlightBackend *b,EyeFlightOwner *o,unsigned index,EyeFlightToken token,
    unsigned render_thread,unsigned long long child,int ordered,void *d,void *c,void *l,void *r,unsigned format) {
    if(!child)return EFO_REJECTED;
    /* Only BUSY is safe to repeat: prepare returns it before publishing the
       child queue, interval or COM references. XR copy-offer briefly shares
       this metadata lock with main preparation of an independent slot. Keep
       the retry inside this real root, bounded; never wait for a GPU fence. */
    for(unsigned attempt=0;attempt<4;++attempt) {
        int result=prepare(b,o,index,token,render_thread,child,ordered,d,c,l,r,format);
        if(result!=EFO_BUSY)return result;
        if(attempt<3)SwitchToThread();
    }
    return EFO_BUSY;
}
int eye_flight_backend_begin(EyeFlightBackend *b,EyeFlightOwner *o,unsigned index,
    void (*issue)(void (*)(int),int),void (*callback)(int)) {
    if(!owner(b) || !o || index>=EFO_CAPACITY || !issue || !callback)return EFO_REJECTED;
    LOCK(b);EyeFlightNativeSlot *s=&b->slots[index];
    if(!s->occupied || __atomic_load_n(&s->render.state,__ATOMIC_ACQUIRE)!=ERI_QUEUED ||
       __atomic_load_n(&s->render.begin_issued,__ATOMIC_ACQUIRE))return unlock_backend(b,EFO_REJECTED);
    int result=eye_flight_issue_render(o,s->frame.token,s->frame.ticket);
    if(result!=EFO_OK)return unlock_backend(b,result);
    /* Callback can run immediately; it never reenters the metadata lock. */
    if(!eye_interval_issue_begin(&s->render,issue,callback)) {
        s->fault_pending=1;(void)eye_flight_fault(o,s->frame.token);return unlock_backend(b,EFO_REJECTED);
    }
    return unlock_backend(b,EFO_OK);
}
int eye_flight_backend_cancel_unissued(EyeFlightBackend *b,EyeFlightOwner *o,unsigned index) {
    if(!owner(b) || !o || index>=EFO_CAPACITY)return EFO_REJECTED;
    LOCK(b);EyeFlightNativeSlot *s=&b->slots[index];
    if(!s->occupied || __atomic_load_n(&s->render.state,__ATOMIC_ACQUIRE)!=ERI_QUEUED ||
       __atomic_load_n(&s->render.begin_issued,__ATOMIC_ACQUIRE))return unlock_backend(b,EFO_REJECTED);
    int result=eye_flight_cancel_unissued(o,s->frame.token,s->frame.ticket);
    if(result!=EFO_OK)return unlock_backend(b,result);
    if(!eye_interval_cancel(&s->render,s->frame.ticket)) {
        s->fault_pending=1;return unlock_backend(b,EFO_REJECTED);
    }
    s->render_taken=s->render_ack=1;return unlock_backend(b,EFO_OK);
}
int eye_flight_backend_end(EyeFlightBackend *b,unsigned index,int ordered,
    void (*issue)(void (*)(int),int),void (*callback)(int)) {
    if(!owner(b) || index>=EFO_CAPACITY || !issue || !callback)return EFO_REJECTED;
    LOCK(b);EyeFlightNativeSlot *s=&b->slots[index];
#ifdef GF2VR_PRODUCER_TRANSFER
    /* Actual interval storage may reset after its drain. This once fact lives
       through the flight, so root return never queues a second End. */
    if(s->occupied && s->producer_end_issued)return unlock_backend(b,EFO_OK);
#endif
    int ok=s->occupied && eye_interval_issue_end(&s->render,s->frame.ticket,ordered,issue,callback);
    return unlock_backend(b,ok);
}
int eye_flight_backend_cpu_return(EyeFlightBackend *b,EyeFlightOwner *o,unsigned index,int success) {
    if(!owner(b) || !o || index>=EFO_CAPACITY)return EFO_REJECTED;
    LOCK(b);EyeFlightNativeSlot *s=&b->slots[index];
    int result=s->occupied?eye_flight_cpu_return(o,s->frame.token,s->frame.ticket,success):EFO_REJECTED;
    return unlock_backend(b,result);
}
#ifdef GF2VR_PRODUCER_TRANSFER
int eye_flight_backend_producer_end(EyeFlightBackend *b,EyeFlightOwner *o,unsigned index,
    int ordered,void (*issue)(void (*)(int),int),void (*callback)(int)) {
    if(!owner(b) || !o || index>=EFO_CAPACITY || !ordered || !issue || !callback)return EFO_REJECTED;
    LOCK(b);EyeFlightNativeSlot *s=&b->slots[index];
#ifdef GF2VR_PRODUCER_HANDOFF_TIMING
    if(!s->occupied || s->producer_end_issued || s->fault_pending)
        return unlock_backend(b,EFO_REJECTED);
    /* Same immutable ticket, numeric mailbox only. The native callback may
       execute inside issue_end: its Signal stamp may precede phase 51. */
    bank_admission_observe_record(s->frame.ticket.serial,s->frame.ticket.generation,
        s->frame.ticket.epoch,50,s->frame.token.frame,0,0);
    int issued=eye_interval_issue_end(&s->render,s->frame.ticket,1,issue,callback);
    bank_admission_observe_record(s->frame.ticket.serial,s->frame.ticket.generation,
        s->frame.ticket.epoch,51,s->frame.token.frame,0,issued);
    if(!issued)return unlock_backend(b,EFO_REJECTED);
#else
    if(!s->occupied || s->producer_end_issued || s->fault_pending ||
       !eye_interval_issue_end(&s->render,s->frame.ticket,1,issue,callback))
        return unlock_backend(b,EFO_REJECTED);
#endif
    s->producer_end_issued=1;
    int r=eye_flight_private_producer(o,s->frame.token,s->frame.ticket);
    s->producer_ack_pending=r==EFO_BUSY;
    /* Generation cancellation can race the actual queue issue. It revokes
       early permission, not the known issued End or its drain obligation.
       Whole-root CPU/current checks still decide final publication. */
    return unlock_backend(b,EFO_OK);
}
#endif
#ifdef GF2VR_XR_SUBMISSION_ACK
#ifndef GF2VR_TWO_FLIGHT_EARLY_COPY
#error XR submission acknowledgment requires the existing early-copy contract
#endif
int eye_flight_backend_publish_submission(EyeFlightBackend *b,EyeFlightOwner *o,EyeFlightToken t) {
    if(!o)return EFO_REJECTED;
    LOCK(b);EyeFlightNativeSlot *s=find(b,t);
    if(!s || s->fault_pending)return unlock_backend(b,EFO_REJECTED);
    if(s->render_submitted || s->render_ack)return unlock_backend(b,EFO_OK);
    EyeIntervalResult submitted={0};
    /* PENDING/DONE acquire publishes coverage and the immutable exact ticket.
       Main take/reset is excluded by this lock; render polling never changes
       these fields. A queue submission is not either GPU drain. */
    if(!eye_interval_submitted(&s->render,&submitted) ||
       submitted.ticket.serial!=s->frame.ticket.serial ||
       submitted.ticket.generation!=s->frame.ticket.generation ||
       submitted.ticket.epoch!=s->frame.ticket.epoch)return unlock_backend(b,EFO_REJECTED);
    int result=eye_flight_render_submitted(o,t,submitted.ticket);
    if(result==EFO_OK) {
        s->submitted_result=submitted;s->render_submitted=1;
        ++b->xr_submission_acks;
#ifdef GF2VR_BANK_COPY_DELIVERY_OBSERVE
        BDO_NOTE(s->frame.ticket,15,submitted.draw_mask|((unsigned long long)submitted.clear_mask<<8));
#endif
    }
    return unlock_backend(b,result);
}
#endif
int eye_flight_backend_offer_copy(EyeFlightBackend *b,EyeFlightOwner *o,EyeFlightToken t,void *l,void *r) {
    if(!o || !l || !r || l==r)return EFO_REJECTED;
    LOCK(b);EyeFlightNativeSlot *s=find(b,t);
    if(!s ||
#ifdef GF2VR_TWO_FLIGHT_EARLY_COPY
       !((s->render_submitted && s->submitted_result.draw_mask==3 && s->submitted_result.clear_mask==3) ||
         (s->render_ack && s->render_result.draw_mask==3 && s->render_result.clear_mask==3)) ||
#else
       !s->render_ack || s->render_result.draw_mask!=3 || s->render_result.clear_mask!=3 ||
#endif
       s->copy_requested)return unlock_backend(b,EFO_REJECTED);
    int result=eye_flight_issue_copy(o,t,s->frame.ticket);
    if(result==EFO_OK){s->destinations[0]=l;s->destinations[1]=r;
        /* Before publication: main cannot dispatch this request before its stamp. */
        BDO_NOTE(s->frame.ticket,11,s->render_submitted|((unsigned long long)s->render_ack<<1));
#ifdef GF2VR_RENDER_OWNER_COPY
        if(s->copy.native_dispatch) {
            if(!eye_event_post_native(&s->copy,s->frame.ticket,l,r)) {
                s->fault_pending=1;(void)eye_flight_fault(o,t);
                return unlock_backend(b,EFO_REJECTED); /* retain numeric lease */
            }
            __atomic_store_n(&s->copy_started,1,__ATOMIC_RELEASE); /* posted, not GPU completed */
        }
#endif
        __atomic_store_n(&s->copy_requested,1,__ATOMIC_RELEASE);}
    return unlock_backend(b,result);
}
void eye_flight_backend_callback(EyeFlightBackend *b,int id) {
    if(!b || id<=0)return;
    /* IDs are process-global and monotonic. Every helper validates its own
       published ID/thread/state. No mutable active-slot alias or native frame
       metadata is read from the render thread. */
    for(unsigned i=0;i<EFO_CAPACITY;++i) {
        /* Legacy fixture-only route cannot bypass a production queue guard. */
        if(__atomic_load_n(&b->slots[i].child_queue,__ATOMIC_ACQUIRE))continue;
        eye_interval_callback(&b->slots[i].render,id);
        eye_event_callback(&b->slots[i].copy,id);
    }
}
void eye_flight_backend_callback_on_queue(EyeFlightBackend *b,int id,unsigned long long child) {
    if(!b || id<=0 || !child)return;
    for(unsigned i=0;i<EFO_CAPACITY;++i) {
        if(__atomic_load_n(&b->slots[i].child_queue,__ATOMIC_ACQUIRE)!=child)continue;
        eye_interval_callback(&b->slots[i].render,id);
        eye_event_callback(&b->slots[i].copy,id);
    }
}
void eye_flight_backend_poll(EyeFlightBackend *b) {
    if(!b)return;
    for(unsigned i=0;i<EFO_CAPACITY;++i) {
        eye_interval_poll(&b->slots[i].render);
        eye_event_poll(&b->slots[i].copy);
    }
}
#ifdef GF2VR_RENDER_OWNER_COPY
void eye_flight_backend_service_copies(EyeFlightBackend *b,void *device,void *context) {
    if(!b || GetCurrentThreadId()!=b->render_thread)return;
    for(unsigned i=0;i<EFO_CAPACITY;++i)
        eye_event_service_native(&b->slots[i].copy,device,context);
}
#endif
#ifdef GF2VR_MAIN_TAIL_COMPLETION
static int reconcile_mode(EyeFlightBackend *b,EyeFlightOwner *o,
    void (*issue)(void (*)(int),int),void (*callback)(int),int completion_only) {
    if(!owner(b) || !o || (!completion_only && (!issue || !callback)))return EFO_REJECTED;
#else
int eye_flight_backend_reconcile(EyeFlightBackend *b,EyeFlightOwner *o,
    void (*issue)(void (*)(int),int),void (*callback)(int)) {
    if(!owner(b) || !o || !issue || !callback)return EFO_REJECTED;
#endif
    LOCK(b);int result=EFO_OK;
    for(unsigned i=0;i<EFO_CAPACITY;++i) {
        EyeFlightNativeSlot *s=&b->slots[i];if(!s->occupied)continue;
#ifdef GF2VR_PRODUCER_TRANSFER
        if(s->producer_ack_pending) {
            int r=eye_flight_private_producer(o,s->frame.token,s->frame.ticket);
            if(r==EFO_BUSY){result=r;continue;}
            s->producer_ack_pending=0;
            /* Late CPU failure revokes early permission; retain real drains. */
        }
#endif
#ifdef GF2VR_BANK_FENCE_COMPLETION
        /* This owner never touches the immediate context. Real marker drain
           can retire detached GPU refs before the next render-owner tick;
           numeric/source/image retirement still follows its existing owners. */
        eye_interval_fence_notify(&s->render,0);
        eye_event_fence_complete(&s->copy);
#endif
        unsigned state=__atomic_load_n(&s->render.state,__ATOMIC_ACQUIRE);
        unsigned copy=__atomic_load_n(&s->copy.state,__ATOMIC_ACQUIRE);
        if(s->fault_pending || state==ERI_FAULT || copy==EEC_FAULT) {
            s->fault_pending=1;
            int r=eye_flight_fault(o,s->frame.token);if(r==EFO_BUSY)result=r;
            continue;
        }
#ifdef GF2VR_TWO_FLIGHT_EARLY_COPY
        if(!s->render_submitted) {
            EyeIntervalResult submitted={0};
            if(eye_interval_submitted(&s->render,&submitted)) {
                int r=eye_flight_render_submitted(o,s->frame.token,submitted.ticket);
                if(r==EFO_BUSY){result=r;continue;}
                if(r==EFO_OK){s->submitted_result=submitted;s->render_submitted=1;
                    BDO_NOTE(s->frame.ticket,15,submitted.draw_mask|((unsigned long long)submitted.clear_mask<<8));}
            }
        }
#endif
        if(!s->render_taken && state==ERI_DONE) {
#ifdef GF2VR_BANK_NATIVE_OBSERVE
            /* DONE acquire above publishes existing fence timing. take resets
               the interval; copy only scalars before it, emit only on success. */
            EyeGpuTiming timing=s->render.timing;
#endif
            if(!eye_interval_take(&s->render,&s->render_result))continue;
            s->render_taken=1;
#ifdef GF2VR_BANK_NATIVE_OBSERVE
            const unsigned long long stamps[]={timing.signal,timing.first_pending,
                timing.last_pending,timing.first_complete};
            for(unsigned n=0;n<4;++n)
                bank_native_observe_record(s->frame.ticket.serial,s->frame.ticket.generation,
                    s->frame.ticket.epoch,s->frame.token.frame,42+n,stamps[n],
                    timing.polls,timing.pending);
#endif
        }
        if(s->render_taken && !s->render_ack) {
            #ifdef GF2VR_EVENT_NOTICE_HANDOFF
            EyeFlightSnapshot acknowledged;int r=eye_flight_snapshot(o,s->frame.token,&acknowledged);
            if(r==EFO_OK && (acknowledged.flags&(EFF_FAULT|EFF_RECLAIMING)))r=EFO_REJECTED;
            if(r==EFO_OK && !(acknowledged.flags&EFF_RENDER_DRAINED))
                r=eye_flight_render_drained(o,s->frame.token,s->frame.ticket);
#else
            int r=eye_flight_render_drained(o,s->frame.token,s->frame.ticket);
#endif
            if(r!=EFO_OK){result=r;continue;}s->render_ack=1;
            BAO_ACK(s,21);
            BDO_NOTE(s->frame.ticket,16,s->copy_requested);
        }
        if(s->copy_requested && !s->copy_started) {
#ifdef GF2VR_MAIN_TAIL_COMPLETION
            /* Completion opportunity is not an approved command issue point.
               Original frame-entry reconciliation retains this exact request. */
            if(completion_only){result=EFO_BUSY;continue;}
#endif
            if(!eye_event_prepare(&s->copy,s->frame.copy_id,s->frame.ticket,b->render_thread,1,
                s->refs[0],s->refs[1],s->refs[2],s->refs[3],s->destinations[0],s->destinations[1]) ||
               !eye_event_issue(&s->copy,issue,callback)) {
                s->fault_pending=1;int r=eye_flight_fault(o,s->frame.token);result=r==EFO_BUSY?r:EFO_REJECTED;
                continue;
            }
            s->copy_started=1;
        }
        if(!s->copy_taken && __atomic_load_n(&s->copy.state,__ATOMIC_ACQUIRE)==EEC_DONE) {
            EyeGpuTicket ticket;
#ifdef GF2VR_BANK_COPY_DELIVERY_OBSERVE
            EyeGpuTiming copy_timing=s->copy.timing;
#endif
            if(!eye_event_take(&s->copy,&ticket))continue;
            if(ticket.serial!=s->frame.ticket.serial || ticket.generation!=s->frame.ticket.generation ||
               ticket.epoch!=s->frame.ticket.epoch) {
                s->fault_pending=1;(void)eye_flight_fault(o,s->frame.token);result=EFO_REJECTED;continue;
            }
            s->copy_taken=1;
#ifdef GF2VR_BANK_COPY_DELIVERY_OBSERVE
            const unsigned long long stamps[]={copy_timing.signal,copy_timing.first_pending,
                copy_timing.last_pending,copy_timing.first_complete};
            for(unsigned n=0;n<4;++n)
                bank_native_observe_record(ticket.serial,ticket.generation,ticket.epoch,
                    s->frame.token.frame,46+n,stamps[n],copy_timing.polls,copy_timing.pending);
#endif
        }
        if(s->copy_taken && !s->copy_ack) {
            #ifdef GF2VR_EVENT_NOTICE_HANDOFF
            EyeFlightSnapshot acknowledged;int r=eye_flight_snapshot(o,s->frame.token,&acknowledged);
            if(r==EFO_OK && (acknowledged.flags&(EFF_FAULT|EFF_RECLAIMING)))r=EFO_REJECTED;
            if(r==EFO_OK && !(acknowledged.flags&EFF_COPY_DRAINED))
                r=eye_flight_copy_drained(o,s->frame.token,s->frame.ticket);
#else
            int r=eye_flight_copy_drained(o,s->frame.token,s->frame.ticket);
#endif
            if(r!=EFO_OK){result=r;continue;}
            s->copy_ack=1;s->destinations[0]=s->destinations[1]=0;
#ifdef GF2VR_MAIN_TAIL_COMPLETION
            if(completion_only)__atomic_fetch_add(&b->main_tail_copy_acks,1,__ATOMIC_RELAXED);
#endif
            BAO_ACK(s,22);
            BDO_NOTE(s->frame.ticket,17,1);
        }
    }
    return unlock_backend(b,result);
}
#ifdef GF2VR_MAIN_TAIL_COMPLETION
int eye_flight_backend_reconcile(EyeFlightBackend *b,EyeFlightOwner *o,
    void (*issue)(void (*)(int),int),void (*callback)(int)) {
    return reconcile_mode(b,o,issue,callback,0);
}
int eye_flight_backend_reconcile_completed(EyeFlightBackend *b,EyeFlightOwner *o) {
    return reconcile_mode(b,o,0,0,1);
}
#endif
int eye_flight_backend_retire(EyeFlightBackend *b,EyeFlightOwner *o,EyeFlightToken t) {
    if(!owner(b) || !o)return EFO_REJECTED;
    LOCK(b);EyeFlightNativeSlot *s=find(b,t);EyeFlightSnapshot f;
    int result=eye_flight_snapshot(o,t,&f);
    if(result!=EFO_OK)return unlock_backend(b,result);
#ifdef GF2VR_RENDER_OWNER_COPY
    if(!s || !(f.flags&EFF_RECLAIMING) || f.flags&EFF_FAULT || !s->render_ack ||
       (s->copy_requested && !s->copy_ack) ||
       __atomic_load_n(&s->render.state,__ATOMIC_ACQUIRE)!=ERI_IDLE)return unlock_backend(b,EFO_REJECTED);
    if(!s->copy_requested && !(f.flags&EFF_COPY_ISSUED) &&
       __atomic_load_n(&s->copy.state,__ATOMIC_ACQUIRE)==EEC_PRIMED)
        (void)eye_event_cancel_prime(&s->copy);
    if(
       __atomic_load_n(&s->copy.state,__ATOMIC_ACQUIRE)!=EEC_IDLE)return unlock_backend(b,EFO_REJECTED);
#else
    if(!s || !(f.flags&EFF_RECLAIMING) || f.flags&EFF_FAULT || !s->render_ack ||
       (s->copy_requested && !s->copy_ack) ||
       __atomic_load_n(&s->render.state,__ATOMIC_ACQUIRE)!=ERI_IDLE ||
       __atomic_load_n(&s->copy.state,__ATOMIC_ACQUIRE)!=EEC_IDLE)return unlock_backend(b,EFO_REJECTED);
#endif
    for(unsigned i=0;i<4;++i){IUnknown_Release((IUnknown *)s->refs[i]);s->refs[i]=0;}
    /* Helpers keep their own lifetime event clocks. Do not zero those. */
    s->frame=(EyeFlightSnapshot){0};s->render_result=(EyeIntervalResult){0};
    s->occupied=s->render_taken=s->render_ack=s->render_submitted=s->copy_requested=s->copy_started=s->copy_taken=s->copy_ack=s->fault_pending=0;
    s->submitted_result=(EyeIntervalResult){0};
#ifdef GF2VR_PRODUCER_TRANSFER
    s->producer_end_issued=s->producer_ack_pending=0;
#endif
    __atomic_store_n(&s->child_queue,0,__ATOMIC_RELEASE);
    return unlock_backend(b,EFO_OK);
}

#ifdef GF2VR_EVENT_NOTICE_HANDOFF
/* A sole-XR event notice is only a scheduling hint. Keep the old main-owner
   reconcile/take/retire APIs and fields unchanged. Re-read the actual native
   fence through the existing exclusive interval/event CAS before numeric ack. */
int eye_flight_backend_notice(EyeFlightBackend *b,EyeFlightOwner *o,EyeGpuTicket ticket,
                              unsigned drain_only,unsigned xr_thread,unsigned *new_ack) {
    if(new_ack)*new_ack=0;
    if(!b||!o||!new_ack||!xr_thread||xr_thread!=GetCurrentThreadId()||
       !ticket.serial||!ticket.generation||!ticket.epoch||drain_only>1)return EFO_REJECTED;
    LOCK(b);EyeFlightNativeSlot *s=0;
    for(unsigned i=0;i<EFO_CAPACITY;++i){EyeFlightNativeSlot *candidate=&b->slots[i];
        if(candidate->occupied&&candidate->frame.ticket.serial==ticket.serial&&
           candidate->frame.ticket.generation==ticket.generation&&candidate->frame.ticket.epoch==ticket.epoch){s=candidate;break;}}
    if(!s||s->fault_pending)return unlock_backend(b,EFO_REJECTED);
    EyeFlightSnapshot before;int r=eye_flight_snapshot(o,s->frame.token,&before);
    if(r!=EFO_OK)return unlock_backend(b,r);
    if(before.flags&(EFF_FAULT|EFF_RECLAIMING))return unlock_backend(b,EFO_REJECTED);
    unsigned flag=drain_only?EFF_RENDER_DRAINED:EFF_COPY_DRAINED;
    if(before.flags&flag)return unlock_backend(b,EFO_OK); /* exact one-shot already acknowledged */
    if(drain_only){
        if(!s->render_submitted&&!s->render_taken)return unlock_backend(b,EFO_BUSY);
        eye_interval_fence_notify(&s->render,0);
        unsigned state=__atomic_load_n(&s->render.state,__ATOMIC_ACQUIRE);
        EyeIntervalResult proof=s->render_taken?s->render_result:
            state==ERI_DONE?(EyeIntervalResult){s->render.ticket,s->render.draw_mask,s->render.clear_mask,s->render.copied}:(EyeIntervalResult){0};
        if(!s->render_taken&&state!=ERI_DONE)return unlock_backend(b,state==ERI_FAULT?EFO_REJECTED:EFO_BUSY);
        if(proof.ticket.serial!=ticket.serial||proof.ticket.generation!=ticket.generation||proof.ticket.epoch!=ticket.epoch||
           proof.draw_mask!=3||proof.clear_mask!=3)return unlock_backend(b,EFO_REJECTED);
        r=eye_flight_render_drained(o,s->frame.token,ticket);
    }else{
        if(!s->copy_requested||!s->copy_started)return unlock_backend(b,EFO_BUSY);
        eye_event_fence_complete(&s->copy);
        unsigned state=__atomic_load_n(&s->copy.state,__ATOMIC_ACQUIRE);
        if(!s->copy_taken&&state!=EEC_DONE)return unlock_backend(b,state==EEC_FAULT?EFO_REJECTED:EFO_BUSY);
        if(!s->copy_taken&&(s->copy.ticket.serial!=ticket.serial||s->copy.ticket.generation!=ticket.generation||
           s->copy.ticket.epoch!=ticket.epoch||s->copy.drain_only))return unlock_backend(b,EFO_REJECTED);
        r=eye_flight_copy_drained(o,s->frame.token,ticket);
    }
    if(r==EFO_OK)*new_ack=1; /* no native taken/ack fields, destinations, source, or XR lease mutation */
    return unlock_backend(b,r);
}
int eye_flight_backend_retirement_ready(EyeFlightBackend *b,EyeFlightToken t) {
    if(!owner(b))return EFO_REJECTED;
    LOCK(b);EyeFlightNativeSlot *s=find(b,t);
    int ready=s&&s->render_taken&&s->render_ack&&
        __atomic_load_n(&s->render.state,__ATOMIC_ACQUIRE)==ERI_IDLE&&
        (!s->copy_requested||(s->copy_taken&&s->copy_ack&&__atomic_load_n(&s->copy.state,__ATOMIC_ACQUIRE)==EEC_IDLE));
    return unlock_backend(b,ready?EFO_OK:EFO_BUSY);
}
#endif
