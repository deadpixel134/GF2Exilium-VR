#ifndef GF2_POSE_COMPOSITION_H
#define GF2_POSE_COMPOSITION_H
#include "xr_pose_coordinates.h"
typedef struct {
    double (*sin)(double),(*cos)(double),(*sqrt)(double);
    double (*atan2)(double,double);
} PoseMath;
/* Caller selects this presentation anchor for a source generation.
   Authored pitch requires a target-approved policy; authored roll is never used.
   No live-entry HMD reference capture is accepted here.
   units_per_meter is an explicit target input, not a guessed game scale. */
typedef struct {
    unsigned long long source_generation;
    float position[3],yaw,units_per_meter,pitch;
} PoseSourceAnchor;
static int pose_read(const PoseMath *m,const ManagedEyePose *p,double q[4]) {
    float checked;double norm=0;
    for(unsigned i=0;i<3;++i) if(!eye_optics_float(p->position[i],&checked)) return 0;
    for(unsigned i=0;i<4;++i) {
        if(!eye_optics_float(p->rotation[i],&checked)) return 0;
        q[i]=p->rotation[i];norm+=q[i]*q[i];
    }
    if(!(norm>=0.999 && norm<=1.001)) return 0;
    double length=m->sqrt(norm);
    if(!(length>0.999 && length<1.001)) return 0;
    for(unsigned i=0;i<4;++i) q[i]/=length;
    return 1;
}
static void pose_multiply(const double a[4],const double b[4],double out[4]) {
    double q[4]={a[3]*b[0]+a[0]*b[3]+a[1]*b[2]-a[2]*b[1],
                 a[3]*b[1]-a[0]*b[2]+a[1]*b[3]+a[2]*b[0],
                 a[3]*b[2]+a[0]*b[1]-a[1]*b[0]+a[2]*b[3],
                 a[3]*b[3]-a[0]*b[0]-a[1]*b[1]-a[2]*b[2]};
    for(unsigned i=0;i<4;++i) out[i]=q[i];
}
static void pose_rotate(const double q[4],const double v[3],double out[3]) {
    double vector[4]={v[0],v[1],v[2],0},inverse[4]={-q[0],-q[1],-q[2],q[3]},temp[4],result[4];
    pose_multiply(q,vector,temp);pose_multiply(temp,inverse,result);
    for(unsigned i=0;i<3;++i) out[i]=result[i];
}
/* Unity Yaw(Y) * Pitch(X) * Roll(Z). The scalar decomposition cannot assign
   yaw/roll independently at the vertical singularity: reject that frame rather
   than invent roll. Wider pitch/gimbal recovery is not claimed accepted. */
static int pose_angles(const PoseMath *m,const double q[4],double angles[3]) {
    double x=q[0],y=q[1],z=q[2],w=q[3];
    double r10=2*(x*y+z*w),r11=1-2*(x*x+z*z),r12=2*(y*z-x*w);
    double c=m->sqrt(r10*r10+r11*r11);
    if(!(c>0.000001)) return 0;
    angles[0]=m->atan2(2*(x*z+y*w),1-2*(x*x+y*y));
    angles[1]=m->atan2(-r12,c);angles[2]=m->atan2(r10,r11);
    float checked;
    for(unsigned i=0;i<3;++i) if(!eye_optics_float(angles[i],&checked)) return 0;
    return 1;
}
static int pose_compose(const PoseMath *m,const UnityTrackingInput *input,
                         const PoseSourceAnchor *anchor,ManagedEyePose out[2]) {
#ifdef GF2VR_TOUCH_PRIVATE
    touch_p3_begin();
#endif
#line 57
    if(!m || !m->sin || !m->cos || !m->sqrt || !m->atan2 || !input || !anchor || !out ||
       !input->reference_epoch || !input->frame_id || input->display_time<=0 || !anchor->source_generation) return 0;
    float checked;
    if(!eye_optics_float(anchor->yaw,&checked) || !eye_optics_float(anchor->pitch,&checked) ||
       !(anchor->pitch>-1.5707953267948966 && anchor->pitch<1.5707953267948966) ||
       !eye_optics_float(anchor->units_per_meter,&checked) ||
       !(anchor->units_per_meter>0)) return 0;
    for(unsigned i=0;i<3;++i) if(!eye_optics_float(anchor->position[i],&checked)) return 0;
    double head[4],reference[4],angles[3],neutral[3];
    if(!pose_read(m,&input->head,head) || !pose_read(m,&input->reference_head,reference) ||
       !pose_angles(m,head,angles) || !pose_angles(m,reference,neutral)) return 0;
    double delta[3];
    for(unsigned i=0;i<3;++i) {
        double difference=angles[i]-neutral[i];
        delta[i]=m->atan2(m->sin(difference),m->cos(difference));
    }
    /* Default pitch is zero; the main follow policy supplies live authored pitch.
       Physical reference/deltas stay independent from source animation. */
    double source_yaw=m->atan2(m->sin(anchor->yaw),m->cos(anchor->yaw));
#ifdef GF2VR_NAVIGATION_POSE
    double artificial[2]={0};float locomotion_offset[3]={0};
    if(!GF2VR_NAVIGATION_POSE(m,input,anchor,source_yaw,delta,neutral[0],artificial,locomotion_offset))return 0;
    double yaw=source_yaw+artificial[0]+delta[0],pitch=delta[1]+artificial[1],roll=delta[2];
#else
    double yaw=source_yaw+delta[0],pitch=delta[1],roll=delta[2];
#endif
#ifdef GF2VR_NAVIGATION_POSE
    if(!(delta[1]>-1.5707953267948966 && delta[1]<1.5707953267948966)) return 0;
#else
    if(!(pitch>-1.5707953267948966 && pitch<1.5707953267948966)) return 0;
#endif
    pitch+=anchor->pitch;
    /* Avoid flipping the horizon when authored and physical pitch sum past
       vertical. This bounds the composed orientation, not either input. */
    if(pitch>1.570795) pitch=1.570795;
    if(pitch< -1.570795) pitch= -1.570795;
    double yq[4]={0,m->sin(yaw/2),0,m->cos(yaw/2)};
    double pq[4]={m->sin(pitch/2),0,0,m->cos(pitch/2)};
    double rq[4]={0,0,m->sin(roll/2),m->cos(roll/2)},temp[4],final[4];
    pose_multiply(yq,pq,temp);pose_multiply(temp,rq,final);
    /* Head translation uses a roll/pitch-free navigation basis. */
#ifdef GF2VR_NAVIGATION_POSE
    double heading=source_yaw+artificial[0]-neutral[0];
#else
    double heading=source_yaw-neutral[0];
#endif
    double navigation[4]={0,m->sin(heading/2),0,m->cos(heading/2)};
    float navigation_offset[3]={0};
#ifdef GF2VR_NAVIGATION_OFFSET
    if(!GF2VR_NAVIGATION_OFFSET(input,anchor,navigation,navigation_offset))return 0;
#endif
#ifdef GF2VR_NAVIGATION_POSE
    for(unsigned i=0;i<3;++i)if(locomotion_offset[i]!=0 &&
        !eye_optics_float((double)navigation_offset[i]+locomotion_offset[i],&navigation_offset[i]))return 0;
#endif
    double displacement[3],translated[3];
    for(unsigned i=0;i<3;++i) displacement[i]=(double)input->head.position[i]-input->reference_head.position[i];
    pose_rotate(navigation,displacement,translated);
#ifdef GF2VR_WORLD_SCALE
    /* Physical displacement only; source position, accumulated navigation and
       calibrated optics retain their independent ownership. */
    double head_scale=stereo_world_head_scale();
    for(unsigned i=0;i<3;++i) {
        translated[i]*=head_scale;
        if(!__builtin_isfinite(translated[i]))return 0;
    }
#endif
#ifdef GF2VR_TOUCH_PRIVATE
#ifdef GF2VR_WORLD_SCALE
    touch_p2_composed(input,anchor,navigation,navigation_offset,head_scale);
#else
    touch_p2_composed(input,anchor,navigation,navigation_offset,1);
#endif
#endif
    double inverse_head[4]={-head[0],-head[1],-head[2],head[3]};
#line 125
    ManagedEyePose result[2]={0};
#ifdef GF2VR_STEREO_DEPTH_POSE
    double depth_eye_offsets[2][3];
#endif
    for(unsigned eye=0;eye<2;++eye) {
        double eye_q[4],relative_q[4],final_eye[4],offset[3],local_offset[3],world_offset[3];
        if(!pose_read(m,&input->eyes[eye],eye_q)) return 0;
        for(unsigned i=0;i<3;++i) offset[i]=(double)input->eyes[eye].position[i]-input->head.position[i];
        pose_rotate(inverse_head,offset,local_offset);pose_rotate(final,local_offset,world_offset);
#ifdef GF2VR_STEREO_DEPTH_POSE
        for(unsigned i=0;i<3;++i)depth_eye_offsets[eye][i]=world_offset[i];
#endif
        /* Only the physical per-eye calibration is quaternion-composed; the
           physical head delta above is scalar, so origin tilt cannot leak roll. */
        pose_multiply(inverse_head,eye_q,relative_q);pose_multiply(final,relative_q,final_eye);
        for(unsigned i=0;i<3;++i)
            if(!eye_optics_float(anchor->position[i]+navigation_offset[i]+(translated[i]+world_offset[i])*anchor->units_per_meter,
                                 &result[eye].position[i])) return 0;
        for(unsigned i=0;i<4;++i)
            if(!eye_optics_float(final_eye[i],&result[eye].rotation[i])) return 0;
        double verify[4];if(!pose_read(m,&result[eye],verify)) return 0;
    }
#ifdef GF2VR_STEREO_DEPTH_POSE
#ifdef GF2VR_WORLD_SCALE
    /* Calibrated eye midpoint need not coincide with the reported head
       position. Move that common physical offset with the head term too;
       Stereo Depth below independently owns only the bilateral baseline. */
    if(head_scale!=1)for(unsigned axis=0;axis<3;++axis) {
        double center=depth_eye_offsets[0][axis]*.5+depth_eye_offsets[1][axis]*.5;
        translated[axis]+=center*(head_scale-1);
        for(unsigned eye=0;eye<2;++eye)
            if(!eye_optics_float(anchor->position[axis]+navigation_offset[axis]+(translated[axis]+depth_eye_offsets[eye][axis])*anchor->units_per_meter,&result[eye].position[axis]))return 0;
    }
#endif
    if(!GF2VR_STEREO_DEPTH_POSE(m,input,anchor,depth_eye_offsets,translated,navigation_offset,result))return 0;
#endif
#ifdef GF2VR_TOUCH_PRIVATE
    touch_p3_final(m,input,anchor,result,final,inverse_head);
#endif
    out[0]=result[0];out[1]=result[1];
#line 162
#ifdef GF2VR_PLACEMENT_COMPOSE
    GF2VR_PLACEMENT_COMPOSE(input,anchor,navigation,navigation_offset,displacement,translated,result,yaw,pitch,roll);
#endif
    return 1;
}
#endif
