#ifndef GF2_AUX_PANEL_PLACEMENT_H
#define GF2_AUX_PANEL_PLACEMENT_H
/* Pure auxiliary-only placement. Inputs/output share the current predicted
   VIEW space. No headset-reference capture, source pose or resource ownership. */
typedef struct {float x,y,z;} AuxPlacementVector;
typedef struct {float x,y,z,w;} AuxPlacementQuaternion;
typedef struct {AuxPlacementVector position;AuxPlacementQuaternion orientation;} AuxPlacementPose;
typedef struct {
    unsigned configured,viewer_facing;
    double offset[3],rotation[3],maximum_width,maximum_height,screen_tilt;
    unsigned apply_angles_when_facing;
} AuxPlacementSettings;
typedef struct {double (*sin)(double),(*cos)(double),(*sqrt)(double),(*atan2)(double,double);} AuxPlacementMath;
static AuxPlacementSettings aux_placement_defaults(void) {
    return (AuxPlacementSettings){0,1,{0,-.10,.01},{-67,3,0},.66,.66,45,0};
}
static int aux_placement_float(double value,float *out) {
    if(!out||!__builtin_isfinite(value)||value>3.4028234663852886e38||value< -3.4028234663852886e38)return 0;
    float f=(float)value;if(value!=0 && f==0)return 0;*out=f;return 1;
}
static int aux_placement_settings_valid(const AuxPlacementSettings *s) {
    float f;if(!s||s->viewer_facing>1||s->apply_angles_when_facing>1||!aux_placement_float(s->maximum_width,&f)||f<=0||
        !aux_placement_float(s->maximum_height,&f)||f<=0||!aux_placement_float(s->screen_tilt,&f))return 0;
    for(unsigned i=0;i<3;++i)if(!aux_placement_float(s->offset[i],&f)||!aux_placement_float(s->rotation[i],&f))return 0;
    return 1;
}
static AuxPlacementQuaternion aux_placement_multiply(AuxPlacementQuaternion a,AuxPlacementQuaternion b) {
    return (AuxPlacementQuaternion){a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,
        a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,
        a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z};
}
static AuxPlacementVector aux_placement_rotate(AuxPlacementQuaternion q,AuxPlacementVector v) {
    AuxPlacementVector t={2*(q.y*v.z-q.z*v.y),2*(q.z*v.x-q.x*v.z),2*(q.x*v.y-q.y*v.x)};
    return (AuxPlacementVector){v.x+q.w*t.x+q.y*t.z-q.z*t.y,
        v.y+q.w*t.y+q.z*t.x-q.x*t.z,v.z+q.w*t.z+q.x*t.y-q.y*t.x};
}
static int aux_placement_normalize(const AuxPlacementMath *m,AuxPlacementQuaternion *q) {
    double n=(double)q->x*q->x+(double)q->y*q->y+(double)q->z*q->z+(double)q->w*q->w;
    if(!__builtin_isfinite(n)||n<.999||n>1.001)return 0;
    double length=m->sqrt(n);if(!__builtin_isfinite(length)||length<=0)return 0;
    q->x=(float)(q->x/length);q->y=(float)(q->y/length);q->z=(float)(q->z/length);q->w=(float)(q->w/length);return 1;
}
static int aux_placement_pose(const AuxPlacementSettings *s,const AuxPlacementMath *m,
    AuxPlacementPose hand,AuxPlacementPose *out) {
    if(!out||!m||!m->sin||!m->cos||!m->sqrt||!m->atan2||!aux_placement_settings_valid(s)||
       !__builtin_isfinite(hand.position.x)||!__builtin_isfinite(hand.position.y)||!__builtin_isfinite(hand.position.z)||
       !aux_placement_normalize(m,&hand.orientation))return 0;
    AuxPlacementVector offset=aux_placement_rotate(hand.orientation,
        (AuxPlacementVector){(float)s->offset[0],(float)s->offset[1],(float)s->offset[2]});
    AuxPlacementPose result={0};
    if(!aux_placement_float((double)hand.position.x+offset.x,&result.position.x)||
       !aux_placement_float((double)hand.position.y+offset.y,&result.position.y)||
       !aux_placement_float((double)hand.position.z+offset.z,&result.position.z))return 0;
    AuxPlacementQuaternion base=hand.orientation;
    if(s->viewer_facing) {
        /* Upright yaw toward VIEW origin, matching reference's eye-midpoint
           facing policy. OFF takes only controller orientation, never head aim. */
        double yaw=m->atan2(-(double)result.position.x,-(double)result.position.z)/2;
        base=(AuxPlacementQuaternion){0,(float)m->sin(yaw),0,(float)m->cos(yaw)};
    }
    result.orientation=base;
    if(!s->viewer_facing||s->apply_angles_when_facing) {
        const double radians=3.14159265358979323846/360;
        double p=s->rotation[0]*radians,y=s->rotation[1]*radians,r=s->rotation[2]*radians;
        AuxPlacementQuaternion pitch={(float)m->sin(p),0,0,(float)m->cos(p)};
        AuxPlacementQuaternion yaw={0,(float)m->sin(y),0,(float)m->cos(y)};
        AuxPlacementQuaternion roll={0,0,(float)m->sin(r),(float)m->cos(r)};
        result.orientation=aux_placement_multiply(base,aux_placement_multiply(aux_placement_multiply(yaw,pitch),roll));
        /* A final screen-local X rotation, independent of controller Euler order.
           Quad front is local +Z: negative X turns that normal toward local +Y.
           Skip zero to preserve existing quaternion arithmetic exactly. */
        if(s->screen_tilt!=0) {
            double t=-s->screen_tilt*radians;
            AuxPlacementQuaternion tilt={(float)m->sin(t),0,0,(float)m->cos(t)};
            result.orientation=aux_placement_multiply(result.orientation,tilt);
        }
        }
    if(!aux_placement_normalize(m,&result.orientation))return 0;
    *out=result;return 1;
}
static int aux_placement_size(const AuxPlacementSettings *s,float front_width,float front_height,float *width,float *height) {
    if(!aux_placement_settings_valid(s)||!__builtin_isfinite(front_width)||!__builtin_isfinite(front_height)||
       front_width<=0||front_height<=0)return 0;
    double w=s->maximum_width,h=w*(double)front_height/front_width;
    if(h>s->maximum_height){h=s->maximum_height;w=h*(double)front_width/front_height;}
    return aux_placement_float(w,width)&&aux_placement_float(h,height)&&*width>0&&*height>0;
}
#endif
