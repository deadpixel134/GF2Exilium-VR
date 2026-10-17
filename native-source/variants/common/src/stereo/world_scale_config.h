#ifndef GF2_WORLD_SCALE_CONFIG_H
#define GF2_WORLD_SCALE_CONFIG_H
#include "world_scale_policy.h"
#include "stereo_depth_config.h"
static int world_scale_bool(WorldDragJson *j,unsigned *out){
    if(wd_json_word(j,"true")){*out=1;return 1;}
    if(wd_json_word(j,"false")){*out=0;return 1;}return 0;
}
static int world_scale_config_parse(const char *text,unsigned length,WorldScaleSettings *out){
    if(!text||!out||!length||length>4096)return 0;
    if(length>=3&&(unsigned char)text[0]==239&&(unsigned char)text[1]==187&&(unsigned char)text[2]==191){text+=3;length-=3;}
    WorldDragJson j={text,text+length,0};WorldScaleSettings s=world_scale_defaults();unsigned seen=0;
    if(!wd_json_char(&j,'{')||wd_json_char(&j,'}'))return 0;
    do{
        char key[64]={0};if(!wd_json_string(&j,key,64)||!wd_json_char(&j,':'))return 0;
        if(wd_json_equal(key,"schemaVersion")){
            double v;if((seen&1)||!stereo_depth_number(&j,&v)||v!=1)return 0;seen|=1;
        }else if(wd_json_equal(key,"worldScale")){
            if((seen&2)||!stereo_depth_number(&j,&s.world_scale))return 0;seen|=2;
        }else if(wd_json_equal(key,"linkStereoDepthToWorldScale")){
            if((seen&4)||!world_scale_bool(&j,&s.link_depth))return 0;seen|=4;
        }else if(wd_json_equal(key,"linkMovementSpeedToWorldScale")){
            if((seen&8)||!world_scale_bool(&j,&s.link_movement))return 0;seen|=8;
        }else if(!wd_json_skip(&j))return 0;
        if(wd_json_char(&j,'}'))break;if(!wd_json_char(&j,','))return 0;
    }while(1);
    WorldScaleValues v;wd_json_ws(&j);
    if(j.p!=j.end||(seen&3)!=3||!world_scale_values(s,1,&v))return 0;
    *out=s;return 1;
}
#endif
