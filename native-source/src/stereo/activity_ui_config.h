#ifndef GF2_ACTIVITY_UI_CONFIG_H
#define GF2_ACTIVITY_UI_CONFIG_H
#include "world_drag_config.h"
static int activity_ui_config_parse(const char *text,unsigned length,unsigned *enabled) {
    if(!text || !length || length>4096 || !enabled)return 0;
    if(length>=3 && (unsigned char)text[0]==239 && (unsigned char)text[1]==187 && (unsigned char)text[2]==191){text+=3;length-=3;}
    WorldDragJson j={text,text+length,0};unsigned seen=0,value=1;
    if(!wd_json_char(&j,'{'))return 0;
    if(!wd_json_char(&j,'}'))do {
        char key[64]={0};if(!wd_json_string(&j,key,64) || !wd_json_char(&j,':'))return 0;
        if(wd_json_equal(key,"activityPrimaryUiEnabled")) {
            if(seen)return 0;seen=1;
            if(wd_json_word(&j,"true"))value=1;else if(wd_json_word(&j,"false"))value=0;else return 0;
        }else if(!wd_json_skip(&j))return 0;
        if(wd_json_char(&j,'}'))break;if(!wd_json_char(&j,','))return 0;
    }while(1);
    wd_json_ws(&j);if(j.p!=j.end)return 0;*enabled=value;return 1;
}
#endif
