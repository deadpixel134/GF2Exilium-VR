#ifndef GF2_CONTROLLER_INPUT_H
#define GF2_CONTROLLER_INPUT_H
/* Deterministic panel interaction policy. No XR calls, OS input, shared memory
   or game/scene assumptions. Owner supplies current presented panel geometry,
   focus/tracking eligibility and one session/presentation/role token. */
typedef struct {float x,y,z;} ControllerVector;
typedef struct {float x,y,z,w;} ControllerQuaternion;
typedef struct {ControllerVector position;ControllerQuaternion orientation;} ControllerPose;
typedef struct {
    ControllerPose pose;float width,height;
    float u0,v0,u1,v1; /* real game content inside the presented texture */
    unsigned client_width,client_height;
} ControllerPanel;
typedef struct {unsigned x,y;float u,v;ControllerVector point;unsigned client_width,client_height;} ControllerHit;
static int controller_finite(float f) {return f>=-1000000.0f && f<=1000000.0f;}
static int controller_vector_valid(ControllerVector v) {
    return controller_finite(v.x)&&controller_finite(v.y)&&controller_finite(v.z);
}
static int controller_quaternion_valid(ControllerQuaternion q) {
    if(!controller_finite(q.x)||!controller_finite(q.y)||!controller_finite(q.z)||!controller_finite(q.w))return 0;
    float n=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;return n>0.999f && n<1.001f;
}
static ControllerVector controller_rotate(ControllerQuaternion q,ControllerVector v) {
    ControllerVector t={2*(q.y*v.z-q.z*v.y),2*(q.z*v.x-q.x*v.z),2*(q.x*v.y-q.y*v.x)};
    return (ControllerVector){v.x+q.w*t.x+q.y*t.z-q.z*t.y,
        v.y+q.w*t.y+q.z*t.x-q.x*t.z,v.z+q.w*t.z+q.x*t.y-q.y*t.x};
}
/* Aim and panel must be in the same current XR space/predicted time. Rays
   facing away, back-face hits, margins, nonfinite/invalid poses reject. */
static int controller_panel_hit(const ControllerPose *aim,const ControllerPanel *p,ControllerHit *out) {
    if(!out)return 0;*out=(ControllerHit){0};
    if(!aim||!p||!controller_vector_valid(aim->position)||!controller_vector_valid(p->pose.position)||
       !controller_quaternion_valid(aim->orientation)||!controller_quaternion_valid(p->pose.orientation)||
       !controller_finite(p->width)||!controller_finite(p->height)||p->width<=0||p->height<=0||
       !p->client_width||!p->client_height||p->client_width>32768||p->client_height>32768||
       !(p->u0>=0&&p->v0>=0&&p->u1<=1&&p->v1<=1&&p->u0<p->u1&&p->v0<p->v1))return 0;
    ControllerQuaternion inverse={-p->pose.orientation.x,-p->pose.orientation.y,-p->pose.orientation.z,p->pose.orientation.w};
    ControllerVector relative={aim->position.x-p->pose.position.x,aim->position.y-p->pose.position.y,aim->position.z-p->pose.position.z};
    ControllerVector local=controller_rotate(inverse,relative);
    ControllerVector direction=controller_rotate(inverse,controller_rotate(aim->orientation,(ControllerVector){0,0,-1}));
    if(!(local.z>0 && direction.z<-0.00001f))return 0;
    float distance=-local.z/direction.z;if(!(distance>0&&distance<=20))return 0;
    float u=0.5f+(local.x+distance*direction.x)/p->width;
    float v=0.5f-(local.y+distance*direction.y)/p->height;
    if(!(u>=p->u0&&u<=p->u1&&v>=p->v0&&v<=p->v1))return 0;
    out->u=u;out->v=v;
    out->client_width=p->client_width;out->client_height=p->client_height;
    out->x=(unsigned)(((u-p->u0)/(p->u1-p->u0))*(p->client_width-1)+0.5f);
    out->y=(unsigned)(((v-p->v0)/(p->v1-p->v0))*(p->client_height-1)+0.5f);
    out->point=(ControllerVector){aim->position.x+distance*controller_rotate(aim->orientation,(ControllerVector){0,0,-1}).x,
        aim->position.y+distance*controller_rotate(aim->orientation,(ControllerVector){0,0,-1}).y,
        aim->position.z+distance*controller_rotate(aim->orientation,(ControllerVector){0,0,-1}).z};
    return 1;
}
enum {CONTROLLER_MOVE,CONTROLLER_DOWN,CONTROLLER_UP,CONTROLLER_BACK};
typedef int (*ControllerEmit)(void *,unsigned,unsigned,unsigned);
typedef struct {
    unsigned long long token,stable_time,last_time;
    unsigned pressed,primary_armed,back_armed,back_down,stable,have_previous,dragging;
    unsigned x,y,previous_x,previous_y,stable_x,stable_y;
    unsigned press_x,press_y,press_raw_x,press_raw_y;
} ControllerInput;
/* Cancellation never emits a fresh press or move. A failed release remains
   owned and is retried; an unsuccessful sink cannot silently clear held input. */
static int controller_input_cancel(ControllerInput *s,ControllerEmit emit,void *context) {
    s->primary_armed=s->back_armed=s->back_down=s->stable=s->have_previous=s->dragging=0;
    s->token=0;s->last_time=0;
    if(s->pressed) {
        if(!emit || !emit(context,CONTROLLER_UP,s->x,s->y))return 0;
        s->pressed=0;
    }
    return 1;
}
static unsigned controller_distance(unsigned a,unsigned b) {return a>b?a-b:b-a;}
/* Resolution-relative elliptical deadbands: hover0.4%, drag-start0.8% of
   each client axis (minimum3/6px). Initial tuning, not an accepted constant.
   Anchored distance prevents sub-threshold per-frame slow motion being lost. */
static int controller_near(const ControllerHit *h,unsigned x,unsigned y,unsigned per_thousand) {
    unsigned w=h->client_width?h->client_width:1920,ht=h->client_height?h->client_height:1080;
    if(w>32768||ht>32768||h->x>=w||h->y>=ht||x>=w||y>=ht)return 0;
    unsigned rx=(w*per_thousand+999)/1000,ry=(ht*per_thousand+999)/1000;
    unsigned minimum=per_thousand==4?3:6;if(rx<minimum)rx=minimum;if(ry<minimum)ry=minimum;
    unsigned dx=controller_distance(h->x,x),dy=controller_distance(h->y,y);
    if(dx>rx||dy>ry)return 0;
    unsigned long long a=rx*rx,b=ry*ry;
    return (unsigned long long)dx*dx*b+(unsigned long long)dy*dy*a<=a*b;
}
/* Read-only preview used by the XR cursor before submission and by policy
   after successful submission. Raw hit stays authoritative for eligibility.
   Never reuse a world-space point from a prior moving hand-panel pose. */
static void controller_input_coordinates(const ControllerInput *s,unsigned long long now,const ControllerHit *h,
    int primary,unsigned *x,unsigned *y) {
    *x=h->x;*y=h->y;
    if(s->pressed) {
        if(!primary) {*x=s->x;*y=s->y;}
        else if(!s->dragging&&controller_near(h,s->press_raw_x,s->press_raw_y,8)) {*x=s->press_x;*y=s->press_y;}
    } else if(s->stable&&now>=s->stable_time&&now-s->stable_time<=150&&
              ((primary&&controller_near(h,s->stable_x,s->stable_y,32))||
               (!primary&&controller_near(h,s->stable_x,s->stable_y,4)))) {
        *x=s->stable_x;*y=s->stable_y;
    }
}
/* valid means focus + tracked aim + actually presented panel + content hit.
   token must change on session/presentation/role/geometry ownership change.
   Buttons need release after cancellation, so held recovery cannot click. */
static int controller_input_step(ControllerInput *s,unsigned long long token,unsigned long long now,
    int valid,const ControllerHit *hit,int face,int trigger,int back,ControllerEmit emit,void *context) {
    if(!s)return 0;
    if(!valid||!token||!hit||!emit||!now||now<s->last_time)
        return controller_input_cancel(s,emit,context);
    if(s->token!=token) {
        if(!controller_input_cancel(s,emit,context))return 0;
        s->token=token;
    }
    s->last_time=now;
    int primary=face||trigger;
    if(!primary)s->primary_armed=1;
    if(!back)s->back_armed=1;
    unsigned x,y;controller_input_coordinates(s,now,hit,primary,&x,&y);
    if(primary && !s->pressed && s->primary_armed) {
        /* Both Face and Trigger latch the latest stabilized released point.
           No prior fresh valid hover means release/repress, never a delayed click. */
        if(!s->stable || now<s->stable_time || now-s->stable_time>150||
           !controller_near(hit,s->stable_x,s->stable_y,32)) {s->primary_armed=0;return 1;}
        if(!emit(context,CONTROLLER_MOVE,x,y))return controller_input_cancel(s,emit,context);
        if(!emit(context,CONTROLLER_DOWN,x,y))return controller_input_cancel(s,emit,context);
        s->pressed=1;s->x=x;s->y=y;s->primary_armed=0;
        s->press_x=x;s->press_y=y;s->press_raw_x=hit->x;s->press_raw_y=hit->y;s->dragging=0;
    } else if(s->pressed && !primary) {
        /* A normal click releases at its held coordinate, not the noisy
           release-frame ray. Cancellation still emits only UP, never MOVE. */
        if(!emit(context,CONTROLLER_MOVE,s->x,s->y))return controller_input_cancel(s,emit,context);
        if(!emit(context,CONTROLLER_UP,s->x,s->y))return controller_input_cancel(s,emit,context);
        s->pressed=s->dragging=0;
    } else {
        if(s->pressed&&!controller_near(hit,s->press_raw_x,s->press_raw_y,8))s->dragging=1;
        if(!emit(context,CONTROLLER_MOVE,x,y))return controller_input_cancel(s,emit,context);
        s->x=x;s->y=y;
    }
    if(back && s->back_armed && !s->back_down) {
        /* Back cancels a drag before delivering Back/Cancel. */
        if(s->pressed) {
            if(!emit(context,CONTROLLER_UP,s->x,s->y))return controller_input_cancel(s,emit,context);
            s->pressed=s->dragging=0;s->primary_armed=0;
        }
        if(!emit(context,CONTROLLER_BACK,x,y))return controller_input_cancel(s,emit,context);
    }
    s->back_down=back!=0;
    if(!primary && !back) {
        /* Every released sample supplies a fresh anchored hover point. Small
           tremor renews it without requiring two nearly identical raw samples.
           Release-frame motion isn't a new pre-press aim point. */
        s->stable=1;s->stable_x=x;s->stable_y=y;s->stable_time=now;
        s->previous_x=x;s->previous_y=y;s->have_previous=1;
    }
    return 1;
}
#endif
