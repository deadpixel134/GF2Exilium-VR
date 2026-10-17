#include "eye_flight_owner.h"
#ifdef GF2VR_RESERVE_STATE_OBSERVE
#include "reserve_state_observe.h"
static unsigned long long reserve_observe_counts[RSO_COUNT];
static void reserve_observe_add(unsigned index) {
    __atomic_fetch_add(&reserve_observe_counts[index],1,__ATOMIC_RELAXED);
}
void eye_reserve_observe_take(unsigned long long values[RSO_COUNT]) {
    if(!values)return;
    for(unsigned i=0;i<RSO_COUNT;++i)
        values[i]=__atomic_exchange_n(&reserve_observe_counts[i],0,__ATOMIC_RELAXED);
}
#endif
static int lock_owner(EyeFlightOwner *o) {
    unsigned expected=0;
    return o && __atomic_compare_exchange_n(&o->locked,&expected,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED);
}
static int unlock_owner(EyeFlightOwner *o,int result) {
    __atomic_store_n(&o->locked,0,__ATOMIC_RELEASE);return result;
}
static int token_equal(EyeFlightToken a,EyeFlightToken b) {
    return a.frame && a.generation && a.epoch && a.frame==b.frame &&
        a.generation==b.generation && a.epoch==b.epoch;
}
static EyeFlightSnapshot *find(EyeFlightOwner *o,EyeFlightToken t) {
    for(unsigned i=0;i<EFO_CAPACITY;++i) if(token_equal(o->slots[i].token,t)) return &o->slots[i];
    return 0;
}
static int current(EyeFlightOwner *o,const EyeFlightSnapshot *s) {
    return s && s->token.generation==o->generation && s->token.epoch==o->epoch &&
        !(s->flags&(EFF_CANCELLED|EFF_FAULT|EFF_RECLAIMING));
}
static int ticket_equal(const EyeFlightSnapshot *s,EyeGpuTicket t) {
    return s && t.serial && t.generation && t.epoch && s->ticket.serial==t.serial &&
        s->ticket.generation==t.generation && s->ticket.epoch==t.epoch &&
        !(s->flags&(EFF_FAULT|EFF_RECLAIMING));
}
static int production_open(const EyeFlightSnapshot *s) {
    if(!s)return 0;
    if(!(s->flags&EFF_XR_ENDED))return 1;
#ifdef GF2VR_ASYNC_PRESENTATION
    return (s->flags&EFF_DISPLAY_DETACHED) && !(s->flags&EFF_PRESENTATION_RETIRED);
#else
    return 0;
#endif
}
#ifdef GF2VR_NUMERIC_RESIDENCY
#include "numeric_residency.h"
__declspec(dllimport) int __stdcall QueryPerformanceCounter(long long *);
__declspec(dllimport) unsigned long __stdcall GetLastError(void);
__declspec(dllimport) void __stdcall SetLastError(unsigned long);
#ifdef GF2VR_NUMERIC_RESIDENCY_PERSIST
#define NR_CAPACITY 262144u
#else
#define NR_CAPACITY 65536u
#endif
#line 53
static struct {unsigned ready;NumericResidencyRow row;} nr_ring[NR_CAPACITY];
static unsigned nr_count,nr_overflow,nr_offer_busy,nr_reserve_busy;
/* Existing owner lock held. No resources, engine/XR calls, lock or IO. */
static void nr_record(EyeFlightOwner *o,const EyeFlightSnapshot *s,EyeFlightToken token,
                      unsigned type,int result,unsigned reason) {
 unsigned saved=GetLastError();long long stamp=0;
 if(!QueryPerformanceCounter(&stamp)){SetLastError(saved);return;}
 unsigned index=__atomic_fetch_add(&nr_count,1,__ATOMIC_RELAXED);
 if(index>=NR_CAPACITY){__atomic_fetch_add(&nr_overflow,1,__ATOMIC_RELAXED);SetLastError(saved);return;}
 NumericResidencyRow r={.owner=(unsigned long long)o,.qpc=(unsigned long long)stamp,
   .frame=token.frame,.generation=token.generation?token.generation:o->generation,.epoch=token.epoch?token.epoch:o->epoch,
   .serial=s?s->ticket.serial:0,.last_frame=o->last_frame,.last_begun=o->last_begun,
   .type=type,.slot=EFO_CAPACITY,.result=(unsigned)result,.reason=reason,.flags=s?s->flags:0};
 for(unsigned i=0;i<EFO_CAPACITY;++i){const EyeFlightSnapshot *p=&o->slots[i];
   if(p==s)r.slot=i;
   if(!p->token.frame)r.free_mask|=1u<<i;
   if(current(o,p)&&p->renderable&&!(p->flags&EFF_RESERVED)&&production_open(p))r.eligible_mask|=1u<<i;
 }
 nr_ring[index].row=r;__atomic_store_n(&nr_ring[index].ready,1,__ATOMIC_RELEASE);
 SetLastError(saved);
}
#ifdef GF2VR_NUMERIC_LOSS_OBSERVE
/* MPSC append of scalar evidence only. owner is an opaque identity, NEVER read.
   In particular a transferred ticket can have no live numeric/source slot. */
void numeric_residency_loss(unsigned long long owner,unsigned long long frame,
 unsigned long long generation,unsigned long long epoch,unsigned long long serial,
 unsigned boundary,unsigned loss_reason,int result,unsigned flags){
 unsigned saved=GetLastError();long long stamp=0;
 if(!QueryPerformanceCounter(&stamp)){SetLastError(saved);return;}
 unsigned index=__atomic_fetch_add(&nr_count,1,__ATOMIC_RELAXED);
 if(index>=NR_CAPACITY){__atomic_fetch_add(&nr_overflow,1,__ATOMIC_RELAXED);SetLastError(saved);return;}
 NumericResidencyRow r={.owner=owner,.qpc=(unsigned long long)stamp,.frame=frame,
  .generation=generation,.epoch=epoch,.serial=serial,.type=NR_LOSS,.slot=EFO_CAPACITY,
  .result=(unsigned)result,.reason=boundary,.loss_reason=loss_reason,.flags=flags};
 nr_ring[index].row=r;__atomic_store_n(&nr_ring[index].ready,1,__ATOMIC_RELEASE);
 SetLastError(saved);
}
#endif
int numeric_residency_read(unsigned *cursor,NumericResidencyRow *out){
 if(!cursor||!out||*cursor>=NR_CAPACITY||!__atomic_load_n(&nr_ring[*cursor].ready,__ATOMIC_ACQUIRE))return 0;
 *out=nr_ring[*cursor].row;++*cursor;return 1; /* Immutable process-lifetime rows. */
}
unsigned numeric_residency_count(void){return __atomic_load_n(&nr_count,__ATOMIC_ACQUIRE);}
unsigned numeric_residency_overflow(void){return __atomic_load_n(&nr_overflow,__ATOMIC_ACQUIRE);}
unsigned numeric_residency_busy(unsigned offer){return __atomic_load_n(offer?&nr_offer_busy:&nr_reserve_busy,__ATOMIC_ACQUIRE);}
#define NR_NOTE(o,s,t,type,result,reason) nr_record(o,s,t,type,result,reason)
#else
#define NR_NOTE(o,s,t,type,result,reason) ((void)0)
#endif
static int can_begin(EyeFlightOwner *o,EyeFlightSnapshot *s) {
    if(!s || s->flags&(EFF_XR_BEGUN|EFF_XR_ENDED|EFF_FAULT|EFF_RECLAIMING)) return 0;
    for(unsigned i=0;i<EFO_CAPACITY;++i) {
        EyeFlightSnapshot *p=&o->slots[i];
        if(!p->token.frame || p==s || p->flags&EFF_XR_ENDED) continue;
        if(p->token.frame<s->token.frame || p->flags&EFF_XR_BEGUN) return 0;
    }
    return 1;
}
static int can_release(EyeFlightSnapshot *s,unsigned mask) {
    return s && mask && !(mask&~3u) && !(s->flags&(EFF_FAULT|EFF_RECLAIMING)) &&
        (s->acquired_mask&mask)==mask && (s->waited_mask&mask)==mask &&
        !(s->released_mask&mask) &&
#ifdef GF2VR_PRODUCER_TRANSFER
        (!(s->flags&EFF_COPY_ISSUED) || (s->flags&EFF_CPU_RETURNED)) &&
#endif
#ifdef GF2VR_TWO_FLIGHT_EARLY_COPY
        (!(s->flags&EFF_COPY_ISSUED) || (s->flags&EFF_RENDER_DRAINED)) &&
#endif
        (!(s->flags&EFF_COPY_ISSUED) || s->flags&EFF_COPY_DRAINED);
}
static int can_end(EyeFlightOwner *o,EyeFlightSnapshot *s,long long time,int publish) {
    if(!s || !(s->flags&EFF_XR_BEGUN) || s->flags&(EFF_XR_ENDED|EFF_FAULT|EFF_RECLAIMING) ||
       time!=s->display_time || s->acquired_mask!=s->released_mask ||
       ((s->flags&EFF_COPY_ISSUED) && !(s->flags&EFF_COPY_DRAINED))) return 0;
    if(!publish) return 1;
    unsigned required=EFF_RENDER_ISSUED|EFF_CPU_RETURNED|EFF_RENDER_DRAINED|EFF_COPY_ISSUED|EFF_COPY_DRAINED;
    return current(o,s) && s->renderable && s->cpu_ok &&
        (s->flags&required)==required && s->acquired_mask==3 && s->waited_mask==3 && s->released_mask==3;
}
#define LOCK(o) do {if(!(o)) return EFO_REJECTED; if(!lock_owner(o)) return EFO_BUSY;} while(0)
int eye_flight_activate(EyeFlightOwner *o,unsigned long long generation,unsigned long long epoch) {
    LOCK(o);
    int ok=generation && epoch && !o->generation && !o->epoch;
    for(unsigned i=0;i<EFO_CAPACITY;++i) if(o->slots[i].token.frame) ok=0;
    if(ok) {
        if(epoch!=o->last_epoch)o->last_display_time=0;
        o->generation=generation;o->epoch=o->last_epoch=epoch;
    }
    return unlock_owner(o,ok);
}
int eye_flight_stop(EyeFlightOwner *o) {
    LOCK(o);o->generation=o->epoch=0;
    for(unsigned i=0;i<EFO_CAPACITY;++i) if(o->slots[i].token.frame) {o->slots[i].flags|=EFF_CANCELLED;NR_NOTE(o,&o->slots[i],o->slots[i].token,NR_STOP,EFO_OK,NR_REASON_NONE);}
    return unlock_owner(o,EFO_OK);
}
int eye_flight_available(EyeFlightOwner *o,unsigned *free_slots,unsigned long long *generation,unsigned long long *epoch) {
    if(!free_slots || !generation || !epoch)return EFO_REJECTED;
    *free_slots=0;*generation=*epoch=0;LOCK(o);
    for(unsigned i=0;i<EFO_CAPACITY;++i)if(!o->slots[i].token.frame)++*free_slots;
    *generation=o->generation;*epoch=o->epoch;return unlock_owner(o,EFO_OK);
}
int eye_flight_offer(EyeFlightOwner *o,EyeFlightToken t,long long time,int renderable,unsigned *slot) {
    if(!slot) return EFO_REJECTED;
    *slot=EFO_CAPACITY;
#ifdef GF2VR_NUMERIC_RESIDENCY
    if(!o)return EFO_REJECTED;
    if(!lock_owner(o)){__atomic_fetch_add(&nr_offer_busy,1,__ATOMIC_RELAXED);return EFO_BUSY;}
#else
    LOCK(o);
#endif
    if(!t.frame || !t.generation || !t.epoch || t.generation!=o->generation || t.epoch!=o->epoch ||
       t.frame<=o->last_frame || time<=0 || time<=o->last_display_time || o->last_frame>o->last_begun)
    {
#ifdef GF2VR_NUMERIC_RESIDENCY
        unsigned reason=!t.frame||!t.generation||!t.epoch?NR_REASON_TOKEN:
            t.generation!=o->generation||t.epoch!=o->epoch?NR_REASON_GENERATION:
            t.frame<=o->last_frame?NR_REASON_FRAME:time<=0||time<=o->last_display_time?NR_REASON_TIME:NR_REASON_ONE_AHEAD;
        NR_NOTE(o,0,t,NR_OFFER_REJECT,EFO_REJECTED,reason);
#endif
        return unlock_owner(o,EFO_REJECTED);
    }
    for(unsigned i=0;i<EFO_CAPACITY;++i) if(!o->slots[i].token.frame) {
        o->slots[i]=(EyeFlightSnapshot){.token=t,.display_time=time,.renderable=renderable!=0};
        o->last_frame=t.frame;o->last_display_time=time;*slot=i;
        NR_NOTE(o,&o->slots[i],t,NR_OFFER,EFO_OK,NR_REASON_NONE);
        return unlock_owner(o,EFO_OK);
    }
    NR_NOTE(o,0,t,NR_OFFER_REJECT,EFO_REJECTED,NR_REASON_FULL);
    return unlock_owner(o,EFO_REJECTED);
}
int eye_flight_snapshot(EyeFlightOwner *o,EyeFlightToken t,EyeFlightSnapshot *out) {
    if(!out) return EFO_REJECTED;
    *out=(EyeFlightSnapshot){0};
    LOCK(o);EyeFlightSnapshot *s=find(o,t);
    if(s) *out=*s;
    return unlock_owner(o,s!=0);
}
int eye_flight_slot(EyeFlightOwner *o,EyeFlightToken t,unsigned *slot) {
    if(!slot)return EFO_REJECTED;
    *slot=EFO_CAPACITY;LOCK(o);
    for(unsigned i=0;i<EFO_CAPACITY;++i)if(token_equal(o->slots[i].token,t)) {
        *slot=i;return unlock_owner(o,EFO_OK);
    }
    return unlock_owner(o,EFO_REJECTED);
}
static int reserve_render(EyeFlightOwner *o,EyeFlightSnapshot *s,unsigned long long serial,EyeFlightSnapshot *out) {
    int ok=current(o,s) && s->renderable && serial && serial>o->last_serial &&
        !(s->flags&EFF_RESERVED) && production_open(s) && o->last_event_id<=0x7ffffffc;
    if(ok) {
        s->ticket=(EyeGpuTicket){serial,s->token.generation,s->token.epoch};s->flags|=EFF_RESERVED;
        s->begin_id=++o->last_event_id;s->end_id=++o->last_event_id;s->copy_id=++o->last_event_id;
        o->last_serial=serial;*out=*s;
        NR_NOTE(o,s,s->token,NR_RESERVE,EFO_OK,NR_REASON_NONE);
    }
    return ok;
}
int eye_flight_reserve_render(EyeFlightOwner *o,EyeFlightToken t,unsigned long long serial,EyeFlightSnapshot *out) {
    if(!out)return EFO_REJECTED;
    *out=(EyeFlightSnapshot){0};LOCK(o);
    return unlock_owner(o,reserve_render(o,find(o,t),serial,out));
}
#ifdef GF2VR_RESERVE_STATE_OBSERVE
/* Caller already holds the ORIGINAL numeric lock. No payload, API, wait or IO.
   Only failed selection is counted. Original predicates/results remain below. */
static void reserve_observe_none(EyeFlightOwner *o) {
    unsigned empty=0;
    reserve_observe_add(RSO_NO_ELIGIBLE);
    for(unsigned i=0;i<EFO_CAPACITY;++i) {
        const EyeFlightSnapshot *s=&o->slots[i];
        if(!s->token.frame){++empty;reserve_observe_add(RSO_EMPTY);}
        if(!current(o,s)) {
            reserve_observe_add(RSO_NOT_CURRENT);
            if(!s->token.frame)continue;
            if(s->flags&EFF_RECLAIMING)reserve_observe_add(RSO_RECLAIMING);
            else if(s->flags&EFF_FAULT)reserve_observe_add(RSO_FAULT);
            else if(s->flags&EFF_CANCELLED)reserve_observe_add(RSO_CANCELLED);
            else reserve_observe_add(RSO_STALE);
        } else if(!s->renderable)reserve_observe_add(RSO_NOT_RENDERABLE);
        else if(s->flags&EFF_RESERVED) {
            reserve_observe_add(RSO_RESERVED);
            if(!(s->flags&EFF_CPU_RETURNED))reserve_observe_add(RSO_RESERVED_CPU);
            else if((s->flags&EFF_RENDER_ISSUED) && !(s->flags&EFF_RENDER_DRAINED))
                reserve_observe_add(RSO_RESERVED_RENDER);
            else if(!(s->flags&EFF_COPY_ISSUED))reserve_observe_add(RSO_RESERVED_PRE_COPY);
            else if(!(s->flags&EFF_COPY_DRAINED))reserve_observe_add(RSO_RESERVED_COPY);
            else if(s->acquired_mask!=s->released_mask)reserve_observe_add(RSO_RESERVED_LEASES);
            else if(s->flags&EFF_PRODUCER_TRANSFERRED)reserve_observe_add(RSO_RESERVED_TRANSFERRED);
            else reserve_observe_add(RSO_RESERVED_TRANSFER);
        } else if(!production_open(s))reserve_observe_add(RSO_CLOSED);
    }
    reserve_observe_add(RSO_EMPTY_0+empty);
}
#endif
int eye_flight_reserve_next_render(EyeFlightOwner *o,unsigned long long serial,EyeFlightSnapshot *out,unsigned *slot) {
#ifdef GF2VR_RESERVE_STATE_OBSERVE
    reserve_observe_add(RSO_ATTEMPTS);
    if(!out || !slot){reserve_observe_add(RSO_INVALID);return EFO_REJECTED;}
    *out=(EyeFlightSnapshot){0};*slot=EFO_CAPACITY;
    if(!o){reserve_observe_add(RSO_INVALID);return EFO_REJECTED;}
    if(!lock_owner(o)){reserve_observe_add(RSO_BUSY);return EFO_BUSY;}
    reserve_observe_add(RSO_LOCKED);
#else
    if(!out || !slot)return EFO_REJECTED;
#ifdef GF2VR_NUMERIC_RESIDENCY
    *out=(EyeFlightSnapshot){0};*slot=EFO_CAPACITY;
    if(!o)return EFO_REJECTED;
    if(!lock_owner(o)){__atomic_fetch_add(&nr_reserve_busy,1,__ATOMIC_RELAXED);return EFO_BUSY;}
#else
    *out=(EyeFlightSnapshot){0};*slot=EFO_CAPACITY;LOCK(o);
#endif
#endif
    unsigned index=EFO_CAPACITY;
    for(unsigned i=0;i<EFO_CAPACITY;++i) {
        EyeFlightSnapshot *s=&o->slots[i];
        if(!current(o,s) || !s->renderable || s->flags&EFF_RESERVED || !production_open(s))continue;
        if(index==EFO_CAPACITY || s->token.frame<o->slots[index].token.frame)index=i;
    }
    int ok=index<EFO_CAPACITY && reserve_render(o,&o->slots[index],serial,out);
    if(ok)*slot=index;
#ifdef GF2VR_NUMERIC_RESIDENCY
    EyeFlightSnapshot *observed=index<EFO_CAPACITY?&o->slots[index]:0;
    unsigned reason=ok?NR_REASON_NONE:!observed?NR_REASON_NO_ELIGIBLE:
       !serial||serial<=o->last_serial?NR_REASON_SERIAL:NR_REASON_EVENT_LIMIT;
    if(!ok)NR_NOTE(o,observed,observed?observed->token:(EyeFlightToken){0},NR_RESERVE_REJECT,ok,reason);
#endif
#ifdef GF2VR_RESERVE_STATE_OBSERVE
    if(ok)reserve_observe_add(RSO_OK);
    else if(index==EFO_CAPACITY)reserve_observe_none(o);
    else if(!serial || serial<=o->last_serial)reserve_observe_add(RSO_SERIAL_REJECT);
    else if(o->last_event_id>0x7ffffffc)reserve_observe_add(RSO_EVENT_LIMIT);
    else reserve_observe_add(RSO_OTHER_REJECT);
#endif
    return unlock_owner(o,ok);
}
int eye_flight_issue_render(EyeFlightOwner *o,EyeFlightToken t,EyeGpuTicket ticket) {
    LOCK(o);EyeFlightSnapshot *s=find(o,t);
    int ok=current(o,s) && ticket_equal(s,ticket) && s->flags&EFF_RESERVED &&
        !(s->flags&(EFF_RENDER_ISSUED|EFF_CPU_RETURNED)) && production_open(s);
    if(ok) {s->flags|=EFF_RENDER_ISSUED;NR_NOTE(o,s,t,NR_ISSUE,ok,NR_REASON_NONE);}
    return unlock_owner(o,ok);
}
int eye_flight_cancel_unissued(EyeFlightOwner *o,EyeFlightToken t,EyeGpuTicket ticket) {
    LOCK(o);EyeFlightSnapshot *s=find(o,t);
    int ok=ticket_equal(s,ticket) && s->flags&EFF_RESERVED && !(s->flags&(EFF_RENDER_ISSUED|EFF_CPU_RETURNED));
    if(ok) {s->flags|=EFF_CANCELLED|EFF_CPU_RETURNED;s->cpu_ok=0;NR_NOTE(o,s,t,NR_CANCEL,ok,NR_REASON_NONE);}
    return unlock_owner(o,ok);
}
int eye_flight_cpu_return(EyeFlightOwner *o,EyeFlightToken t,EyeGpuTicket ticket,int success) {
    LOCK(o);EyeFlightSnapshot *s=find(o,t);
    int ok=ticket_equal(s,ticket) && s->flags&EFF_RENDER_ISSUED && !(s->flags&EFF_CPU_RETURNED);
    if(ok) {s->flags|=EFF_CPU_RETURNED;s->cpu_ok=success!=0;if(!success)s->flags|=EFF_CANCELLED;NR_NOTE(o,s,t,NR_CPU,success,NR_REASON_NONE);}
    return unlock_owner(o,ok);
}
int eye_flight_render_drained(EyeFlightOwner *o,EyeFlightToken t,EyeGpuTicket ticket) {
    LOCK(o);EyeFlightSnapshot *s=find(o,t);
    int ok=ticket_equal(s,ticket) && s->flags&EFF_RENDER_ISSUED && !(s->flags&EFF_RENDER_DRAINED);
    if(ok){s->flags|=EFF_RENDER_DRAINED;NR_NOTE(o,s,t,NR_RENDER_DRAIN,ok,NR_REASON_NONE);}
    return unlock_owner(o,ok);
}
int eye_flight_render_submitted(EyeFlightOwner *o,EyeFlightToken t,EyeGpuTicket ticket) {
    LOCK(o);EyeFlightSnapshot *s=find(o,t);
#ifdef GF2VR_PRODUCER_TRANSFER
    int cpu_ready=s && s->cpu_ok && (s->flags&EFF_CPU_RETURNED);
    if(s && (s->flags&EFF_PRIVATE_PRODUCER) && !(s->flags&EFF_CPU_RETURNED))cpu_ready=1;
    int ok=current(o,s) && ticket_equal(s,ticket) && cpu_ready &&
        (s->flags&EFF_RENDER_ISSUED) &&
#else
    int ok=current(o,s) && ticket_equal(s,ticket) && s->cpu_ok &&
        (s->flags&(EFF_RENDER_ISSUED|EFF_CPU_RETURNED))==(EFF_RENDER_ISSUED|EFF_CPU_RETURNED) &&
#endif
        !(s->flags&(EFF_RENDER_SUBMITTED|EFF_FAULT|EFF_CANCELLED|EFF_RECLAIMING)) && production_open(s);
    if(ok)s->flags|=EFF_RENDER_SUBMITTED;
    return unlock_owner(o,ok);
}
int eye_flight_event(EyeFlightOwner *o,int id,EyeFlightSnapshot *out,unsigned *phase) {
    if(!out || !phase) return EFO_REJECTED;
    *out=(EyeFlightSnapshot){0};*phase=0;
    if(id<=0) return EFO_REJECTED;
    LOCK(o);
    for(unsigned i=0;i<EFO_CAPACITY;++i) {
        EyeFlightSnapshot *s=&o->slots[i];unsigned p=0;
        if(!s->token.frame || s->flags&(EFF_FAULT|EFF_RECLAIMING))continue;
        if((s->flags&EFF_RENDER_ISSUED) && !(s->flags&EFF_RENDER_DRAINED)) {
            if(id==s->begin_id)p=EFO_BEGIN_EVENT;
            if(id==s->end_id)p=EFO_END_EVENT;
        }
        if((s->flags&EFF_COPY_ISSUED) && !(s->flags&EFF_COPY_DRAINED) && id==s->copy_id)p=EFO_COPY_EVENT;
        if(p) {*out=*s;*phase=p;return unlock_owner(o,EFO_OK);}
    }
    return unlock_owner(o,EFO_REJECTED);
}
#ifdef GF2VR_PRODUCER_TRANSFER
int eye_flight_private_producer(EyeFlightOwner *o,EyeFlightToken t,EyeGpuTicket ticket) {
    LOCK(o);EyeFlightSnapshot *s=find(o,t);
    int ok=current(o,s) && ticket_equal(s,ticket) && (s->flags&EFF_RENDER_ISSUED) &&
        !(s->flags&EFF_PRIVATE_PRODUCER) && production_open(s) &&
        (!(s->flags&EFF_CPU_RETURNED) || s->cpu_ok);
    if(ok)s->flags|=EFF_PRIVATE_PRODUCER;
    return unlock_owner(o,ok);
}
#endif
int eye_flight_can_begin(EyeFlightOwner *o,EyeFlightToken t) {
    LOCK(o);return unlock_owner(o,can_begin(o,find(o,t)));
}
int eye_flight_begun(EyeFlightOwner *o,EyeFlightToken t) {
    LOCK(o);EyeFlightSnapshot *s=find(o,t);int ok=can_begin(o,s);
    if(ok) {s->flags|=EFF_XR_BEGUN;o->last_begun=t.frame;NR_NOTE(o,s,t,NR_BEGIN,ok,NR_REASON_NONE);}
    return unlock_owner(o,ok);
}
int eye_flight_acquired(EyeFlightOwner *o,EyeFlightToken t,unsigned mask) {
    LOCK(o);EyeFlightSnapshot *s=find(o,t);
    int ok=s && mask && !(mask&~3u) && s->flags&EFF_XR_BEGUN &&
        production_open(s) &&
        !(s->flags&(EFF_FAULT|EFF_RECLAIMING|EFF_COPY_ISSUED)) && !(s->acquired_mask&mask);
    if(ok)s->acquired_mask|=mask;
    return unlock_owner(o,ok);
}
int eye_flight_waited(EyeFlightOwner *o,EyeFlightToken t,unsigned mask) {
    LOCK(o);EyeFlightSnapshot *s=find(o,t);
    int ok=s && mask && !(mask&~3u) && !(s->flags&(EFF_FAULT|EFF_RECLAIMING)) && production_open(s) &&
        (s->acquired_mask&mask)==mask && !(s->waited_mask&mask);
    if(ok)s->waited_mask|=mask;
    return unlock_owner(o,ok);
}
int eye_flight_issue_copy(EyeFlightOwner *o,EyeFlightToken t,EyeGpuTicket ticket) {
    LOCK(o);EyeFlightSnapshot *s=find(o,t);
#ifdef GF2VR_TWO_FLIGHT_EARLY_COPY
    unsigned required=EFF_RENDER_ISSUED|EFF_CPU_RETURNED|EFF_XR_BEGUN;
    int render_ready=s && (s->flags&(EFF_RENDER_SUBMITTED|EFF_RENDER_DRAINED));
#else
    unsigned required=EFF_RENDER_ISSUED|EFF_CPU_RETURNED|EFF_RENDER_DRAINED|EFF_XR_BEGUN;
#endif
#ifdef GF2VR_PRODUCER_TRANSFER
    int cpu_ready=s && s->cpu_ok;
    if(s && (s->flags&EFF_PRIVATE_PRODUCER) && !(s->flags&EFF_CPU_RETURNED)) {
        required &= ~EFF_CPU_RETURNED;cpu_ready=1;
    }
    int ok=current(o,s) && ticket_equal(s,ticket) && cpu_ready && s->renderable &&
#else
    int ok=current(o,s) && ticket_equal(s,ticket) && s->cpu_ok && s->renderable &&
#endif
        (s->flags&required)==required && !(s->flags&EFF_COPY_ISSUED) && production_open(s) &&
#ifdef GF2VR_TWO_FLIGHT_EARLY_COPY
        render_ready &&
#endif
        s->acquired_mask==3 && s->waited_mask==3 && !s->released_mask;
    if(ok)s->flags|=EFF_COPY_ISSUED;
    return unlock_owner(o,ok);
}
int eye_flight_copy_drained(EyeFlightOwner *o,EyeFlightToken t,EyeGpuTicket ticket) {
    LOCK(o);EyeFlightSnapshot *s=find(o,t);
    int ok=ticket_equal(s,ticket) && s->flags&EFF_COPY_ISSUED && !(s->flags&EFF_COPY_DRAINED);
    if(ok){s->flags|=EFF_COPY_DRAINED;NR_NOTE(o,s,t,NR_COPY_DRAIN,ok,NR_REASON_NONE);}
    return unlock_owner(o,ok);
}
int eye_flight_can_release(EyeFlightOwner *o,EyeFlightToken t,unsigned mask) {
    LOCK(o);return unlock_owner(o,can_release(find(o,t),mask));
}
int eye_flight_released(EyeFlightOwner *o,EyeFlightToken t,unsigned mask) {
    LOCK(o);EyeFlightSnapshot *s=find(o,t);int ok=can_release(s,mask);
    if(ok)s->released_mask|=mask;
    return unlock_owner(o,ok);
}
int eye_flight_can_end(EyeFlightOwner *o,EyeFlightToken t,long long time,int publish) {
    LOCK(o);return unlock_owner(o,can_end(o,find(o,t),time,publish));
}
int eye_flight_ended(EyeFlightOwner *o,EyeFlightToken t,long long time,int publish) {
    LOCK(o);EyeFlightSnapshot *s=find(o,t);int ok=can_end(o,s,time,publish);
    if(ok) {s->flags|=EFF_XR_ENDED;if(!publish)s->flags|=EFF_CANCELLED;}
    return unlock_owner(o,ok);
}
#ifdef GF2VR_ASYNC_PRESENTATION
static int retain_offer_shape(const EyeFlightSnapshot *s,long long time) {
    return s && s->renderable && time==s->display_time && (s->flags&EFF_XR_BEGUN) &&
        !(s->flags&(EFF_XR_ENDED|EFF_FAULT|EFF_RECLAIMING|EFF_COPY_ISSUED)) &&
        !s->acquired_mask && !s->waited_mask && !s->released_mask;
}
int eye_flight_can_retain_offer(EyeFlightOwner *o,EyeFlightToken t,long long time) {
    LOCK(o);EyeFlightSnapshot *s=find(o,t);
    int ok=current(o,s) && retain_offer_shape(s,time) &&
        (!(s->flags&EFF_CPU_RETURNED) || s->cpu_ok);
    return unlock_owner(o,ok);
}
int eye_flight_retained_offer(EyeFlightOwner *o,EyeFlightToken t,long long time) {
    LOCK(o);EyeFlightSnapshot *s=find(o,t);
    /* A real End can reenter stop or CPU cancellation. Still acknowledge that
       API fact, retaining CANCELLED instead of resurrecting the producer. */
    int ok=retain_offer_shape(s,time);
    if(ok)s->flags|=EFF_XR_ENDED|EFF_DISPLAY_DETACHED|EFF_PRODUCER_RETAINED;
    return unlock_owner(o,ok);
}
int eye_flight_cancel_retained_offer(EyeFlightOwner *o,EyeFlightToken t) {
    LOCK(o);EyeFlightSnapshot *s=find(o,t);
    unsigned required=EFF_XR_ENDED|EFF_DISPLAY_DETACHED|EFF_PRODUCER_RETAINED;
    int ok=s && (s->flags&required)==required &&
        !(s->flags&(EFF_FAULT|EFF_RECLAIMING|EFF_PRESENTATION_RETIRED));
    if(ok){s->flags|=EFF_CANCELLED;NR_NOTE(o,s,t,NR_CANCEL,ok,NR_REASON_NONE);}
    return unlock_owner(o,ok);
}
static int can_detach(EyeFlightOwner *o,EyeFlightSnapshot *s,long long time) {
    unsigned required=EFF_XR_BEGUN|EFF_RESERVED|EFF_RENDER_ISSUED|EFF_CPU_RETURNED;
    return current(o,s) && s->renderable && s->cpu_ok && time==s->display_time &&
        (s->flags&required)==required && !(s->flags&EFF_XR_ENDED);
}
int eye_flight_can_detach(EyeFlightOwner *o,EyeFlightToken t,long long time) {
    LOCK(o);return unlock_owner(o,can_detach(o,find(o,t),time));
}
int eye_flight_detached(EyeFlightOwner *o,EyeFlightToken t,long long time) {
    LOCK(o);EyeFlightSnapshot *s=find(o,t);
    /* Actual End may reenter source stop. A terminal acknowledgement must still
       record the real End, keeping cancellation (never reviving publication). */
    unsigned required=EFF_XR_BEGUN|EFF_RESERVED|EFF_RENDER_ISSUED|EFF_CPU_RETURNED;
    int ok=s && s->renderable && s->cpu_ok && time==s->display_time &&
        (s->flags&required)==required && !(s->flags&(EFF_XR_ENDED|EFF_FAULT|EFF_RECLAIMING));
    if(ok)s->flags|=EFF_XR_ENDED|EFF_DISPLAY_DETACHED;
    return unlock_owner(o,ok);
}
static int can_detached_dispose(EyeFlightOwner *o,EyeFlightSnapshot *s,int published) {
    unsigned required=EFF_DISPLAY_DETACHED|EFF_XR_ENDED|EFF_CPU_RETURNED|EFF_RENDER_DRAINED;
    int drained=s && (s->flags&required)==required;
    /* No issued render means there is no GPU fence to fabricate. This exception
       is discard-only, explicitly revoked, and requires main's CPU cancellation
       acknowledgement if it had already reserved a managed/native ticket. */
    if(!published && s && (s->flags&EFF_PRODUCER_RETAINED) &&
       (s->flags&EFF_CANCELLED) && !(s->flags&EFF_RENDER_ISSUED))
        drained=(s->flags&(EFF_DISPLAY_DETACHED|EFF_XR_ENDED))==
            (EFF_DISPLAY_DETACHED|EFF_XR_ENDED) &&
            (!(s->flags&EFF_RESERVED) || (s->flags&EFF_CPU_RETURNED));
    int ok=drained &&
        !(s->flags&(EFF_FAULT|EFF_RECLAIMING|EFF_PRESENTATION_RETIRED)) &&
        s->acquired_mask==s->released_mask &&
        (!(s->flags&EFF_COPY_ISSUED) || (s->flags&EFF_COPY_DRAINED));
    if(published)ok=ok && current(o,s) && s->cpu_ok &&
        (s->flags&EFF_COPY_ISSUED) && s->acquired_mask==3 && s->waited_mask==3;
    return ok;
}
int eye_flight_can_detached_dispose(EyeFlightOwner *o,EyeFlightToken t,int published) {
    LOCK(o);return unlock_owner(o,can_detached_dispose(o,find(o,t),published));
}
int eye_flight_detached_dispose(EyeFlightOwner *o,EyeFlightToken t,int published) {
    LOCK(o);EyeFlightSnapshot *s=find(o,t);int ok=can_detached_dispose(o,s,published);
    if(ok) {s->flags|=EFF_PRESENTATION_RETIRED;if(!published)s->flags|=EFF_CANCELLED;}
    return unlock_owner(o,ok);
}
#ifdef GF2VR_BANK_TRANSFER
int eye_flight_transfer(EyeFlightOwner *o,EyeFlightToken t,EyeGpuTicket ticket) {
    LOCK(o);EyeFlightSnapshot *s=find(o,t);
    unsigned required=EFF_XR_ENDED|EFF_DISPLAY_DETACHED|EFF_RENDER_ISSUED|
        EFF_CPU_RETURNED|EFF_RENDER_DRAINED|EFF_COPY_ISSUED|EFF_COPY_DRAINED;
    int ok=current(o,s) && ticket_equal(s,ticket) && s->renderable && s->cpu_ok &&
        (s->flags&required)==required &&
        !(s->flags&(EFF_CANCELLED|EFF_FAULT|EFF_RECLAIMING|EFF_PRESENTATION_RETIRED)) &&
        s->acquired_mask==3 && s->waited_mask==3 && s->released_mask==3;
    if(ok){
#ifdef GF2VR_NUMERIC_RESIDENCY
        if(!(s->flags&EFF_PRODUCER_TRANSFERRED)){s->flags|=EFF_PRODUCER_TRANSFERRED;NR_NOTE(o,s,t,NR_TRANSFER,ok,NR_REASON_NONE);}
#else
        s->flags|=EFF_PRODUCER_TRANSFERRED;
#endif
    } /* idempotent exact ack, not End */
    return unlock_owner(o,ok);
}
#endif
#endif
int eye_flight_fault(EyeFlightOwner *o,EyeFlightToken t) {
    LOCK(o);EyeFlightSnapshot *s=find(o,t);
    int ok=s && !(s->flags&EFF_RECLAIMING);
    if(ok){s->flags|=EFF_FAULT|EFF_CANCELLED;NR_NOTE(o,s,t,NR_CANCEL,ok,NR_REASON_NONE);}
    return unlock_owner(o,ok);
}
int eye_flight_session_stopped(EyeFlightOwner *o,EyeFlightToken terminal[EFO_CAPACITY]) {
    if(!terminal)return EFO_REJECTED;
    for(unsigned i=0;i<EFO_CAPACITY;++i)terminal[i]=(EyeFlightToken){0};
    LOCK(o);
    if(o->generation)return unlock_owner(o,EFO_REJECTED);
    for(unsigned i=0;i<EFO_CAPACITY;++i) {
        EyeFlightSnapshot *s=&o->slots[i];
        if(s->token.frame && (s->flags&EFF_FAULT || s->acquired_mask!=s->released_mask ||
            ((s->flags&EFF_COPY_ISSUED) && !(s->flags&EFF_COPY_DRAINED))))
            return unlock_owner(o,EFO_REJECTED);
    }
    for(unsigned i=0;i<EFO_CAPACITY;++i)if(o->slots[i].token.frame) {
        o->slots[i].flags|=EFF_XR_ENDED|EFF_CANCELLED;terminal[i]=o->slots[i].token;
    }
    /* Seal the old wait/admission barrier on actual session termination.
       Do NOT set XR_BEGUN on unbegun slots or rewind unique frame IDs. */
    o->last_begun=o->last_frame;
    return unlock_owner(o,EFO_OK);
}
int eye_flight_take(EyeFlightOwner *o,EyeFlightToken t,EyeFlightSnapshot *out) {
    if(!out)return EFO_REJECTED;
    *out=(EyeFlightSnapshot){0};
    LOCK(o);EyeFlightSnapshot *s=find(o,t);
    int ok=s && s->flags&EFF_XR_ENDED && !(s->flags&(EFF_FAULT|EFF_RECLAIMING)) &&
        (!(s->flags&EFF_DISPLAY_DETACHED) || (s->flags&EFF_PRESENTATION_RETIRED)
#ifdef GF2VR_BANK_TRANSFER
         || (s->flags&EFF_PRODUCER_TRANSFERRED)
#endif
        ) &&
        (!(s->flags&EFF_RESERVED) || s->flags&EFF_CPU_RETURNED) &&
        (!(s->flags&EFF_RENDER_ISSUED) || s->flags&EFF_RENDER_DRAINED) &&
        (!(s->flags&EFF_COPY_ISSUED) || s->flags&EFF_COPY_DRAINED) && s->acquired_mask==s->released_mask;
    if(ok) {s->flags|=EFF_RECLAIMING;*out=*s;NR_NOTE(o,s,t,NR_TAKE,ok,NR_REASON_NONE);}
    return unlock_owner(o,ok);
}
int eye_flight_retire_ack(EyeFlightOwner *o,EyeFlightToken t) {
    LOCK(o);EyeFlightSnapshot *s=find(o,t);
    int ok=s && s->flags&EFF_RECLAIMING && !(s->flags&EFF_FAULT);
    if(ok){NR_NOTE(o,s,t,NR_RETIRE,ok,NR_REASON_NONE);*s=(EyeFlightSnapshot){0};}
    return unlock_owner(o,ok);
}
