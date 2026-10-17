#ifndef GF2_PRODUCER_PREPARE_LIFETIME_H
#define GF2_PRODUCER_PREPARE_LIFETIME_H
#include <stdint.h>
/* Adapter preparation belongs to a quiescent owner boundary, never to a
   borrowed/reserved frame. This core owns no engine, XR or GPU resource. */
typedef struct {
    uint64_t owner;
    unsigned attempted,preparing,ready,active,stopped,cleanup_attempted,cleanup_ok;
} ProducerPrepareLifetime;
typedef struct {void *context;int (*prepare)(void *);int (*detach)(void *);} ProducerPrepareOps;
static int pps_owner(const ProducerPrepareLifetime *s,uint64_t thread) {
    return s && thread && __atomic_load_n(&s->owner,__ATOMIC_ACQUIRE)==thread;
}
static int pps_init(ProducerPrepareLifetime *s,uint64_t thread) {
    if(!s || !thread || __atomic_load_n(&s->owner,__ATOMIC_ACQUIRE))return 0;
    __atomic_store_n(&s->owner,thread,__ATOMIC_RELEASE);return 1;
}
static int pps_cleanup(ProducerPrepareLifetime *s,const ProducerPrepareOps *ops) {
    if(s->active || s->preparing)return 0;
    if(!s->cleanup_attempted) {
        s->cleanup_attempted=1;
        s->cleanup_ok=ops && ops->detach && ops->detach(ops->context)==1;
    }
    return s->cleanup_ok;
}
static int pps_stop(ProducerPrepareLifetime *s,uint64_t thread,const ProducerPrepareOps *ops) {
    if(!pps_owner(s,thread))return 0;
    s->stopped=1;s->ready=0;
    return pps_cleanup(s,ops);
}
static int pps_prepare(ProducerPrepareLifetime *s,uint64_t thread,int quiescent,const ProducerPrepareOps *ops) {
    if(!pps_owner(s,thread))return 0;
    if(s->ready)return 1;
    if(!quiescent || s->attempted || s->stopped || !ops || !ops->prepare)return 0;
    s->attempted=s->preparing=1;
    int okay=ops->prepare(ops->context)==1;
    s->preparing=0;
    if(okay && !s->stopped){s->ready=1;return 1;}
    (void)pps_stop(s,thread,ops);return 0;
}
static int pps_activate(ProducerPrepareLifetime *s,uint64_t thread) {
    if(!pps_owner(s,thread) || !s->ready || s->stopped || s->active)return 0;
    s->active=1;return 1;
}
static int pps_finish(ProducerPrepareLifetime *s,uint64_t thread,int stop,const ProducerPrepareOps *ops) {
    if(!pps_owner(s,thread) || !s->active)return 0;
    s->active=0;
    if(stop || s->stopped)return pps_stop(s,thread,ops);
    return 1;
}
#endif
