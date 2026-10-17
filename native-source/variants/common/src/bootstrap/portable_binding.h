#ifndef GF2VR_PORTABLE_BINDING_H
#define GF2VR_PORTABLE_BINDING_H
#include "portable_paths.h"
static Gf2vrPaths gf2vr_paths;
static unsigned short gf2vr_module_path[GF2VR_PATH_CAPACITY];
static unsigned short gf2vr_process_path[GF2VR_PATH_CAPACITY];
void gf2vr_version_set_self(HINSTANCE);
#ifndef GF2VR_PORTABLE_LOG
#define GF2VR_PORTABLE_LOG L"vrmod\\logs\\bootstrap\\BOOTSTRAP-PORTABLE-001.jsonl"
#endif
static void gf2vr_native_initialize(HINSTANCE instance) {
    gf2vr_version_set_self(instance);
    DWORD mn=GetModuleFileNameW(instance,gf2vr_module_path,GF2VR_PATH_CAPACITY);
    DWORD en=GetModuleFileNameW(NULL,gf2vr_process_path,GF2VR_PATH_CAPACITY);
    if(!mn || mn>=GF2VR_PATH_CAPACITY || !en || en>=GF2VR_PATH_CAPACITY) return;
    gf2vr_paths_build(&gf2vr_paths,gf2vr_module_path,gf2vr_process_path,GF2VR_PORTABLE_LOG);
}
#undef GF2VR_GAME_EXE
#undef GF2VR_PROBE_LOG
#undef GF2VR_XR_LOADER
#undef GF2VR_RENDER_QUALITY_CONFIG
#undef GF2VR_STEREO_DEPTH_CONFIG
#undef GF2VR_NAVIGATION_CONFIG
#undef GF2VR_WORLD_DRAG_CONFIG
#undef GF2VR_ACTIVITY_UI_CONFIG
#undef GF2VR_CONTROLLER_FACE_CONFIG_PATH
#undef GF2VR_WORLD_SCALE_CONFIG
#define GF2VR_GAME_EXE gf2vr_paths.game
#define GF2VR_PROBE_LOG gf2vr_paths.log
#define GF2VR_XR_LOADER gf2vr_paths.loader
#define GF2VR_RENDER_QUALITY_CONFIG gf2vr_paths.render
#define GF2VR_STEREO_DEPTH_CONFIG gf2vr_paths.depth
#define GF2VR_NAVIGATION_CONFIG gf2vr_paths.navigation
#define GF2VR_WORLD_DRAG_CONFIG gf2vr_paths.drag
#define GF2VR_ACTIVITY_UI_CONFIG gf2vr_paths.activity_ui
#define GF2VR_CONTROLLER_FACE_CONFIG_PATH gf2vr_paths.controller_input
#define GF2VR_WORLD_SCALE_CONFIG gf2vr_paths.world_scale
#endif
