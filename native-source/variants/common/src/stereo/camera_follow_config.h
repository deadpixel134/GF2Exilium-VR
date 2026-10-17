#ifndef GF2_CAMERA_FOLLOW_CONFIG_H
#define GF2_CAMERA_FOLLOW_CONFIG_H
#include "world_drag_config.h"
#include "render_scale_config.h"
/* Independent optional startup selection in the existing navigation document.
   A bad follow member never disables the accepted locomotion/key contract. */
static int camera_follow_config_parse(const char *text,unsigned length,unsigned *out){
    if(!text||!length||length>4096||!out)return 0;
    if(length>=3&&(unsigned char)text[0]==239&&(unsigned char)text[1]==187&&(unsigned char)text[2]==191){text+=3;length-=3;}
    WorldDragJson j={text,text+length,0};unsigned top=0,seen=0,enabled=1;
    if(!wd_json_char(&j,'{'))return 0;
    if(!wd_json_char(&j,'}'))do{
        char key[64]={0};if(!wd_json_string(&j,key,64)||!wd_json_char(&j,':'))return 0;
        if(wd_json_equal(key,"schemaVersion")){
            const char *start=j.p;if(top&1||!wd_json_skip(&j))return 0;RenderScaleJson r={start,j.p};double v=0;
            if(!rs_number(&r,&v)||r.p!=r.end||v!=1)return 0;top|=1;
        }else if(wd_json_equal(key,"navigation")){
            if(top&2||!wd_json_char(&j,'{'))return 0;top|=2;
            if(!wd_json_char(&j,'}'))do{
                char name[64]={0};if(!wd_json_string(&j,name,64)||!wd_json_char(&j,':'))return 0;
                if(wd_json_equal(name,"cameraFollowEnabled")){
                    if(seen)return 0;seen=1;
                    if(wd_json_word(&j,"true"))enabled=1;else if(wd_json_word(&j,"false"))enabled=0;else return 0;
                }else if(!wd_json_skip(&j))return 0;
                if(wd_json_char(&j,'}'))break;if(!wd_json_char(&j,','))return 0;
            }while(1);
        }else if(!wd_json_skip(&j))return 0;
        if(wd_json_char(&j,'}'))break;if(!wd_json_char(&j,','))return 0;
    }while(1);
    wd_json_ws(&j);if(j.p!=j.end||top!=3)return 0;*out=enabled;return 1;
}
/* Main-thread first-use only, no writes/hot reload. Default/fallback is ON;
   later physical RIGHT clicks override the selection only in this process. */
static unsigned camera_follow_config_load(unsigned *loaded,unsigned *disabled,const wchar_t *path){
    if(*loaded)return 0;*loaded=1;unsigned enabled=1,status=1;
    DWORD saved=GetLastError();HANDLE f=CreateFileW(path,0x80000000u,7,0,3,128,0);
    if(f!=(HANDLE)(unsigned long long)-1){
        static char text[4097];DWORD n=0;int ok=ReadFile(f,text,sizeof(text),&n,0);CloseHandle(f);
        if(ok&&n<sizeof(text)&&camera_follow_config_parse(text,n,&enabled))status=2;else status=3;
    }else if(GetLastError()!=2&&GetLastError()!=3)status=3;
    *disabled=enabled?0:1;SetLastError(saved);return status;
}
#endif
