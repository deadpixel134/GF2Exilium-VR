#ifndef GF2VR_PERFORMANCE_FILE_H
#define GF2VR_PERFORMANCE_FILE_H
#include "performance_config.h"
#ifndef PERFORMANCE_FILE_HOST_TEST
__declspec(dllimport) BOOL WINAPI ReadFile(HANDLE,void *,DWORD,DWORD *,void *);
#endif
static PerformanceConfig performance_config_load_buffer(const unsigned short *path,char *text,DWORD capacity) {
    DWORD saved=GetLastError();PerformanceConfig c=performance_config_default();
    HANDLE file=CreateFileW(path,0x80000000u,7,0,3,128,0);
    if(file==INVALID_HANDLE_VALUE) {
        DWORD error=GetLastError();if(error!=2&&error!=3)c.invalid=PERFORMANCE_BAD_FILE;
    } else {
        DWORD size=0;
        if(ReadFile(file,text,capacity,&size,0))c=performance_config_parse(text,size);
        else c.invalid=PERFORMANCE_BAD_FILE;
        CloseHandle(file);
    }
    SetLastError(saved);return c;
}
static PerformanceConfig performance_config_load(const unsigned short *path) {
    static char text[16385];return performance_config_load_buffer(path,text,sizeof(text));
}
#endif
