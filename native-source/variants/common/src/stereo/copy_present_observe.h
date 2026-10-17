#ifndef GF2_COPY_PRESENT_OBSERVE_H
#define GF2_COPY_PRESENT_OBSERVE_H
#include "eye_gpu_copy.h"
#define CPO_CAPACITY 384u
enum { CPO_PRE_ENTER=1,CPO_ORIGINAL_ENTER,CPO_ORIGINAL_RETURN,CPO_POST_RETURN,
       CPO_COPY_BEGIN,CPO_COPY_SIGNAL,CPO_FENCE_POLL };
typedef struct {
    EyeGpuTicket ticket;
    unsigned long long qpc,present,device,context,value;
    unsigned thread,edge,phase;
} CopyPresentObservation;
void copy_present_observe_arm(void);
void copy_present_observe_enter(void *,void *);
void copy_present_observe_mark(unsigned,unsigned long long);
void copy_present_observe_copy(EyeGpuTicket,void *,void *,unsigned,unsigned long long);
int copy_present_observe_take(unsigned *,CopyPresentObservation *);
#ifdef GF2_COPY_PRESENT_IMPLEMENTATION
#ifndef CPO_TICK
#define CPO_TICK() GetTickCount64()
#endif
/* Finite numeric MPSC log, not an owner/completion authority. No GPU/COM calls,
   allocations, retained resources or callback file I/O. Only the existing
   guarded selected Present sets this render-owner bracket. Other owners' real
   fence observations carry phase/present0 and never read render-owned fields. */
static struct {unsigned ready;CopyPresentObservation row;} cpo_rows[CPO_CAPACITY];
static unsigned cpo_claimed;
static unsigned long long cpo_fresh_ms;
static unsigned cpo_render_thread;
static struct {unsigned long long ordinal,device,context;unsigned phase;} cpo_render;
void copy_present_observe_arm(void) {
    DWORD saved=GetLastError();unsigned long long empty=0,now=CPO_TICK();
    if(now)__atomic_compare_exchange_n(&cpo_fresh_ms,&empty,now,0,__ATOMIC_RELEASE,__ATOMIC_RELAXED);
    SetLastError(saved);
}
static void cpo_record(CopyPresentObservation row) {
    DWORD saved=GetLastError();
    unsigned long long start=__atomic_load_n(&cpo_fresh_ms,__ATOMIC_ACQUIRE),now=CPO_TICK();
    if(!start || now<start || now-start<2000){SetLastError(saved);return;}
    unsigned n=__atomic_load_n(&cpo_claimed,__ATOMIC_RELAXED);
    if(n>=CPO_CAPACITY){SetLastError(saved);return;}
    LARGE_INTEGER stamp={0};
    if(QueryPerformanceCounter(&stamp)) {
        row.qpc=(unsigned long long)stamp.QuadPart;row.thread=GetCurrentThreadId();
        for(unsigned retry=0;retry<4 && n<CPO_CAPACITY;++retry) {
            if(__atomic_compare_exchange_n(&cpo_claimed,&n,n+1,0,__ATOMIC_RELAXED,__ATOMIC_RELAXED)) {
                cpo_rows[n].row=row;__atomic_store_n(&cpo_rows[n].ready,1,__ATOMIC_RELEASE);break;
            }
        }
    }
    SetLastError(saved);
}
void copy_present_observe_enter(void *device,void *context) {
    /* Call ONLY inside the existing selected-chain/thread/busy/device guards. */
    if(!device || !context)return;
    unsigned caller=GetCurrentThreadId(),empty=0;
    __atomic_compare_exchange_n(&cpo_render_thread,&empty,caller,0,__ATOMIC_RELEASE,__ATOMIC_RELAXED);
    if(__atomic_load_n(&cpo_render_thread,__ATOMIC_ACQUIRE)!=caller || cpo_render.phase)return;
    ++cpo_render.ordinal;cpo_render.device=(unsigned long long)device;
    cpo_render.context=(unsigned long long)context;cpo_render.phase=1;
    CopyPresentObservation r={0};r.present=cpo_render.ordinal;r.device=cpo_render.device;
    r.context=cpo_render.context;r.edge=CPO_PRE_ENTER;r.phase=1;cpo_record(r);
}
void copy_present_observe_mark(unsigned edge,unsigned long long value) {
    if(__atomic_load_n(&cpo_render_thread,__ATOMIC_ACQUIRE)!=GetCurrentThreadId() || !cpo_render.phase)return;
    if(edge==CPO_ORIGINAL_ENTER && cpo_render.phase==1)cpo_render.phase=2;
    else if(edge==CPO_ORIGINAL_RETURN && cpo_render.phase==2)cpo_render.phase=3;
    else if(edge!=CPO_POST_RETURN || cpo_render.phase!=3)return;
    CopyPresentObservation r={0};r.present=cpo_render.ordinal;r.device=cpo_render.device;
    r.context=cpo_render.context;r.edge=edge;r.phase=cpo_render.phase;r.value=value;cpo_record(r);
    if(edge==CPO_POST_RETURN){cpo_render.phase=0;cpo_render.device=cpo_render.context=0;}
}
void copy_present_observe_copy(EyeGpuTicket ticket,void *device,void *context,
                              unsigned edge,unsigned long long value) {
    if(!ticket.serial || !ticket.generation || !ticket.epoch || !device || !context ||
       edge<CPO_COPY_BEGIN || edge>CPO_FENCE_POLL)return;
    CopyPresentObservation r={0};r.ticket=ticket;r.device=(unsigned long long)device;
    r.context=(unsigned long long)context;r.edge=edge;r.value=value;
    if(__atomic_load_n(&cpo_render_thread,__ATOMIC_ACQUIRE)==GetCurrentThreadId() &&
       cpo_render.phase && r.device==cpo_render.device && r.context==cpo_render.context) {
        r.present=cpo_render.ordinal;r.phase=cpo_render.phase;
    }
    cpo_record(r);
}
int copy_present_observe_take(unsigned *cursor,CopyPresentObservation *out) {
    if(!cursor || !out || *cursor>=CPO_CAPACITY ||
       !__atomic_load_n(&cpo_rows[*cursor].ready,__ATOMIC_ACQUIRE))return 0;
    *out=cpo_rows[(*cursor)++].row;return 1;
}
#endif
#endif
