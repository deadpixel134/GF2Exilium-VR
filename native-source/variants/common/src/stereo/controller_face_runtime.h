#ifndef GF2_CONTROLLER_FACE_RUNTIME_H
#define GF2_CONTROLLER_FACE_RUNTIME_H
#include "controller_face_config.h"
/* XR action-set owner only, once per newly created action transport. Bindings
   are fixed before attachment; no file polling/hot rebind of held input. */
#ifdef GF2VR_CONTROLLER_HAND_ROLES
static unsigned controller_input_config_load(unsigned *roles) {
#else
static unsigned controller_face_config_load(void) {
#endif
    DWORD saved=GetLastError();unsigned swap=0,status=1;
#ifdef GF2VR_CONTROLLER_HAND_ROLES
    *roles=0;
#endif
    HANDLE file=CreateFileW(GF2VR_CONTROLLER_FACE_CONFIG_PATH,0x80000000u,7,0,3,128,0);
    if(file!=(HANDLE)(unsigned long long)-1) {
        static char text[4097];DWORD length=0;
        int read=ReadFile(file,text,sizeof(text),&length,0);CloseHandle(file);
#ifdef GF2VR_CONTROLLER_HAND_ROLES
        if(!read || length>=sizeof(text) || !controller_input_config_parse(text,length,&swap,roles)){status=0;swap=*roles=0;}
#else
        if(!read || length>=sizeof(text) || !controller_face_config_parse(text,length,&swap)){status=0;swap=0;}
#endif
        else status=2;
    }else if(GetLastError()!=2 && GetLastError()!=3)status=0;
    unsigned flags=swap;
#ifdef GF2VR_CONTROLLER_HAND_ROLES
    flags|=*roles<<1;
#endif
    event("controller_face_config",((unsigned long long)status<<32)|flags,GetCurrentThreadId());
    SetLastError(saved);return swap;
}
#ifdef GF2VR_CONTROLLER_HAND_ROLES
static unsigned controller_face_config_load(void) {unsigned roles=0;return controller_input_config_load(&roles);}
#endif
#endif
