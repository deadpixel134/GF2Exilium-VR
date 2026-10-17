#ifndef GF2_WORLD_DRAG_H
#define GF2_WORLD_DRAG_H
/* Pure distance policy. No XR handles, view/head pose, dt, speed or UI writes. */
enum {WD_LEFT_GRIP,WD_LEFT_TRIGGER,WD_RIGHT_GRIP,WD_RIGHT_TRIGGER,WD_NONE};
typedef struct {float x,y,z;} WorldDragVector;
typedef struct {
    unsigned enabled,leftGripActivation,leftTriggerActivation,rightGripActivation,rightTriggerActivation;
    unsigned trackLeftHand,trackRightHand;
} WorldDragSettings;
typedef struct {
    float values[4];unsigned active[4],consumed[4],tracked[2];WorldDragVector position[2];
    unsigned focused,immersive;
    unsigned long long time_ms,reference_epoch,source_generation;
    /* Stable XR predictedDisplayTime identifies a sample independently of
       the coarser monotonic millisecond clock used for the stale limit. */
    unsigned long long sample_id;
} WorldDragFrame;
typedef struct {
    unsigned owner,hand,suppressed[4],initialized;
    float previous_values[4];WorldDragVector previous_position;
    unsigned long long last_ms,epoch,generation,sample_id;
    WorldDragSettings settings;
} WorldDrag;
static WorldDragSettings world_drag_defaults(void) {return (WorldDragSettings){0,0,0,1,0,0,1};}
static int world_drag_settings_valid(const WorldDragSettings *s) {
    if(!s||s->enabled>1||s->leftGripActivation>1||s->leftTriggerActivation>1||
       s->rightGripActivation>1||s->rightTriggerActivation>1||s->trackLeftHand>1||s->trackRightHand>1)return 0;
    return !s->enabled||((s->leftGripActivation||s->leftTriggerActivation||s->rightGripActivation||s->rightTriggerActivation)&&
                        (s->trackLeftHand||s->trackRightHand));
}
static int world_drag_finite(float f) {return f>=-3.402823466e38f&&f<=3.402823466e38f;}
static int world_drag_position_valid(WorldDragVector p) {return world_drag_finite(p.x)&&world_drag_finite(p.y)&&world_drag_finite(p.z);}
static int world_drag_settings_equal(WorldDragSettings a,WorldDragSettings b) {
    return a.enabled==b.enabled&&a.leftGripActivation==b.leftGripActivation&&a.leftTriggerActivation==b.leftTriggerActivation&&
        a.rightGripActivation==b.rightGripActivation&&a.rightTriggerActivation==b.rightTriggerActivation&&
        a.trackLeftHand==b.trackLeftHand&&a.trackRightHand==b.trackRightHand;
}
static void world_drag_cancel(WorldDrag *s) {s->owner=WD_NONE;s->hand=2;s->previous_position=(WorldDragVector){0};}
/* External reset preserves suppression; missing/inactive samples are never
   interpreted as physical releases. All four need observed release to rearm. */
static void world_drag_reset(WorldDrag *s) {
    world_drag_cancel(s);for(unsigned i=0;i<4;++i)s->suppressed[i]=1;
    s->initialized=0;s->last_ms=s->epoch=s->generation=s->sample_id=0;
}
static int world_drag_sample_advances(const WorldDrag *s,const WorldDragFrame *f) {
    if(!f->time_ms||f->time_ms<s->last_ms)return 0;
    /* Adjacent 120Hz XR frames can share one GetTickCount64 tick. A strictly
       newer XR prediction is a new sample, not stale input. Duplicate/backward
       predictions and transitions to/from unidentified input still fail open. */
    if(f->sample_id||s->sample_id)return f->sample_id&&s->sample_id&&f->sample_id>s->sample_id;
    return f->time_ms>s->last_ms;
}
static WorldDragVector world_drag_update(WorldDrag *s,const WorldDragSettings *settings,const WorldDragFrame *f) {
    WorldDragVector zero={0};if(!s||!f||!settings)return zero;
    float values[4];
    int boundary=!s->initialized||s->epoch!=f->reference_epoch||s->generation!=f->source_generation||
        !world_drag_settings_equal(s->settings,*settings)||!world_drag_sample_advances(s,f)||
        (s->last_ms&&f->time_ms-s->last_ms>250);
    int eligible=world_drag_settings_valid(settings)&&settings->enabled&&f->focused&&f->immersive&&
        f->reference_epoch&&f->source_generation&&f->time_ms;
    if(boundary||!eligible)world_drag_cancel(s);
    for(unsigned i=0;i<4;++i) {
        values[i]=world_drag_finite(f->values[i])?f->values[i]:0;
        if(boundary||!eligible||!f->active[i]||f->consumed[i])s->suppressed[i]=1;
        if(f->active[i]&&values[i]<=0.25f)s->suppressed[i]=0;
    }
    s->initialized=1;s->settings=*settings;s->epoch=f->reference_epoch;s->generation=f->source_generation;
    s->last_ms=f->time_ms;s->sample_id=f->sample_id;
    WorldDragVector delta={0};
    if(eligible&&!boundary) {
        if(s->owner<WD_NONE) {
            unsigned i=s->owner,h=s->hand;
            if(!f->active[i]||f->consumed[i]||values[i]<=0.25f||h>1||!f->tracked[h]||!world_drag_position_valid(f->position[h])) {
                if(!f->active[i]||values[i]>0.25f)s->suppressed[i]=1;
                world_drag_cancel(s);
            } else {
                WorldDragVector p=f->position[h];
                delta=(WorldDragVector){p.x-s->previous_position.x,p.y-s->previous_position.y,p.z-s->previous_position.z};
                s->previous_position=p;
                if(!world_drag_position_valid(delta)) {s->suppressed[i]=1;world_drag_cancel(s);delta=zero;}
            }
        } else {
            unsigned activation[4]={settings->leftGripActivation,settings->leftTriggerActivation,
                                    settings->rightGripActivation,settings->rightTriggerActivation};
            for(unsigned i=0;i<4;++i) {
                if(!activation[i]||!f->active[i]||f->consumed[i]||s->suppressed[i]||values[i]<0.72f||s->previous_values[i]>=0.72f)continue;
                unsigned same=i/2,h=(same==0&&settings->trackLeftHand)||(same==1&&settings->trackRightHand)?same:
                    settings->trackLeftHand?0:settings->trackRightHand?1:2;
                if(h>1||!f->tracked[h]||!world_drag_position_valid(f->position[h])) {s->suppressed[i]=1;continue;}
                s->owner=i;s->hand=h;s->previous_position=f->position[h];break; /* zero start */
            }
        }
    }
    for(unsigned i=0;i<4;++i)s->previous_values[i]=values[i];
    return delta;
}
/* Unity conversion + camera-opposite sign. q is navigation-only, normalized,
   supplied by navigation owner, never the live physical HMD quaternion. */
static int world_drag_camera_delta(WorldDragVector hand,const float q[4],float multiplier,WorldDragVector *out) {
    if(!out)return 0;*out=(WorldDragVector){0};
    if(!q||!world_drag_position_valid(hand)||!world_drag_finite(multiplier)||multiplier<=0)return 0;
    float n=0;for(unsigned i=0;i<4;++i) {if(!world_drag_finite(q[i]))return 0;n+=q[i]*q[i];}
    if(!(n>=0.999f&&n<=1.001f))return 0;
    WorldDragVector v={-hand.x,-hand.y,hand.z};
    WorldDragVector t={2*(q[1]*v.z-q[2]*v.y),2*(q[2]*v.x-q[0]*v.z),2*(q[0]*v.y-q[1]*v.x)};
    WorldDragVector r={(v.x+q[3]*t.x+q[1]*t.z-q[2]*t.y)*multiplier,
        (v.y+q[3]*t.y+q[2]*t.x-q[0]*t.z)*multiplier,(v.z+q[3]*t.z+q[0]*t.y-q[1]*t.x)*multiplier};
    if(!world_drag_position_valid(r))return 0;*out=r;return 1;
}
#endif
