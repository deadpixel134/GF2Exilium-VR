#ifndef GF2_AUX_PANEL_CONFIG_H
#define GF2_AUX_PANEL_CONFIG_H
#include "aux_panel_placement.h"
#include "stereo_depth_config.h"
/* Optional auxiliaryPanel object in existing controller_input.json. Missing
   object preserves the deployed pose exactly; new settings are restart-applied. */
static int aux_placement_number(WorldDragJson *j,double *out) {
    wd_json_ws(j);unsigned negative=j->p<j->end&&*j->p=='-';if(negative)++j->p;
    double value;if(!stereo_depth_number(j,&value))return 0;
    value=negative?-value:value;float checked;if(!aux_placement_float(value,&checked))return 0;*out=value;return 1;
}
static int aux_placement_config_parse(const char *text,unsigned length,AuxPlacementSettings *out) {
    if(!text||!out||!length||length>4096)return 0;
    if(length>=3&&(unsigned char)text[0]==239&&(unsigned char)text[1]==187&&(unsigned char)text[2]==191){text+=3;length-=3;}
    WorldDragJson j={text,text+length,0};AuxPlacementSettings s=aux_placement_defaults();unsigned root=0;
    if(!wd_json_char(&j,'{'))return 0;
    if(!wd_json_char(&j,'}'))do {
        char key[64]={0};if(!wd_json_string(&j,key,64)||!wd_json_char(&j,':'))return 0;
        if(wd_json_equal(key,"schemaVersion")) {
            if((root&1)||!wd_json_word(&j,"1"))return 0;root|=1;
        }else if(wd_json_equal(key,"auxiliaryPanel")) {
            if((root&2)||!wd_json_char(&j,'{'))return 0;root|=2;s.configured=1;unsigned seen=0;
            if(!wd_json_char(&j,'}'))do {
                char name[64]={0};if(!wd_json_string(&j,name,64)||!wd_json_char(&j,':'))return 0;
                const char *names[]={"viewerFacing","offsetX","offsetY","offsetZ","rotationPitch","rotationYaw","rotationRoll","maximumWidth","maximumHeight","screenTilt","applyAnglesWhenFacing"};
                unsigned k=0;while(k<11&&!wd_json_equal(name,names[k]))++k;
                if(k==11){if(!wd_json_skip(&j))return 0;}
                else {
                    if(seen&(1u<<k))return 0;seen|=1u<<k;
                    if(k==0||k==10){unsigned *flag=k==0?&s.viewer_facing:&s.apply_angles_when_facing;
                        if(wd_json_word(&j,"true"))*flag=1;else if(wd_json_word(&j,"false"))*flag=0;else return 0;}
                    else {double *field=k<4?&s.offset[k-1]:k<7?&s.rotation[k-4]:k==7?&s.maximum_width:k==8?&s.maximum_height:&s.screen_tilt;
                        if(!aux_placement_number(&j,field))return 0;}
                }
                if(wd_json_char(&j,'}'))break;if(!wd_json_char(&j,','))return 0;
            }while(1);
        }else if(!wd_json_skip(&j))return 0;
        if(wd_json_char(&j,'}'))break;if(!wd_json_char(&j,','))return 0;
    }while(1);
    wd_json_ws(&j);if(j.p!=j.end||!aux_placement_settings_valid(&s))return 0;
    *out=s;return 1;
}
#endif
