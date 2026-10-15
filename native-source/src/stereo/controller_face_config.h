#ifndef GF2_CONTROLLER_FACE_CONFIG_H
#define GF2_CONTROLLER_FACE_CONFIG_H
#include "world_drag_config.h"
/* Version1 dedicated controllerInput JSON. Unknown fields are validated/skipped
   for future Settings; the two recognized keys cannot be duplicated. */
#ifdef GF2VR_CONTROLLER_HAND_ROLES
static int controller_input_config_parse(const char *text,unsigned length,unsigned *swap,unsigned *roles) {
#else
static int controller_face_config_parse(const char *text,unsigned length,unsigned *swap) {
#endif
    if(!text || !length || length>4096 || !swap)return 0;
    if(length>=3 && (unsigned char)text[0]==239 && (unsigned char)text[1]==187 && (unsigned char)text[2]==191){text+=3;length-=3;}
    WorldDragJson j={text,text+length,0};unsigned seen=0,value=0;
#ifdef GF2VR_CONTROLLER_HAND_ROLES
    unsigned hand_value=0;if(!roles)return 0;
#endif
    if(!wd_json_char(&j,'{'))return 0;
    if(!wd_json_char(&j,'}'))do {
        char key[64]={0};if(!wd_json_string(&j,key,64) || !wd_json_char(&j,':'))return 0;
        if(wd_json_equal(key,"schemaVersion")) {
            if(seen&1 || !wd_json_word(&j,"1"))return 0;seen|=1;
        }else if(wd_json_equal(key,"controllerInput")) {
            if(seen&2 || !wd_json_char(&j,'{'))return 0;seen|=2;unsigned fields=0;
            if(!wd_json_char(&j,'}'))do {
                char name[64]={0};if(!wd_json_string(&j,name,64) || !wd_json_char(&j,':'))return 0;
                if(wd_json_equal(name,"swapFaceButtons")) {
                    if(fields&1)return 0;fields|=1;
                    if(wd_json_word(&j,"true"))value=1;else if(wd_json_word(&j,"false"))value=0;else return 0;
                }
#ifdef GF2VR_CONTROLLER_HAND_ROLES
                else if(wd_json_equal(name,"swapPanelPointerHands")) {
                    if(fields&2)return 0;fields|=2;
                    if(wd_json_word(&j,"true"))hand_value=1;else if(wd_json_word(&j,"false"))hand_value=0;else return 0;
                }
#endif
                else if(!wd_json_skip(&j))return 0;
                if(wd_json_char(&j,'}'))break;if(!wd_json_char(&j,','))return 0;
            }while(1);
        }else if(!wd_json_skip(&j))return 0;
        if(wd_json_char(&j,'}'))break;if(!wd_json_char(&j,','))return 0;
    }while(1);
    wd_json_ws(&j);if(j.p!=j.end)return 0;*swap=value;
#ifdef GF2VR_CONTROLLER_HAND_ROLES
    *roles=hand_value;
#endif
    return 1;
}
#ifdef GF2VR_CONTROLLER_HAND_ROLES
static int controller_face_config_parse(const char *text,unsigned length,unsigned *swap) {
    unsigned roles=0;return controller_input_config_parse(text,length,swap,&roles);
}
#endif
#endif
