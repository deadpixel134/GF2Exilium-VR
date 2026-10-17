#ifndef GF2_NAVIGATION_HANDOFF_H
#define GF2_NAVIGATION_HANDOFF_H
#include "navigation_policy.h"
/* POD input history, not an input replay queue. Main pins eye time before XR
   can publish again. Select one compatible sample instead of continually
   replacing the deferred input with a newer future sample. Cancellation is
   always current, and the existing policy owns age/delta/once-sample guards. */
#define NAVIGATION_HISTORY_CAPACITY 64u
typedef struct {
    unsigned lock,eligible,valid[2],next,count;
    unsigned long long sequence,cancel,hand_cancel[2];
    NavigationSample latest,history[NAVIGATION_HISTORY_CAPACITY];
} NavigationHandoff;
static void navigation_lock(NavigationHandoff *h) {
    for(;;){unsigned zero=0;if(__atomic_compare_exchange_n(&h->lock,&zero,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return;}
}
static void navigation_unlock(NavigationHandoff *h){__atomic_store_n(&h->lock,0,__ATOMIC_RELEASE);}
static void navigation_publish(NavigationHandoff *h,NavigationSample sample) {
    unsigned valid[2];for(unsigned i=0;i<2;++i)valid[i]=sample.eligible&&sample.active[i]&&sample.tracked[i]&&navigation_axis_valid(sample.x[i],sample.y[i]);
    navigation_lock(h);
    if(h->eligible!=sample.eligible)++h->cancel;
    h->eligible=sample.eligible;
    for(unsigned i=0;i<2;++i){if(h->valid[i]&&!valid[i])++h->hand_cancel[i];h->valid[i]=valid[i];sample.hand_cancel[i]=h->hand_cancel[i];}
    sample.sequence=++h->sequence;sample.cancel=h->cancel;h->latest=sample;
    h->history[h->next]=sample;h->next=(h->next+1u)%NAVIGATION_HISTORY_CAPACITY;
    if(h->count<NAVIGATION_HISTORY_CAPACITY)++h->count;
    navigation_unlock(h);
}
static NavigationSample navigation_read(NavigationHandoff *h) {
    navigation_lock(h);NavigationSample sample=h->latest;navigation_unlock(h);return sample;
}
static NavigationSample navigation_read_for_pose(NavigationHandoff *h,
    unsigned long long epoch,unsigned long long generation,unsigned long long now,long long pose_time,NavigationSettings cfg) {
    navigation_lock(h);
    NavigationSample latest=h->latest,sample=latest;
    if(latest.eligible&&latest.epoch==epoch&&latest.generation==generation&&latest.tick_ms&&
       now>=latest.tick_ms&&now-latest.tick_ms<=250&&pose_time>0&&latest.display_time>pose_time) {
        for(unsigned n=0;n<h->count;++n) {
            unsigned index=(h->next+NAVIGATION_HISTORY_CAPACITY-1u-n)%NAVIGATION_HISTORY_CAPACITY;
            const NavigationSample *p=&h->history[index];
            if(p->eligible&&p->epoch==epoch&&p->generation==generation&&p->cancel==latest.cancel&&
               p->display_time>0&&p->display_time<=pose_time){sample=*p;break;}
        }
    }
    navigation_unlock(h);
    /* Loss remains immediate even if the pinned eye time precedes it. Never
       revive a hand across an invalid interval. Neutral release can suppress
       an older held command; it cannot create future motion or a snap. */
    for(unsigned i=0;i<2;++i) {
        if(sample.hand_cancel[i]!=latest.hand_cancel[i]||!latest.active[i]||!latest.tracked[i]||
           !navigation_axis_valid(latest.x[i],latest.y[i])) {
            sample.active[i]=sample.tracked[i]=0;sample.x[i]=sample.y[i]=0;
        } else {
            double x=latest.x[i],y=latest.y[i];
            int neutral=i==cfg.locomotion_hand?x*x+y*y<=cfg.deadzone*cfg.deadzone:
                navigation_abs(x)<=.25&&navigation_abs(y)<=.25;
            if(neutral){sample.x[i]=latest.x[i];sample.y[i]=latest.y[i];}
        }
        sample.hand_cancel[i]=latest.hand_cancel[i];
    }
    return sample;
}
#endif
