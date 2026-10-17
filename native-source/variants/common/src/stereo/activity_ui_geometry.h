#ifndef GF2_ACTIVITY_UI_GEOMETRY_H
#define GF2_ACTIVITY_UI_GEOMETRY_H
/* Whole original UI geometry, drawn by private cameras only. The camera-space
   plane is kept view-front; calibrated eye offsets/cant retain real disparity.
   No physical reference, source Transform, Canvas or world-scale mutation. */
static int activity_ui_eye_poses(const PoseMath *m,const UnityTrackingInput *input,
    const ManagedEyePose *source,float plane,float distance,ManagedEyePose out[2]) {
    float checked;double head[4],sq[4];
    if(!m || !m->sqrt || !input || !source || !out ||
       !eye_optics_float(plane,&checked) || !eye_optics_float(distance,&checked) ||
       !(plane>0 && distance>0) || !input->reference_epoch || !input->frame_id ||
       !pose_read(m,&input->head,head) || !pose_read(m,source,sq))return 0;
    double inverse[4]={-head[0],-head[1],-head[2],head[3]},scale=(double)plane/distance;
    ManagedEyePose result[2];
    for(unsigned eye=0;eye<2;++eye) {
        double eq[4],relative[4],final[4],offset[3],local[3],world[3];
        if(!pose_read(m,&input->eyes[eye],eq))return 0;
        for(unsigned n=0;n<3;++n)offset[n]=(double)input->eyes[eye].position[n]-input->head.position[n];
        pose_rotate(inverse,offset,local);pose_rotate(sq,local,world);
        pose_multiply(inverse,eq,relative);pose_multiply(sq,relative,final);
        for(unsigned n=0;n<3;++n)if(!eye_optics_float(source->position[n]+world[n]*scale,&result[eye].position[n]))return 0;
        for(unsigned n=0;n<4;++n)if(!eye_optics_float(final[n],&result[eye].rotation[n]))return 0;
    }
    out[0]=result[0];out[1]=result[1];return 1;
}
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
/* Whole-root planes share source UI layers. Require a separating clip plane in
   each other camera's final frustum; do not knowingly draw both roots twice.
   Inconclusive/overlapping bounds reject this optional route, never crop widgets
   or mutate original layer/hierarchy/Canvas geometry to obtain isolation. */
static int activity_ui_root_excluded(const float geometry[20],const ManagedEyeMatrix views[2],const ManagedEyeMatrix projections[2]) {
    float checked;for(unsigned i=0;i<20;++i)if(!eye_optics_float(geometry[i],&checked))return 0;
    if(!(geometry[18]>0 && geometry[19]>0) || geometry[3]!=0 || geometry[7]!=0 || geometry[11]!=0 || geometry[15]!=1)return 0;
    for(unsigned eye=0;eye<2;++eye){unsigned outside[6]={0};
        for(unsigned corner=0;corner<4;++corner){double local[4]={geometry[16]+(corner&1?geometry[18]:0),geometry[17]+(corner&2?geometry[19]:0),0,1};
            double world[4]={0},camera[4]={0},clip[4]={0};
            for(unsigned row=0;row<4;++row)for(unsigned col=0;col<4;++col)world[row]+=geometry[col*4+row]*local[col];
            for(unsigned row=0;row<4;++row)for(unsigned col=0;col<4;++col)camera[row]+=views[eye].elements[col*4+row]*world[col];
            for(unsigned row=0;row<4;++row)for(unsigned col=0;col<4;++col)clip[row]+=projections[eye].elements[col*4+row]*camera[col];
            for(unsigned row=0;row<3;++row){outside[row*2]+=clip[row]<-clip[3];outside[row*2+1]+=clip[row]>clip[3];}
        }
        unsigned separated=0;for(unsigned i=0;i<6;++i)if(outside[i]==4)separated=1;if(!separated)return 0;
    }return 1;
}
/* Isolate two complete planar UI roots by private-camera depth, only when the
   foreign root is strictly behind the entire owned rectangle in BOTH eyes.
   Keep poses, view, X/Y projection and source near plane unchanged. Publication
   is atomic and still requires the original full-root exclusion predicate. */
static int activity_ui_depth_isolate(const float owned[20],const float foreign[20],float near_z,float far_z,
    const ManagedEyeMatrix views[2],ManagedEyeMatrix projections[2]) {
    if(!owned || !foreign || !views || !projections)return 0;
    float checked;
    if(!eye_optics_float(near_z,&checked) || !eye_optics_float(far_z,&checked) || !(near_z>0 && far_z>near_z))return 0;
    for(unsigned root=0;root<2;++root){const float *g=root?foreign:owned;
        for(unsigned n=0;n<20;++n)if(!eye_optics_float(g[n],&checked))return 0;
        if(!(g[18]>0 && g[19]>0) || g[3]!=0 || g[7]!=0 || g[11]!=0 || g[15]!=1)return 0;
    }
    double cameras[2][2][4][4]={0};double own_max=0,foreign_min=3.4028234663852886e38;
    for(unsigned eye=0;eye<2;++eye){
        for(unsigned n=0;n<16;++n)if(!eye_optics_float(views[eye].elements[n],&checked) ||
            !eye_optics_float(projections[eye].elements[n],&checked))return 0;
        const float *p=projections[eye].elements;
        if(p[3]!=0 || p[7]!=0 || p[11]!=-1 || p[15]!=0)return 0;
        for(unsigned root=0;root<2;++root){const float *g=root?foreign:owned;
            for(unsigned corner=0;corner<4;++corner){
                double local[4]={g[16]+(corner&1?g[18]:0),g[17]+(corner&2?g[19]:0),0,1},world[4]={0};
                double *camera=cameras[eye][root][corner];
                for(unsigned row=0;row<4;++row)for(unsigned col=0;col<4;++col)world[row]+=g[col*4+row]*local[col];
                for(unsigned row=0;row<4;++row)for(unsigned col=0;col<4;++col)camera[row]+=views[eye].elements[col*4+row]*world[col];
                for(unsigned n=0;n<4;++n)if(!eye_optics_float(camera[n],&checked))return 0;
                double depth=-camera[2];
                if(!root){if(depth>own_max)own_max=depth;}else if(depth<foreign_min)foreign_min=depth;
            }
        }
    }
    ManagedEyeMatrix result[2]={projections[0],projections[1]};
    int excluded=activity_ui_root_excluded(foreign,views,projections);
    if(!excluded){
        if(!(foreign_min>own_max && own_max>near_z))return 0;
        double limit=own_max+(foreign_min-own_max)*0.5;if(limit>far_z)limit=far_z;
        float far_plane;
        if(!eye_optics_float(limit,&far_plane) || !(far_plane>own_max && far_plane<foreign_min && far_plane>near_z && far_plane<=far_z))return 0;
        for(unsigned eye=0;eye<2;++eye){
            if(!eye_optics_float(-((double)far_plane+near_z)/((double)far_plane-near_z),&result[eye].elements[10]) ||
               !eye_optics_float(-2.0*far_plane*near_z/((double)far_plane-near_z),&result[eye].elements[14]))return 0;
        }
    }
    /* Own full rectangle must remain inside every final clip plane, including
       near/far and calibrated cant; foreign rejection alone is insufficient. */
    for(unsigned eye=0;eye<2;++eye)for(unsigned corner=0;corner<4;++corner){double clip[4]={0};
        for(unsigned row=0;row<4;++row)for(unsigned col=0;col<4;++col)clip[row]+=result[eye].elements[col*4+row]*cameras[eye][0][corner][col];
        for(unsigned n=0;n<4;++n)if(!eye_optics_float(clip[n],&checked))return 0;
        if(!(clip[3]>0))return 0;
        for(unsigned n=0;n<3;++n)if(!(clip[n]>=-clip[3] && clip[n]<=clip[3]))return 0;
    }
    if(!activity_ui_root_excluded(foreign,views,result))return 0;
    projections[0]=result[0];projections[1]=result[1];return 1;
}
/* Preserve the complete orthographic Canvas rectangle, rather than interpreting
   planeDistance as its perspective size. Only derived private camera poses are
   moved; the original root matrix/rectangle and relative widget layout stay
   untouched. Calibrated binocular offsets retain the existing2m plane depth. */
static int activity_ui_planar_optics_at_depth(const PoseMath *m,const UnityTrackingInput *input,
    const SourcePoseSnapshot *source,const float geometry[20],float minimum_depth,ManagedEyePose poses[2],
    ManagedEyeMatrix views[2],ManagedEyeMatrix projections[2]) {
    if(!m || !m->sqrt || !source || !geometry || !input)return 0;
    float checked;if(!eye_optics_float(minimum_depth,&checked) || minimum_depth<0)return 0;
    for(unsigned i=0;i<20;++i)if(!eye_optics_float(geometry[i],&checked))return 0;
    if(geometry[3]!=0 || geometry[7]!=0 || geometry[11]!=0 || geometry[15]!=1 ||
       !(geometry[18]>0 && geometry[19]>0))return 0;
    double q[4],axis[3][3],unit[3][3]={{1,0,0},{0,1,0},{0,0,1}};
    if(!pose_read(m,&source->pose,q))return 0;
    for(unsigned i=0;i<3;++i)pose_rotate(q,unit[i],axis[i]);
    double sx=m->sqrt((double)geometry[0]*geometry[0]+(double)geometry[1]*geometry[1]+(double)geometry[2]*geometry[2]);
    double sy=m->sqrt((double)geometry[4]*geometry[4]+(double)geometry[5]*geometry[5]+(double)geometry[6]*geometry[6]);
    if(!(sx>0 && sy>0))return 0;
    /* Screen-space Camera roots must be aligned with their source frame. Refuse
       sheared/tilted/mirrored geometry instead of guessing a new UI transform. */
    for(unsigned i=0;i<3;++i)if(geometry[i]/sx-axis[0][i]<-0.0002 || geometry[i]/sx-axis[0][i]>0.0002 ||
        geometry[4+i]/sy-axis[1][i]<-0.0002 || geometry[4+i]/sy-axis[1][i]>0.0002)return 0;
    double center[3],half_x=sx*geometry[18]*0.5,half_y=sy*geometry[19]*0.5;
    double local_x=geometry[16]+geometry[18]*0.5,local_y=geometry[17]+geometry[19]*0.5;
    for(unsigned i=0;i<3;++i)center[i]=geometry[12+i]+geometry[i]*local_x+geometry[4+i]*local_y;
    double depth=0;
    for(unsigned eye=0;eye<2;++eye){const EyeFrustumTangents *f=&input->tangents[eye];
        double spans[4]={-f->left,f->right,-f->down,f->up};
        for(unsigned i=0;i<4;++i){if(!(spans[i]>0) || !eye_optics_float(spans[i],&checked))return 0;
            double required=(i<2?half_x:half_y)/spans[i];if(required>depth)depth=required;
        }
    }
    /* Calibrated cant/IPD are checked below against every final corner. Margin
       keeps the full original viewport away from edge clipping; never crop it. */
    depth*=1.125;
    if(minimum_depth>depth)depth=minimum_depth;
    float plane;if(!eye_optics_float(depth,&plane) || !(plane>source->near_z && plane<source->far_z))return 0;
    ManagedEyePose front=source->pose,result[2];ManagedEyeMatrix v[2],p[2];
    for(unsigned i=0;i<3;++i)if(!eye_optics_float(center[i]-axis[2][i]*depth,&front.position[i]))return 0;
    if(!activity_ui_eye_poses(m,input,&front,plane,2.0f,result) ||
       !eye_optics_build(result,input->tangents,source->near_z,source->far_z,v,p))return 0;
    for(unsigned eye=0;eye<2;++eye)for(unsigned corner=0;corner<4;++corner){
        double world[4]={0,0,0,1},camera[4]={0},clip[4]={0};
        for(unsigned i=0;i<3;++i)world[i]=center[i]+axis[0][i]*half_x*(corner&1?1:-1)+axis[1][i]*half_y*(corner&2?1:-1);
        for(unsigned row=0;row<4;++row)for(unsigned col=0;col<4;++col)camera[row]+=v[eye].elements[col*4+row]*world[col];
        for(unsigned row=0;row<4;++row)for(unsigned col=0;col<4;++col)clip[row]+=p[eye].elements[col*4+row]*camera[col];
        if(!(clip[3]>0))return 0;
        for(unsigned row=0;row<3;++row)if(!(clip[row]>=-clip[3] && clip[row]<=clip[3]))return 0;
    }
    for(unsigned eye=0;eye<2;++eye){poses[eye]=result[eye];views[eye]=v[eye];projections[eye]=p[eye];}
    return 1;
}
static int activity_ui_planar_optics(const PoseMath *m,const UnityTrackingInput *input,
    const SourcePoseSnapshot *source,const float geometry[20],ManagedEyePose poses[2],
    ManagedEyeMatrix views[2],ManagedEyeMatrix projections[2]) {
    return activity_ui_planar_optics_at_depth(m,input,source,geometry,0,poses,views,projections);
}
/* Match the ordinary UI's complete apparent rectangle, keeping the dialogue
   layout/aspect intact. Read current source-facing geometry, not a magic
   distance, camera name, physical neutral or an original Canvas scale write. */
static int activity_ui_reference_depth(const PoseMath *m,const SourcePoseSnapshot *reference,
    const float normal[20],const float dialogue[20],float *out) {
    if(!m || !m->sqrt || !reference || !normal || !dialogue || !out)return 0;
    float checked;double extents[2][2],q[4],axes[3][3],units[3][3]={{1,0,0},{0,1,0},{0,0,1}};
    if(!pose_read(m,&reference->pose,q) || !eye_optics_float(reference->near_z,&checked) ||
       !eye_optics_float(reference->far_z,&checked) || !(reference->near_z>0 && reference->far_z>reference->near_z))return 0;
    for(unsigned i=0;i<3;++i)pose_rotate(q,units[i],axes[i]);
    for(unsigned root=0;root<2;++root){const float *g=root?dialogue:normal;
        for(unsigned i=0;i<20;++i)if(!eye_optics_float(g[i],&checked))return 0;
        if(g[3]!=0 || g[7]!=0 || g[11]!=0 || g[15]!=1 || !(g[18]>0 && g[19]>0))return 0;
        for(unsigned axis=0;axis<2;++axis){double length=m->sqrt((double)g[axis*4]*g[axis*4]+
            (double)g[axis*4+1]*g[axis*4+1]+(double)g[axis*4+2]*g[axis*4+2]);
            if(!(length>0) || !eye_optics_float(length,&checked))return 0;
            extents[root][axis]=length*g[18+axis];
            if(!(extents[root][axis]>0) || !eye_optics_float(extents[root][axis],&checked))return 0;
            if(!root)for(unsigned i=0;i<3;++i){double difference=g[axis*4+i]/length-axes[axis][i];
                if(!(difference>=-0.0002 && difference<=0.0002))return 0;
            }
        }
    }
    double depth=0,x=normal[16]+normal[18]*0.5,y=normal[17]+normal[19]*0.5;
    for(unsigned i=0;i<3;++i){double center=normal[12+i]+normal[i]*x+normal[4+i]*y;
        depth+=(center-reference->pose.position[i])*axes[2][i];}
    if(!eye_optics_float(depth,&checked) || !(depth>reference->near_z && depth<reference->far_z))return 0;
    double ratio_x=extents[1][0]/extents[0][0],ratio_y=extents[1][1]/extents[0][1];
    float result;if(!eye_optics_float(depth*(ratio_x>ratio_y?ratio_x:ratio_y),&result) || !(result>0))return 0;
    *out=result;return 1;
}
#endif
#endif
