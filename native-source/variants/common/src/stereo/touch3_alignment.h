#ifndef GF2VR_PRIVATE_TOUCH3_ALIGNMENT_H
#define GF2VR_PRIVATE_TOUCH3_ALIGNMENT_H

/* Engine-free final-eye hand basis. Raw eyes are already converted to Unity
 * coordinates; raw hands still use XR coordinates (Z reflected exactly once).
 * No eye, navigation, reference or game state is mutated here. Failed captures
 * invalidate the basis; failed transforms never touch the caller's output. */
#include "touch2_pose.h"
typedef struct Touch3Basis {
    unsigned valid;
    unsigned long long epoch,generation,frame;
    long long time;
    double midpoint[3],raw_midpoint[3],rotation[4];
    double raw_ipd,rendered_ipd,units,k;
} Touch3Basis;

static void touch3_invalidate(Touch3Basis *basis) {
    if(basis)basis->valid=0;
}
static int touch3_current(const Touch3Basis *basis,unsigned long long epoch,
    unsigned long long generation,unsigned long long frame,long long time) {
    return basis && basis->valid && touch2_identity(epoch,frame,time) && generation &&
        basis->epoch==epoch && basis->generation==generation &&
        basis->frame==frame && basis->time==time;
}
static int touch3_capture(Touch3Basis *out,unsigned long long epoch,
    unsigned long long generation,unsigned long long frame,long long time,
    const float raw_eyes[2][3],const float rendered_eyes[2][3],
    const double rotation[4],double units,double (*square_root)(double)) {
    if(!out)return 0;
    touch3_invalidate(out);
    if(!touch2_identity(epoch,frame,time) || !generation || !raw_eyes ||
       !rendered_eyes || !rotation || !square_root || !touch2_finite(units) || units<=0)return 0;
    Touch3Basis b={.epoch=epoch,.generation=generation,.frame=frame,.time=time,.units=units};
    double norm=0,raw_squared=0,rendered_squared=0;
    for(unsigned i=0;i<4;++i) {
        if(!touch2_finite(rotation[i]))return 0;
        norm+=rotation[i]*rotation[i];b.rotation[i]=rotation[i];
    }
    if(!touch2_finite(norm) || norm<.999999 || norm>1.000001)return 0;
    for(unsigned axis=0;axis<3;++axis) {
        for(unsigned eye=0;eye<2;++eye)
            if(!touch2_finite(raw_eyes[eye][axis]) || !touch2_finite(rendered_eyes[eye][axis]))return 0;
        double r=(double)raw_eyes[1][axis]-raw_eyes[0][axis];
        double v=(double)rendered_eyes[1][axis]-rendered_eyes[0][axis];
        raw_squared+=r*r;rendered_squared+=v*v;
        b.raw_midpoint[axis]=(double)raw_eyes[0][axis]*.5+(double)raw_eyes[1][axis]*.5;
        b.midpoint[axis]=(double)rendered_eyes[0][axis]*.5+(double)rendered_eyes[1][axis]*.5;
    }
    if(!touch2_finite(raw_squared) || !touch2_finite(rendered_squared) || raw_squared<.0001 || rendered_squared<=0)return 0;
    b.raw_ipd=square_root(raw_squared);b.rendered_ipd=square_root(rendered_squared);
    if(!touch2_finite(b.raw_ipd) || b.raw_ipd<.01 || !touch2_finite(b.rendered_ipd) || b.rendered_ipd<=0)return 0;
    b.k=(b.rendered_ipd/units)/b.raw_ipd;
    if(!touch2_finite(b.k) || b.k<=0 || !touch2_finite(b.k*units) || b.k*units<=0)return 0;
    b.valid=1;*out=b;return 1;
}
static int touch3_transform(const Touch3Basis *basis,const float raw_hand[3],float out[3]) {
    if(!basis || !basis->valid || !raw_hand || !out || !touch2_finite(basis->k) || basis->k<=0 ||
       !touch2_finite(basis->units) || basis->units<=0)return 0;
    double norm=0,displacement[3];
    for(unsigned i=0;i<4;++i) {
        if(!touch2_finite(basis->rotation[i]))return 0;
        norm+=basis->rotation[i]*basis->rotation[i];
    }
    if(!touch2_finite(norm) || norm<.999999 || norm>1.000001)return 0;
    for(unsigned axis=0;axis<3;++axis) {
        if(!touch2_finite(raw_hand[axis]) || !touch2_finite(basis->midpoint[axis]) ||
           !touch2_finite(basis->raw_midpoint[axis]))return 0;
        displacement[axis]=(axis==2?-(double)raw_hand[axis]:(double)raw_hand[axis])-basis->raw_midpoint[axis];
    }
    const double *q=basis->rotation;
    double t[3]={2*(q[1]*displacement[2]-q[2]*displacement[1]),
                 2*(q[2]*displacement[0]-q[0]*displacement[2]),
                 2*(q[0]*displacement[1]-q[1]*displacement[0])};
    double rotated[3]={displacement[0]+q[3]*t[0]+q[1]*t[2]-q[2]*t[1],
                       displacement[1]+q[3]*t[1]+q[2]*t[0]-q[0]*t[2],
                       displacement[2]+q[3]*t[2]+q[0]*t[1]-q[1]*t[0]};
    float result[3];double scale=basis->k*basis->units;
    if(!touch2_finite(scale) || scale<=0)return 0;
    for(unsigned axis=0;axis<3;++axis) {
        double value=basis->midpoint[axis]+rotated[axis]*scale;
        if(!touch2_finite(value) || value>FLT_MAX || value< -FLT_MAX)return 0;
        result[axis]=(float)value;
    }
    for(unsigned axis=0;axis<3;++axis)out[axis]=result[axis];
    return 1;
}
#endif
