#ifndef GF2_SOURCE_POSE_READER_H
#define GF2_SOURCE_POSE_READER_H
#include "managed_eye_binding.h"
#ifndef GF2VR_ADMISSION_STEP
#define GF2VR_ADMISSION_STEP(n) ((void)0)
#endif
/* Read-only snapshot from a caller-attributed source on the Unity main thread.
   Camera identity alone is NOT source eligibility; no selection logic here. */
typedef struct {
    ManagedEyeApi calls;
    void *get_position,*get_rotation,*get_near,*get_far;
    void *vector_class,*quaternion_class,*float_class;
} SourcePoseApi;
typedef struct {ManagedEyePose pose;float near_z,far_z;} SourcePoseSnapshot;
static int source_pose_bind(const ManagedEyeBinding *eye,SourcePoseApi *out) {
    if(!out) return 0;
    *out=(SourcePoseApi){0};
    if(!eye || !eye->type_root) return 0;
    HINSTANCE module=GetModuleHandleW(L"GameAssembly.dll");if(!module) return 0;
    unsigned char (*generic)(void *)=(void *)GetProcAddress(module,"il2cpp_method_is_generic");
    int (*size)(void *,unsigned *)=(void *)GetProcAddress(module,"il2cpp_class_value_size");
    if(!generic || !size) return 0;
    SourcePoseApi a={0};a.calls=eye->api;
    void *transform=render_class("UnityEngine.CoreModule.dll","UnityEngine","Transform");
    a.vector_class=render_class("UnityEngine.CoreModule.dll","UnityEngine","Vector3");
    a.quaternion_class=render_class("UnityEngine.CoreModule.dll","UnityEngine","Quaternion");
    a.float_class=render_class("mscorlib.dll","System","Single");
    unsigned alignment=0;
    if(!a.vector_class || !a.quaternion_class || !a.float_class ||
       size(a.vector_class,&alignment)!=12 || !alignment || alignment>_Alignof(float) ||
       size(a.quaternion_class,&alignment)!=16 || !alignment || alignment>_Alignof(float) ||
       size(a.float_class,&alignment)!=4 || !alignment || alignment>_Alignof(float)) return 0;
    a.get_position=eye_binding_method(transform,"get_position",0,"UnityEngine.Vector3",0,generic);
    a.get_rotation=eye_binding_method(transform,"get_rotation",0,"UnityEngine.Quaternion",0,generic);
    a.get_near=eye_binding_method(a.calls.camera_class,"get_nearClipPlane",0,"System.Single",0,generic);
    a.get_far=eye_binding_method(a.calls.camera_class,"get_farClipPlane",0,"System.Single",0,generic);
    if(!a.get_position || !a.get_rotation || !a.get_near || !a.get_far) return 0;
    *out=a;return 1;
}
static int source_pose_value(const SourcePoseApi *a,void *method,void *object,void *klass,void *out,unsigned size) {
    void *boxed=0;
    if(!managed_eye_call(&a->calls,method,object,0,&boxed) || !boxed) return 0;
    unsigned root=a->calls.pin(boxed,1);if(!root) return 0;
    void *value=0;
    if(a->calls.object_class(boxed)==klass) value=a->calls.unbox(boxed);
    if(value) for(unsigned i=0;i<size;++i) ((unsigned char *)out)[i]=((unsigned char *)value)[i];
    a->calls.unpin(root);return value!=0;
}
static int source_pose_read(const SourcePoseApi *a,void *source,SourcePoseSnapshot *out) {
    if(!a || !source || !out || !a->calls.pin || !a->calls.unpin || !a->calls.invoke ||
       !a->calls.object_class || !a->calls.unbox || !a->calls.get_transform ||
       !a->get_position || !a->get_rotation || !a->get_near || !a->get_far ||
       a->calls.object_class(source)!=a->calls.camera_class) return 0;
    unsigned source_root=a->calls.pin(source,1);if(!source_root) return 0;
    SourcePoseSnapshot snapshot={0};unsigned transform_root=0;void *transform=0;
    int ok=managed_eye_call(&a->calls,a->calls.get_transform,source,0,&transform) && transform;
    if(ok) {transform_root=a->calls.pin(transform,1);ok=transform_root!=0;}
    if(ok) ok=source_pose_value(a,a->get_position,transform,a->vector_class,snapshot.pose.position,12) &&
              source_pose_value(a,a->get_rotation,transform,a->quaternion_class,snapshot.pose.rotation,16) &&
              source_pose_value(a,a->get_near,source,a->float_class,&snapshot.near_z,4) &&
              source_pose_value(a,a->get_far,source,a->float_class,&snapshot.far_z,4);
    if(transform_root) a->calls.unpin(transform_root);
    a->calls.unpin(source_root);
    if(!ok) return 0;
    for(unsigned i=0;i<9;++i) {
        float f=i<3?snapshot.pose.position[i]:i<7?snapshot.pose.rotation[i-3]:i==7?snapshot.near_z:snapshot.far_z;
        union {float value;unsigned bits;} v;v.value=f;
        if((v.bits&0x7f800000u)==0x7f800000u) return 0;
    }
    double norm=0;for(unsigned i=0;i<4;++i) norm+=(double)snapshot.pose.rotation[i]*snapshot.pose.rotation[i];
    if(!(norm>=0.999 && norm<=1.001 && snapshot.near_z>0 && snapshot.far_z>snapshot.near_z)) return 0;
    *out=snapshot;return 1;
}
#endif
