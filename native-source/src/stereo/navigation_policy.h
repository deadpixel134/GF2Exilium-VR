#ifndef GF2_NAVIGATION_POLICY_H
#define GF2_NAVIGATION_POLICY_H
/* Portable main-owner policy. Distances are meters, angles radians. No XR,
   Unity, pointer/UI injection, source mutation or physical-origin ownership. */
typedef struct {
    unsigned locomotion_enabled,locomotion_hand,turn_enabled,smooth;
    double movement_speed,locomotion_scale,snap_degrees,smooth_degrees,deadzone;
} NavigationSettings;
typedef struct {
    unsigned long long sequence,cancel,hand_cancel[2],epoch,generation,tick_ms;
    long long display_time;
    unsigned eligible,active[2],tracked[2];
    float x[2],y[2];
#ifdef GF2VR_CAMERA_RETURN
    unsigned camera_return_active,camera_return_down;
#endif
#ifdef GF2VR_CAMERA_FOLLOW_TOGGLE
    unsigned camera_follow_active,camera_follow_down;
#endif
#ifdef GF2VR_NAVIGATION_BLOCK_OBSERVE
    unsigned observed_eligible,observed_inhibited,observed_drag_owner;
#endif
} NavigationSample;
typedef struct {
    unsigned long long epoch,generation,sequence,cancel,hand_cancel[2];
    long long last_time;
    unsigned move_armed,turn_armed,settings_valid;
    NavigationSettings settings;
    double yaw,pitch,offset[3];
} NavigationState;
typedef struct {double (*sin)(double),(*cos)(double),(*sqrt)(double),(*atan2)(double,double);} NavigationMath;
typedef struct {double angles[2],dt,effective_speed;float offset[3];unsigned moved,turned,rejected;} NavigationOutput;
static NavigationSettings navigation_defaults(void) {
    return (NavigationSettings){1,0,1,0,1.0,1.0,30.0,60.0,.2}; /* LEFT move, RIGHT turn */
}
static int navigation_settings_valid(NavigationSettings s) {
    return s.locomotion_enabled<=1 && s.locomotion_hand<2 && s.turn_enabled<=1 && s.smooth<=1 &&
        __builtin_isfinite(s.movement_speed) && s.movement_speed>=0 &&
        __builtin_isfinite(s.locomotion_scale) && s.locomotion_scale>=0 &&
        __builtin_isfinite(s.movement_speed*s.locomotion_scale) &&
        __builtin_isfinite(s.snap_degrees) && s.snap_degrees>=0 &&
        __builtin_isfinite(s.smooth_degrees) && s.smooth_degrees>=0 &&
        __builtin_isfinite(s.deadzone) && s.deadzone>=0 && s.deadzone<1;
}
static int navigation_settings_same(NavigationSettings a,NavigationSettings b) {
    return a.locomotion_enabled==b.locomotion_enabled && a.locomotion_hand==b.locomotion_hand &&
        a.turn_enabled==b.turn_enabled && a.smooth==b.smooth && a.movement_speed==b.movement_speed &&
        a.locomotion_scale==b.locomotion_scale && a.snap_degrees==b.snap_degrees &&
        a.smooth_degrees==b.smooth_degrees && a.deadzone==b.deadzone;
}
static int navigation_axis_valid(float x,float y) {
    return __builtin_isfinite(x)&&__builtin_isfinite(y)&&x>=-1&&x<=1&&y>=-1&&y<=1;
}
static double navigation_abs(double x) {return x<0?-x:x;}
static int navigation_output(const NavigationState *s,float units,NavigationOutput *out) {
    if(!__builtin_isfinite(units)||units<=0)return 0;
    out->angles[0]=s->yaw;out->angles[1]=s->pitch;
    for(unsigned i=0;i<3;++i) {
        double value=s->offset[i]*units;float applied=(float)value;
        if(!__builtin_isfinite(value)||!__builtin_isfinite(applied))return 0;
        out->offset[i]=applied;
    }
    return 1;
}
/* One raw sample is consumed once, even when Base/Overlay/repeated root poses
   compose it again. Loss/role-change neutralizes latches, not accumulated user
   navigation; source/reference replacement alone resets presentation state. */
static int navigation_update(NavigationState *s,NavigationSettings cfg,const NavigationMath *m,
    const NavigationSample *sample,unsigned long long epoch,unsigned long long generation,
    unsigned long long now,long long pose_time,double source_yaw,double source_pitch,
    double physical_yaw,double physical_pitch,const double *head_delta,double reference_yaw,
    float units,NavigationOutput *out) {
    if(!s||!m||!m->sin||!m->cos||!m->sqrt||!m->atan2||!out||!epoch||!generation)return 0;
    *out=(NavigationOutput){0};
    if(s->epoch!=epoch||s->generation!=generation) {
        *s=(NavigationState){0};s->epoch=epoch;s->generation=generation;
    }
    if(!navigation_settings_valid(cfg)) {s->move_armed=s->turn_armed=0;out->rejected=1;return navigation_output(s,units,out);}
    int changed=!s->settings_valid||!navigation_settings_same(s->settings,cfg);
    if(changed) {s->settings=cfg;s->settings_valid=1;s->move_armed=s->turn_armed=0;s->last_time=0;}
    if(!sample||!sample->eligible||sample->epoch!=epoch||sample->generation!=generation||
       !sample->tick_ms||now<sample->tick_ms||now-sample->tick_ms>250||sample->display_time<=0||pose_time<=0) {
        s->move_armed=s->turn_armed=0;s->last_time=0;return navigation_output(s,units,out);
    }
    if(sample->cancel!=s->cancel) {s->move_armed=s->turn_armed=0;s->last_time=0;s->cancel=sample->cancel;}
    unsigned move=cfg.locomotion_hand,turn=1u-move;
    if(s->hand_cancel[move]!=sample->hand_cancel[move])s->move_armed=0;
    if(s->hand_cancel[turn]!=sample->hand_cancel[turn])s->turn_armed=0;
    s->hand_cancel[0]=sample->hand_cancel[0];s->hand_cancel[1]=sample->hand_cancel[1];
    /* Main pins the eye pose before reading this independently published XR
       input. A newer valid input waits for its pose; it is not tracking/focus
       loss and must not strand a held stick behind the neutral-rearm latch.
       Process real cancellation above even while the pose is behind. */
    if(pose_time<sample->display_time)return navigation_output(s,units,out);
    if(pose_time-sample->display_time>250000000ll) {
        s->move_armed=s->turn_armed=0;s->last_time=0;return navigation_output(s,units,out);
    }
    if(!sample->sequence||sample->sequence<=s->sequence)return navigation_output(s,units,out);
    s->sequence=sample->sequence;
    long long previous=s->last_time;
    if(previous>0&&sample->display_time<=previous) {
        s->move_armed=s->turn_armed=0;return navigation_output(s,units,out);
    }
    s->last_time=sample->display_time;
    double dt=previous>0 && sample->display_time>previous?(double)(sample->display_time-previous)*1e-9:0;
    int step=dt>0&&dt<=.1; /* long frames discard time; never catch up */
    out->dt=step?dt:0;out->effective_speed=cfg.movement_speed*cfg.locomotion_scale;
    if(previous && !step)s->turn_armed=0;
    int move_valid=sample->active[move]&&sample->tracked[move]&&navigation_axis_valid(sample->x[move],sample->y[move]);
    int turn_valid=sample->active[turn]&&sample->tracked[turn]&&navigation_axis_valid(sample->x[turn],sample->y[turn]);
    double tx=turn_valid?sample->x[turn]:0,ty=turn_valid?sample->y[turn]:0;
    if(!turn_valid||!cfg.turn_enabled)s->turn_armed=0;
    else if(navigation_abs(tx)<=.25&&navigation_abs(ty)<=.25)s->turn_armed=1;
    double yaw=s->yaw,pitch=s->pitch;const double rad=.017453292519943295;
    int horizontal=navigation_abs(tx)>=navigation_abs(ty);
    double dominant=horizontal?tx:ty,angle=0;
    if(turn_valid&&cfg.turn_enabled) {
        if(!cfg.smooth&&s->turn_armed&&navigation_abs(dominant)>=.7) {
            angle=(dominant<0?-1:1)*cfg.snap_degrees*rad;s->turn_armed=0;
        } else if(cfg.smooth&&s->turn_armed&&step&&navigation_abs(dominant)>cfg.deadzone)
            angle=(dominant<0?-1:1)*(navigation_abs(dominant)-cfg.deadzone)/(1-cfg.deadzone)*cfg.smooth_degrees*rad*dt;
    }
    if(angle!=0) {
        if(horizontal)yaw=m->atan2(m->sin(yaw+angle),m->cos(yaw+angle));
        else pitch-=angle; /* positive stick Y looks UP in Unity's pitch convention */
    }
    if(pitch>1.570795)pitch=1.570795;if(pitch< -1.570795)pitch= -1.570795;
    double offset[3]={s->offset[0],s->offset[1],s->offset[2]};
    /* Artificial yaw pivots at the current physical head, not the initial
       room reference. Keep the single yaw navigation basis without orbiting
       a user who has physically leaned/walked away from that reference. */
    if(head_delta&&yaw!=s->yaw) {
        double old=source_yaw+s->yaw-reference_yaw,next=source_yaw+yaw-reference_yaw;
        offset[0]+=(m->cos(old)*head_delta[0]+m->sin(old)*head_delta[2])-
            (m->cos(next)*head_delta[0]+m->sin(next)*head_delta[2]);
        offset[2]+=(-m->sin(old)*head_delta[0]+m->cos(old)*head_delta[2])-
            (-m->sin(next)*head_delta[0]+m->cos(next)*head_delta[2]);
    }
    if(!move_valid||!cfg.locomotion_enabled)s->move_armed=0;
    double mx=move_valid?sample->x[move]:0,my=move_valid?sample->y[move]:0;
    double length=m->sqrt(mx*mx+my*my);
    if(move_valid&&cfg.locomotion_enabled&&length<=cfg.deadzone)s->move_armed=1;
    if(move_valid&&cfg.locomotion_enabled&&s->move_armed&&step&&length>cfg.deadzone) {
        double magnitude=length>1?1:length,factor=(magnitude-cfg.deadzone)/(1-cfg.deadzone)/length;
        mx*=factor;my*=factor;
        double final_yaw=source_yaw+yaw+physical_yaw,final_pitch=source_pitch+pitch+physical_pitch;
        if(final_pitch>1.570795)final_pitch=1.570795;if(final_pitch< -1.570795)final_pitch= -1.570795;
        double distance=cfg.movement_speed*cfg.locomotion_scale*dt; /* WorldScale factor1 until supported */
        offset[0]+=distance*(mx*m->cos(final_yaw)+my*m->sin(final_yaw)*m->cos(final_pitch));
        offset[1]+=distance*(-my*m->sin(final_pitch));
        offset[2]+=distance*(-mx*m->sin(final_yaw)+my*m->cos(final_yaw)*m->cos(final_pitch));
        out->moved=distance!=0;
    }
    NavigationState candidate=*s;candidate.yaw=yaw;candidate.pitch=pitch;
    for(unsigned i=0;i<3;++i)candidate.offset[i]=offset[i];
    NavigationOutput proposed={0};
    if(!__builtin_isfinite(yaw)||!__builtin_isfinite(pitch)||!navigation_output(&candidate,units,&proposed)) {
        out->moved=0;out->rejected=1;s->move_armed=s->turn_armed=0;return navigation_output(s,units,out);
    }
    s->yaw=yaw;s->pitch=pitch;for(unsigned i=0;i<3;++i)s->offset[i]=offset[i];
    out->angles[0]=proposed.angles[0];out->angles[1]=proposed.angles[1];
    for(unsigned i=0;i<3;++i)out->offset[i]=proposed.offset[i];
    out->turned=angle!=0;return 1;
}
#endif
