#ifndef GF2_PRODUCER_SCOPE_NATIVE_H
#define GF2_PRODUCER_SCOPE_NATIVE_H
#include <stdint.h>
/* Scalar main-owner bridge core. Original Submit attribution, pending-header
   read, source/current and queue facts must be supplied by the game adapter.
   This module performs no Unity/XR/D3D calls and owns no resource retirement. */
enum {PSN_BEGIN=1,PSN_END=2,PSN_TAIL=3,PSN_FAULT=4,PSN_MEMBER_BEGIN=5,PSN_MEMBER_END=6};
enum {PSN_NOT_QUEUED=0,PSN_QUEUED=1,PSN_UNCERTAIN=-1};
typedef struct {uint64_t serial,generation,epoch;} PsnToken;
typedef struct {
    unsigned owner;
    uint64_t active,last_serial,failed_serial,context,child,submit_sequence,open_submit;
    PsnToken token;
    int cameras[2],open;
    unsigned started,ended,submit_depth,tail_attempted,tail_queued,tail_uncertain,issuing;
#ifdef GF2VR_ACTIVITY_STACK
    int member_open;
    uint64_t member_submit;
#endif
} ProducerScopeNative;
typedef struct {
    PsnToken token;
    uint64_t context,child;
    unsigned current,empty_pending,ordered;
} PsnBarrierFacts;
typedef int (*PsnIssue)(void *context,PsnToken token);
static int psn_same(PsnToken a,PsnToken b) {
    return a.serial && a.generation && a.epoch && a.serial==b.serial &&
        a.generation==b.generation && a.epoch==b.epoch;
}
static void psn_fail(ProducerScopeNative *s,uint64_t serial) {
    uint64_t old=__atomic_load_n(&s->failed_serial,__ATOMIC_ACQUIRE);
    while(old<serial && !__atomic_compare_exchange_n(&s->failed_serial,&old,serial,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE)) {}
}
#ifdef PSN_FAILURE_TRACE
/* Same monotonic atomic failure transition. Only its winning writer records
   origin; already-failed follow-up calls cannot overwrite the first cause. */
static void psn_fail_traced(ProducerScopeNative *s,uint64_t serial,const char *file,unsigned line) {
    uint64_t old=__atomic_load_n(&s->failed_serial,__ATOMIC_ACQUIRE);
    while(old<serial) {
        if(__atomic_compare_exchange_n(&s->failed_serial,&old,serial,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE)) {
            PSN_FAILURE_TRACE(serial,file,line);return;
        }
    }
}
#define psn_fail(s,serial) psn_fail_traced((s),(serial),__FILE__,__LINE__)
#endif
static int psn_current(ProducerScopeNative *s,unsigned thread,uint64_t serial) {
    if(!s || !serial || __atomic_load_n(&s->active,__ATOMIC_ACQUIRE)!=serial)return 0;
    if(thread!=s->owner) {psn_fail(s,serial);return 0;}
    if(s->issuing) {psn_fail(s,serial);return 0;}
    return __atomic_load_n(&s->failed_serial,__ATOMIC_ACQUIRE)<serial;
}
static int psn_arm(ProducerScopeNative *s,unsigned thread,PsnToken t,uint64_t context,uint64_t child,int l,int r) {
    if(!s || !thread || (s->owner && s->owner!=thread) || __atomic_load_n(&s->active,__ATOMIC_ACQUIRE) ||
       !psn_same(t,t) || t.serial>INT64_MAX || t.serial<=s->last_serial || !context || !child || !l || !r || l==r)return 0;
    s->owner=thread;s->last_serial=t.serial;s->token=t;s->context=context;s->child=child;
    s->cameras[0]=l;s->cameras[1]=r;s->open=0;s->started=s->ended=s->submit_depth=0;
    s->submit_sequence=s->open_submit=0;s->tail_attempted=s->tail_queued=s->tail_uncertain=0;
    s->issuing=0;
#ifdef GF2VR_ACTIVITY_STACK
    s->member_open=0;s->member_submit=0;
#endif
    __atomic_store_n(&s->active,t.serial,__ATOMIC_RELEASE);return 1;
}
static int psn_submit_enter(ProducerScopeNative *s,unsigned thread,PsnToken t,uint64_t context,uint64_t child) {
    if(!psn_current(s,thread,t.serial))return 0;
    if(!psn_same(t,s->token) || context!=s->context || child!=s->child || s->submit_depth) {psn_fail(s,t.serial);return 0;}
    s->submit_depth=1;return 1;
}
static int psn_submit_return(ProducerScopeNative *s,unsigned thread,PsnToken t,uint64_t context,uint64_t child,int success) {
    if(!psn_current(s,thread,t.serial))return 0;
    if(!psn_same(t,s->token) || context!=s->context || child!=s->child || s->submit_depth!=1 ||
       !success || s->submit_sequence==UINT64_MAX) {psn_fail(s,t.serial);return 0;}
    s->submit_depth=0;++s->submit_sequence;return 1;
}
static int psn_notify(ProducerScopeNative *s,unsigned thread,uint64_t serial,int op,int camera,
    const PsnBarrierFacts *facts,PsnIssue issue,void *ctx) {
    if(!psn_current(s,thread,serial))return 0;
    if(op==PSN_FAULT) {psn_fail(s,serial);return 0;}
    unsigned eye=camera==s->cameras[0]?1u:camera==s->cameras[1]?2u:0;
#ifdef GF2VR_ACTIVITY_STACK
    if(op==PSN_MEMBER_BEGIN) {
        if(!camera || !s->open || s->member_open || s->submit_depth || s->tail_attempted) {psn_fail(s,serial);return 0;}
        s->member_open=camera;s->member_submit=s->submit_sequence;return 1;
    }
    if(op==PSN_MEMBER_END) {
        if(!camera || !s->open || s->member_open!=camera || s->submit_depth ||
           s->submit_sequence<=s->member_submit) {psn_fail(s,serial);return 0;}
        s->member_open=0;return 1;
    }
    if(s->member_open && (op==PSN_BEGIN || op==PSN_END || op==PSN_TAIL)) {psn_fail(s,serial);return 0;}
#endif
    if(op==PSN_BEGIN) {
        if(!eye || s->open || s->started&eye || s->tail_attempted || s->submit_depth) {psn_fail(s,serial);return 0;}
        s->open=camera;s->open_submit=s->submit_sequence;s->started|=eye;return 1;
    }
    if(op==PSN_END) {
        if(!eye || s->open!=camera || !(s->started&eye) || s->ended&eye || s->submit_depth ||
           s->submit_sequence<=s->open_submit) {psn_fail(s,serial);return 0;}
        s->open=0;s->ended|=eye;return 1;
    }
    if(op!=PSN_TAIL) {psn_fail(s,serial);return 0;}
    if(eye || !camera || s->open || s->started!=3 || s->ended!=3 || s->submit_depth || s->tail_attempted) {
        psn_fail(s,serial);return 0;
    }
    s->tail_attempted=1;
    /* Rejection here intentionally leaves the legacy whole-root path eligible.
       Pending End commands are never discarded or flushed to force this route. */
    if(!facts || !issue || !psn_same(facts->token,s->token) || facts->context!=s->context ||
       facts->child!=s->child || !facts->current || !facts->empty_pending || !facts->ordered)return 0;
    s->issuing=1;
    int result=issue(ctx,s->token);
    s->issuing=0;
    if(result==PSN_QUEUED)s->tail_queued=1;
    else if(result!=PSN_NOT_QUEUED) {s->tail_uncertain=1;psn_fail(s,serial);}
    return result==PSN_QUEUED && __atomic_load_n(&s->failed_serial,__ATOMIC_ACQUIRE)<serial;
}
/* Bit1 is CPU scope valid, bit2 queued, bit4 uncertain. Queued/uncertain state
   MUST be reconciled with the backend's actual end_issued state; it never
   grants GPU completion/Release, and remains visible after late root failure. */
static unsigned psn_finish(ProducerScopeNative *s,unsigned thread,uint64_t serial,int cpu_success) {
    if(!s || !serial || __atomic_load_n(&s->active,__ATOMIC_ACQUIRE)!=serial)return 0;
    if(thread!=s->owner) {psn_fail(s,serial);return 0;}
    if(s->issuing) {psn_fail(s,serial);return 0;}
    unsigned valid=cpu_success && !s->open && s->started==3 && s->ended==3 && !s->submit_depth &&
        __atomic_load_n(&s->failed_serial,__ATOMIC_ACQUIRE)<serial;
#ifdef GF2VR_ACTIVITY_STACK
    if(s->member_open)valid=0;
#endif
    unsigned result=valid|(s->tail_queued<<1)|(s->tail_uncertain<<2);
    __atomic_store_n(&s->active,0,__ATOMIC_RELEASE);return result;
}
#endif
