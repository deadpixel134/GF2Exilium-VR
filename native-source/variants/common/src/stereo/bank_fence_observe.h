#ifndef GF2_BANK_FENCE_OBSERVE_H
#define GF2_BANK_FENCE_OBSERVE_H
/* Diagnostic numeric mailbox only. Never retain COM/engine/XR resources, issue
   GPU commands, wait, or perform callback file I/O. Process lifetime cap. */
#ifdef GF2VR_BANK_ADMISSION_OBSERVE
#ifdef GF2VR_PRODUCER_HANDOFF_TIMING
#define BAO_LAST_PHASE 51
#elif defined(GF2VR_BANK_COPY_DELIVERY_OBSERVE)
#define BAO_LAST_PHASE 49
#elif defined(GF2VR_BANK_NATIVE_OBSERVE)
#define BAO_LAST_PHASE 45
#elif defined(GF2VR_BANK_CPU_OBSERVE)
#define BAO_LAST_PHASE 41
#elif defined(GF2VR_BANK_TRANSFER_OBSERVE)
#define BAO_LAST_PHASE 31
#else
#define BAO_LAST_PHASE 28
#endif
#define BFO_CAPACITY 384
#else
#define BFO_CAPACITY 96
#endif
typedef struct {
    unsigned long long serial,generation,epoch,qpc,value;
    unsigned thread,phase,drain_only,tried;
    unsigned hr[4];
} BankFenceObservation;
void bank_fence_observe_record(const BankFenceObservation *);
int bank_fence_observe_take(unsigned *,BankFenceObservation *);
#ifdef GF2VR_BANK_READMIT_OBSERVE
int bank_fence_observe_started(void);
#endif
#ifdef GF2VR_BANK_ADMISSION_OBSERVE
void bank_admission_observe_record(unsigned long long,unsigned long long,unsigned long long,
    unsigned,unsigned long long,unsigned,int);
#endif
#ifdef GF2VR_BANK_NATIVE_OBSERVE
/* Existing fence timestamp, emitted later by the DONE-acquiring main owner.
   thread is the snapshot emitter, not the historical Signal/poll caller. */
void bank_native_observe_record(unsigned long long,unsigned long long,unsigned long long,
    unsigned long long,unsigned,unsigned long long,unsigned,unsigned);
#endif
#ifdef GF2VR_BANK_DISPATCH_OBSERVE
/* Numeric only: ticket fields must already belong to the caller. */
void bank_dispatch_observe_record(unsigned long long,unsigned long long,unsigned long long,
                                  unsigned,unsigned long long);
#endif
#ifdef GF2_BANK_FENCE_OBSERVE_IMPLEMENTATION
static struct {unsigned state;BankFenceObservation row;} bfo_rows[BFO_CAPACITY];
static unsigned bfo_claimed;
#if defined(GF2VR_BANK_CYCLE_OBSERVE) && !defined(BOUNDARY_TEST)
static unsigned bfo_cycle_started;
static unsigned long long bfo_cycle_after_ms;
#ifndef BFO_CYCLE_TICK
#define BFO_CYCLE_TICK() GetTickCount64()
#endif
#endif
#if defined(GF2VR_BANK_GPU_OBSERVE) || defined(GF2VR_BANK_EYE_WRITE_OBSERVE)
int bank_cycle_observe_ready(void) {
#if defined(GF2VR_BANK_CYCLE_OBSERVE) && !defined(BOUNDARY_TEST)
    return __atomic_load_n(&bfo_cycle_started,__ATOMIC_ACQUIRE) && GetTickCount64()>=bfo_cycle_after_ms;
#else
    return 1;
#endif
}
#endif
#if defined(GF2VR_BANK_READMIT_OBSERVE) && !defined(BOUNDARY_TEST)
static unsigned bfo_started;
#endif
#ifdef GF2VR_BANK_READMIT_OBSERVE
int bank_fence_observe_started(void) {
#if defined(GF2VR_BANK_READMIT_OBSERVE) && !defined(BOUNDARY_TEST)
    return __atomic_load_n(&bfo_started,__ATOMIC_ACQUIRE)!=0;
#else
    return 1;
#endif
}
#endif
#ifdef GF2VR_BANK_ADMISSION_OBSERVE
void bank_admission_observe_record(unsigned long long serial,unsigned long long generation,
    unsigned long long epoch,unsigned phase,unsigned long long frame,unsigned flags,int result) {
    /* Reservation attempts have no token; do not invent one or query ownership.
       Offer/retirement are bracketed: other owners may act before return. */
    if(phase<21 || phase>BAO_LAST_PHASE || __atomic_load_n(&bfo_claimed,__ATOMIC_RELAXED)>=BFO_CAPACITY)return;
    DWORD saved=GetLastError();LARGE_INTEGER at={0};
    if(QueryPerformanceCounter(&at)) {
        BankFenceObservation r={0};r.serial=serial;r.generation=generation;r.epoch=epoch;
        r.qpc=(unsigned long long)at.QuadPart;r.thread=GetCurrentThreadId();
        r.phase=phase;r.value=frame;r.drain_only=flags;r.tried=(unsigned)result;
        bank_fence_observe_record(&r);
    }
    SetLastError(saved);
}
#endif
#ifdef GF2VR_BANK_NATIVE_OBSERVE
void bank_native_observe_record(unsigned long long serial,unsigned long long generation,
    unsigned long long epoch,unsigned long long frame,unsigned phase,
    unsigned long long stamp,unsigned polls,unsigned pending) {
    if(!serial || !generation || !epoch || !frame || !stamp || phase<42 ||
#ifdef GF2VR_BANK_COPY_DELIVERY_OBSERVE
       phase>49 ||
#else
       phase>45 ||
#endif
       __atomic_load_n(&bfo_claimed,__ATOMIC_RELAXED)>=BFO_CAPACITY)return;
    DWORD saved=GetLastError();
    BankFenceObservation r={0};r.serial=serial;r.generation=generation;r.epoch=epoch;
    r.qpc=stamp;r.thread=GetCurrentThreadId();r.phase=phase;r.value=frame;
    r.drain_only=1; /* historical timestamp scalar snapshot */
    r.tried=1;r.hr[0]=polls;r.hr[1]=pending;
    bank_fence_observe_record(&r);SetLastError(saved);
}
#endif
#ifdef GF2VR_BANK_DISPATCH_OBSERVE
void bank_dispatch_observe_record(unsigned long long serial,unsigned long long generation,
    unsigned long long epoch,unsigned phase,unsigned long long value) {
    if(!serial || !generation || !epoch || phase<11 || phase>17 ||
       __atomic_load_n(&bfo_claimed,__ATOMIC_RELAXED)>=BFO_CAPACITY)return;
    DWORD saved=GetLastError();LARGE_INTEGER at={0};
    if(QueryPerformanceCounter(&at)) {
        BankFenceObservation r={0};r.serial=serial;r.generation=generation;r.epoch=epoch;
        r.qpc=(unsigned long long)at.QuadPart;r.thread=GetCurrentThreadId();
        r.phase=phase;r.value=value;bank_fence_observe_record(&r);
    }
    SetLastError(saved);
}
#endif
void bank_fence_observe_record(const BankFenceObservation *r) {
    if(!r)return;
#if defined(GF2VR_BANK_CYCLE_OBSERVE) && !defined(BOUNDARY_TEST)
    /* Only diagnostic admission changes. Existing producer/XR/GPU decisions
       never read this state. First genuine publication starts a 2s warmup. */
    if(!__atomic_load_n(&bfo_cycle_started,__ATOMIC_ACQUIRE)) {
        if(r->phase!=31 || r->tried!=1 || !r->serial || !r->generation ||
           !r->epoch || !r->value)return;
        bfo_cycle_after_ms=BFO_CYCLE_TICK()+2000;
        __atomic_store_n(&bfo_cycle_started,1,__ATOMIC_RELEASE);
    }
    if(BFO_CYCLE_TICK()<bfo_cycle_after_ms || !r->serial || !r->generation ||
       !r->epoch ||
#ifdef GF2VR_BANK_COPY_DELIVERY_OBSERVE
#ifdef GF2VR_XR_READMIT_CAUSE_OBSERVE
       (!r->value && r->phase!=16 && r->phase!=32)
#else
       (!r->value && r->phase!=16)
#endif
#else
       !r->value
#endif
       )return; /* dispatch16's copy-requested=0 is a valid observation */
    switch(r->phase) {
    case 21: case 22: case 23: case 24: case 29: case 30: case 31: break;
#ifdef GF2VR_BANK_CPU_OBSERVE
    case 40: case 41: break;
#endif
#ifdef GF2VR_BANK_NATIVE_OBSERVE
    case 42: case 43: case 44: case 45: break;
#endif
#ifdef GF2VR_BANK_COPY_DELIVERY_OBSERVE
    case 46: case 47: case 48: case 49: break;
    case 11: case 12: case 13: case 14: case 15: case 16: case 17: break;
#endif
#ifdef GF2VR_PRODUCER_HANDOFF_TIMING
    case 50: case 51: break; /* actual producer-End issue bracket only */
#endif
    case 28: if(r->tried!=1)return;break; /* failed reserve flood has no cycle */
#ifdef GF2VR_XR_READMIT_CAUSE_OBSERVE
    case 32: break; /* Existing sole-XR readmit reasons, including value0. */
#endif
    default: return;
    }
#endif
#if defined(GF2VR_BANK_READMIT_OBSERVE) && !defined(BOUNDARY_TEST)
    /* Production decisions unchanged. Only a real successful fresh publication
       can start this diagnostic budget; startup floods consume no rows. */
    if(!bank_fence_observe_started()) {
        if(r->phase!=31 || r->tried!=1 || !r->serial || !r->generation || !r->epoch)return;
        __atomic_store_n(&bfo_started,1,__ATOMIC_RELEASE);
    }
#endif
    unsigned n=__atomic_load_n(&bfo_claimed,__ATOMIC_RELAXED);
    /* Bounded contention work: losing an observation cannot affect production. */
    for(unsigned retry=0;retry<4 && n<BFO_CAPACITY;++retry) {
        if(__atomic_compare_exchange_n(&bfo_claimed,&n,n+1,0,__ATOMIC_RELAXED,__ATOMIC_RELAXED)) {
            bfo_rows[n].row=*r;
            __atomic_store_n(&bfo_rows[n].state,1,__ATOMIC_RELEASE);return;
        }
    }
}
int bank_fence_observe_take(unsigned *cursor,BankFenceObservation *out) {
    if(!cursor || !out || *cursor>=BFO_CAPACITY)return 0;
    unsigned n=*cursor;
    if(__atomic_load_n(&bfo_rows[n].state,__ATOMIC_ACQUIRE)!=1)return 0;
    *out=bfo_rows[n].row;
    __atomic_store_n(&bfo_rows[n].state,2,__ATOMIC_RELEASE);
    *cursor=n+1;return 1; /* sole main consumer; never recycle rows */
}
#endif
#endif
