#ifndef GF2_CONTROLLER_GAME_MODE_H
#define GF2_CONTROLLER_GAME_MODE_H
/* Portable hold policy. The destination converts the selected stick to WASD.
   This policy owns mode, physical face keys and failed-release rearming. */
enum {CGM_F=1,CGM_ESCAPE=2,CGM_W=4,CGM_A=8,CGM_S=16,CGM_D=32};
typedef struct {
    unsigned long long sequence,tick_ms,epoch;
    unsigned focused,panel_hand,move_hand,tracked[2],trigger_active[2],face_active[2],axis_active[2];
    float trigger[2],x[2],y[2];
    unsigned primary[2],secondary[2]; /* physical X/A andY/B, never face-swap logical actions */
} ControllerGameFrame;
typedef struct {
    void *context;
    int (*axis)(void *,float,float);
    int (*key)(void *,unsigned,int);
} ControllerGameSink;
typedef struct {
    unsigned long long sequence,tick_ms,epoch;
    unsigned initialized,held,blocked,rearm,panel_hand,move_hand,owned_keys,armed_faces,axis_owned;
    float axis_x,axis_y;
} ControllerGameMode;
static int controller_game_finite(float v) {return v==v && v<=3.402823466e38f && v>=-3.402823466e38f;}
static int controller_game_axis_valid(float v) {return controller_game_finite(v)&&v>=-1.0f&&v<=1.0f;}
/* Retry failed neutral/release before allowing any new destination output.
   A failed axis write may have side effects, so axis ownership is retained. */
static int controller_game_cleanup(ControllerGameMode *s,const ControllerGameSink *sink) {
    if(s->axis_owned && sink && sink->axis && sink->axis(sink->context,0,0)){
        s->axis_owned=0;s->axis_x=s->axis_y=0;
    }
    for(unsigned key=CGM_F;key<=CGM_ESCAPE;key<<=1)if(s->owned_keys&key){
        if(sink && sink->key && sink->key(sink->context,key,0))s->owned_keys&=~key;
    }
    return !s->axis_owned&&!s->owned_keys;
}
static int controller_game_cancel(ControllerGameMode *s,const ControllerGameSink *sink) {
    if(!s)return 0;
    unsigned had_mode=s->held||s->axis_owned||s->owned_keys||s->blocked||s->rearm;
    s->held=0;s->rearm=had_mode;s->armed_faces=0;
    s->blocked=!controller_game_cleanup(s,sink);return !s->blocked;
}
static int controller_game_step(ControllerGameMode *s,const ControllerGameFrame *f,
                               unsigned long long now,const ControllerGameSink *sink) {
    if(!s)return 0;
    if(!f || !sink || !sink->axis || !sink->key || !f->focused || !f->epoch || !f->sequence ||
       !f->tick_ms || now<f->tick_ms || now-f->tick_ms>250 || f->panel_hand>1 || f->move_hand>1 ||
       !f->tracked[f->panel_hand] || !f->trigger_active[f->panel_hand] ||
       !controller_game_finite(f->trigger[f->panel_hand]) || f->trigger[f->panel_hand]<0 ||
       f->trigger[f->panel_hand]>1){controller_game_cancel(s,sink);return 0;}
    if(s->initialized && (f->epoch!=s->epoch || f->panel_hand!=s->panel_hand || f->move_hand!=s->move_hand ||
       f->sequence<=s->sequence || f->tick_ms<s->tick_ms || f->tick_ms-s->tick_ms>250)){
        controller_game_cancel(s,sink);
        s->sequence=f->sequence;s->tick_ms=f->tick_ms;s->epoch=f->epoch;
        s->panel_hand=f->panel_hand;s->move_hand=f->move_hand;return 0;
    }
    s->initialized=1;s->sequence=f->sequence;s->tick_ms=f->tick_ms;s->epoch=f->epoch;
    s->panel_hand=f->panel_hand;s->move_hand=f->move_hand;
    if(s->blocked){s->blocked=!controller_game_cleanup(s,sink);if(s->blocked)return 0;}
    float trigger=f->trigger[f->panel_hand];
    if(s->rearm){if(trigger<=0.25f)s->rearm=0;return 1;}
    unsigned held=s->held?trigger>0.25f:trigger>=0.72f;
    unsigned faces_down=0,faces_available=0;
    for(unsigned h=0;h<2;++h)if(f->tracked[h]&&f->face_active[h]){
        faces_available|=3u<<(h*2);
        if(f->primary[h])faces_down|=1u<<(h*2);
        if(f->secondary[h])faces_down|=2u<<(h*2);
    }
    if(!held){
        if(s->held&&!controller_game_cleanup(s,sink)){s->held=0;s->blocked=s->rearm=1;return 0;}
        s->held=0;s->armed_faces=0;return 1;
    }
    if(!s->held)s->armed_faces=faces_available&~faces_down;
    else {s->armed_faces&=faces_available;s->armed_faces|=faces_available&~faces_down;}
    unsigned eligible_faces=faces_down&s->armed_faces;
    unsigned down=((eligible_faces&5)?CGM_F:0)|((eligible_faces&10)?CGM_ESCAPE:0);
    s->held=1;
    if(!f->tracked[f->move_hand] || !f->axis_active[f->move_hand] ||
       !controller_game_axis_valid(f->x[f->move_hand]) || !controller_game_axis_valid(f->y[f->move_hand])){
        controller_game_cancel(s,sink);return 0;
    }
    /* Only the locomotion hand reaches the WASD destination. View turn and
       the physical HMD reference retain their existing owners. */
    s->axis_owned=1;
    if(!sink->axis(sink->context,f->x[f->move_hand],f->y[f->move_hand])){
        controller_game_cancel(s,sink);return 0;
    }
    s->axis_x=f->x[f->move_hand];s->axis_y=f->y[f->move_hand];
    for(unsigned key=CGM_F;key<=CGM_ESCAPE;key<<=1){
        if(!(down&key)){
            if((s->owned_keys&key)&&!sink->key(sink->context,key,0)){controller_game_cancel(s,sink);return 0;}
            s->owned_keys&=~key;
        }else if(!(s->owned_keys&key)){
            if(!sink->key(sink->context,key,1)){controller_game_cancel(s,sink);return 0;}
            s->owned_keys|=key;
        }
    }
    return 1;
}
/* The locomotion consumer must also honor cleanup/neutral rearm, so a failed
   game neutral cannot race a fresh VR-movement packet. View turn is separate. */
static int controller_game_move_consumed(const ControllerGameMode *s) {
    return s && (s->held||s->axis_owned||s->owned_keys||s->blocked||s->rearm);
}
#endif
