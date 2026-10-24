#ifndef GF2_WORLD_DRAG_RUNTIME_H
#define GF2_WORLD_DRAG_RUNTIME_H
#include "world_drag_handoff.h"
#include "world_drag_config.h"
/* XR owns state/settings; main owns applied offset. Shared data is only POD
   distance accumulator. Never alter source/game transforms or XR reference. */
static WorldDrag stereo_drag;
static WorldDragSettings stereo_drag_settings;
static WorldDragHandoff stereo_drag_handoff;
static struct {unsigned long long epoch,generation;WorldDragVector offset;} stereo_navigation;
#ifdef BOUNDARY_TEST
static int world_drag_test_focus;
#endif
static int stereo_world_drag_game_focused(void) {
#ifdef BOUNDARY_TEST
    if(world_drag_test_focus)return world_drag_test_focus>0;
#endif
    HINSTANCE user=GetModuleHandleW(L"user32.dll");
    void *(WINAPI *foreground)(void)=user?(void *)GetProcAddress(user,"GetForegroundWindow"):0;
    DWORD (WINAPI *process)(void *,DWORD *)=user?(void *)GetProcAddress(user,"GetWindowThreadProcessId"):0;
    DWORD pid=0;void *window=foreground?foreground():0;
    return window&&process&&process(window,&pid)&&pid==GetCurrentProcessId();
}
#ifndef _WINDOWS_
__declspec(dllimport) BOOL WINAPI ReadFile(HANDLE,void *,DWORD,DWORD *,void *);
#endif
static void stereo_world_drag_open(void) {
    world_drag_reset(&stereo_drag);world_drag_handoff_clear(&stereo_drag_handoff);
    stereo_drag_settings=world_drag_defaults();
    HANDLE file=CreateFileW(GF2VR_WORLD_DRAG_CONFIG,0x80000000u,7,0,3,128,0);
    if(file==(HANDLE)(U64)-1) {
        DWORD error=GetLastError();event(error==2||error==3?"world_drag_default_disabled":"world_drag_config_read_failed",error,GetCurrentThreadId());return;
    }
    static char text[4097];DWORD length=0;int ok=ReadFile(file,text,sizeof(text),&length,0);CloseHandle(file);
    if(!ok||length>=sizeof(text)||!world_drag_config_parse(text,length,&stereo_drag_settings)) {
        stereo_drag_settings=world_drag_defaults();event("world_drag_config_invalid_disabled",length,GetCurrentThreadId());return;
    }
    event("world_drag_config_loaded",stereo_drag_settings.enabled,GetCurrentThreadId());
#ifdef GF2VR_CONTROLLER_HAND_ROLES
    if(stereo_drag_settings.enabled&&(stereo_drag_settings.leftGripActivation||stereo_drag_settings.rightGripActivation))
        event("world_drag_panel_grip_reserved",0,GetCurrentThreadId());
#else
    if(stereo_drag_settings.enabled&&stereo_drag_settings.leftGripActivation)
        event("world_drag_left_grip_reserved_for_panel",0,GetCurrentThreadId());
#endif
}
static void stereo_world_drag_reset(void) {world_drag_reset(&stereo_drag);world_drag_handoff_clear(&stereo_drag_handoff);}
/* Arbitrated by the actual presentation owner, after successful EndFrame.
   Pointer route supplies real presented-panel hit/click-enabled state,
   never merely 'pointer enabled'. Consumed flags supplied by it are retained. */
static void world_drag_arbitrate(WorldDragFrame *f,unsigned panel_hand,int grip_toggle,unsigned pointer_hand,int trigger_click,int actual_panel_hit) {
    if(grip_toggle&&panel_hand<2)f->consumed[panel_hand*2]=1;
    if(trigger_click&&actual_panel_hit&&pointer_hand<2)f->consumed[pointer_hand*2+1]=1;
}
static void stereo_world_drag_end(WorldDragFrame frame,int eligible,unsigned long long epoch,unsigned long long generation) {
    unsigned long long now=GetTickCount64();
#ifdef GF2VR_CONTROLLER_GAME_MODE
    unsigned game_consumed=controller_game_drag_consumed();
    for(unsigned n=0;n<4;++n)if(game_consumed&(1u<<n))frame.consumed[n]=1;
#endif
    eligible=eligible&&stereo_world_drag_game_focused();
#ifdef GF2VR_CAMERA_RETURN
    (void)stereo_camera_return_end(eligible&&frame.time_ms&&now>=frame.time_ms&&now-frame.time_ms<=250,
        epoch,generation,frame.time_ms,frame.sample_id);
#endif
    frame.focused=eligible;frame.immersive=eligible;
    frame.reference_epoch=epoch;frame.source_generation=generation;
    /* Both sample age and policy inter-update interval must be fresh. */
    if(!frame.time_ms||now<frame.time_ms||now-frame.time_ms>250) {stereo_world_drag_reset();return;}
    world_drag_arbitrate(&frame,
#ifdef GF2VR_CONTROLLER_HAND_ROLES
        stereo_panel_role_hand(),
#else
        0,
#endif
        1,1,0,0); /* Configured panel Grip; caller adds actual pointer hit. */
    unsigned previous=stereo_drag.initialized?stereo_drag.owner:WD_NONE;
    int released=previous<WD_NONE&&frame.active[previous]&&!frame.consumed[previous]&&
        world_drag_finite(frame.values[previous])&&frame.values[previous]<=0.25f&&stereo_drag.hand<2&&
        frame.tracked[stereo_drag.hand]&&world_drag_position_valid(frame.position[stereo_drag.hand])&&
        world_drag_sample_advances(&stereo_drag,&frame)&&frame.time_ms-stereo_drag.last_ms<=250&&
        stereo_drag.epoch==epoch&&stereo_drag.generation==generation&&world_drag_settings_equal(stereo_drag.settings,stereo_drag_settings);
    WorldDragVector hand=world_drag_update(&stereo_drag,&stereo_drag_settings,&frame);
    if(!eligible||(previous<WD_NONE&&stereo_drag.owner==WD_NONE&&!released))world_drag_handoff_clear(&stereo_drag_handoff);
    else if(stereo_drag.owner<WD_NONE&&!world_drag_handoff_add(&stereo_drag_handoff,hand,epoch,generation,frame.time_ms))stereo_world_drag_reset();
    if(previous!=stereo_drag.owner)event("world_drag_owner",stereo_drag.owner,GetCurrentThreadId());
}
/* Called on game-main from pose composition, before either managed eye is
   mutated. Basis is source/artificial yaw relative to calibrated reference,
   independent of current physical head pitch/roll. One distance consumption;
   repeated renders use the accumulated offset, never reconsume that distance. */
static int stereo_world_drag_navigation(unsigned long long epoch,unsigned long long generation,const double rotation[4],float units,float output[3]) {
#ifdef GF2VR_NAVIGATION_CONTINUITY
    stereo_navigation_continuity_pose(epoch,generation);
#endif
    if(stereo_navigation.epoch!=epoch||stereo_navigation.generation!=generation) {
        stereo_navigation.epoch=epoch;stereo_navigation.generation=generation;stereo_navigation.offset=(WorldDragVector){0};
    }
    WorldDragVector raw=world_drag_handoff_take(&stereo_drag_handoff,epoch,generation,GetTickCount64()),camera;
    float q[4];for(unsigned i=0;i<4;++i)q[i]=(float)rotation[i];
    /* Match physical head translation, retaining accumulated game displacement
       and the independently selected physical hand/buttons. */
#ifdef GF2VR_WORLD_SCALE
    double scaled_units=(double)units*stereo_world_head_scale();
    if(!__builtin_isfinite(scaled_units)||scaled_units<=0||scaled_units>3.4028234663852886e38)return 0;
    units=(float)scaled_units;if(units<=0)return 0;
#endif
    if(!world_drag_camera_delta(raw,q,units,&camera))return 0;
    WorldDragVector sum={stereo_navigation.offset.x+camera.x,stereo_navigation.offset.y+camera.y,stereo_navigation.offset.z+camera.z};
    if(!world_drag_position_valid(sum))return 0;
    stereo_navigation.offset=sum;output[0]=sum.x;output[1]=sum.y;output[2]=sum.z;return 1;
}
#define GF2VR_NAVIGATION_OFFSET(input,anchor,q,out) stereo_world_drag_navigation((input)->reference_epoch,(anchor)->source_generation,q,(anchor)->units_per_meter,out)
#endif
