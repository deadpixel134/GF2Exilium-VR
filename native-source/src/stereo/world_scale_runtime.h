#ifndef GF2_WORLD_SCALE_RUNTIME_H
#define GF2_WORLD_SCALE_RUNTIME_H
#include "world_scale_config.h"
/* Game-main owner. One immutable process config shared by every eligible
   source; no context override requested/proved. Generation-owned navigation
   continues to retire normally; physical reference is never touched here. */
static WorldScaleSettings stereo_world_scale_settings;
static unsigned stereo_world_scale_loaded,stereo_world_scale_valid=1;
#ifdef BOUNDARY_TEST
static const wchar_t *world_scale_test_config_path;
#endif
static void stereo_world_scale_load(void){
    if(stereo_world_scale_loaded)return;
    stereo_world_scale_loaded=1;stereo_world_scale_settings=world_scale_defaults();
    DWORD saved=GetLastError();unsigned status=1;const wchar_t *path=GF2VR_WORLD_SCALE_CONFIG;
#ifdef BOUNDARY_TEST
    if(world_scale_test_config_path)path=world_scale_test_config_path;
#endif
    HANDLE file=CreateFileW(path,0x80000000u,7,0,3,128,0);
    if(file!=(HANDLE)(U64)-1){
        static char text[4097];DWORD n=0;int ok=ReadFile(file,text,sizeof(text),&n,0);CloseHandle(file);
        if(!ok||n>=sizeof(text)||!world_scale_config_parse(text,n,&stereo_world_scale_settings))status=0;else status=2;
    }else if(GetLastError()!=2&&GetLastError()!=3)status=0;
    stereo_world_scale_valid=status!=0;
    if(!status){stereo_world_scale_settings=world_scale_defaults();stereo_world_scale_settings.link_depth=0;stereo_world_scale_settings.link_movement=0;}
    event(status?"world_scale_config_ready":"world_scale_config_invalid_neutral",status,GetCurrentThreadId());SetLastError(saved);
}
static double stereo_world_head_scale(void){stereo_world_scale_load();return 1/stereo_world_scale_settings.world_scale;}
static int stereo_world_depth(double manual,double *out){
    stereo_world_scale_load();WorldScaleValues v;
    if(!out||!world_scale_values(stereo_world_scale_settings,manual,&v))return 0;
    *out=v.depth_scale;return 1;
}
static double stereo_world_speed_scale(void){stereo_world_scale_load();return stereo_world_scale_settings.link_movement?stereo_world_scale_settings.world_scale:1;}
#endif
