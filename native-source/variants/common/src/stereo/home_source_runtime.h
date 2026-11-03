#ifndef GF2_HOME_SOURCE_RUNTIME_H
#define GF2_HOME_SOURCE_RUNTIME_H
/* Read the game's already-existing Home owner and cached Camera. In current
   Steam metadata the singleton getter is a backing-field read, whereas the
   Camera getter lazily writes the game's cache. Neither getter is invoked.
   Unique exact field types tolerate obfuscated names without guessing aliases. */
#include "home_source_read.h"
#ifdef GF2VR_COMMANDER_HOME_SOURCE
#include "home_commander_runtime.h"
#endif
#ifdef GF2VR_HOME_SCENE_OBSERVE
#include "home_scene_observation_runtime.h"
#endif
static HomeSourceOwner home_source_owner;
static int home_source_bind(const ManagedEyeApi *calls) {
    HomeSourceOwner *h=&home_source_owner;
    if(h->attempted)return h->ready;
    h->attempted=1;if(!calls)return 0;h->values.calls=*calls;
    HINSTANCE game=GetModuleHandleW(L"GameAssembly.dll");if(!game)return 0;
    void *(*fields)(void *,void **)=(void *)il2cpp_export(game,"il2cpp_class_get_fields");
    void *(*field_type)(void *)=(void *)il2cpp_export(game,"il2cpp_field_get_type");
    const char *(*field_name)(void *)=(void *)il2cpp_export(game,"il2cpp_field_get_name");
    unsigned (*flags)(void *)=(void *)il2cpp_export(game,"il2cpp_field_get_flags");
    unsigned char (*generic)(void *)=(void *)il2cpp_export(game,"il2cpp_method_is_generic");
    h->static_read=(void *)il2cpp_export(game,"il2cpp_field_static_get_value");
    h->instance_read=(void *)il2cpp_export(game,"il2cpp_field_get_value");
    if(!fields || !field_type || !field_name || !flags || !generic ||
       !h->static_read || !h->instance_read)return 0;
    h->owner_class=render_class("Assembly-CSharp.dll","","T0BGHelper");
    h->bool_class=render_class("mscorlib.dll","System","Boolean");
    void *behaviour=render_class("UnityEngine.CoreModule.dll","UnityEngine","Behaviour");
    if(!h->owner_class || !h->bool_class || !behaviour)return 0;
    h->live=eye_binding_method(behaviour,"get_isActiveAndEnabled",0,"System.Boolean",0,generic);
    if(!h->live)return 0;
    void *iterator=0;unsigned complete=0;
    for(unsigned i=0;i<128;++i) {
        void *f=fields(h->owner_class,&iterator);if(!f){complete=1;break;}
        unsigned bits=flags(f);if(bits&0x40u)continue; /* literal */
        void **slot=0;void *type=field_type(f);
        if((bits&0x10u) && eye_binding_type(type,"T0BGHelper"))slot=&h->singleton;
        else if(!(bits&0x10u) && eye_binding_type(type,"UnityEngine.Camera"))slot=&h->camera;
        else if(eye_binding_type(type,"System.Boolean")) {
            const char *name=field_name(f);
            if((bits&0x10u) && source_equal(name,"CurrActive"))slot=&h->active;
            if(!(bits&0x10u) && source_equal(name,"isGachaModel"))slot=&h->gacha;
        }
        if(slot){if(*slot)return 0;*slot=f;}
    }
    h->ready=complete && h->singleton && h->camera && h->active && h->gacha;
    return h->ready;
}
static void *home_source_camera(const ManagedEyeApi *calls) {
    DWORD saved=GetLastError();void *camera=0;unsigned cold=!home_source_owner.attempted;
    if(home_source_bind(calls))camera=home_source_read(&home_source_owner);
#ifdef GF2VR_HOME_SCENE_OBSERVE
    home_scene_observe(calls);
#endif
#ifdef GF2VR_HOME_OWNER_OBSERVE
    /* First failed existing guard only; no extra managed reads or calls.
       stage low byte: 0 unbound,1 singleton,2 owner root,3 owner class,
       4 active/gacha/owner liveness,5 cached Camera,6 Camera root/class,
       7 Camera liveness,8 reentrant field check,9 accepted.
       Following bytes: actual active,gacha,last liveness value;255 unread. */
    static U64 previous_read;static unsigned read_observed;
    U64 read=home_source_owner.read_observation;
    if(!read_observed || previous_read!=read) {
        read_observed=1;previous_read=read;
        event("home_source_read_guard",read,GetCurrentThreadId());
    }
#endif
    if(cold)event("home_source_owner_binding",((U64)home_source_owner.ready<<32)|
        (home_source_owner.singleton!=0)|((U64)(home_source_owner.camera!=0)<<1)|
        ((U64)(home_source_owner.active!=0)<<2)|((U64)(home_source_owner.gacha!=0)<<3),GetCurrentThreadId());
    static void *previous;static unsigned observed;
    if(!observed || previous!=camera) {
        observed=1;previous=camera;
        event("home_source_owner_camera",(U64)camera,GetCurrentThreadId());
    }
#ifdef GF2VR_COMMANDER_HOME_SOURCE
    unsigned commander_scope=0;
    void *commander=home_commander_camera(calls,&commander_scope);
    if(commander_scope)camera=commander; /* Known Commander may not fall back to an old T0 view. */
#endif
    SetLastError(saved);return camera;
}
#define GF2VR_HOME_SOURCE_CAMERA(a) home_source_camera(&(a)->binding->refs.settings.calls)
#endif
