#ifndef GF2VR_LIVE_SETTINGS_CONFIG_H
#define GF2VR_LIVE_SETTINGS_CONFIG_H
#include "live_settings_wire.h"
#include "touch2_config.h"
#include "render_scale_config.h"
#include "performance_config.h"
#include "world_scale_config.h"
#include "navigation_config.h"
#include "camera_follow_config.h"
#include "activity_ui_config.h"
#include "controller_face_config.h"
#include "aux_panel_config.h"
#include "contact_haptic_config.h"
#include "../bootstrap/runtime_logging_config.h"
typedef struct {
    double eye_scale,depth;
    WorldScaleSettings world;NavigationSettings navigation;WorldDragSettings drag;
    PerformanceConfig performance;AuxPlacementSettings panel;ContactHapticSettings haptics;
    unsigned follow,activity,face,roles,hands,logging;
} LiveSettingsValues;
static int live_settings_strict_json(const LiveSettingsDocument *d){
    Touch2Json j={(const unsigned char *)d->text,d->length,0};
    if(d->length>=3&&j.text[0]==239&&j.text[1]==187&&j.text[2]==191)j.position=3;
    touch2_json_ws(&j);
    if(j.position==j.length||j.text[j.position]!='{'||!touch2_json_value(&j,0,1))return 0;
    touch2_json_ws(&j);return j.position==j.length;
}
static int live_settings_config_parse(const LiveSettingsDocuments *docs,LiveSettingsValues *values){
    if(!docs||!values||(docs->count!=9&&docs->count!=10))return 0;
    for(unsigned i=0;i<docs->count;++i)
        if(!docs->files[i].length||docs->files[i].length>live_settings_limit(i)||!live_settings_strict_json(&docs->files[i]))return 0;
    LiveSettingsValues v={0};const LiveSettingsDocument *d=docs->files;
#define LSC_PARSE(index,fn,out) if(!fn(d[index].text,d[index].length,out))return 0
    LSC_PARSE(LSD_RENDER,render_scale_config_parse,&v.eye_scale);
    LSC_PARSE(LSD_DEPTH,stereo_depth_config_parse,&v.depth);
    LSC_PARSE(LSD_WORLD,world_scale_config_parse,&v.world);
    LSC_PARSE(LSD_NAVIGATION,navigation_config_parse,&v.navigation);
    LSC_PARSE(LSD_NAVIGATION,camera_follow_config_parse,&v.follow);
    LSC_PARSE(LSD_DRAG,world_drag_config_parse,&v.drag);
    LSC_PARSE(LSD_ACTIVITY,activity_ui_config_parse,&v.activity);
    LSC_PARSE(LSD_INPUT,aux_placement_config_parse,&v.panel);
    LSC_PARSE(LSD_INPUT,runtime_logging_config_parse,&v.logging);
    LSC_PARSE(LSD_HANDS,touch2_config_parse,&v.hands);
    LSC_PARSE(LSD_HANDS,contact_haptic_config_parse,&v.haptics);
#undef LSC_PARSE
    if(!controller_input_config_parse(d[LSD_INPUT].text,d[LSD_INPUT].length,&v.face,&v.roles))return 0;
    v.performance=performance_config_parse(d[LSD_PERFORMANCE].text,d[LSD_PERFORMANCE].length);
    if(v.performance.invalid)return 0;
    WorldScaleValues spatial;double speed;
    if(!world_scale_values(v.world,v.depth,&spatial)||
       !world_scale_speed(v.navigation.movement_speed,v.navigation.locomotion_scale,spatial.speed_scale,&speed))return 0;
    *values=v;return 1;
}
#endif
