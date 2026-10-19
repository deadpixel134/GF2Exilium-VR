#ifndef GF2VR_RUNTIME_LOGGING_H
#define GF2VR_RUNTIME_LOGGING_H
#include "runtime_logging_state.h"
#ifdef GF2VR_OPTIONAL_LOGGING
#include "runtime_logging_config.h"
unsigned gf2vr_runtime_logging_enabled;
__declspec(dllimport) BOOL WINAPI ReadFile(HANDLE,void *,DWORD,DWORD *,void *);
__declspec(dllimport) BOOL WINAPI CreateDirectoryW(const unsigned short *,void *);
static void gf2vr_logging_initialize(void){
 DWORD saved=GetLastError();unsigned value=0;
 if(gf2vr_paths.ready){
  HANDLE file=CreateFileW(GF2VR_CONTROLLER_FACE_CONFIG_PATH,0x80000000u,7,0,3,128,0);
  if(file!=INVALID_HANDLE_VALUE){
   static char text[4097];DWORD length=0;BOOL ok=ReadFile(file,text,sizeof(text),&length,0);CloseHandle(file);
   if(!ok||length>=sizeof(text)||!runtime_logging_config_parse(text,length,&value))value=0;
  }
 }
 if(value){
  unsigned short path[GF2VR_PATH_CAPACITY];unsigned n=gf2vr_path_length(gf2vr_paths.root);
  if(n&&n<GF2VR_PATH_CAPACITY){
   if(gf2vr_path_join(path,gf2vr_paths.root,n,L"vrmod\\logs"))CreateDirectoryW(path,0);
   if(gf2vr_path_join(path,gf2vr_paths.root,n,L"vrmod\\logs\\bootstrap"))CreateDirectoryW(path,0);
  }
 }
 __atomic_store_n(&gf2vr_runtime_logging_enabled,value,__ATOMIC_RELEASE);SetLastError(saved);
}
#else
#define gf2vr_logging_initialize() ((void)0)
#endif
#endif
