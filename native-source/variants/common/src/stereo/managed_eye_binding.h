#ifdef GF2VR_STEAM_COMPAT
#line 1
#include "il2cpp_export.h"
#ifndef GF2_MANAGED_EYE_BINDING_H
#define GF2_MANAGED_EYE_BINDING_H
#include "managed_eye_camera.h"
/* Composition dependency: initialized SourceApi sa, render_class,
   render_type_name/render_free, source_equal and Win32 module lookup.
   Invoke on the attributed Unity main thread. This binds public exported APIs
   and managed method metadata; no native method-body pointer is called. */
typedef struct {ManagedEyeApi api;unsigned type_root;} ManagedEyeBinding;
static int eye_binding_type(void *type,const char *expected) {
    if(!type) return 0;
    char *name=render_type_name(type);
    int same=source_equal(name,expected);
    if(name) render_free(name);
    return same;
}
/* Exact declaring class and every argument, including RenderTexture's ctor,
   staticness, full argument and return type; overloaded/generic matches are
   rejected rather than resolved by the first-parameter-kind shortcut. */
static void *eye_binding_method_args(void *klass,const char *name,unsigned count,const char **params,
                                const char *result,int is_static,
                                unsigned char (*is_generic)(void *)) {
    if(!klass) return 0;
    void *iterator=0,*found=0;
    for(unsigned i=0;i<2048;++i) {
        void *m=sa.class_get_methods(klass,&iterator);
        if(!m) return found;
        unsigned impl=0;
        if(!source_equal(sa.method_get_name(m),name) ||
           sa.method_get_param_count(m)!=count ||
           ((sa.method_get_flags(m,&impl)&16)!=0)!=is_static || is_generic(m) ||
           !eye_binding_type(sa.method_get_return_type(m),result)) continue;
        int match=1;
        for(unsigned j=0;j<count;++j)
            if(!eye_binding_type(sa.method_get_param(m,j),params[j])) {match=0;break;}
        if(!match) continue;
        if(found) return 0;
        found=m;
    }
    return 0; /* Incomplete enumeration is not a unique binding. */
}
static void *eye_binding_method(void *klass,const char *name,const char *param,
                                const char *result,int is_static,
                                unsigned char (*is_generic)(void *)) {
    return eye_binding_method_args(klass,name,param?1u:0u,&param,result,is_static,is_generic);
}
static int eye_binding_bind(ManagedEyeBinding *b) {
    if(b->type_root || !sa.runtime_invoke || !render_type_name || !render_free) return 0;
    HINSTANCE module=GetModuleHandleW(L"GameAssembly.dll");if(!module) return 0;
    void *(*class_type)(void *)=(void *)il2cpp_export(module,"il2cpp_class_get_type");
    void *(*type_object)(void *)=(void *)il2cpp_export(module,"il2cpp_type_get_object");
    unsigned char (*is_generic)(void *)=(void *)il2cpp_export(module,"il2cpp_method_is_generic");
    int (*value_size)(void *,unsigned *)=(void *)il2cpp_export(module,"il2cpp_class_value_size");
    ManagedEyeApi a={0};
    a.object_new=(void *)il2cpp_export(module,"il2cpp_object_new");
    if(!class_type || !type_object || !is_generic || !value_size || !a.object_new) return 0;
    void *matrix=render_class("UnityEngine.CoreModule.dll","UnityEngine","Matrix4x4");
    unsigned alignment=0;
    if(!matrix || value_size(matrix,&alignment)!=(int)sizeof(ManagedEyeMatrix) ||
       !alignment || alignment>_Alignof(ManagedEyeMatrix)) return 0;
    void *vector=render_class("UnityEngine.CoreModule.dll","UnityEngine","Vector3");
    void *quaternion=render_class("UnityEngine.CoreModule.dll","UnityEngine","Quaternion");
    if(!vector || value_size(vector,&alignment)!=12 || !alignment || alignment>_Alignof(float) ||
       !quaternion || value_size(quaternion,&alignment)!=16 || !alignment || alignment>_Alignof(float)) return 0;
    a.game_object_class=render_class("UnityEngine.CoreModule.dll","UnityEngine","GameObject");
    a.camera_class=render_class("UnityEngine.CoreModule.dll","UnityEngine","Camera");
    void *behaviour=render_class("UnityEngine.CoreModule.dll","UnityEngine","Behaviour");
    void *object=render_class("UnityEngine.CoreModule.dll","UnityEngine","Object");
    a.ctor=eye_binding_method(a.game_object_class,".ctor",0,"System.Void",0,is_generic);
    a.set_active=eye_binding_method(a.game_object_class,"SetActive","System.Boolean","System.Void",0,is_generic);
    a.add_component=eye_binding_method(a.game_object_class,"AddComponent","System.Type","UnityEngine.Component",0,is_generic);
    a.copy_from=eye_binding_method(a.camera_class,"CopyFrom","UnityEngine.Camera","System.Void",0,is_generic);
    a.set_enabled=eye_binding_method(behaviour,"set_enabled","System.Boolean","System.Void",0,is_generic);
    a.set_target=eye_binding_method(a.camera_class,"set_targetTexture","UnityEngine.RenderTexture","System.Void",0,is_generic);
#ifdef GF2VR_ACTIVITY_VIEWPORT
    /* Do not make an Activity-only capability a startup requirement for Main.
       A stack requires it before publishing either private eye. */
    a.rect_class=render_class("UnityEngine.CoreModule.dll","UnityEngine","Rect");
    if(a.rect_class && value_size(a.rect_class,&alignment)==16 && alignment && alignment<=_Alignof(float)) {
        a.get_pixel_rect=eye_binding_method(a.camera_class,"get_pixelRect",0,"UnityEngine.Rect",0,is_generic);
        a.set_pixel_rect=eye_binding_method(a.camera_class,"set_pixelRect","UnityEngine.Rect","System.Void",0,is_generic);
    } else a.rect_class=0;
#endif
    a.destroy=eye_binding_method(object,"Destroy","UnityEngine.Object","System.Void",1,is_generic);
    /* Optional recovery capability: absence must retain the old conservative
       quarantine, not prevent otherwise accepted stereo startup. */
    a.bool_class=render_class("mscorlib.dll","System","Boolean");
    if(a.bool_class && value_size(a.bool_class,&alignment)==1 && alignment==1)
        a.object_alive=eye_binding_method(object,"op_Implicit","UnityEngine.Object","System.Boolean",1,is_generic);
    a.target_class=render_class("UnityEngine.CoreModule.dll","UnityEngine","RenderTexture");
    const char *ctor_params[]={"System.Int32","System.Int32","System.Int32"};
    a.target_ctor=eye_binding_method_args(a.target_class,".ctor",3,ctor_params,"System.Void",0,is_generic);
    a.target_create=eye_binding_method(a.target_class,"Create",0,"System.Boolean",0,is_generic);
    a.target_is_created=eye_binding_method(a.target_class,"IsCreated",0,"System.Boolean",0,is_generic);
    a.target_release=eye_binding_method(a.target_class,"Release",0,"System.Void",0,is_generic);
    a.set_view=eye_binding_method(a.camera_class,"set_worldToCameraMatrix","UnityEngine.Matrix4x4","System.Void",0,is_generic);
    a.set_projection=eye_binding_method(a.camera_class,"set_projectionMatrix","UnityEngine.Matrix4x4","System.Void",0,is_generic);
    a.reset_culling=eye_binding_method(a.camera_class,"ResetCullingMatrix",0,"System.Void",0,is_generic);
    void *component=render_class("UnityEngine.CoreModule.dll","UnityEngine","Component");
    void *transform=render_class("UnityEngine.CoreModule.dll","UnityEngine","Transform");
    a.get_transform=eye_binding_method(component,"get_transform",0,"UnityEngine.Transform",0,is_generic);
    const char *pose_params[]={"UnityEngine.Vector3","UnityEngine.Quaternion"};
    a.set_position_rotation=eye_binding_method_args(transform,"SetPositionAndRotation",2,pose_params,"System.Void",0,is_generic);
    if(!a.ctor || !a.set_active || !a.add_component || !a.copy_from ||
       !a.set_enabled || !a.set_target || !a.destroy || !a.target_ctor ||
       !a.target_create || !a.target_is_created || !a.target_release || !sa.object_unbox ||
       !a.set_view || !a.set_projection || !a.reset_culling || !a.get_transform || !a.set_position_rotation) return 0;
    void *type=class_type(a.camera_class);if(!type) return 0;
    a.camera_type=type_object(type);if(!a.camera_type) return 0;
    unsigned root=sa.gchandle_new(a.camera_type,1);if(!root) return 0;
    a.object_class=sa.object_get_class;a.pin=sa.gchandle_new;a.unpin=sa.gchandle_free;
    a.invoke=sa.runtime_invoke;a.unbox=sa.object_unbox;
    b->api=a;b->type_root=root;return 1;
}
static int eye_binding_release(ManagedEyeBinding *b,ManagedEyes *p) {
    if(!b->type_root) return 1;
    if(!managed_eyes_retire(&b->api,p)) return 0;
    b->api.unpin(b->type_root);b->type_root=0;b->api=(ManagedEyeApi){0};return 1;
}
#endif

#else
#line 1
#ifndef GF2_MANAGED_EYE_BINDING_H
#define GF2_MANAGED_EYE_BINDING_H
#include "managed_eye_camera.h"
/* Composition dependency: initialized SourceApi sa, render_class,
   render_type_name/render_free, source_equal and Win32 module lookup.
   Invoke on the attributed Unity main thread. This binds public exported APIs
   and managed method metadata; no native method-body pointer is called. */
typedef struct {ManagedEyeApi api;unsigned type_root;} ManagedEyeBinding;
static int eye_binding_type(void *type,const char *expected) {
    if(!type) return 0;
    char *name=render_type_name(type);
    int same=source_equal(name,expected);
    if(name) render_free(name);
    return same;
}
/* Exact declaring class and every argument, including RenderTexture's ctor,
   staticness, full argument and return type; overloaded/generic matches are
   rejected rather than resolved by the first-parameter-kind shortcut. */
static void *eye_binding_method_args(void *klass,const char *name,unsigned count,const char **params,
                                const char *result,int is_static,
                                unsigned char (*is_generic)(void *)) {
    if(!klass) return 0;
    void *iterator=0,*found=0;
    for(unsigned i=0;i<2048;++i) {
        void *m=sa.class_get_methods(klass,&iterator);
        if(!m) return found;
        unsigned impl=0;
        if(!source_equal(sa.method_get_name(m),name) ||
           sa.method_get_param_count(m)!=count ||
           ((sa.method_get_flags(m,&impl)&16)!=0)!=is_static || is_generic(m) ||
           !eye_binding_type(sa.method_get_return_type(m),result)) continue;
        int match=1;
        for(unsigned j=0;j<count;++j)
            if(!eye_binding_type(sa.method_get_param(m,j),params[j])) {match=0;break;}
        if(!match) continue;
        if(found) return 0;
        found=m;
    }
    return 0; /* Incomplete enumeration is not a unique binding. */
}
static void *eye_binding_method(void *klass,const char *name,const char *param,
                                const char *result,int is_static,
                                unsigned char (*is_generic)(void *)) {
    return eye_binding_method_args(klass,name,param?1u:0u,&param,result,is_static,is_generic);
}
static int eye_binding_bind(ManagedEyeBinding *b) {
    if(b->type_root || !sa.runtime_invoke || !render_type_name || !render_free) return 0;
    HINSTANCE module=GetModuleHandleW(L"GameAssembly.dll");if(!module) return 0;
    void *(*class_type)(void *)=(void *)GetProcAddress(module,"il2cpp_class_get_type");
    void *(*type_object)(void *)=(void *)GetProcAddress(module,"il2cpp_type_get_object");
    unsigned char (*is_generic)(void *)=(void *)GetProcAddress(module,"il2cpp_method_is_generic");
    int (*value_size)(void *,unsigned *)=(void *)GetProcAddress(module,"il2cpp_class_value_size");
    ManagedEyeApi a={0};
    a.object_new=(void *)GetProcAddress(module,"il2cpp_object_new");
    if(!class_type || !type_object || !is_generic || !value_size || !a.object_new) return 0;
    void *matrix=render_class("UnityEngine.CoreModule.dll","UnityEngine","Matrix4x4");
    unsigned alignment=0;
    if(!matrix || value_size(matrix,&alignment)!=(int)sizeof(ManagedEyeMatrix) ||
       !alignment || alignment>_Alignof(ManagedEyeMatrix)) return 0;
    void *vector=render_class("UnityEngine.CoreModule.dll","UnityEngine","Vector3");
    void *quaternion=render_class("UnityEngine.CoreModule.dll","UnityEngine","Quaternion");
    if(!vector || value_size(vector,&alignment)!=12 || !alignment || alignment>_Alignof(float) ||
       !quaternion || value_size(quaternion,&alignment)!=16 || !alignment || alignment>_Alignof(float)) return 0;
    a.game_object_class=render_class("UnityEngine.CoreModule.dll","UnityEngine","GameObject");
    a.camera_class=render_class("UnityEngine.CoreModule.dll","UnityEngine","Camera");
    void *behaviour=render_class("UnityEngine.CoreModule.dll","UnityEngine","Behaviour");
    void *object=render_class("UnityEngine.CoreModule.dll","UnityEngine","Object");
    a.ctor=eye_binding_method(a.game_object_class,".ctor",0,"System.Void",0,is_generic);
    a.set_active=eye_binding_method(a.game_object_class,"SetActive","System.Boolean","System.Void",0,is_generic);
    a.add_component=eye_binding_method(a.game_object_class,"AddComponent","System.Type","UnityEngine.Component",0,is_generic);
    a.copy_from=eye_binding_method(a.camera_class,"CopyFrom","UnityEngine.Camera","System.Void",0,is_generic);
    a.set_enabled=eye_binding_method(behaviour,"set_enabled","System.Boolean","System.Void",0,is_generic);
    a.set_target=eye_binding_method(a.camera_class,"set_targetTexture","UnityEngine.RenderTexture","System.Void",0,is_generic);
#ifdef GF2VR_ACTIVITY_VIEWPORT
    /* Do not make an Activity-only capability a startup requirement for Main.
       A stack requires it before publishing either private eye. */
    a.rect_class=render_class("UnityEngine.CoreModule.dll","UnityEngine","Rect");
    if(a.rect_class && value_size(a.rect_class,&alignment)==16 && alignment && alignment<=_Alignof(float)) {
        a.get_pixel_rect=eye_binding_method(a.camera_class,"get_pixelRect",0,"UnityEngine.Rect",0,is_generic);
        a.set_pixel_rect=eye_binding_method(a.camera_class,"set_pixelRect","UnityEngine.Rect","System.Void",0,is_generic);
    } else a.rect_class=0;
#endif
    a.destroy=eye_binding_method(object,"Destroy","UnityEngine.Object","System.Void",1,is_generic);
    /* Optional recovery capability: absence must retain the old conservative
       quarantine, not prevent otherwise accepted stereo startup. */
    a.bool_class=render_class("mscorlib.dll","System","Boolean");
    if(a.bool_class && value_size(a.bool_class,&alignment)==1 && alignment==1)
        a.object_alive=eye_binding_method(object,"op_Implicit","UnityEngine.Object","System.Boolean",1,is_generic);
    a.target_class=render_class("UnityEngine.CoreModule.dll","UnityEngine","RenderTexture");
    const char *ctor_params[]={"System.Int32","System.Int32","System.Int32"};
    a.target_ctor=eye_binding_method_args(a.target_class,".ctor",3,ctor_params,"System.Void",0,is_generic);
    a.target_create=eye_binding_method(a.target_class,"Create",0,"System.Boolean",0,is_generic);
    a.target_is_created=eye_binding_method(a.target_class,"IsCreated",0,"System.Boolean",0,is_generic);
    a.target_release=eye_binding_method(a.target_class,"Release",0,"System.Void",0,is_generic);
    a.set_view=eye_binding_method(a.camera_class,"set_worldToCameraMatrix","UnityEngine.Matrix4x4","System.Void",0,is_generic);
    a.set_projection=eye_binding_method(a.camera_class,"set_projectionMatrix","UnityEngine.Matrix4x4","System.Void",0,is_generic);
    a.reset_culling=eye_binding_method(a.camera_class,"ResetCullingMatrix",0,"System.Void",0,is_generic);
    void *component=render_class("UnityEngine.CoreModule.dll","UnityEngine","Component");
    void *transform=render_class("UnityEngine.CoreModule.dll","UnityEngine","Transform");
    a.get_transform=eye_binding_method(component,"get_transform",0,"UnityEngine.Transform",0,is_generic);
    const char *pose_params[]={"UnityEngine.Vector3","UnityEngine.Quaternion"};
    a.set_position_rotation=eye_binding_method_args(transform,"SetPositionAndRotation",2,pose_params,"System.Void",0,is_generic);
    if(!a.ctor || !a.set_active || !a.add_component || !a.copy_from ||
       !a.set_enabled || !a.set_target || !a.destroy || !a.target_ctor ||
       !a.target_create || !a.target_is_created || !a.target_release || !sa.object_unbox ||
       !a.set_view || !a.set_projection || !a.reset_culling || !a.get_transform || !a.set_position_rotation) return 0;
    void *type=class_type(a.camera_class);if(!type) return 0;
    a.camera_type=type_object(type);if(!a.camera_type) return 0;
    unsigned root=sa.gchandle_new(a.camera_type,1);if(!root) return 0;
    a.object_class=sa.object_get_class;a.pin=sa.gchandle_new;a.unpin=sa.gchandle_free;
    a.invoke=sa.runtime_invoke;a.unbox=sa.object_unbox;
    b->api=a;b->type_root=root;return 1;
}
static int eye_binding_release(ManagedEyeBinding *b,ManagedEyes *p) {
    if(!b->type_root) return 1;
    if(!managed_eyes_retire(&b->api,p)) return 0;
    b->api.unpin(b->type_root);b->type_root=0;b->api=(ManagedEyeApi){0};return 1;
}
#endif

#endif
