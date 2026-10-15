/* Windows x64 universal tail thunks preserve undocumented export ABI as well as
   public APIs. Initialization does not load DLLs, wait, or resolve imports. */
typedef unsigned long DWORD;
typedef void *Module;
#ifndef GF2VR_VERSION_API
#define GF2VR_VERSION_API __declspec(dllimport)
#endif
__declspec(dllimport) DWORD __stdcall GetLastError(void);
__declspec(dllimport) void __stdcall SetLastError(DWORD);
GF2VR_VERSION_API unsigned __stdcall GetSystemDirectoryW(unsigned short *,unsigned);
GF2VR_VERSION_API Module __stdcall LoadLibraryExW(const unsigned short *,void *,DWORD);
GF2VR_VERSION_API int __stdcall FreeLibrary(Module);
GF2VR_VERSION_API void * __stdcall GetProcAddress(Module,const char *);
void *gf2vr_version_targets[17];
static Module system_version,own_module;
void gf2vr_version_set_self(Module module) { own_module=module; }
static const char *const names[17]={
 "GetFileVersionInfoA","GetFileVersionInfoByHandle","GetFileVersionInfoExA",
 "GetFileVersionInfoExW","GetFileVersionInfoSizeA","GetFileVersionInfoSizeExA",
 "GetFileVersionInfoSizeExW","GetFileVersionInfoSizeW","GetFileVersionInfoW",
 "VerFindFileA","VerFindFileW","VerInstallFileA","VerInstallFileW",
 "VerLanguageNameA","VerLanguageNameW","VerQueryValueA","VerQueryValueW"};
void *gf2vr_version_resolve(unsigned index) {
    DWORD saved=GetLastError();
    if(index>=17) { SetLastError(127); return 0; }
    Module module=__atomic_load_n(&system_version,__ATOMIC_ACQUIRE);
    if(!module) {
        unsigned short path[1024]; unsigned n=GetSystemDirectoryW(path,1024);
        if(!n || n>=1024-13) { SetLastError(122); return 0; }
        const unsigned short tail[]=L"\\version.dll";
        for(unsigned i=0;i<13;++i) path[n+i]=tail[i];
        /* Absolute system path, system-only dependencies. Never the application
           directory, CWD or first matching version.dll module. */
        module=LoadLibraryExW(path,0,0x800);
        if(!module) return 0;
        if(module==own_module) { FreeLibrary(module); SetLastError(127); return 0; }
        Module empty=0;
        if(!__atomic_compare_exchange_n(&system_version,&empty,module,0,
                                        __ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE)) {
            FreeLibrary(module); module=empty;
        }
        /* One retained module reference for the process; no unload/joins in
           detach. Concurrent first calls share Windows' loader and use no
           custom lock, once-wait or spin that can invert the loader lock. */
    }
    void *target=GetProcAddress(module,names[index]);
    if(!target) { SetLastError(127); return 0; }
    __atomic_store_n(&gf2vr_version_targets[index],target,__ATOMIC_RELEASE);
    SetLastError(saved); return target;
}
