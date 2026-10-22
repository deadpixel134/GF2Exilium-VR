#ifdef GF2VR_STEAM_COMPAT
#line 1
#include "il2cpp_export.h"
#ifndef GF2_CAMERA_DATA_REFERENCES_H
#define GF2_CAMERA_DATA_REFERENCES_H
#include "camera_data_settings_binding.h"
/* Declared metadata from CAMERA-DATA-001. Main-thread private preparation only.
   No raw managed field offsets or mutable source-list sharing. Nonempty camera
   stacks require owned overlay cloning and are rejected by this initial route.
   A return value here is NOT render/freshness/complete-pair eligibility. */
typedef struct {void *count,*item,*clear,*add;} CameraDataListApi;
typedef struct {
    CameraDataSettingsApi settings;
    void *get_trigger,*set_trigger,*get_cameras,*get_stack,*get_passes,*get_output;
    void *int_class,*string_class,*transform_class,*output_class;
    void *(*class_type)(void *);
    unsigned char (*generic)(void *);
    unsigned bound;
} CameraDataReferencesApi;
static int camera_data_references_bind_contract(const CameraDataSettingsApi *settings,
    CameraDataReferencesApi *out,void *(*find)(const char *,const char *,const char *),
    int (*size)(void *,unsigned *),void *(*class_type_fn)(void *),unsigned char (*generic_fn)(void *)) {
    if(!out) return 0;*out=(CameraDataReferencesApi){0};
    if(!settings || !settings->bound || !find || !size || !class_type_fn || !generic_fn) return 0;
    CameraDataReferencesApi a={0};a.settings=*settings;a.class_type=class_type_fn;a.generic=generic_fn;
    a.int_class=find("mscorlib.dll","System","Int32");
    a.string_class=find("mscorlib.dll","System","String");
    a.transform_class=find("UnityEngine.CoreModule.dll","UnityEngine","Transform");
    a.output_class=find("Unity.RenderPipelines.Universal.Runtime.dll","UnityEngine.Rendering.Universal","CameraOutput");
    unsigned alignment=0;
    if(!a.int_class || !a.string_class || !a.transform_class || !a.output_class ||
       size(a.int_class,&alignment)!=4 || !alignment || alignment>_Alignof(unsigned) ||
       size(a.output_class,&alignment)!=4 || !alignment || alignment>_Alignof(unsigned)) return 0;
    void *k=settings->component_class;
    a.get_trigger=eye_binding_method(k,"get_volumeTrigger",0,"UnityEngine.Transform",0,generic_fn);
    a.set_trigger=eye_binding_method(k,"set_volumeTrigger","UnityEngine.Transform","System.Void",0,generic_fn);
    a.get_cameras=eye_binding_method(k,"get_cameras",0,"System.Collections.Generic.List<UnityEngine.Camera>",0,generic_fn);
    a.get_stack=eye_binding_method(k,"get_cameraStack",0,"System.Collections.Generic.List<UnityEngine.Camera>",0,generic_fn);
    a.get_passes=eye_binding_method(k,"get_executeOrderPass",0,"System.Collections.Generic.List<System.String>",0,generic_fn);
    a.get_output=eye_binding_method(k,"get_cameraOutput",0,"UnityEngine.Rendering.Universal.CameraOutput",0,generic_fn);
    if(!a.get_trigger || !a.set_trigger || !a.get_cameras || !a.get_stack || !a.get_passes || !a.get_output) return 0;
    a.bound=1;*out=a;return 1;
}
static int camera_data_references_bind(const CameraDataSettingsApi *settings,CameraDataReferencesApi *out) {
    if(!out) return 0;*out=(CameraDataReferencesApi){0};
    HINSTANCE module=GetModuleHandleW(L"GameAssembly.dll");if(!module) return 0;
    return camera_data_references_bind_contract(settings,out,render_class,
        (void *)il2cpp_export(module,"il2cpp_class_value_size"),
        (void *)il2cpp_export(module,"il2cpp_class_get_type"),
        (void *)il2cpp_export(module,"il2cpp_method_is_generic"));
}
/* Bind the closed runtime list class, never guessed List<T> field layout. */
static int camera_data_list_bind(const CameraDataReferencesApi *a,void *list,int strings,CameraDataListApi *out) {
    *out=(CameraDataListApi){0};
    void *k=a->settings.calls.object_class(list);if(!k) return 0;
    if(!eye_binding_type(a->class_type(k),strings?"System.Collections.Generic.List<System.String>":
                                                      "System.Collections.Generic.List<UnityEngine.Camera>")) return 0;
    CameraDataListApi l={0};
    l.count=eye_binding_method(k,"get_Count",0,"System.Int32",0,a->generic);
    if(!l.count) return 0;
    if(strings) {
        l.item=eye_binding_method(k,"get_Item","System.Int32","System.String",0,a->generic);
        l.clear=eye_binding_method(k,"Clear",0,"System.Void",0,a->generic);
        l.add=eye_binding_method(k,"Add","System.String","System.Void",0,a->generic);
        if(!l.item || !l.clear || !l.add) return 0;
    }
    *out=l;return 1;
}
static int camera_data_u32(const CameraDataReferencesApi *a,void *method,void *object,void *klass,unsigned *out) {
    const ManagedEyeApi *c=&a->settings.calls;void *boxed=0;
    if(!managed_eye_call(c,method,object,0,&boxed) || !boxed) return 0;
    unsigned root=c->pin(boxed,1);if(!root) return 0;
    void *value=c->object_class(boxed)==klass?c->unbox(boxed):0;
    if(value) for(unsigned i=0;i<4;++i) ((unsigned char *)out)[i]=((unsigned char *)value)[i];
    c->unpin(root);return value!=0;
}
/* Actual URP AEC9: get_cameras delegates to get_cameraStack. On Overlay,
   both warn (including a managed stack trace) and return null. Preserve that
   public result without invoking unsupported getters on the render path.
   Base keeps the original getter/renderer checks; unknown roles fail closed. */
static int camera_data_stack_list(const CameraDataReferencesApi *a,void *component,void *getter,void **out) {
    if(!out)return 0;*out=0;
    if(!a || !a->bound || !a->settings.bound || !component ||
       (getter!=a->get_cameras && getter!=a->get_stack) ||
       a->settings.calls.object_class(component)!=a->settings.component_class)return 0;
    unsigned role=~0u;
    if(!camera_data_u32(a,a->settings.get[4],component,a->settings.value_class[4],&role))return 0;
    /* Exact enum: Overlay1, CustomUILinear2, CustomUIGamma3 are non-Base. */
    if(role>=1 && role<=3)return 1;
    if(role!=0)return 0;
    return managed_eye_call(&a->settings.calls,getter,component,0,out);
}
/* Bounded storage guard, not truncation or a production duration/frame cap.
   Larger lists remain ineligible until a checked dynamic snapshot is supplied. */
#define CAMERA_DATA_PASS_CAPACITY 64
#ifdef GF2VR_ACTIVITY_STACK
static int camera_data_references_copy_mode(const CameraDataReferencesApi *a,void *source,void *left,void *right,unsigned owned_stack) {
#else
static int camera_data_references_copy(const CameraDataReferencesApi *a,void *source,void *left,void *right) {
#endif
    GF2VR_PREPARE_STEP(600);
    if(!a || !a->bound || !source || !left || !right || source==left || source==right || left==right) return 0;
    const ManagedEyeApi *c=&a->settings.calls;
    void *objects[]={source,left,right};unsigned object_roots[3]={0};
    void *lists[3][3]={{0}};unsigned list_roots[3][3]={{0}};
    CameraDataListApi api[3][3]={0};
    void *passes[CAMERA_DATA_PASS_CAPACITY]={0};unsigned pass_roots[CAMERA_DATA_PASS_CAPACITY]={0};
    void *trigger=0;unsigned trigger_root=0,count=0,output=0;int ok=0;
    void *getters[]={a->get_cameras,a->get_stack,a->get_passes};
    for(unsigned i=0;i<3;++i) {
    GF2VR_PREPARE_STEP(601);
        if(c->object_class(objects[i])!=a->settings.component_class) goto done;
    GF2VR_PREPARE_STEP(602);
        object_roots[i]=c->pin(objects[i],1);if(!object_roots[i]) goto done;
        for(unsigned j=0;j<3;++j) {
    GF2VR_PREPARE_STEP(603);
            if(j<2 ? !camera_data_stack_list(a,objects[i],getters[j],&lists[i][j]) :
               !managed_eye_call(c,getters[j],objects[i],0,&lists[i][j])) goto done;
    GF2VR_PREPARE_STEP(604);
            if(!lists[i][j]) {if(i && lists[0][j]) goto done;continue;}
    GF2VR_PREPARE_STEP(605);
            if(i && !lists[0][j]) goto done; /* Preserve null vs allocated shape. */
    GF2VR_PREPARE_STEP(606);
            list_roots[i][j]=c->pin(lists[i][j],1);if(!list_roots[i][j]) goto done;
    GF2VR_PREPARE_STEP(607);
            if(!camera_data_list_bind(a,lists[i][j],j==2,&api[i][j])) goto done;
    GF2VR_PREPARE_STEP(608);
            for(unsigned before=0;before<i;++before)
                for(unsigned other=0;other<3;++other)
                    if(lists[i][j]==lists[before][other]) goto done;
            unsigned length=0;
    GF2VR_PREPARE_STEP(609);
            if(!camera_data_u32(a,api[i][j].count,lists[i][j],a->int_class,&length)) goto done;
    GF2VR_PREPARE_STEP(610);
#ifdef GF2VR_ACTIVITY_STACK
            if(j<2 && length && !owned_stack) goto done; /* Ordinary pairs still reject stacks. */
            if(j<2 && owned_stack && length>=8)goto done; /* Group owner checks/remaps every edge. */
#else
            if(j<2 && length) goto done; /* Do not render or drop original overlays. */
#endif
    GF2VR_PREPARE_STEP(611);
            if(j==2 && !i) {count=length;if(count>CAMERA_DATA_PASS_CAPACITY) goto done;}
        }
    GF2VR_PREPARE_STEP(612);
        if(i && ((lists[i][0]==lists[i][1])!=(lists[0][0]==lists[0][1]))) goto done;
    }
    GF2VR_PREPARE_STEP(613);
    if(!managed_eye_call(c,a->get_trigger,source,0,&trigger)) goto done;
    if(trigger) {
    GF2VR_PREPARE_STEP(614);
        trigger_root=c->pin(trigger,1);if(!trigger_root || c->object_class(trigger)!=a->transform_class) goto done;
    }
    GF2VR_PREPARE_STEP(615);
    if(!camera_data_u32(a,a->get_output,source,a->output_class,&output)) goto done;
    for(unsigned i=0;i<count;++i) {
        int index=(int)i;void *args[]={&index};
    GF2VR_PREPARE_STEP(616);
        if(!managed_eye_call(c,api[0][2].item,lists[0][2],args,&passes[i]) || !passes[i]) goto done;
    GF2VR_PREPARE_STEP(617);
        pass_roots[i]=c->pin(passes[i],1);
        if(!pass_roots[i] || c->object_class(passes[i])!=a->string_class) goto done;
    }
    /* Only immutable strings and the explicit read-only Transform reference
       are shared. Null trigger stays null; no assumed auto-trigger remapping.
       Component owner must retain/revalidate source generation through rendering. */
    for(unsigned eye=1;eye<3;++eye) {
        void *args[]={trigger};
    GF2VR_PREPARE_STEP(618);
        if(!managed_eye_call(c,a->set_trigger,objects[eye],args,0)) goto done;
        if(lists[eye][2]) {
    GF2VR_PREPARE_STEP(619);
            if(!managed_eye_call(c,api[eye][2].clear,lists[eye][2],0,0)) goto done;
            for(unsigned i=0;i<count;++i) {
                void *item_args[]={passes[i]};
    GF2VR_PREPARE_STEP(620);
                if(!managed_eye_call(c,api[eye][2].add,lists[eye][2],item_args,0)) goto done;
            }
        }
        /* cameraOutput has no recorded independent backing field. Do not invoke
           its setter and risk overriding authored scalar settings. Require the
           derived result to agree, otherwise fail preparation explicitly. */
        unsigned actual=0;
    GF2VR_PREPARE_STEP(621);
        if(!camera_data_u32(a,a->get_output,objects[eye],a->output_class,&actual)) goto done;
        GF2VR_PREPARE_STEP(622);
        if(actual!=output) goto done;
    }
    ok=1;
done:
    for(unsigned i=CAMERA_DATA_PASS_CAPACITY;i>0;--i) if(pass_roots[i-1]) c->unpin(pass_roots[i-1]);
    if(trigger_root) c->unpin(trigger_root);
    for(unsigned i=3;i>0;--i) {
        for(unsigned j=3;j>0;--j) if(list_roots[i-1][j-1]) c->unpin(list_roots[i-1][j-1]);
        if(object_roots[i-1]) c->unpin(object_roots[i-1]);
    }
    return ok;
}
#ifdef GF2VR_ACTIVITY_STACK
static int camera_data_references_copy(const CameraDataReferencesApi *a,void *source,void *left,void *right) {
    return camera_data_references_copy_mode(a,source,left,right,0);
}
#endif
#endif

#else
#line 1
#ifndef GF2_CAMERA_DATA_REFERENCES_H
#define GF2_CAMERA_DATA_REFERENCES_H
#include "camera_data_settings_binding.h"
/* Declared metadata from CAMERA-DATA-001. Main-thread private preparation only.
   No raw managed field offsets or mutable source-list sharing. Nonempty camera
   stacks require owned overlay cloning and are rejected by this initial route.
   A return value here is NOT render/freshness/complete-pair eligibility. */
typedef struct {void *count,*item,*clear,*add;} CameraDataListApi;
typedef struct {
    CameraDataSettingsApi settings;
    void *get_trigger,*set_trigger,*get_cameras,*get_stack,*get_passes,*get_output;
    void *int_class,*string_class,*transform_class,*output_class;
    void *(*class_type)(void *);
    unsigned char (*generic)(void *);
    unsigned bound;
} CameraDataReferencesApi;
static int camera_data_references_bind_contract(const CameraDataSettingsApi *settings,
    CameraDataReferencesApi *out,void *(*find)(const char *,const char *,const char *),
    int (*size)(void *,unsigned *),void *(*class_type_fn)(void *),unsigned char (*generic_fn)(void *)) {
    if(!out) return 0;*out=(CameraDataReferencesApi){0};
    if(!settings || !settings->bound || !find || !size || !class_type_fn || !generic_fn) return 0;
    CameraDataReferencesApi a={0};a.settings=*settings;a.class_type=class_type_fn;a.generic=generic_fn;
    a.int_class=find("mscorlib.dll","System","Int32");
    a.string_class=find("mscorlib.dll","System","String");
    a.transform_class=find("UnityEngine.CoreModule.dll","UnityEngine","Transform");
    a.output_class=find("Unity.RenderPipelines.Universal.Runtime.dll","UnityEngine.Rendering.Universal","CameraOutput");
    unsigned alignment=0;
    if(!a.int_class || !a.string_class || !a.transform_class || !a.output_class ||
       size(a.int_class,&alignment)!=4 || !alignment || alignment>_Alignof(unsigned) ||
       size(a.output_class,&alignment)!=4 || !alignment || alignment>_Alignof(unsigned)) return 0;
    void *k=settings->component_class;
    a.get_trigger=eye_binding_method(k,"get_volumeTrigger",0,"UnityEngine.Transform",0,generic_fn);
    a.set_trigger=eye_binding_method(k,"set_volumeTrigger","UnityEngine.Transform","System.Void",0,generic_fn);
    a.get_cameras=eye_binding_method(k,"get_cameras",0,"System.Collections.Generic.List<UnityEngine.Camera>",0,generic_fn);
    a.get_stack=eye_binding_method(k,"get_cameraStack",0,"System.Collections.Generic.List<UnityEngine.Camera>",0,generic_fn);
    a.get_passes=eye_binding_method(k,"get_executeOrderPass",0,"System.Collections.Generic.List<System.String>",0,generic_fn);
    a.get_output=eye_binding_method(k,"get_cameraOutput",0,"UnityEngine.Rendering.Universal.CameraOutput",0,generic_fn);
    if(!a.get_trigger || !a.set_trigger || !a.get_cameras || !a.get_stack || !a.get_passes || !a.get_output) return 0;
    a.bound=1;*out=a;return 1;
}
static int camera_data_references_bind(const CameraDataSettingsApi *settings,CameraDataReferencesApi *out) {
    if(!out) return 0;*out=(CameraDataReferencesApi){0};
    HINSTANCE module=GetModuleHandleW(L"GameAssembly.dll");if(!module) return 0;
    return camera_data_references_bind_contract(settings,out,render_class,
        (void *)GetProcAddress(module,"il2cpp_class_value_size"),
        (void *)GetProcAddress(module,"il2cpp_class_get_type"),
        (void *)GetProcAddress(module,"il2cpp_method_is_generic"));
}
/* Bind the closed runtime list class, never guessed List<T> field layout. */
static int camera_data_list_bind(const CameraDataReferencesApi *a,void *list,int strings,CameraDataListApi *out) {
    *out=(CameraDataListApi){0};
    void *k=a->settings.calls.object_class(list);if(!k) return 0;
    if(!eye_binding_type(a->class_type(k),strings?"System.Collections.Generic.List<System.String>":
                                                      "System.Collections.Generic.List<UnityEngine.Camera>")) return 0;
    CameraDataListApi l={0};
    l.count=eye_binding_method(k,"get_Count",0,"System.Int32",0,a->generic);
    if(!l.count) return 0;
    if(strings) {
        l.item=eye_binding_method(k,"get_Item","System.Int32","System.String",0,a->generic);
        l.clear=eye_binding_method(k,"Clear",0,"System.Void",0,a->generic);
        l.add=eye_binding_method(k,"Add","System.String","System.Void",0,a->generic);
        if(!l.item || !l.clear || !l.add) return 0;
    }
    *out=l;return 1;
}
static int camera_data_u32(const CameraDataReferencesApi *a,void *method,void *object,void *klass,unsigned *out) {
    const ManagedEyeApi *c=&a->settings.calls;void *boxed=0;
    if(!managed_eye_call(c,method,object,0,&boxed) || !boxed) return 0;
    unsigned root=c->pin(boxed,1);if(!root) return 0;
    void *value=c->object_class(boxed)==klass?c->unbox(boxed):0;
    if(value) for(unsigned i=0;i<4;++i) ((unsigned char *)out)[i]=((unsigned char *)value)[i];
    c->unpin(root);return value!=0;
}
/* Actual URP AEC9: get_cameras delegates to get_cameraStack. On Overlay,
   both warn (including a managed stack trace) and return null. Preserve that
   public result without invoking unsupported getters on the render path.
   Base keeps the original getter/renderer checks; unknown roles fail closed. */
static int camera_data_stack_list(const CameraDataReferencesApi *a,void *component,void *getter,void **out) {
    if(!out)return 0;*out=0;
    if(!a || !a->bound || !a->settings.bound || !component ||
       (getter!=a->get_cameras && getter!=a->get_stack) ||
       a->settings.calls.object_class(component)!=a->settings.component_class)return 0;
    unsigned role=~0u;
    if(!camera_data_u32(a,a->settings.get[4],component,a->settings.value_class[4],&role))return 0;
    /* Exact enum: Overlay1, CustomUILinear2, CustomUIGamma3 are non-Base. */
    if(role>=1 && role<=3)return 1;
    if(role!=0)return 0;
    return managed_eye_call(&a->settings.calls,getter,component,0,out);
}
/* Bounded storage guard, not truncation or a production duration/frame cap.
   Larger lists remain ineligible until a checked dynamic snapshot is supplied. */
#define CAMERA_DATA_PASS_CAPACITY 64
#ifdef GF2VR_ACTIVITY_STACK
static int camera_data_references_copy_mode(const CameraDataReferencesApi *a,void *source,void *left,void *right,unsigned owned_stack) {
#else
static int camera_data_references_copy(const CameraDataReferencesApi *a,void *source,void *left,void *right) {
#endif
    GF2VR_PREPARE_STEP(600);
    if(!a || !a->bound || !source || !left || !right || source==left || source==right || left==right) return 0;
    const ManagedEyeApi *c=&a->settings.calls;
    void *objects[]={source,left,right};unsigned object_roots[3]={0};
    void *lists[3][3]={{0}};unsigned list_roots[3][3]={{0}};
    CameraDataListApi api[3][3]={0};
    void *passes[CAMERA_DATA_PASS_CAPACITY]={0};unsigned pass_roots[CAMERA_DATA_PASS_CAPACITY]={0};
    void *trigger=0;unsigned trigger_root=0,count=0,output=0;int ok=0;
    void *getters[]={a->get_cameras,a->get_stack,a->get_passes};
    for(unsigned i=0;i<3;++i) {
    GF2VR_PREPARE_STEP(601);
        if(c->object_class(objects[i])!=a->settings.component_class) goto done;
    GF2VR_PREPARE_STEP(602);
        object_roots[i]=c->pin(objects[i],1);if(!object_roots[i]) goto done;
        for(unsigned j=0;j<3;++j) {
    GF2VR_PREPARE_STEP(603);
            if(j<2 ? !camera_data_stack_list(a,objects[i],getters[j],&lists[i][j]) :
               !managed_eye_call(c,getters[j],objects[i],0,&lists[i][j])) goto done;
    GF2VR_PREPARE_STEP(604);
            if(!lists[i][j]) {if(i && lists[0][j]) goto done;continue;}
    GF2VR_PREPARE_STEP(605);
            if(i && !lists[0][j]) goto done; /* Preserve null vs allocated shape. */
    GF2VR_PREPARE_STEP(606);
            list_roots[i][j]=c->pin(lists[i][j],1);if(!list_roots[i][j]) goto done;
    GF2VR_PREPARE_STEP(607);
            if(!camera_data_list_bind(a,lists[i][j],j==2,&api[i][j])) goto done;
    GF2VR_PREPARE_STEP(608);
            for(unsigned before=0;before<i;++before)
                for(unsigned other=0;other<3;++other)
                    if(lists[i][j]==lists[before][other]) goto done;
            unsigned length=0;
    GF2VR_PREPARE_STEP(609);
            if(!camera_data_u32(a,api[i][j].count,lists[i][j],a->int_class,&length)) goto done;
    GF2VR_PREPARE_STEP(610);
#ifdef GF2VR_ACTIVITY_STACK
            if(j<2 && length && !owned_stack) goto done; /* Ordinary pairs still reject stacks. */
            if(j<2 && owned_stack && length>=8)goto done; /* Group owner checks/remaps every edge. */
#else
            if(j<2 && length) goto done; /* Do not render or drop original overlays. */
#endif
    GF2VR_PREPARE_STEP(611);
            if(j==2 && !i) {count=length;if(count>CAMERA_DATA_PASS_CAPACITY) goto done;}
        }
    GF2VR_PREPARE_STEP(612);
        if(i && ((lists[i][0]==lists[i][1])!=(lists[0][0]==lists[0][1]))) goto done;
    }
    GF2VR_PREPARE_STEP(613);
    if(!managed_eye_call(c,a->get_trigger,source,0,&trigger)) goto done;
    if(trigger) {
    GF2VR_PREPARE_STEP(614);
        trigger_root=c->pin(trigger,1);if(!trigger_root || c->object_class(trigger)!=a->transform_class) goto done;
    }
    GF2VR_PREPARE_STEP(615);
    if(!camera_data_u32(a,a->get_output,source,a->output_class,&output)) goto done;
    for(unsigned i=0;i<count;++i) {
        int index=(int)i;void *args[]={&index};
    GF2VR_PREPARE_STEP(616);
        if(!managed_eye_call(c,api[0][2].item,lists[0][2],args,&passes[i]) || !passes[i]) goto done;
    GF2VR_PREPARE_STEP(617);
        pass_roots[i]=c->pin(passes[i],1);
        if(!pass_roots[i] || c->object_class(passes[i])!=a->string_class) goto done;
    }
    /* Only immutable strings and the explicit read-only Transform reference
       are shared. Null trigger stays null; no assumed auto-trigger remapping.
       Component owner must retain/revalidate source generation through rendering. */
    for(unsigned eye=1;eye<3;++eye) {
        void *args[]={trigger};
    GF2VR_PREPARE_STEP(618);
        if(!managed_eye_call(c,a->set_trigger,objects[eye],args,0)) goto done;
        if(lists[eye][2]) {
    GF2VR_PREPARE_STEP(619);
            if(!managed_eye_call(c,api[eye][2].clear,lists[eye][2],0,0)) goto done;
            for(unsigned i=0;i<count;++i) {
                void *item_args[]={passes[i]};
    GF2VR_PREPARE_STEP(620);
                if(!managed_eye_call(c,api[eye][2].add,lists[eye][2],item_args,0)) goto done;
            }
        }
        /* cameraOutput has no recorded independent backing field. Do not invoke
           its setter and risk overriding authored scalar settings. Require the
           derived result to agree, otherwise fail preparation explicitly. */
        unsigned actual=0;
    GF2VR_PREPARE_STEP(621);
        if(!camera_data_u32(a,a->get_output,objects[eye],a->output_class,&actual)) goto done;
        GF2VR_PREPARE_STEP(622);
        if(actual!=output) goto done;
    }
    ok=1;
done:
    for(unsigned i=CAMERA_DATA_PASS_CAPACITY;i>0;--i) if(pass_roots[i-1]) c->unpin(pass_roots[i-1]);
    if(trigger_root) c->unpin(trigger_root);
    for(unsigned i=3;i>0;--i) {
        for(unsigned j=3;j>0;--j) if(list_roots[i-1][j-1]) c->unpin(list_roots[i-1][j-1]);
        if(object_roots[i-1]) c->unpin(object_roots[i-1]);
    }
    return ok;
}
#ifdef GF2VR_ACTIVITY_STACK
static int camera_data_references_copy(const CameraDataReferencesApi *a,void *source,void *left,void *right) {
    return camera_data_references_copy_mode(a,source,left,right,0);
}
#endif
#endif

#endif
