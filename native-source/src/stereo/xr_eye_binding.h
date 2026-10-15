#ifndef GF2_XR_EYE_BINDING_H
#define GF2_XR_EYE_BINDING_H
#include "xr_eye_owner.h"
/* Existing bootstrap Win32 declarations provide module lookup/export lookup.
   Acquire a reference to an already-loaded system CRT; never search/load a DLL
   by relative path. Retain it until the XR owner and all math calls drain. */
__declspec(dllimport) BOOL WINAPI FreeLibrary(HINSTANCE);
typedef struct {HINSTANCE math_module;XrEyeOwnerApi api;} XrEyeBinding;
static int xr_eye_binding_open(XrEyeBinding *b,PFN_xrGetInstanceProcAddr get,XrInstance instance) {
    if(!b || b->math_module) return 0;
    HINSTANCE module=0;
    if(!GetModuleHandleExW(0,L"ucrtbase.dll",&module)) return 0;
    double (*tangent)(double)=(void *)GetProcAddress(module,"tan");
    XrEyeOwnerApi api={0};
    if(!xr_eye_owner_bind(get,instance,tangent,&api)) {
        if(!FreeLibrary(module)) b->math_module=module;
        return 0;
    }
    b->math_module=module;b->api=api;return 1;
}
static int xr_eye_binding_close(XrEyeBinding *b,XrEyeOwner *owner) {
    if(!b || !owner || !xr_eye_owner_close(owner)) return 0;
    if(b->math_module && !FreeLibrary(b->math_module)) return 0;
    *b=(XrEyeBinding){0};return 1;
}
#endif
