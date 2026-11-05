#ifndef GF2_HOME_PRIMARY_RUNTIME_H
#define GF2_HOME_PRIMARY_RUNTIME_H
#include "home_primary_binding.h"
static HomePrimaryBinding hp_api;
static void *hp_transform_class,*hp_parent;
static int hp_named(void *object,const char *identity){
    const ManagedEyeApi *c=&hc_api.values.calls;
    if(!hr_get_name || !hr_string_length || !hr_string_chars)return -1;
    void *name=0;unsigned held=0;int match=-1;
    if(!managed_eye_call(c,hr_get_name,object,0,&name) || !name || !(held=c->pin(name,1)))return -1;
    int n=hr_string_length(name);const unsigned short *s=hr_string_chars(name);
    if(n>=0 && n<=256 && s){
        match=1;unsigned i=0;
        for(;i<(unsigned)n;++i)if(!identity[i] || s[i]!=(unsigned char)identity[i]){match=0;break;}
        if(match && identity[i])match=0;
    }
    c->unpin(held);return match;
}
static int hp_reference_type(void *f,void *klass){
    void *type=hc_field_type(f);
    return type && !hcr_api.byref(type) && sa.type_get_type(type)==18 && hc_class_from_type(type)==klass;
}
static int hp_root_type(void *f){return hp_reference_type(f,hp_transform_class);}
static int hp_camera_type(void *f){return hp_reference_type(f,hc_api.values.calls.camera_class);}
static int hp_root_match(void *root){
    int named=hp_named(root,"SceneCameraRoot");if(named!=1)return named;
    void *parent=0;
    if(!hp_parent || !managed_eye_call(&hc_api.values.calls,hp_parent,root,0,&parent))return -1;
    return parent?0:1; /* Game initializer enumerates scene root GameObjects. */
}
static int hp_camera_match(void *camera,void *root){
    int named=hp_named(camera,"MainCamera");if(named!=1)return named;
    const ManagedEyeApi *c=&hc_api.values.calls;
    void *transform=0,*parent=0;unsigned held=0;int match=-1;
    if(!managed_eye_call(c,c->get_transform,camera,0,&transform) || !transform || !(held=c->pin(transform,1)))return -1;
    if(c->object_class(transform)==hp_transform_class && managed_eye_call(c,hp_parent,transform,0,&parent))match=parent==root;
    c->unpin(held);return match;
}
static int hp_resolve(void *owner,void **camera,unsigned *root){
    int status=home_primary_binding_read(&hp_api,owner,camera,root);
    /* Numeric log attribution only; never dereference/reuse the saved identity. */
    static int previous_status=2;static U64 previous_camera;
    U64 identity=(U64)*camera;
    if(status!=previous_status || identity!=previous_camera){
        previous_status=status;previous_camera=identity;
        event("home_semantic_primary_status",status==1?1:0,GetCurrentThreadId());
        event("home_semantic_primary_camera",identity,GetCurrentThreadId());
    }
    return status;
}
#endif
