#include "il2cpp_export.h"
#ifndef GF2_CAMERA_DATA_OWNER_H
#define GF2_CAMERA_DATA_OWNER_H
#include "camera_data_references.h"
typedef struct {
    CameraDataReferencesApi refs;
    void *type,*get_component;
    unsigned type_root,in_use;
} CameraDataComponentApi;
typedef struct {
    CameraDataComponentApi *binding;
    ManagedEyes *eyes;
    void *source_camera,*components[3];
    unsigned source_root,roots[3],ready,busy,retiring;
    unsigned long long generation;
#ifdef GF2VR_ACTIVITY_STACK
    void *group;
    int (*group_refresh)(void *,int (*)(void *,unsigned long long),void *);
    int (*group_retire)(void *);
#endif
} CameraDataOwner;
static int camera_data_component_bind_contract(const CameraDataReferencesApi *refs,
    CameraDataComponentApi *out,void *component_class,void *(*type_object_fn)(void *)) {
    if(!out || out->type_root || out->in_use) return 0;
    *out=(CameraDataComponentApi){0};
    if(!refs || !refs->bound || !component_class || !type_object_fn) return 0;
    CameraDataComponentApi a={0};a.refs=*refs;
    a.get_component=eye_binding_method(component_class,"GetComponent","System.Type","UnityEngine.Component",0,refs->generic);
    if(!a.get_component) return 0;
    void *type=refs->class_type(refs->settings.component_class);if(!type) return 0;
    a.type=type_object_fn(type);if(!a.type) return 0;
    a.type_root=refs->settings.calls.pin(a.type,1);if(!a.type_root) return 0;
    *out=a;return 1;
}
static int camera_data_component_bind(const CameraDataReferencesApi *refs,CameraDataComponentApi *out) {
    HINSTANCE module=GetModuleHandleW(L"GameAssembly.dll");if(!module) return 0;
    return camera_data_component_bind_contract(refs,out,
        render_class("UnityEngine.CoreModule.dll","UnityEngine","Component"),
        (void *)il2cpp_export(module,"il2cpp_type_get_object"));
}
/* A data owner owns the WHOLE private ManagedEyes preparation lifetime. Always
   retire through this function once attached. It never destroys source objects.
   After a failed preparation retire the GameObjects, not just their components:
   deferred component Destroy would permit duplicate components on immediate reuse.
   Outstanding engine/GPU borrows and failed Destroy retain roots for retry. */
#ifdef GF2VR_ACTIVITY_STACK
static int camera_data_owner_retire_single(CameraDataOwner *o) {
#else
static int camera_data_owner_retire(CameraDataOwner *o) {
#endif
    if(!o || !o->binding) return 1;
    o->retiring=1;o->ready=0;o->eyes->ready=0;o->eyes->matrices_ready=0;
    if(o->busy || o->eyes->borrowed) {
        if(o->busy) GF2VR_RETIRE_FAILURE(1,2);
        if(o->eyes->borrowed) GF2VR_RETIRE_FAILURE(2,2);
        return 0;
    }
    CameraDataComponentApi *b=o->binding;const ManagedEyeApi *c=&b->refs.settings.calls;
    if(!managed_eyes_retire(c,o->eyes)) return 0;
    for(unsigned i=3;i>0;--i) if(o->roots[i-1]) c->unpin(o->roots[i-1]);
    if(o->source_root) c->unpin(o->source_root);
    b->in_use=0;*o=(CameraDataOwner){0};return 1;
}
#ifdef GF2VR_ACTIVITY_STACK
static int camera_data_owner_retire(CameraDataOwner *o) {
    if(o && o->group && o->group_retire)return o->group_retire(o->group);
    return camera_data_owner_retire_single(o);
}
#endif
static int camera_data_component_release(CameraDataComponentApi *b) {
    if(!b || b->in_use) return 0;
    if(b->type_root) b->refs.settings.calls.unpin(b->type_root);
    *b=(CameraDataComponentApi){0};return 1;
}
/* Called before fresh matrices/borrowing; valid checks the actual source/XR
   generation and must be read-only. Recheck again at render/publication time.
   A refresh failure retires the unused pair; a borrow is never torn down here. */
static int camera_data_owner_targets(CameraDataOwner *o,unsigned attach,
    int (*valid)(void *,unsigned long long),void *context) {
    const ManagedEyeApi *c=&o->binding->refs.settings.calls;
    if(!c->set_target || o->eyes->borrowed) return 0;
    for(unsigned i=0;i<2;++i) {
        ManagedEyeSlot *e=&o->eyes->eyes[i];
        if(!e->camera_root || !e->target_root || !e->target || !e->camera ||
           e->camera==o->source_camera || o->retiring || !valid(context,o->generation)) return 0;
        void *args[]={attach?e->target:0};
        if(!managed_eye_call(c,c->set_target,e->camera,args,0)) return 0;
    }
    return !o->retiring && valid(context,o->generation);
}
static int camera_data_owner_refresh(CameraDataOwner *o,int (*valid)(void *,unsigned long long),void *context) {
#ifdef GF2VR_ACTIVITY_STACK
    if(o && o->group && o->group_refresh)return o->group_refresh(o->group,valid,context);
#endif
    if(!o || !o->binding || o->busy || o->retiring || o->eyes->borrowed) return 0;
    o->ready=0;o->eyes->ready=0;o->eyes->matrices_ready=0;
    o->busy=1;
    /* cameraOutput is derived: compare authored settings with both private
       cameras in the same no-target state used during initial preparation.
       Only drained/unborrowed private eyes may change attachments. Keep all
       roots/resources, and restore BOTH before publishing readiness. */
    unsigned targets=o->eyes->targets_ready;
    if(targets) o->eyes->targets_ready=0;
    GF2VR_PREPARE_STEP(320);
    unsigned phase=0;
    int ok=valid && valid(context,o->generation) &&
        (phase=1,!targets || camera_data_owner_targets(o,0,valid,context)) &&
        (phase=2,GF2VR_PREPARE_STEP(321),camera_data_settings_copy(&o->binding->refs.settings,o->components[0],o->components[1],o->components[2])) &&
        (phase=3,GF2VR_PREPARE_STEP(322),camera_data_references_copy(&o->binding->refs,o->components[0],o->components[1],o->components[2])) &&
        (phase=4,GF2VR_PREPARE_STEP(323),valid(context,o->generation)) && !o->retiring &&
        (phase=5,!targets || camera_data_owner_targets(o,1,valid,context));
    o->busy=0;
    if(!ok) {
#ifdef GF2VR_REFRESH_FAILURE
        GF2VR_REFRESH_FAILURE(o,phase);
#else
        (void)phase;
#endif
        camera_data_owner_retire(o);return 0;
    }
    o->eyes->targets_ready=targets;
    o->ready=1;o->eyes->ready=1;return 1;
}
#ifdef GF2VR_ACTIVITY_STACK
static int camera_data_owner_attach(CameraDataComponentApi *b,CameraDataOwner *o,ManagedEyes *eyes,
#else
static int camera_data_owner_prepare(CameraDataComponentApi *b,CameraDataOwner *o,ManagedEyes *eyes,
#endif
    void *source,unsigned long long generation,int (*valid)(void *,unsigned long long),void *context) {
    GF2VR_PREPARE_STEP(300);
    if(!b || !o || !eyes || !b->type_root || b->in_use || o->binding || !generation ||
       !eyes->ready || eyes->borrowed || eyes->matrices_ready || !source || !valid || !valid(context,generation)) return 0;
    const ManagedEyeApi *c=&b->refs.settings.calls;
    GF2VR_PREPARE_STEP(301);
    if(!c->add_component || !c->destroy || !c->camera_class || c->object_class(source)!=c->camera_class) return 0;
    GF2VR_PREPARE_STEP(302);
    for(unsigned i=0;i<2;++i)
        if(!eyes->eyes[i].object_root || !eyes->eyes[i].camera_root ||
           !eyes->eyes[i].camera || eyes->eyes[i].camera==source) return 0;
    b->in_use=1;o->binding=b;o->eyes=eyes;o->generation=generation;o->source_camera=source;o->busy=1;
    eyes->ready=0;eyes->matrices_ready=0;
    GF2VR_PREPARE_STEP(303);
    o->source_root=c->pin(source,1);if(!o->source_root) goto failed;
    for(unsigned i=0;i<3;++i) {
        void *args[]={b->type};
    GF2VR_PREPARE_STEP(304);
        if(!managed_eye_call(c,i?c->add_component:b->get_component,
                            i?eyes->eyes[i-1].object:source,args,&o->components[i]) || !o->components[i]) goto failed;
    GF2VR_PREPARE_STEP(305);
        o->roots[i]=c->pin(o->components[i],1);if(!o->roots[i]) goto failed;
    GF2VR_PREPARE_STEP(306);
        if(c->object_class(o->components[i])!=b->refs.settings.component_class) goto failed;
    GF2VR_PREPARE_STEP(307);
        for(unsigned j=0;j<i;++j) if(o->components[i]==o->components[j]) goto failed;
    }
    GF2VR_PREPARE_STEP(308);
    o->busy=0;if(o->retiring) goto failed;
#ifdef GF2VR_ACTIVITY_STACK
    return 1; /* Whole-stack preparation attaches every member before linking. */
#else
    return camera_data_owner_refresh(o,valid,context);
#endif
failed:
    o->busy=0;camera_data_owner_retire(o);return 0;
}
#ifdef GF2VR_ACTIVITY_STACK
static int camera_data_owner_prepare(CameraDataComponentApi *b,CameraDataOwner *o,ManagedEyes *eyes,
    void *source,unsigned long long generation,int (*valid)(void *,unsigned long long),void *context) {
    return camera_data_owner_attach(b,o,eyes,source,generation,valid,context) &&
        camera_data_owner_refresh(o,valid,context);
}
#endif
#endif
