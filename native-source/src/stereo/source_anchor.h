#ifndef GF2_SOURCE_ANCHOR_H
#define GF2_SOURCE_ANCHOR_H
#include "pose_composition.h"
/* Presentation state only: no XR/head reference is stored or modified. */
typedef struct {PoseSourceAnchor anchor;unsigned long long last_generation;} SourceAnchorState;
static void source_anchor_retire(SourceAnchorState *state) {
    if(state) state->anchor=(PoseSourceAnchor){0};
}
static int source_anchor_capture(SourceAnchorState *state,const PoseMath *math,
                                  unsigned long long generation,const ManagedEyePose *source,
                                  float units_per_meter) {
    if(!state || !math || !math->sqrt || !math->atan2 || !source || !generation ||
       state->anchor.source_generation || generation<=state->last_generation) return 0;
    float checked;
    if(!eye_optics_float(units_per_meter,&checked) || !(units_per_meter>0)) return 0;
    double q[4],angles[3];
    if(!pose_read(math,source,q) || !pose_angles(math,q,angles)) return 0;
    PoseSourceAnchor anchor={0};anchor.source_generation=generation;anchor.units_per_meter=units_per_meter;
    if(!eye_optics_float(angles[0],&anchor.yaw)) return 0;
    for(unsigned i=0;i<3;++i) anchor.position[i]=source->position[i];
    state->anchor=anchor;state->last_generation=generation;return 1;
}
static int source_anchor_current(const SourceAnchorState *state,unsigned long long generation,float units_per_meter) {
    return state && generation && state->anchor.source_generation==generation &&
        state->anchor.units_per_meter==units_per_meter;
}
/* Live presentation update for an already validated source generation. All
   validation precedes commit. No XR reference, accumulation or source mutation. */
static inline int source_anchor_follow(SourceAnchorState *state,const PoseMath *math,
                                        unsigned long long generation,const ManagedEyePose *source,
                                        float units_per_meter) {
    if(!source_anchor_current(state,generation,units_per_meter) || !math ||
       !math->sqrt || !math->atan2 || !source) return 0;
    double q[4],angles[3];
    if(!pose_read(math,source,q) || !pose_angles(math,q,angles)) return 0;
    PoseSourceAnchor next=state->anchor;
    if(!eye_optics_float(angles[0],&next.yaw) || !eye_optics_float(angles[1],&next.pitch) ||
       !(next.pitch>-1.5707953267948966 && next.pitch<1.5707953267948966)) return 0;
    for(unsigned i=0;i<3;++i) next.position[i]=source->position[i];
    state->anchor=next;return 1;
}
#endif
