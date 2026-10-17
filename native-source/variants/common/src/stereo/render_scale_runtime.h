#ifndef GF2_RENDER_SCALE_RUNTIME_H
#define GF2_RENDER_SCALE_RUNTIME_H
#include "render_scale_config.h"
__declspec(dllimport) BOOL WINAPI ReadFile(HANDLE,void *,DWORD,DWORD *,void *);
/* Sole XR session-open owner. Read only once before extent publication. No
   midflight file IO or resize, and invalid config leaves stereo size unavailable. */
static int render_scale_load(const unsigned short *path,double *out){
    DWORD saved=GetLastError();double scale=0;int status=0;
    HANDLE file=CreateFileW(path,0x80000000u,7,0,3,128,0);
    if(file==(HANDLE)(unsigned long long)-1){DWORD error=GetLastError();if(error==2||error==3){scale=1.0;status=1;}else status=-1;}
    else{static char text[4097];DWORD length=0;int ok=ReadFile(file,text,sizeof(text),&length,0);CloseHandle(file);
        if(!ok)status=-1;else if(render_scale_config_parse(text,length,&scale))status=2;}
    if(status>0)*out=scale;SetLastError(saved);return status;
}
#endif
