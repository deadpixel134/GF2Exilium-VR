#ifndef GF2VR_RUNTIME_LOGGING_CONFIG_H
#define GF2VR_RUNTIME_LOGGING_CONFIG_H
#include "../stereo/world_drag_config.h"
/* Reuse the established bounded schema1 JSON grammar. Missing values are OFF;
   invalid/duplicate/unsupported input never enables disk logging. */
static int runtime_logging_config_parse(const char *text,unsigned length,unsigned *enabled){
 if(!text||!length||length>4096||!enabled)return 0;
 if(length>=3&&(unsigned char)text[0]==239&&(unsigned char)text[1]==187&&(unsigned char)text[2]==191){text+=3;length-=3;}
 WorldDragJson j={text,text+length,0};unsigned seen=0,value=0;
 if(!wd_json_char(&j,'{'))return 0;
 if(!wd_json_char(&j,'}'))do{
  char key[64]={0};if(!wd_json_string(&j,key,64)||!wd_json_char(&j,':'))return 0;
  if(wd_json_equal(key,"schemaVersion")){if(seen&1||!wd_json_word(&j,"1"))return 0;seen|=1;}
  else if(wd_json_equal(key,"diagnostics")){
   if(seen&2||!wd_json_char(&j,'{'))return 0;seen|=2;unsigned fields=0;
   if(!wd_json_char(&j,'}'))do{
    char name[64]={0};if(!wd_json_string(&j,name,64)||!wd_json_char(&j,':'))return 0;
    if(wd_json_equal(name,"loggingEnabled")){
     if(fields&1)return 0;fields|=1;
     if(wd_json_word(&j,"true"))value=1;else if(wd_json_word(&j,"false"))value=0;else return 0;
    }else if(!wd_json_skip(&j))return 0;
    if(wd_json_char(&j,'}'))break;if(!wd_json_char(&j,','))return 0;
   }while(1);
  }else if(!wd_json_skip(&j))return 0;
  if(wd_json_char(&j,'}'))break;if(!wd_json_char(&j,','))return 0;
 }while(1);
 wd_json_ws(&j);if(j.p!=j.end)return 0;*enabled=value;return 1;
}
#endif
