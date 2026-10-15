#ifndef GF2_PRESENTATION_BANK_POOL_H
#define GF2_PRESENTATION_BANK_POOL_H
#include "eye_flight_owner.h"
/* Dormant XR-owner-only bank capacity bookkeeping. NO handles, GPU/XR calls,
   thread crossing, resource destruction or manufactured completion. Adapter
   must validate actual acquired/released/drain/End facts before supplying the
   exact owner snapshot. Three physical PAIRS are required by the future adapter:
   two writers plus one protected completed presentation. Numeric flight capacity
   alone is not a physical bank. Not included by the production integration. */
enum { PBP_FREE, PBP_WRITING, PBP_READY, PBP_HELD, PBP_DRAINING };
#ifdef GF2VR_INDEPENDENT_READY_CAPACITY
/* HELD + protected READY + two independent active writing pairs. Numeric
   producer capacity stays two; READY never authorizes reuse or completion. */
#ifdef GF2VR_CAPACITY_EXPERIMENT
enum { PBP_BANKS=EFO_CAPACITY+2, PBP_MAX_WRITERS=EFO_CAPACITY };
#else
enum { PBP_BANKS=4, PBP_MAX_WRITERS=2 };
#endif
#else
enum { PBP_BANKS=3, PBP_MAX_WRITERS=2 };
#endif
typedef struct {
    EyeFlightToken token;
    unsigned state;
    unsigned long long prepared_ms;
    unsigned long long ready_serial;
#ifdef GF2VR_HELD_FRESH_CLOCK
    unsigned long long held_ms; /* Written once after real fresh End/finalization. */
#endif
} PresentationBank;
typedef struct {
    PresentationBank banks[PBP_BANKS];
    unsigned long long generation,epoch,last_frame;
    unsigned stopped;
} PresentationBankPool;
static inline int pbp_same(EyeFlightToken a,EyeFlightToken b) {
    return a.frame && a.frame==b.frame && a.generation==b.generation && a.epoch==b.epoch;
}
static inline int pbp_activate(PresentationBankPool *p,unsigned long long g,unsigned long long e) {
    if(!p || !g || !e)return 0;
    for(unsigned i=0;i<PBP_BANKS;++i)if(p->banks[i].state!=PBP_FREE)return 0;
    p->generation=g;p->epoch=e;p->stopped=0;return 1;
}
/* Called only BEFORE the token's original excluded-layer End. Does not acquire
   an image. Serial adoption/immutable pose stay owned by the existing adapter. */
static inline int pbp_reserve(PresentationBankPool *p,EyeFlightToken t,unsigned long long ms,unsigned *index) {
    if(!p || !index || p->stopped || !t.frame || t.frame<=p->last_frame ||
       t.generation!=p->generation || t.epoch!=p->epoch)return 0;
    unsigned occupied=0,free_bank=PBP_BANKS;
    for(unsigned i=0;i<PBP_BANKS;++i) {
        if(p->banks[i].state==PBP_FREE && free_bank==PBP_BANKS)free_bank=i;
#ifdef GF2VR_INDEPENDENT_READY_CAPACITY
        if(p->banks[i].state!=PBP_FREE && p->banks[i].state!=PBP_HELD &&
           p->banks[i].state!=PBP_READY)++occupied;
#else
        if(p->banks[i].state!=PBP_FREE && p->banks[i].state!=PBP_HELD)++occupied;
#endif
    }
    if(occupied>=PBP_MAX_WRITERS || free_bank==PBP_BANKS)return 0;
    p->banks[free_bank]=(PresentationBank){.token=t,.state=PBP_WRITING,.prepared_ms=ms};
    p->last_frame=t.frame;*index=free_bank;return 1;
}
static inline int pbp_snapshot(PresentationBankPool *p,unsigned i,const EyeFlightSnapshot *s) {
    return p && s && i<PBP_BANKS && p->banks[i].state!=PBP_FREE && pbp_same(p->banks[i].token,s->token);
}
/* Both genuine render/copy drains and real Release of BOTH images required.
   A numeric DONE/CPU-return alone is never ready-for-presentation evidence. */
static inline int pbp_complete(PresentationBankPool *p,unsigned i,const EyeFlightSnapshot *s) {
    if(!pbp_snapshot(p,i,s) || p->stopped || p->banks[i].state!=PBP_WRITING ||
       s->token.generation!=p->generation || s->token.epoch!=p->epoch || !s->cpu_ok ||
       !s->ticket.serial || s->ticket.generation!=s->token.generation || s->ticket.epoch!=s->token.epoch ||
       (s->flags&(EFF_RENDER_ISSUED|EFF_CPU_RETURNED|EFF_RENDER_DRAINED|EFF_COPY_ISSUED|EFF_COPY_DRAINED))!=
                 (EFF_RENDER_ISSUED|EFF_CPU_RETURNED|EFF_RENDER_DRAINED|EFF_COPY_ISSUED|EFF_COPY_DRAINED) ||
       s->flags&(EFF_CANCELLED|EFF_FAULT|EFF_RECLAIMING|EFF_PRESENTATION_RETIRED) ||
       s->acquired_mask!=3 || s->waited_mask!=3 || s->released_mask!=3)return 0;
    p->banks[i].ready_serial=s->ticket.serial;p->banks[i].state=PBP_READY;return 1;
}
/* Adapter invokes only AFTER real successful fresh End and local finalization;
   snapshot must reflect disposition. Prior held bank is unpinned, not destroyed.
   The original preparation clock is never renewed by commit/replay. */
static inline int pbp_publish(PresentationBankPool *p,unsigned i,const EyeFlightSnapshot *s,unsigned long long now) {
    if(!pbp_snapshot(p,i,s) || p->stopped || p->banks[i].state!=PBP_READY ||
       s->ticket.serial!=p->banks[i].ready_serial || s->ticket.generation!=s->token.generation ||
       s->ticket.epoch!=s->token.epoch || !s->cpu_ok ||
       (s->flags&(EFF_RENDER_DRAINED|EFF_COPY_DRAINED))!=(EFF_RENDER_DRAINED|EFF_COPY_DRAINED) ||
       s->acquired_mask!=3 || s->waited_mask!=3 || s->released_mask!=3 ||
       !(s->flags&EFF_PRESENTATION_RETIRED) || !(s->flags&EFF_XR_ENDED) || !(s->flags&EFF_DISPLAY_DETACHED) ||
       s->flags&(EFF_CANCELLED|EFF_FAULT) || now<p->banks[i].prepared_ms ||
       now-p->banks[i].prepared_ms>=250)return 0;
    /* Never regress presentation to an older completed flight. */
    for(unsigned j=0;j<PBP_BANKS;++j)
        if(p->banks[j].state==PBP_HELD && p->banks[j].token.frame>=s->token.frame)return 0;
    for(unsigned j=0;j<PBP_BANKS;++j)if(p->banks[j].state==PBP_HELD)p->banks[j]=(PresentationBank){0};
    p->banks[i].state=PBP_HELD;
#ifdef GF2VR_HELD_FRESH_CLOCK
    p->banks[i].held_ms=now;
#endif
    return 1;
}
#ifdef GF2VR_HELD_FRESH_CLOCK
/* Separate presentation age. Fresh production still uses prepared_ms; replay
   cannot publish again because only READY may become HELD. Never renew here. */
enum { PBP_HELD_MAX_MS=750 }; /* Explicit user2026-10-09; fresh prepared-age remains250. */
static inline int pbp_held_age(const PresentationBank *b,unsigned long long now) {
    return b && b->state==PBP_HELD && b->held_ms && now>=b->held_ms && now-b->held_ms<PBP_HELD_MAX_MS;
}
#endif
static inline int pbp_held(PresentationBankPool *p,unsigned long long now,unsigned *index) {
    if(!p || !index || p->stopped)return 0;
    for(unsigned i=0;i<PBP_BANKS;++i)if(p->banks[i].state==PBP_HELD) {
#ifdef GF2VR_HELD_FRESH_CLOCK
        if(!pbp_held_age(&p->banks[i],now))return 0;
#else
        if(now<p->banks[i].prepared_ms || now-p->banks[i].prepared_ms>=250)return 0;
#endif
        *index=i;return 1;
    }
    return 0;
}
/* Off/focus/source/reference loss stops publication immediately, but writers
   remain occupied until actual disposition/drains authorize cleanup. */
static inline void pbp_stop(PresentationBankPool *p) {
    if(!p)return;
    p->stopped=1;
    for(unsigned i=0;i<PBP_BANKS;++i) {
        if(p->banks[i].state==PBP_HELD)p->banks[i]=(PresentationBank){0};
        else if(p->banks[i].state!=PBP_FREE)p->banks[i].state=PBP_DRAINING;
    }
}
/* Caller supplies the terminal exact snapshot AFTER real disposition. A failed
   GPU owner remains quarantined. No release of held bank by stale main ack. */
static inline int pbp_discard(PresentationBankPool *p,unsigned i,const EyeFlightSnapshot *s) {
    if(!pbp_snapshot(p,i,s) || p->banks[i].state==PBP_HELD ||
       !(s->flags&EFF_XR_ENDED) || !(s->flags&EFF_PRESENTATION_RETIRED) ||
       s->flags&EFF_FAULT ||
       (!(s->flags&EFF_CPU_RETURNED) &&
        ((s->flags&(EFF_RESERVED|EFF_RENDER_ISSUED)) ||
         (s->flags&(EFF_PRODUCER_RETAINED|EFF_CANCELLED|EFF_DISPLAY_DETACHED))!=
                   (EFF_PRODUCER_RETAINED|EFF_CANCELLED|EFF_DISPLAY_DETACHED))) ||
       ((s->flags&EFF_RENDER_ISSUED) && !(s->flags&EFF_RENDER_DRAINED)) ||
       ((s->flags&EFF_COPY_ISSUED) && !(s->flags&EFF_COPY_DRAINED)) ||
       ((s->acquired_mask|s->waited_mask|s->released_mask)&~3u) ||
       s->acquired_mask!=s->released_mask || s->waited_mask!=s->acquired_mask)return 0;
    p->banks[i]=(PresentationBank){0};return 1;
}
#endif
