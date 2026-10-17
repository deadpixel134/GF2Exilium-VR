#ifndef GF2_STEREO_DEPTH_RUNTIME_H
#define GF2_STEREO_DEPTH_RUNTIME_H
#include "stereo_depth_config.h"
/* Unity-main owner, no XR/action/source/reference/resource ownership changes.
   Configuration applies at game restart. Invalid requests keep neutral output
   and emit explicit failure; the persisted setting is never clamped/rewritten. */
static double stereo_depth_requested=1;
static unsigned stereo_depth_loaded,stereo_depth_config_valid=1;
static struct {
    U64 epoch,generation,frame;
    double requested,effective,baseline_m,baseline_game;
    float neutral[2][3],applied[2][3],units;
    unsigned rejected;
} stereo_depth_prepared;
static void stereo_depth_load(void) {
    if(stereo_depth_loaded)return;
    stereo_depth_loaded=1;DWORD saved=GetLastError();unsigned status=1;
    HANDLE file=CreateFileW(GF2VR_STEREO_DEPTH_CONFIG,0x80000000u,7,0,3,128,0);
    if(file!=(HANDLE)(U64)-1) {
        static char text[4097];DWORD n=0;int ok=ReadFile(file,text,sizeof(text),&n,0);CloseHandle(file);
        if(!ok||n>=sizeof(text)||!stereo_depth_config_parse(text,n,&stereo_depth_requested))status=0;
        else status=2;
    }else if(GetLastError()!=2&&GetLastError()!=3)status=0;
    stereo_depth_config_valid=status!=0;
    if(!status)stereo_depth_requested=1;
    event(status?"stereo_depth_config_ready":"stereo_depth_config_invalid_disabled",status,GetCurrentThreadId());
    SetLastError(saved);
}
static int stereo_depth_pose(const void *math,const void *input,const void *anchor,
    const double eye_offsets[2][3],const double *translation,const float *navigation,void *poses) {
    const PoseMath *m=math;const UnityTrackingInput *i=input;const PoseSourceAnchor *a=anchor;ManagedEyePose *p=poses;
    if(!m||!m->sqrt||!i||!a||!eye_offsets||!translation||!navigation||!p)return 0;
    stereo_depth_load();StereoDepthTerms terms={0};terms.units=a->units_per_meter;
    float neutral[2][3],scaled[2][3];double square=0;
    for(unsigned axis=0;axis<3;++axis) {
        terms.anchor[axis]=a->position[axis];terms.navigation[axis]=navigation[axis];terms.head_translation[axis]=translation[axis];
        double difference=eye_offsets[1][axis]-eye_offsets[0][axis];square+=difference*difference;
        for(unsigned eye=0;eye<2;++eye){terms.eye_offset[eye][axis]=eye_offsets[eye][axis];neutral[eye][axis]=p[eye].position[axis];}
    }
#ifdef GF2VR_WORLD_SCALE
    double requested=stereo_depth_requested;
    if(!stereo_world_depth(requested,&requested))return 0;
#else
#define GF2VR_DEPTH_REQUEST stereo_depth_requested
#endif
#ifdef GF2VR_WORLD_SCALE
#define GF2VR_DEPTH_REQUEST requested
#endif
    int ok=stereo_depth_positions(GF2VR_DEPTH_REQUEST,&terms,neutral,scaled);
    /* Three bounded rotation coefficients cannot overflow view translation
       for ordinary positions. For extreme representable values, validate the
       existing pure optics backend before committing either scaled eye. */
    int extreme=0;
    if(ok&&GF2VR_DEPTH_REQUEST!=1)for(unsigned eye=0;eye<2;++eye)for(unsigned axis=0;axis<3;++axis)
        extreme|=scaled[eye][axis]>8.5070586659632215e37f||scaled[eye][axis]< -8.5070586659632215e37f;
    if(extreme) {
        ManagedEyePose candidate[2]={p[0],p[1]};ManagedEyeMatrix views[2],projections[2];
        for(unsigned eye=0;eye<2;++eye)for(unsigned axis=0;axis<3;++axis)candidate[eye].position[axis]=scaled[eye][axis];
        ok=eye_optics_build(candidate,i->tangents,1,2,views,projections);
    }
    stereo_depth_prepared.epoch=i->reference_epoch;stereo_depth_prepared.generation=a->source_generation;
    stereo_depth_prepared.frame=i->frame_id;stereo_depth_prepared.requested=GF2VR_DEPTH_REQUEST;
    stereo_depth_prepared.effective=ok?GF2VR_DEPTH_REQUEST:1;
    stereo_depth_prepared.rejected=!ok||!stereo_depth_config_valid;stereo_depth_prepared.units=a->units_per_meter;
#ifdef GF2VR_WORLD_SCALE
    stereo_depth_prepared.rejected|=!stereo_world_scale_valid;
#endif
    stereo_depth_prepared.baseline_m=m->sqrt(square);
    stereo_depth_prepared.baseline_game=stereo_depth_prepared.baseline_m*a->units_per_meter*stereo_depth_prepared.effective;
    for(unsigned eye=0;eye<2;++eye)for(unsigned axis=0;axis<3;++axis) {
        stereo_depth_prepared.neutral[eye][axis]=neutral[eye][axis];
        if(ok)p[eye].position[axis]=scaled[eye][axis];
        stereo_depth_prepared.applied[eye][axis]=p[eye].position[axis];
    }
#ifdef GF2VR_WORLD_SCALE
    /* Non-neutral world geometry must fail atomically, not mix a scaled head
       with an unrepresentable binocular baseline. Existing neutral/manual
       depth failure keeps its documented neutral fallback. */
    if(!ok&&stereo_world_scale_settings.world_scale!=1)return 0;
#endif
    return 1; /* Explicit failed-control neutral fallback, not stale/flat output. */
}
#undef GF2VR_DEPTH_REQUEST
/* Only called after successful managed optics/pose setters and current-mailbox
   validation. This proves private CPU geometry, not render/GPU/End/pixels. */
static void stereo_depth_applied(const void *input,const void *anchor,const void *poses,const void *ticket) {
    const UnityTrackingInput *i=input;const PoseSourceAnchor *a=anchor;const ManagedEyePose *p=poses;const PoseAppliedTicket *t=ticket;
    static unsigned reports;static U64 epoch,generation,frame,last_ms;
    if(!i||!a||!p||!t||t->reference_epoch!=stereo_depth_prepared.epoch||
       t->source_generation!=stereo_depth_prepared.generation||t->frame_id!=stereo_depth_prepared.frame)return;
    DWORD saved=GetLastError();U64 now=GetTickCount64();
    if(reports>=32||(epoch==t->reference_epoch&&generation==t->source_generation&&
        (frame==t->frame_id||now-last_ms<500))){SetLastError(saved);return;}
    for(unsigned eye=0;eye<2;++eye)for(unsigned axis=0;axis<3;++axis)
        if(p[eye].position[axis]!=stereo_depth_prepared.applied[eye][axis]){SetLastError(saved);return;}
    ++reports;epoch=t->reference_epoch;generation=t->source_generation;frame=t->frame_id;last_ms=now;
    char line[2000],*q=source_prefix(line,"stereo_depth_applied");
    q=literal(q,",\"epoch\":");q=hexadecimal(q,epoch);q=literal(q,",\"generation\":");q=hexadecimal(q,generation);
    q=literal(q,",\"frame\":");q=hexadecimal(q,frame);q=literal(q,",\"rejected\":");q=decimal(q,stereo_depth_prepared.rejected);
#ifdef GF2VR_WORLD_SCALE
    q=literal(q,",\"world_head_manual_depth_speed_f64\":[");
    double spatial_values[]={stereo_world_scale_settings.world_scale,stereo_world_head_scale(),stereo_depth_requested,stereo_world_speed_scale()};
    for(unsigned n=0;n<4;++n){union{double value;U64 bits;}v={.value=spatial_values[n]};if(n)q=literal(q,",");q=hexadecimal(q,v.bits);}
    q=literal(q,"],\"world_links\":");q=decimal(q,stereo_world_scale_settings.link_depth|(stereo_world_scale_settings.link_movement<<1));
#endif
    q=literal(q,",\"units_f32\":");union{float value;unsigned bits;}units={.value=stereo_depth_prepared.units};q=hexadecimal(q,units.bits);
    q=literal(q,",\"ratio_baseline_f64\":[");
    double values[]={stereo_depth_prepared.requested,stereo_depth_prepared.effective,stereo_depth_prepared.baseline_m,stereo_depth_prepared.baseline_game};
    for(unsigned n=0;n<4;++n){union{double value;U64 bits;}v={.value=values[n]};if(n)q=literal(q,",");q=hexadecimal(q,v.bits);}
    q=literal(q,"],\"neutral_positions_f32\":[");
    for(unsigned eye=0;eye<2;++eye)for(unsigned axis=0;axis<3;++axis){union{float value;unsigned bits;}v={.value=stereo_depth_prepared.neutral[eye][axis]};if(eye||axis)q=literal(q,",");q=hexadecimal(q,v.bits);}
    q=literal(q,"],\"applied_positions_f32\":[");
    for(unsigned eye=0;eye<2;++eye)for(unsigned axis=0;axis<3;++axis){union{float value;unsigned bits;}v={.value=p[eye].position[axis]};if(eye||axis)q=literal(q,",");q=hexadecimal(q,v.bits);}
    q=literal(q,"]}\n");source_log(line,q);SetLastError(saved);
}
#endif
