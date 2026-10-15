#ifndef GF2_CAMERA_DATA_SETTINGS_BINDING_H
#define GF2_CAMERA_DATA_SETTINGS_BINDING_H
#include "managed_eye_binding.h"
#include "camera_data_settings.h"
/* Publish no partial API. Metadata-only binding; actual managed property
   behavior and list/reference ownership are not inferred from method names. */
static int camera_data_settings_bind_contract(const ManagedEyeBinding *eye,CameraDataSettingsApi *out,
    void *(*find_class)(const char *,const char *,const char *),
    int (*size)(void *,unsigned *),unsigned char (*generic)(void *)) {
    if(!out) return 0;
    *out=(CameraDataSettingsApi){0};
    if(!eye || !eye->type_root || !find_class || !size || !generic) return 0;
    CameraDataSettingsApi a={0};a.calls=eye->api;
    a.component_class=find_class("Unity.RenderPipelines.Universal.Runtime.dll",
        "UnityEngine.Rendering.Universal","UniversalAdditionalCameraData");
    if(!a.component_class || !a.calls.invoke || !a.calls.object_class ||
       !a.calls.pin || !a.calls.unpin || !a.calls.unbox) return 0;
    for(unsigned i=0;i<CAMERA_DATA_SETTING_COUNT;++i) {
        const CameraDataSettingSpec *s=&camera_data_setting_specs[i];
        a.value_class[i]=find_class(s->image,s->ns,s->name);unsigned alignment=0;
        if(!a.value_class[i] || size(a.value_class[i],&alignment)!=(int)s->size ||
           !alignment || alignment>_Alignof(CameraDataScalar)) return 0;
        a.get[i]=eye_binding_method(a.component_class,s->get,0,s->type,0,generic);
        a.set[i]=eye_binding_method(a.component_class,s->set,s->type,"System.Void",0,generic);
        if(!a.get[i] || !a.set[i]) return 0;
    }
    a.bound=1;*out=a;return 1;
}
static int camera_data_settings_bind(const ManagedEyeBinding *eye,CameraDataSettingsApi *out) {
    if(!out) return 0;
    *out=(CameraDataSettingsApi){0};
    HINSTANCE module=GetModuleHandleW(L"GameAssembly.dll");if(!module) return 0;
    int (*size)(void *,unsigned *)=(void *)GetProcAddress(module,"il2cpp_class_value_size");
    unsigned char (*generic)(void *)=(void *)GetProcAddress(module,"il2cpp_method_is_generic");
    return camera_data_settings_bind_contract(eye,out,render_class,size,generic);
}
#endif
