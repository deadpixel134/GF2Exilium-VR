#ifndef GF2_EYE_OPTICS_H
#define GF2_EYE_OPTICS_H
#include "managed_eye_camera.h"
/* Pure producer math: final Unity-world eye poses and runtime per-eye frustum
   tangents. No source pose policy, reference capture, FOV approximation, GPU
   projection adjustment or eye-separation scaling belongs here. The caller
   must supply current, same-frame poses/FOV and keep XR submission coherent.
   Camera setters consume Unity CPU projection (OpenGL clip convention);
   the render pipeline owns backend/Y/depth adjustment. */
typedef struct {float left,right,down,up;} EyeFrustumTangents;

static int eye_optics_float(double x,float *out) {
    /* Reject non-finite or non-representable results before a float cast. */
    if(!(x>=-3.4028234663852886e38 && x<=3.4028234663852886e38)) return 0;
    *out=(float)x;return 1;
}
static int eye_optics_build(const ManagedEyePose poses[2],
                            const EyeFrustumTangents fov[2],float near_z,float far_z,
                            ManagedEyeMatrix views[2],ManagedEyeMatrix projections[2]) {
    if(!poses || !fov || !views || !projections || views==projections) return 0;
    float checked;
    if(!eye_optics_float(near_z,&checked) || !eye_optics_float(far_z,&checked) ||
       !(near_z>0 && far_z>near_z)) return 0;
    ManagedEyeMatrix v[2]={0},p[2]={0};
    for(unsigned eye=0;eye<2;++eye) {
        const ManagedEyePose *pose=&poses[eye];
        for(unsigned j=0;j<3;++j)
            if(!eye_optics_float(pose->position[j],&checked)) return 0;
        double q[4],norm=0;
        for(unsigned j=0;j<4;++j) {
            if(!eye_optics_float(pose->rotation[j],&checked)) return 0;
            q[j]=pose->rotation[j];norm+=q[j]*q[j];
        }
        if(!(norm>=0.999 && norm<=1.001)) return 0;
        /* Divide products by norm to remove float quaternion roundoff without
           requiring a CRT sqrt. Reject materially invalid rotations above. */
        double x=q[0],y=q[1],z=q[2],w=q[3],s=2/norm;
        double rotation[3][3]={
            {1-s*(y*y+z*z),s*(x*y-z*w),s*(x*z+y*w)},
            {s*(x*y+z*w),1-s*(x*x+z*z),s*(y*z-x*w)},
            {s*(x*z-y*w),s*(y*z+x*w),1-s*(x*x+y*y)}
        };
        /* diag(1,1,-1) * inverse(TR): Unity +Z Transform forward becomes
           camera -Z forward. Store Matrix4x4 column-major value layout. */
        for(unsigned row=0;row<3;++row) {
            double translation=0,sign=row==2?-1:1;
            for(unsigned col=0;col<3;++col) {
                double coefficient=sign*rotation[col][row];
                v[eye].elements[col*4+row]=(float)coefficient;
                translation-=coefficient*pose->position[col];
            }
            if(!eye_optics_float(translation,&v[eye].elements[12+row])) return 0;
        }
        v[eye].elements[15]=1;
        double l=fov[eye].left,r=fov[eye].right,d=fov[eye].down,u=fov[eye].up;
        if(!eye_optics_float(l,&checked) || !eye_optics_float(r,&checked) ||
           !eye_optics_float(d,&checked) || !eye_optics_float(u,&checked) ||
           !(r>l && u>d)) return 0;
        double n=near_z,f=far_z;
        if(!eye_optics_float(2/(r-l),&p[eye].elements[0]) ||
           !eye_optics_float(2/(u-d),&p[eye].elements[5]) ||
           !eye_optics_float((r+l)/(r-l),&p[eye].elements[8]) ||
           !eye_optics_float((u+d)/(u-d),&p[eye].elements[9]) ||
           !eye_optics_float(-(f+n)/(f-n),&p[eye].elements[10]) ||
           !eye_optics_float(-2*f*n/(f-n),&p[eye].elements[14])) return 0;
        p[eye].elements[11]=-1;
        if(!(p[eye].elements[0]>0 && p[eye].elements[5]>0 && p[eye].elements[14]<0)) return 0;
    }
    /* Failure never publishes a half-computed pair. */
    for(unsigned eye=0;eye<2;++eye) {views[eye]=v[eye];projections[eye]=p[eye];}
    return 1;
}
static int managed_eyes_optics(const ManagedEyeApi *a,ManagedEyes *eyes,
                               unsigned long long frame,const ManagedEyePose poses[2],
                               const EyeFrustumTangents fov[2],float near_z,float far_z) {
    ManagedEyeMatrix views[2],projections[2];
    if(!eye_optics_build(poses,fov,near_z,far_z,views,projections)) {
        /* Preserve an outstanding consumer's immutable pair; otherwise reject
           borrowing old matrices after invalid current input. */
        if(!eyes->borrowed) eyes->matrices_ready=0;
        return 0;
    }
    int applied=managed_eyes_matrices(a,eyes,frame,views,projections,poses);
#ifdef GF2VR_OPTICS_OBSERVE
    if(applied) GF2VR_OPTICS_OBSERVE(frame,poses,views,projections);
#endif
    return applied;
}
#endif
