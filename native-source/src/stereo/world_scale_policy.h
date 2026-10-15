#ifndef GF2_WORLD_SCALE_POLICY_H
#define GF2_WORLD_SCALE_POLICY_H
/* W is perceived size (1 = 100%). Distances remain game units per accepted
   meter conversion. Eye/head terms use 1/W; controller speed uses W when
   linked. Never scale source transforms, XR poses/FOV or the flat composite. */
typedef struct {double world_scale;unsigned link_depth,link_movement;} WorldScaleSettings;
typedef struct {double world_scale,head_scale,depth_scale,speed_scale;} WorldScaleValues;
static WorldScaleSettings world_scale_defaults(void){return (WorldScaleSettings){1,1,1};}
static int world_scale_values(WorldScaleSettings s,double manual_depth,WorldScaleValues *out){
    if(!out||!__builtin_isfinite(s.world_scale)||s.world_scale<=0||s.link_depth>1||s.link_movement>1)return 0;
    WorldScaleValues v={s.world_scale,1/s.world_scale,s.link_depth?1/s.world_scale:manual_depth,s.link_movement?s.world_scale:1};
    if(!__builtin_isfinite(v.head_scale)||v.head_scale<=0||!__builtin_isfinite(v.depth_scale)||v.depth_scale<0||
       (s.link_depth&&v.depth_scale==0)||!__builtin_isfinite(v.speed_scale)||v.speed_scale<=0)return 0;
    *out=v;return 1;
}
static int world_scale_speed(double base,double manual,double factor,double *out){
    if(!out||!__builtin_isfinite(base)||base<0||!__builtin_isfinite(manual)||manual<0||
       !__builtin_isfinite(factor)||factor<=0)return 0;
    double speed=base*manual*factor;
    if(!__builtin_isfinite(speed)||(base>0&&manual>0&&speed==0))return 0;
    *out=speed;return 1;
}
#endif
