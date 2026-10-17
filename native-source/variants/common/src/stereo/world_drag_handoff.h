#ifndef GF2_WORLD_DRAG_HANDOFF_H
#define GF2_WORLD_DRAG_HANDOFF_H
#include "world_drag.h"
/* Very short synchronized distance accumulator. Lock protects POD only; never
   XR/managed/OS calls or math-library calls. Producer and consumer each own
   their state outside it. Epoch/generation/age checks discard stale distance. */
typedef struct {
    unsigned lock;
    unsigned long long epoch,generation,tick,first_tick;
    WorldDragVector pending;
} WorldDragHandoff;
static void world_drag_lock(WorldDragHandoff *h) {
    for(;;) {unsigned zero=0;if(__atomic_compare_exchange_n(&h->lock,&zero,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED))return;}
}
static void world_drag_unlock(WorldDragHandoff *h) {__atomic_store_n(&h->lock,0,__ATOMIC_RELEASE);}
static void world_drag_handoff_clear(WorldDragHandoff *h) {
    world_drag_lock(h);h->pending=(WorldDragVector){0};h->epoch=h->generation=h->tick=h->first_tick=0;world_drag_unlock(h);
}
static int world_drag_handoff_add(WorldDragHandoff *h,WorldDragVector delta,unsigned long long epoch,unsigned long long generation,unsigned long long now) {
    if(!epoch||!generation||!now||!world_drag_position_valid(delta))return 0;
    world_drag_lock(h);
    if(h->epoch!=epoch||h->generation!=generation||now<h->tick||now-h->tick>250||
       (h->first_tick&&(now<h->first_tick||now-h->first_tick>250))) {h->pending=(WorldDragVector){0};h->first_tick=0;}
    if(!h->first_tick&&(delta.x||delta.y||delta.z))h->first_tick=now;
    WorldDragVector p={h->pending.x+delta.x,h->pending.y+delta.y,h->pending.z+delta.z};
    int valid=world_drag_position_valid(p);h->pending=valid?p:(WorldDragVector){0};
    h->epoch=epoch;h->generation=generation;h->tick=now;world_drag_unlock(h);return valid;
}
static WorldDragVector world_drag_handoff_take(WorldDragHandoff *h,unsigned long long epoch,unsigned long long generation,unsigned long long now) {
    world_drag_lock(h);WorldDragVector result={0};
    if(epoch&&generation&&h->epoch==epoch&&h->generation==generation&&now>=h->tick&&now-h->tick<=250&&
       (!h->first_tick||(now>=h->first_tick&&now-h->first_tick<=250)))result=h->pending;
    h->pending=(WorldDragVector){0};h->first_tick=0;world_drag_unlock(h);return result;
}
#endif
