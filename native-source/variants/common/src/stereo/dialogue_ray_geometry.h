#ifndef GF2_DIALOGUE_RAY_GEOMETRY_H
#define GF2_DIALOGUE_RAY_GEOMETRY_H
#include "dialogue_ray_surface.h"
/* Derive the physical plane from the ACTUAL calibrated private eye poses.
   activity_ui_eye_poses maps this complete UI to a perceived 2m plane. Undo
   only that mapping; do not fit another panel or change rendering/neutral. */
static int dialogue_ray_geometry(const PoseMath *m,const UnityTrackingInput *i,
    const float g[20],const ManagedEyePose eyes[2],ControllerPanel *out) {
    if(!m || !m->sqrt || !i || !g || !eyes || !out || !i->reference_epoch || !i->frame_id)return 0;
    float checked;for(unsigned n=0;n<20;++n)if(!eye_optics_float(g[n],&checked))return 0;
    if(g[3]!=0 || g[7]!=0 || g[11]!=0 || g[15]!=1 || !(g[18]>0 && g[19]>0))return 0;
    double h[4],e[4],p[4],relative[4],front[4],local[3],center[3],axes[3][3];
    if(!pose_read(m,&i->head,h) || !pose_read(m,&i->eyes[0],e) || !pose_read(m,&eyes[0],p))return 0;
    double hi[4]={-h[0],-h[1],-h[2],h[3]};pose_multiply(hi,e,relative);
    double ri[4]={-relative[0],-relative[1],-relative[2],relative[3]};pose_multiply(p,ri,front);
    double units[3][3]={{1,0,0},{0,1,0},{0,0,1}};
    for(unsigned a=0;a<3;++a)pose_rotate(front,units[a],axes[a]);
    double sx=m->sqrt((double)g[0]*g[0]+(double)g[1]*g[1]+(double)g[2]*g[2]);
    double sy=m->sqrt((double)g[4]*g[4]+(double)g[5]*g[5]+(double)g[6]*g[6]);
    if(!(sx>0 && sy>0))return 0;
    double offset[3];for(unsigned n=0;n<3;++n){
        if(!eye_optics_float(eyes[0].position[n],&checked) || !eye_optics_float(i->eyes[0].position[n],&checked) ||
           !eye_optics_float(i->head.position[n],&checked))return 0;
        if(g[n]/sx-axes[0][n]<-.0002 || g[n]/sx-axes[0][n]>.0002 ||
           g[4+n]/sy-axes[1][n]<-.0002 || g[4+n]/sy-axes[1][n]>.0002)return 0;
        center[n]=g[12+n]+g[n]*(g[16]+g[18]*.5)+g[4+n]*(g[17]+g[19]*.5);
        offset[n]=(double)i->eyes[0].position[n]-i->head.position[n];
    }
    pose_rotate(hi,offset,local);
    double depth=0;for(unsigned n=0;n<3;++n)depth+=(center[n]-eyes[0].position[n])*axes[2][n];
    double divisor=1-local[2]/2.0;if(!(divisor>.5 && divisor<1.5))return 0;
    depth/=divisor;if(!(depth>0) || !eye_optics_float(depth,&checked))return 0;
    /* Both calibrated eyes must describe this same complete plane. */
    double fp[3],world[3];pose_rotate(front,local,world);
    for(unsigned n=0;n<3;++n)fp[n]=eyes[0].position[n]-world[n]*depth/2.0;
    for(unsigned a=0;a<3;++a){double distance=0;
        for(unsigned n=0;n<3;++n)distance+=(center[n]-fp[n])*axes[a][n];
        double expected=a==2?depth:0;if(!(distance-expected>=-.0005 && distance-expected<=.0005))return 0;
    }
    for(unsigned eye=0;eye<2;++eye){double eq[4],q[4];
        if(!pose_read(m,&i->eyes[eye],eq) || !pose_read(m,&eyes[eye],p))return 0;
        pose_multiply(hi,eq,relative);pose_multiply(front,relative,q);
        for(unsigned n=0;n<4;++n)if(!(q[n]-p[n]>=-.0002 && q[n]-p[n]<=.0002))return 0;
        for(unsigned n=0;n<3;++n){if(!eye_optics_float(eyes[eye].position[n],&checked) ||
            !eye_optics_float(i->eyes[eye].position[n],&checked))return 0;
            offset[n]=(double)i->eyes[eye].position[n]-i->head.position[n];}
        pose_rotate(hi,offset,local);pose_rotate(front,local,world);
        for(unsigned n=0;n<3;++n)if(!(fp[n]+world[n]*depth/2.0-eyes[eye].position[n]>=-.0005 &&
            fp[n]+world[n]*depth/2.0-eyes[eye].position[n]<=.0005))return 0;
    }
    ControllerPanel panel={0};
    if(!eye_optics_float(sx*g[18]*2.0/depth,&panel.width) ||
       !eye_optics_float(sy*g[19]*2.0/depth,&panel.height) || !(panel.width>0 && panel.height>0))return 0;
    panel.pose.orientation=(ControllerQuaternion){-i->head.rotation[0],-i->head.rotation[1],i->head.rotation[2],i->head.rotation[3]};
    ControllerVector delta=controller_rotate(panel.pose.orientation,(ControllerVector){0,0,-2});
    panel.pose.position=(ControllerVector){i->head.position[0]+delta.x,i->head.position[1]+delta.y,-i->head.position[2]+delta.z};
    panel.u1=panel.v1=1;
    if(!controller_vector_valid(panel.pose.position) || !controller_quaternion_valid(panel.pose.orientation))return 0;
    *out=panel;return 1;
}
#endif
