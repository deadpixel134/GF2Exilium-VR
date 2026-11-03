#ifndef GF2_HOME_COMMANDER_RUNTIME_H
#define GF2_HOME_COMMANDER_RUNTIME_H
#include "home_commander_read.h"
#ifdef GF2VR_HOME_TIMELINE_SOURCE
#include "home_timeline_read.h"
static HomeTimelineApi ht_api;
static int hc_timeline(void *owner,void **camera,unsigned *root){return home_timeline_read(&ht_api,owner,camera,root);}
#endif
static void *(*hc_fields)(void *,void **);
static void *(*hc_field_type)(void *);
static const char *(*hc_field_name)(void *);
static unsigned (*hc_field_flags)(void *);
static void *(*hc_parent)(void *);
static void *(*hc_class_from_type)(void *);
static void *(*hc_enum_base)(void *);
static unsigned char (*hc_is_enum)(void *);
static HomeCommanderApi hc_api;
static void *hc_field(void *klass,const char *name,const char *type) {
    void *it=0,*found=0;
    for(unsigned n=0;n<128;++n) {
        void *f=hc_fields(klass,&it);if(!f)return found;
        if((hc_field_flags(f)&0x50u) || !source_equal(hc_field_name(f),name))continue;
        if(type) {if(!eye_binding_type(hc_field_type(f),type))continue;}
        else {
            void *k=hc_class_from_type(hc_field_type(f));
            if(!k || !hc_is_enum(k) || !eye_binding_type(hc_enum_base(k),"System.Int32"))continue;
        }
        if(found)return 0;found=f;
    }
    return 0;
}
static void *hc_singleton(void *klass,const char *type) {
    void *found=0;
    for(unsigned p=0;p<4 && klass;++p,klass=hc_parent(klass)) {
        void *it=0;unsigned complete=0;
        for(unsigned n=0;n<128;++n) {
            void *f=hc_fields(klass,&it);if(!f){complete=1;break;}
            unsigned flags=hc_field_flags(f);
            if(!(flags&0x10u) || (flags&0x40u) || !eye_binding_type(hc_field_type(f),type))continue;
            if(found)return 0;found=f;
        }
        if(!complete)return 0;
    }
    return klass?0:found;
}
static int home_commander_bind(const ManagedEyeApi *calls) {
    static unsigned attempted;if(attempted)return hc_api.ready;attempted=1;
    HINSTANCE game=GetModuleHandleW(L"GameAssembly.dll");if(!game || !calls)return 0;
#define HC_API(v,n) v=(void *)il2cpp_export(game,n);if(!v)return 0
    HC_API(hc_fields,"il2cpp_class_get_fields");HC_API(hc_field_type,"il2cpp_field_get_type");
    HC_API(hc_field_name,"il2cpp_field_get_name");HC_API(hc_field_flags,"il2cpp_field_get_flags");
    HC_API(hc_parent,"il2cpp_class_get_parent");HC_API(hc_class_from_type,"il2cpp_class_from_type");
    HC_API(hc_enum_base,"il2cpp_class_enum_basetype");HC_API(hc_is_enum,"il2cpp_class_is_enum");
    HC_API(hc_api.static_read,"il2cpp_field_static_get_value");HC_API(hc_api.instance_read,"il2cpp_field_get_value");
    unsigned char (*generic)(void *);HC_API(generic,"il2cpp_method_is_generic");
#undef HC_API
    hc_api.values.calls=*calls;
    hc_api.game_class=render_class("Assembly-CSharp.dll","","NDCDKOFLOBO");
    hc_api.registry_class=render_class("Assembly-CSharp.dll","","BNGODCBFFOL");
    hc_api.main_class=render_class("Assembly-CSharp.dll","","BEOFGJOOODN");
    hc_api.commander_class=render_class("Assembly-CSharp.dll","","LMCHJAJGMAP");
    hc_api.bool_class=render_class("mscorlib.dll","System","Boolean");
    void *base=render_class("Assembly-CSharp.dll","","BPIABCOHANK");
    void *behaviour=render_class("UnityEngine.CoreModule.dll","UnityEngine","Behaviour");
    if(!hc_api.game_class || !hc_api.registry_class || !hc_api.main_class || !hc_api.commander_class || !hc_api.bool_class || !base || !behaviour)return 0;
    hc_api.game_singleton=hc_singleton(hc_api.game_class,"NDCDKOFLOBO");
    hc_api.registry_singleton=hc_singleton(hc_api.registry_class,"BNGODCBFFOL");
    hc_api.current=hc_field(hc_api.game_class,"HLLNPAPPOGD","BPIABCOHANK");
    hc_api.current_kind=hc_field(hc_api.game_class,"POCDACOFLMK","FEAFCAGNKJG");
    hc_api.background_kind=hc_field(hc_api.main_class,"FJEPHMDFNEO",0);
    hc_api.scene_kind=hc_field(base,"FELDKJEJPDG","FEAFCAGNKJG");
    hc_api.primary=hc_field(hc_api.commander_class,"OCGJALNPJMD","UnityEngine.Camera");
#ifdef GF2VR_HOME_PREVIEW_OBSERVE
    hc_api.observed_secondary=hc_field(hc_api.commander_class,"KDLOKBODPLJ","UnityEngine.Camera");
#endif
    hc_api.lookup=eye_binding_method(hc_api.registry_class,"DGEMNAIAGFF","FEAFCAGNKJG","BPIABCOHANK",0,generic);
    hc_api.live=eye_binding_method(behaviour,"get_isActiveAndEnabled",0,"System.Boolean",0,generic);
#ifdef GF2VR_HOME_TIMELINE_SOURCE
    ht_api.values.calls=*calls;ht_api.read=hc_api.instance_read;ht_api.bool_class=hc_api.bool_class;
    ht_api.root_field=hc_field(hc_api.commander_class,"FMAMNGDIGNO","UnityEngine.GameObject");
    ht_api.gameobject_class=render_class("UnityEngine.CoreModule.dll","UnityEngine","GameObject");
    ht_api.live=hc_api.live;
    ht_api.active=eye_binding_method(ht_api.gameobject_class,"get_activeInHierarchy",0,"System.Boolean",0,generic);
    ht_api.components=eye_binding_method(ht_api.gameobject_class,"GetComponentsInChildren","System.Type","UnityEngine.Component[]",0,generic);
    void *array=render_class("mscorlib.dll","System","Array");
    ht_api.get_value=eye_binding_method(array,"GetValue","System.Int32","System.Object",0,generic);
    ht_api.class_type=(void *)il2cpp_export(game,"il2cpp_class_get_type");
    ht_api.type_object=(void *)il2cpp_export(game,"il2cpp_type_get_object");
    ht_api.length=(void *)il2cpp_export(game,"il2cpp_array_length");
    hc_api.select_timeline=hc_timeline;
#endif
    hc_api.ready=hc_api.game_singleton && hc_api.registry_singleton && hc_api.current && hc_api.current_kind &&
        hc_api.background_kind && hc_api.scene_kind && hc_api.primary && hc_api.lookup && hc_api.live;
    return hc_api.ready;
}
static void *home_commander_camera(const ManagedEyeApi *calls,unsigned *owns_current) {
    hc_api.owns_current=0;
    void *camera=0;if(home_commander_bind(calls))camera=home_commander_read(&hc_api);
    *owns_current=hc_api.owns_current;
    U64 state=((U64)hc_api.ready<<32)|((U64)hc_api.owns_current<<16)|hc_api.stage;
    static U64 previous_state;static void *previous_camera;static unsigned observed;
    if(!observed || previous_state!=state || previous_camera!=camera) {
        observed=1;previous_state=state;previous_camera=camera;
        event("home_commander_owner_state",state,GetCurrentThreadId());
        event("home_commander_owner_camera",(U64)camera,GetCurrentThreadId());
    }
    return camera;
}
#endif
