#ifndef GF2_CAMERA_FOLLOW_SOURCE_H
#define GF2_CAMERA_FOLLOW_SOURCE_H
/* Main-thread presentation owner. No XR reference or original source writes.
   The live source is retained separately so LEFT return works while frozen. */
typedef struct {unsigned disabled;PoseSourceAnchor current;} CameraFollowState;
static CameraFollowState stereo_camera_follow_state;
static int camera_follow_source(CameraFollowState *mode,SourceAnchorState *anchor,const PoseMath *math,
    unsigned long long generation,const ManagedEyePose *source,float units,int returning){
    if(!mode||!anchor)return 0;
    SourceAnchorState live=*anchor;
    if(!source_anchor_follow(&live,math,generation,source,units))return 0;
    /* Capture seeds yaw only. Always initialize approved pitch for a new
       generation, including when the process-local selection is OFF. */
    if(!mode->disabled||returning||mode->current.source_generation!=generation)*anchor=live;
    mode->current=live.anchor;return 1;
}
#endif
