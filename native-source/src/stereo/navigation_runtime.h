#ifndef GF2_NAVIGATION_RUNTIME_H
#define GF2_NAVIGATION_RUNTIME_H
#include "navigation_config.h"
#include "navigation_handoff.h"
/* Short shared POD mailbox; no API/OS/math while locked. Main alone owns
   config and accumulated pose state. Cancellation may originate from either
   frame owner without racing XR-owned previous-validity fields. */
static NavigationHandoff stereo_navigation_input;
static NavigationState stereo_stick_navigation;
static NavigationSettings stereo_navigation_settings;
static unsigned stereo_navigation_config_loaded,stereo_navigation_reports;
#ifdef GF2VR_NAVIGATION_BLOCK_OBSERVE
#include "navigation_block_observe.h"
#endif
#ifdef BOUNDARY_TEST
static const wchar_t *navigation_test_config_path;
#endif
static void stereo_navigation_cancel(void) {
#ifdef GF2VR_CAMERA_RETURN
    camera_return_cancel(&stereo_camera_return);
#ifdef GF2VR_CAMERA_FOLLOW_TOGGLE
    camera_follow_cancel(&stereo_camera_follow);
#endif
#endif
    navigation_publish(&stereo_navigation_input,(NavigationSample){0});
}
static void stereo_navigation_end(const NavigationSample *raw,int eligible,U64 epoch,U64 generation,int inhibited) {
    NavigationSample sample=raw?*raw:(NavigationSample){0};sample.epoch=epoch;sample.generation=generation;
    sample.eligible=eligible && !inhibited && epoch && generation && sample.display_time>0 &&
        sample.tick_ms && stereo_world_drag_game_focused();
#ifdef GF2VR_NAVIGATION_BLOCK_OBSERVE
    sample.observed_eligible=eligible!=0;sample.observed_inhibited=inhibited!=0;
    sample.observed_drag_owner=stereo_drag.owner; /* existing XR-owner boundary */
#endif
#ifdef GF2VR_CONTROLLER_GAME_MODE
    if(__atomic_load_n(&controller_game_consumed,__ATOMIC_ACQUIRE)){
        unsigned h=__atomic_load_n(&controller_game_move_hand,__ATOMIC_ACQUIRE);
        if(h<2){sample.active[h]=0;sample.x[h]=sample.y[h]=0;}
    }
#endif
    navigation_publish(&stereo_navigation_input,sample);
    static unsigned reports,prior=~0u;
    unsigned flags=sample.eligible|(sample.active[0]<<1)|(sample.active[1]<<2)|
        (sample.tracked[0]<<3)|(sample.tracked[1]<<4)|((inhibited!=0)<<5)|
        ((sample.x[0]!=0||sample.y[0]!=0)<<6)|((sample.x[1]!=0||sample.y[1]!=0)<<7);
    if(flags!=prior&&reports<32){prior=flags;++reports;DWORD saved=GetLastError();event("navigation_input_state",flags,GetCurrentThreadId());SetLastError(saved);}
}
static void stereo_navigation_load(void) {
    if(stereo_navigation_config_loaded)return;
    stereo_navigation_config_loaded=1;stereo_navigation_settings=navigation_defaults();
    const wchar_t *path=GF2VR_NAVIGATION_CONFIG;
#ifdef BOUNDARY_TEST
    if(navigation_test_config_path)path=navigation_test_config_path;
#endif
    DWORD saved=GetLastError();HANDLE file=CreateFileW(path,0x80000000u,7,0,3,128,0);
    unsigned status=1;
    if(file!=(HANDLE)(U64)-1) {
        static char text[4097];DWORD n=0;int ok=ReadFile(file,text,sizeof(text),&n,0);CloseHandle(file);
        if(!ok||n>=sizeof(text)||!navigation_config_parse(text,n,&stereo_navigation_settings))status=0;
        else status=2;
    } else if(GetLastError()!=2 && GetLastError()!=3)status=0;
    if(!status){stereo_navigation_settings=navigation_defaults();stereo_navigation_settings.locomotion_enabled=stereo_navigation_settings.turn_enabled=0;}
#ifdef GF2VR_CONTROLLER_GAME_MODE
    __atomic_store_n(&controller_game_move_hand,stereo_navigation_settings.locomotion_hand,__ATOMIC_RELEASE);
#endif
    event(status?"navigation_config_ready":"navigation_config_invalid_disabled",status,GetCurrentThreadId());SetLastError(saved);
}
static int stereo_navigation_pose(const void *math,const void *input,const void *anchor,double source_yaw,
    const double *physical,double reference_yaw,double *angles,float *offset) {
    const PoseMath *m=math;const UnityTrackingInput *i=input;const PoseSourceAnchor *a=anchor;
    if(!m||!i||!a||!physical||!angles||!offset)return 0;
    stereo_navigation_load();NavigationMath functions={m->sin,m->cos,m->sqrt,m->atan2};
    NavigationOutput out;
#ifdef GF2VR_NAVIGATION_BLOCK_OBSERVE
    out=(NavigationOutput){0};
#endif
    double head_delta[3];for(unsigned n=0;n<3;++n)head_delta[n]=(double)i->head.position[n]-i->reference_head.position[n];
#ifdef GF2VR_WORLD_SCALE
    double head_scale=stereo_world_head_scale();
    for(unsigned n=0;n<3;++n)head_delta[n]*=head_scale;
    /* Use a local effective settings copy: never overwrite persisted base
       speed/manual multiplier, hand role, or input history policy. */
    NavigationSettings applied_settings=stereo_navigation_settings;
    double speed;
    if(!world_scale_speed(applied_settings.movement_speed,applied_settings.locomotion_scale,stereo_world_speed_scale(),&speed)) {
        applied_settings.movement_speed=__builtin_nan("");
    }else{applied_settings.movement_speed=speed;applied_settings.locomotion_scale=1;}
#else
#define applied_settings stereo_navigation_settings
#endif
    DWORD saved=GetLastError();U64 now=GetTickCount64();SetLastError(saved);
    NavigationSample sample=navigation_read_for_pose(&stereo_navigation_input,i->reference_epoch,
        a->source_generation,now,i->display_time,stereo_navigation_settings);
#ifdef GF2VR_NAVIGATION_BLOCK_OBSERVE
    NavigationState before=stereo_stick_navigation;
#endif
#ifdef GF2VR_CONTROLLER_GAME_MODE
    /* Suppress historical packets too: main may already have pinned an older
       eye time when XR switches destinations. Turn hand is left intact. */
    if(__atomic_load_n(&controller_game_consumed,__ATOMIC_ACQUIRE)){
        unsigned h=stereo_navigation_settings.locomotion_hand;
        sample.active[h]=0;sample.x[h]=sample.y[h]=0;stereo_stick_navigation.move_armed=0;
    }
#endif
#ifdef GF2VR_CAMERA_RETURN
    if(!stereo_camera_return_pose(m,i,a,physical,source_yaw,reference_yaw,&applied_settings,&sample,now))return 0;
#endif
    int updated=navigation_update(&stereo_stick_navigation,applied_settings,&functions,&sample,
        i->reference_epoch,a->source_generation,now,i->display_time,source_yaw,a->pitch,
        physical[0],physical[1],head_delta,reference_yaw,a->units_per_meter,&out);
#ifdef GF2VR_NAVIGATION_BLOCK_OBSERVE
    navigation_block_observe(&before,&stereo_stick_navigation,&sample,applied_settings,i,a,now,&out,updated);
#endif
    if(!updated)return 0;
    angles[0]=out.angles[0];angles[1]=out.angles[1];for(unsigned n=0;n<3;++n)offset[n]=out.offset[n];
    if((out.moved||out.turned||out.rejected)&&stereo_navigation_reports<32) {
        ++stereo_navigation_reports;DWORD error=GetLastError();static char line[1800];char *p=source_prefix(line,"navigation_applied");
        p=literal(p,",\"epoch\":");p=hexadecimal(p,i->reference_epoch);
        p=literal(p,",\"generation\":");p=hexadecimal(p,a->source_generation);
        p=literal(p,",\"frame\":");p=hexadecimal(p,i->frame_id);
        p=literal(p,",\"sample\":");p=hexadecimal(p,sample.sequence);
        p=literal(p,",\"locomotion_hand\":");p=decimal(p,stereo_navigation_settings.locomotion_hand);
        p=literal(p,",\"moved\":");p=decimal(p,out.moved);p=literal(p,",\"turned\":");p=decimal(p,out.turned);
        p=literal(p,",\"rejected\":");p=decimal(p,out.rejected);
#ifdef GF2VR_WORLD_SCALE
        p=literal(p,",\"world_base_manual_multiplier_f64\":[");
        double scale_values[]={stereo_world_scale_settings.world_scale,stereo_navigation_settings.movement_speed,stereo_navigation_settings.locomotion_scale,stereo_world_speed_scale()};
        for(unsigned n=0;n<4;++n){union{double value;U64 bits;}v={.value=scale_values[n]};if(n)p=literal(p,",");p=hexadecimal(p,v.bits);}
        p=literal(p,"]");
#endif
        p=literal(p,",\"dt_speed_yaw_pitch_f64\":[");
        double evidence[]={out.dt,out.effective_speed,source_yaw,a->pitch,physical[0],physical[1]};
        for(unsigned n=0;n<6;++n){union{double value;U64 bits;} v={.value=evidence[n]};if(n)p=literal(p,",");p=hexadecimal(p,v.bits);}
        p=literal(p,"],\"axes_f32\":[");
        float axes[]={sample.x[0],sample.y[0],sample.x[1],sample.y[1]};
        for(unsigned n=0;n<4;++n){union{float value;unsigned bits;} v={.value=axes[n]};if(n)p=literal(p,",");p=hexadecimal(p,v.bits);}
        p=literal(p,"]");
        p=literal(p,",\"angles_f64\":[");
        for(unsigned n=0;n<2;++n){union{double value;U64 bits;} v={.value=angles[n]};if(n)p=literal(p,",");p=hexadecimal(p,v.bits);}
        p=literal(p,"],\"offset_f32\":[");
        for(unsigned n=0;n<3;++n){union{float value;unsigned bits;} v={.value=offset[n]};if(n)p=literal(p,",");p=hexadecimal(p,v.bits);}
        p=literal(p,"]}\n");source_log(line,p);SetLastError(error);
    }
    return 1;
}
#ifndef GF2VR_WORLD_SCALE
#undef applied_settings
#endif
#endif
