#ifndef GF2_STEREO_DEPTH_CONFIG_H
#define GF2_STEREO_DEPTH_CONFIG_H
#include "stereo_depth_policy.h"
#include "world_drag_config.h"
#include "render_scale_config.h"
static int stereo_depth_number(WorldDragJson *j,double *out) {
    wd_json_ws(j);const char *start=j->p;
    if(start==j->end||*start<'0'||*start>'9'||!wd_json_skip(j))return 0;
    RenderScaleJson r={start,j->p};double number;
    if(rs_number(&r,&number)&&r.p==j->p){*out=number;return 1;}
    /* Zero explicitly removes game-world separation, never swaps the eyes. */
    int zero=1;for(const char *p=start;p<j->p&&*p!='e'&&*p!='E';++p)if(*p!='0'&&*p!='.')zero=0;
    if(zero){*out=0;return 1;}return 0;
}
static int stereo_depth_config_parse(const char *text,unsigned length,double *out) {
    if(!text||!length||length>4096||!out)return 0;
    if(length>=3&&(unsigned char)text[0]==239&&(unsigned char)text[1]==187&&(unsigned char)text[2]==191){text+=3;length-=3;}
    WorldDragJson j={text,text+length,0};unsigned seen=0;double scale=1;
    if(!wd_json_char(&j,'{')||wd_json_char(&j,'}'))return 0;
    do {
        char key[64]={0};if(!wd_json_string(&j,key,64)||!wd_json_char(&j,':'))return 0;
        if(wd_json_equal(key,"schemaVersion")) {
            double version;if(seen&1||!stereo_depth_number(&j,&version)||version!=1)return 0;seen|=1;
        } else if(wd_json_equal(key,"stereoDepthScale")) {
            if(seen&2||!stereo_depth_number(&j,&scale))return 0;seen|=2;
        } else if(!wd_json_skip(&j))return 0;
        if(wd_json_char(&j,'}'))break;if(!wd_json_char(&j,','))return 0;
    }while(1);
    wd_json_ws(&j);if(j.p!=j.end||seen!=3||!stereo_depth_scale_valid(scale))return 0;
    *out=scale;return 1;
}
#endif
