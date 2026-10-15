#ifndef GF2_STEREO_DEPTH_POLICY_H
#define GF2_STEREO_DEPTH_POLICY_H
/* Pure final-producer geometry. Scale the bilateral baseline about its actual
   calibrated midpoint; keep the common eye offset, head motion and navigation.
   Runtime IPD/FOV/cant and physical tracking are independent inputs. */
typedef struct {
    double eye_offset[2][3],head_translation[3];
    float anchor[3],navigation[3],units;
} StereoDepthTerms;
static int stereo_depth_scale_valid(double scale) {
    return __builtin_isfinite(scale)&&scale>=0;
}
static int stereo_depth_positions(double scale,const StereoDepthTerms *terms,
                                  const float neutral[2][3],float out[2][3]) {
    if(!terms||!neutral||!out||!stereo_depth_scale_valid(scale)||
       !__builtin_isfinite(terms->units)||terms->units<=0)return 0;
    for(unsigned i=0;i<3;++i) {
        if(!__builtin_isfinite(terms->anchor[i])||!__builtin_isfinite(terms->navigation[i])||
           !__builtin_isfinite(terms->head_translation[i]))return 0;
        for(unsigned eye=0;eye<2;++eye)
            if(!__builtin_isfinite(terms->eye_offset[eye][i])||!__builtin_isfinite(neutral[eye][i]))return 0;
    }
    float result[2][3];
    if(scale==1) {
        /* Preserve the accepted float payload, including signed zero. */
        for(unsigned eye=0;eye<2;++eye)for(unsigned i=0;i<3;++i)out[eye][i]=neutral[eye][i];
        return 1;
    }
    for(unsigned i=0;i<3;++i) {
        double left=terms->eye_offset[0][i],right=terms->eye_offset[1][i];
        double center=left*.5+right*.5;
        for(unsigned eye=0;eye<2;++eye) {
            double offset=center+scale*(terms->eye_offset[eye][i]-center);
            double position=terms->anchor[i]+terms->navigation[i]+
                (terms->head_translation[i]+offset)*terms->units;
            if(!__builtin_isfinite(offset)||!__builtin_isfinite(position)||
               position< -3.4028234663852886e38||position>3.4028234663852886e38)return 0;
            result[eye][i]=(float)position;
        }
    }
    if(scale>0) {
        int requested_separation=0,represented_separation=0;
        for(unsigned i=0;i<3;++i) {
            requested_separation|=terms->eye_offset[0][i]!=terms->eye_offset[1][i];
            represented_separation|=result[0][i]!=result[1][i];
        }
        if(requested_separation&&!represented_separation)return 0;
    }
    for(unsigned eye=0;eye<2;++eye)for(unsigned i=0;i<3;++i)out[eye][i]=result[eye][i];
    return 1;
}
#endif
