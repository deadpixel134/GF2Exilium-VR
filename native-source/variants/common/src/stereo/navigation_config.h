#ifndef GF2_NAVIGATION_CONFIG_H
#define GF2_NAVIGATION_CONFIG_H
#include "navigation_policy.h"
#include "world_drag_config.h"
#include "render_scale_config.h"
static int navigation_json_number(WorldDragJson *j,double *out) {
    wd_json_ws(j);const char *start=j->p;
    if(start==j->end||*start<'0'||*start>'9')return 0;
    if(!wd_json_skip(j))return 0;const char *end=j->p;
    RenderScaleJson r={start,end};double number=0;
    if(rs_number(&r,&number)&&r.p==end){*out=number;return 1;}
    /* The scale parser deliberately excludes0. For speed/deadzone/turn,
       JSON zero is a valid semantic zero, never a recommendation clamp. */
    const char *coefficient=start;int all_zero=1;
    while(coefficient<end&&*coefficient!='e'&&*coefficient!='E') {
        if(*coefficient!='0'&&*coefficient!='.')all_zero=0;++coefficient;
    }
    if(all_zero){*out=0;return 1;}return 0;
}
static int navigation_config_parse(const char *text,unsigned length,NavigationSettings *out) {
    if(!text||!length||length>4096||!out)return 0;
    if(length>=3&&(unsigned char)text[0]==239&&(unsigned char)text[1]==187&&(unsigned char)text[2]==191){text+=3;length-=3;}
    WorldDragJson j={text,text+length,0};NavigationSettings cfg=navigation_defaults();unsigned top=0,seen=0;
    if(!wd_json_char(&j,'{'))return 0;
    if(!wd_json_char(&j,'}'))do {
        char key[64]={0};if(!wd_json_string(&j,key,64)||!wd_json_char(&j,':'))return 0;
        if(wd_json_equal(key,"schemaVersion")) {
            double value;if(top&1||!navigation_json_number(&j,&value)||value!=1)return 0;top|=1;
        } else if(wd_json_equal(key,"navigation")) {
            if(top&2||!wd_json_char(&j,'{'))return 0;top|=2;
            if(!wd_json_char(&j,'}'))do {
                char name[64]={0};if(!wd_json_string(&j,name,64)||!wd_json_char(&j,':'))return 0;
                const char *names[]={"locomotionEnabled","locomotionHand","turnEnabled","turnMode","movementSpeed","locomotionScale","snapAngleDegrees","smoothTurnDegreesPerSecond","stickDeadzone"};
                unsigned index=9;for(unsigned i=0;i<9;++i)if(wd_json_equal(name,names[i]))index=i;
                if(index==9){if(!wd_json_skip(&j))return 0;}
                else {
                    if(seen&(1u<<index))return 0;seen|=1u<<index;
                    if(index==0||index==2) {
                        unsigned value;if(wd_json_word(&j,"true"))value=1;else if(wd_json_word(&j,"false"))value=0;else return 0;
                        if(index==0)cfg.locomotion_enabled=value;else cfg.turn_enabled=value;
                    } else if(index==1||index==3) {
                        char value[16]={0};if(!wd_json_string(&j,value,16))return 0;
                        if(index==1){if(wd_json_equal(value,"left"))cfg.locomotion_hand=0;else if(wd_json_equal(value,"right"))cfg.locomotion_hand=1;else return 0;}
                        else{if(wd_json_equal(value,"snap"))cfg.smooth=0;else if(wd_json_equal(value,"smooth"))cfg.smooth=1;else return 0;}
                    } else {
                        double value;if(!navigation_json_number(&j,&value))return 0;
                        if(index==4)cfg.movement_speed=value;else if(index==5)cfg.locomotion_scale=value;
                        else if(index==6)cfg.snap_degrees=value;else if(index==7)cfg.smooth_degrees=value;else cfg.deadzone=value;
                    }
                }
                if(wd_json_char(&j,'}'))break;if(!wd_json_char(&j,','))return 0;
            }while(1);
        } else if(!wd_json_skip(&j))return 0;
        if(wd_json_char(&j,'}'))break;if(!wd_json_char(&j,','))return 0;
    }while(1);
    wd_json_ws(&j);if(j.p!=j.end||top!=3||!navigation_settings_valid(cfg))return 0;
    *out=cfg;return 1;
}
#endif
