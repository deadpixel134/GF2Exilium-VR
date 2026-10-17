#ifndef GF2_FOCUS_PROJECTION_H
#define GF2_FOCUS_PROJECTION_H
/* Exact Unity column-major camera matrices, before GPU depth/Y conversion.
   This is an eligibility guard for the evidenced TAA overwrite, not an optical
   calibration. A different projection shape must pass through unchanged. */
static int focus_projection_finite(float f) {
    union {float f;unsigned u;} b;b.f=f;return (b.u&0x7f800000u)!=0x7f800000u;
}
static float focus_projection_abs(float f) {return f<0?-f:f;}
static int focus_projection_correct(const float views[2][16],const float bases[2][16],
    const float view[16],const float incoming[16],float output[16]) {
    int selected=-1;
    for(unsigned i=0;i<16;++i)
        if(!focus_projection_finite(view[i]) || !focus_projection_finite(incoming[i])) return 0;
    for(unsigned e=0;e<2;++e) {
        int match=1;
        for(unsigned i=0;i<16;++i) {
            if(!focus_projection_finite(views[e][i]) || !focus_projection_finite(bases[e][i]) ||
               views[e][i]!=view[i] || (i!=8 && i!=9 && bases[e][i]!=incoming[i])) match=0;
        }
        if(match) {if(selected>=0) return 0;selected=(int)e;}
    }
    if(selected<0) return 0;
    const float *base=bases[selected];
#ifdef GF2VR_COMMON_STEREO_POLICY
    /* Either principal axis may provide the non-ambiguous overwrite witness.
       With |base_axis|>.02 and |jitter_axis|<.01, corrected |axis|>.01;
       the next call rejects it. Ordinary base P also fails the jitter bound.
       When BOTH axes are small, attribution stays ambiguous: pass through. */
    if((focus_projection_abs(base[8])<=0.02f && focus_projection_abs(base[9])<=0.02f) ||
       focus_projection_abs(incoming[8])>=0.01f || focus_projection_abs(incoming[9])>=0.01f ||
       base[11]!=-1 || base[15]!=0) return 0;
#else
    /* Exact accepted legacy policy when the common candidate is undefined. */
    if(focus_projection_abs(base[8])<=0.02f || focus_projection_abs(incoming[8])>=0.01f ||
       focus_projection_abs(incoming[9])>=0.01f || base[11]!=-1 || base[15]!=0) return 0;
#endif
    for(unsigned i=0;i<16;++i) output[i]=incoming[i];
    output[8]+=base[8];output[9]+=base[9];
    return selected+1;
}
#endif
