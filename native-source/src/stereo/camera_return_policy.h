#ifndef GF2_CAMERA_RETURN_POLICY_H
#define GF2_CAMERA_RETURN_POLICY_H
/* Compute a presentation compensation, not a new physical reference. Source
   yaw/pitch are authoritative; physical roll and calibrated eye cant remain.
   Output offset is meters, matching the existing navigation integrator. */
static int camera_return_target(const PoseMath *m,const UnityTrackingInput *i,const PoseSourceAnchor *a,
    const double physical[3],double source_yaw,double reference_yaw,double head_scale,
    double angles[2],double offset[3]){
    if(!m||!m->sin||!m->cos||!m->sqrt||!m->atan2||!i||!a||!physical||!angles||!offset||
       !__builtin_isfinite(head_scale)||head_scale<=0||!__builtin_isfinite(source_yaw)||!__builtin_isfinite(reference_yaw)||
       !(physical[1]>-1.5707953267948966&&physical[1]<1.5707953267948966))return 0;
    for(unsigned n=0;n<3;++n)if(!__builtin_isfinite(physical[n]))return 0;
    double head[4];if(!pose_read(m,&i->head,head))return 0;
    for(unsigned eye=0;eye<2;++eye){double q[4];if(!pose_read(m,&i->eyes[eye],q))return 0;}
    double yq[4]={0,m->sin(source_yaw/2),0,m->cos(source_yaw/2)};
    double pq[4]={m->sin(a->pitch/2),0,0,m->cos(a->pitch/2)};
    double rq[4]={0,0,m->sin(physical[2]/2),m->cos(physical[2]/2)},temp[4],final[4];
    pose_multiply(yq,pq,temp);pose_multiply(temp,rq,final);
    double common[3],inverse[4]={-head[0],-head[1],-head[2],head[3]},local[3],world[3];
    for(unsigned n=0;n<3;++n)common[n]=((double)i->eyes[0].position[n]+i->eyes[1].position[n])*.5-i->head.position[n];
    pose_rotate(inverse,common,local);pose_rotate(final,local,world);
    double heading=source_yaw-physical[0]-reference_yaw;
    double rotation[4]={0,m->sin(heading/2),0,m->cos(heading/2)},displacement[3],translated[3];
    for(unsigned n=0;n<3;++n)displacement[n]=(double)i->head.position[n]-i->reference_head.position[n];
    pose_rotate(rotation,displacement,translated);double result[3];float checked;
    for(unsigned n=0;n<3;++n){result[n]=-(translated[n]+world[n])*head_scale;
        if(!__builtin_isfinite(result[n])||!eye_optics_float(result[n]*a->units_per_meter,&checked))return 0;}
    angles[0]=-physical[0];angles[1]=-physical[1];for(unsigned n=0;n<3;++n)offset[n]=result[n];return 1;
}
#endif
