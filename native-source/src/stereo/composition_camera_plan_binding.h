#ifndef GF2_COMPOSITION_CAMERA_PLAN_BINDING_H
#define GF2_COMPOSITION_CAMERA_PLAN_BINDING_H
#include "composition_camera_plan.h"
/* Requires initialized SourceApi plus exact eye_binding_method helpers. */
static int composition_camera_bind(CompositionCameraApi *out){
    if(!out || out->camera_class || !sa.runtime_invoke || !sa.object_get_class ||
       !sa.array_length || !sa.gchandle_new || !sa.gchandle_free || !sa.object_unbox)return 0;
    HINSTANCE module=GetModuleHandleW(L"GameAssembly.dll");if(!module)return 0;
    void *(*array_class)(void *,unsigned)=(void *)GetProcAddress(module,"il2cpp_array_class_get");
    unsigned char (*generic)(void *)=(void *)GetProcAddress(module,"il2cpp_method_is_generic");
    if(!array_class || !generic)return 0;
    CompositionCameraApi a={0};
    a.camera_class=render_class("UnityEngine.CoreModule.dll","UnityEngine","Camera");
    a.int_class=render_class("mscorlib.dll","System","Int32");
    void *array=render_class("mscorlib.dll","System","Array");
    void *object=render_class("UnityEngine.CoreModule.dll","UnityEngine","Object");
    if(!a.camera_class || !a.int_class || !array || !object)return 0;
    a.array_class=array_class(a.camera_class,1);
    a.get_value=eye_binding_method(array,"GetValue","System.Int32","System.Object",0,generic);
    a.get_instance_id=eye_binding_method(object,"GetInstanceID",0,"System.Int32",0,generic);
    if(!a.array_class || !a.get_value || !a.get_instance_id)return 0;
    a.object_class=sa.object_get_class;a.array_length=sa.array_length;
    a.pin=sa.gchandle_new;a.unpin=sa.gchandle_free;
    a.invoke=sa.runtime_invoke;a.unbox=sa.object_unbox;
    *out=a;return 1;
}
#endif
