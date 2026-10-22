#ifdef GF2VR_STEAM_COMPAT
#line 1
#include "il2cpp_export.h"
#ifndef GF2_SOURCE_LIVE_OWNER_H
#define GF2_SOURCE_LIVE_OWNER_H
#include "source_eye_prepare.h"
#include "source_eye_pool.h"
#include "source_render_flights.h"
#include "main_test_grant.h"
#include "eye_native_targets.h"
/* Persistent, address-stable owner. Never copy after initialization: internal
   pointers and outstanding render frames refer to this storage. Unity thread
   only. XR mailbox and publication storage remain externally owned. */
typedef struct SourceLiveOwner {
    unsigned long thread;
    unsigned busy,ready,retiring,stage,pipeline_root;
    int width,height;
    void *pipeline,*method;
    ManagedEyeBinding eye;
    CameraBatchApi batch;
    CameraDataComponentApi component;
    CameraDataComponentApi second_component;
#ifdef GF2VR_CAPACITY_EXPERIMENT
    CameraDataComponentApi extra_component[EFO_CAPACITY-2];
#endif
    SourceEyePool pool;
    SourceRenderFlights flights;
    unsigned pair_count;
    CameraDataOwner data;
    ManagedEyes eyes;
    SourcePoseApi reader;
    SourceTopologyApi topology_api;
    SourceTopology topology;
    SourceGeneration generation;
    SourceEyePreparation preparation;
    SourceAnchorState anchor;
    MainTestGrant manual;
    PoseMathBinding math;
    EyeNativeApi native;
#ifdef GF2VR_ACTIVITY_STACK
    ActivitySourceGroup activity;
#endif
    XrEyeMailbox *mailbox;
    /* Tests substitute external binding steps/CRT close, not owner transitions. */
    int (*bind_step)(struct SourceLiveOwner *,unsigned);
    int (*close_math)(PoseMathBinding *);
    void (*release_pipeline)(unsigned);
} SourceLiveOwner;
static int source_live_batch(SourceLiveOwner *o) {
    HINSTANCE game=GetModuleHandleW(L"GameAssembly.dll");if(!game) return 0;
    unsigned char (*generic)(void *)=(void *)il2cpp_export(game,"il2cpp_method_is_generic");
    void *(*array_class)(void *,unsigned)=(void *)il2cpp_export(game,"il2cpp_array_class_get");
    void *(*array_new)(void *,unsigned long long)=(void *)il2cpp_export(game,"il2cpp_array_new");
    if(!generic || !array_class || !array_new || !sa.array_length) return 0;
    CameraBatchApi b={0};b.camera_class=o->eye.api.camera_class;b.array_class=array_class(b.camera_class,1);
    void *array=render_class("mscorlib.dll","System","Array");
    b.get_value=eye_binding_method(array,"GetValue","System.Int32","System.Object",0,generic);
    const char *params[]={"System.Object","System.Int32"};
    b.set_value=eye_binding_method_args(array,"SetValue",2,params,"System.Void",0,generic);
    if(!b.array_class || !b.get_value || !b.set_value) return 0;
    b.array_new=array_new;b.array_length=sa.array_length;b.object_class=o->eye.api.object_class;
    b.pin=o->eye.api.pin;b.unpin=o->eye.api.unpin;b.invoke=o->eye.api.invoke;
    o->batch=b;return 1;
}
static int source_live_bind_step(SourceLiveOwner *o,unsigned step) {
    switch(step) {
    case 0:
        if(!sa.gchandle_new || !sa.gchandle_free) return 0;
        o->release_pipeline=sa.gchandle_free;
        o->pipeline_root=sa.gchandle_new(o->pipeline,1);return o->pipeline_root!=0;
    case 1:return eye_binding_bind(&o->eye);
    case 2:return source_live_batch(o);
    case 3: {
        CameraDataSettingsApi settings={0};CameraDataReferencesApi refs={0};
        return camera_data_settings_bind(&o->eye,&settings) &&
            camera_data_references_bind(&settings,&refs) && camera_data_component_bind(&refs,&o->component);
    }
    case 4:return source_pose_bind(&o->eye,&o->reader);
    case 5:return source_topology_bind(&o->eye,&o->topology_api);
    case 6:return eye_native_bind(&o->eye,&o->native);
    case 7:return pose_math_open(&o->math);
    case 8: {
        HINSTANCE game=GetModuleHandleW(L"GameAssembly.dll");if(!game) return 0;
        unsigned char (*generic)(void *)=(void *)il2cpp_export(game,"il2cpp_method_is_generic");
        void *klass=render_class("Unity.RenderPipelines.Universal.Runtime.dll",
            "UnityEngine.Rendering.Universal","UniversalRenderPipeline");
        if(!generic || !klass || o->eye.api.object_class(o->pipeline)!=klass) return 0;
        const char *params[]={"UnityEngine.Rendering.ScriptableRenderContext","UnityEngine.Camera[]"};
        void *method=eye_binding_method_args(klass,"Render",2,params,"System.Void",0,generic);
        return method && method==o->method;
    }
#ifdef GF2VR_CAPACITY_EXPERIMENT
    case 9:return camera_data_component_bind(&o->component.refs,&o->second_component);
    case 10:return camera_data_component_bind(&o->component.refs,&o->extra_component[0]);
#else
    case 9:return camera_data_component_bind(&o->component.refs,&o->second_component);
#endif
    default:return 0;
    }
}
/* May return pending. No binding/module/root reset until actual eye/data borrows
   and failed managed destruction are reconciled. No XR session teardown here. */
static int source_live_close(SourceLiveOwner *o,unsigned long thread) {
    if(!o || !thread || thread!=o->thread) return 0;
    o->ready=0;o->retiring=1;main_test_revoke(&o->manual);
    source_generation_invalidate(&o->generation);
    if(o->busy) return 0;
    o->busy=1;
    int clean=source_generation_collect(&o->generation,thread);
    /* A failed/uninjected root may hold no eye borrow but still owns a numeric
       flight and callback transaction. Do not unload its bindings/math early. */
    for(unsigned i=0;i<EFO_CAPACITY;++i)if(o->flights.slots[i].occupied ||
        o->flights.slots[i].frame.serial || o->flights.slots[i].scope.render.active)clean=0;
    if(clean && o->pool.source) clean=source_eye_pool_unbind(&o->pool,thread);
#ifdef GF2VR_ACTIVITY_STACK
    if(clean && o->activity.binding) {
        if(o->activity.busy)clean=0;
        else {
            activity_snapshot_drop(&o->activity,&o->activity.pending);
            activity_snapshot_drop(&o->activity,&o->activity.active);
        }
    }
#endif
#ifdef GF2VR_CAPACITY_EXPERIMENT
    if(clean)for(unsigned i=0;i<EFO_CAPACITY-2;++i)if(!camera_data_component_release(&o->extra_component[i]))clean=0;
#endif
    if(clean) clean=camera_data_component_release(&o->second_component);
    if(clean) clean=camera_data_component_release(&o->component);
    if(clean && o->pipeline_root) {
        unsigned root=o->pipeline_root;o->pipeline_root=0;o->release_pipeline(root);
    }
    if(clean) clean=eye_binding_release(&o->eye,&o->eyes);
    if(clean && o->close_math) clean=o->close_math(&o->math);
    o->busy=0;
    if(clean) *o=(SourceLiveOwner){0};
    return clean;
}
static int source_live_open_mode_with(SourceLiveOwner *o,unsigned long thread,void *pipeline,void *method,
    XrEyeMailbox *mailbox,unsigned long long *publication,unsigned long long *clock,int width,int height,float units,
    int (*bind_step)(SourceLiveOwner *,unsigned),int (*close_math)(PoseMathBinding *),unsigned pairs) {
    float checked;
    if(!o || !thread || o->thread || o->busy || o->ready || o->retiring || !pipeline || !method ||
       !mailbox || !publication || !clock || clock==publication || width<=0 || height<=0 || !(units>0) || !eye_optics_float(units,&checked) ||
       !bind_step || !close_math || (pairs!=1 && pairs!=EFO_CAPACITY)) return 0;
    /* o is zero-initialized, stable storage. Construct links before any managed
       bind can reenter and ask this owner to retire. */
    o->thread=thread;o->busy=1;o->pipeline=pipeline;o->method=method;o->mailbox=mailbox;
    o->width=width;o->height=height;o->bind_step=bind_step;o->close_math=close_math;
    o->pair_count=pairs;
    o->manual.units_per_meter=units;
#ifdef GF2VR_MAIN_HANDOFF
    o->manual.handoff_enabled=1;
#endif
#ifdef GF2VR_MAIN_FLAT_TRACKING
    o->manual.flat_tracking=1;
#endif
    o->topology.api=&o->topology_api;o->topology.batch=&o->batch;
    o->topology.attribute=main_test_attribute;o->topology.context=&o->manual;
    o->generation.owner_thread=thread;o->generation.api=&o->batch;
    o->generation.data=pairs==1?&o->data:0;
    o->generation.anchor=&o->anchor;o->generation.publication=publication;
    o->generation.generation_clock=clock;
    o->generation.observe=source_topology_observe;o->generation.observe_context=&o->topology;
    if(pairs==1) {
        o->preparation=(SourceEyePreparation){&o->generation,&o->component,&o->eyes,0,0,0,0};
        o->generation.retire_prepared=source_eye_retire;o->generation.retire_context=&o->preparation;
    }
    __atomic_store_n(publication,0,__ATOMIC_RELEASE);
    for(unsigned step=0;step<8+pairs;++step) {
        o->stage=step;
        if(!bind_step(o,step) || o->retiring) {
            o->busy=0;(void)source_live_close(o,thread);return 0;
        }
    }
    /* Bind both fixed pairs BEFORE publishing ready or admitting a source.
       The pool never aliases generation.data to an active slot. */
#ifdef GF2VR_CAPACITY_EXPERIMENT
    CameraDataComponentApi *bindings[EFO_CAPACITY]={&o->component,&o->second_component,&o->extra_component[0]};
    if(pairs==EFO_CAPACITY && !source_eye_pool_bind_many(&o->pool,&o->generation,bindings,EFO_CAPACITY,thread)) {
#else
    if(pairs==2 && !source_eye_pool_bind(&o->pool,&o->generation,&o->component,&o->second_component,thread)) {
#endif
        o->busy=0;(void)source_live_close(o,thread);return 0;
    }
#ifdef GF2VR_ACTIVITY_STACK
    if(pairs==EFO_CAPACITY) {
        o->activity.source=&o->generation;o->activity.topology=&o->topology;
        o->activity.binding=&o->component;o->activity.manual=&o->manual;
        o->topology.attribute=activity_source_attribute;o->topology.context=&o->activity;
        o->generation.extension=&o->activity;o->generation.extension_collect=activity_source_collect;
        o->generation.extension_adopt=activity_source_adopt;o->pool.activity=&o->activity;
    }
#endif
    o->busy=0;o->ready=1;return 1;
}
static int source_live_open_with(SourceLiveOwner *o,unsigned long thread,void *pipeline,void *method,
    XrEyeMailbox *mailbox,unsigned long long *publication,unsigned long long *clock,int width,int height,float units,
    int (*bind_step)(SourceLiveOwner *,unsigned),int (*close_math)(PoseMathBinding *)) {
    return source_live_open_mode_with(o,thread,pipeline,method,mailbox,publication,clock,width,height,units,
        bind_step,close_math,1);
}
static int source_live_open_two_with(SourceLiveOwner *o,unsigned long thread,void *pipeline,void *method,
    XrEyeMailbox *mailbox,unsigned long long *publication,unsigned long long *clock,int width,int height,float units,
    int (*bind_step)(SourceLiveOwner *,unsigned),int (*close_math)(PoseMathBinding *)) {
    return source_live_open_mode_with(o,thread,pipeline,method,mailbox,publication,clock,width,height,units,
        bind_step,close_math,EFO_CAPACITY);
}
static int source_live_open_two(SourceLiveOwner *o,unsigned long thread,void *pipeline,void *method,
    XrEyeMailbox *mailbox,unsigned long long *publication,unsigned long long *clock,int width,int height,float units) {
    return source_live_open_two_with(o,thread,pipeline,method,mailbox,publication,clock,width,height,units,
        source_live_bind_step,pose_math_close);
}
static int source_live_prepare_slot(SourceLiveOwner *o,unsigned slot,unsigned long thread) {
    GF2VR_PREPARE_STEP(100);
    if(!o || !o->ready || o->busy || o->retiring || thread!=o->thread) return 0;
    if(o->pair_count==EFO_CAPACITY) return source_eye_pool_prepare(&o->pool,slot,thread,o->width,o->height);
    return o->pair_count==1 && !slot && source_eye_prepare(&o->preparation,thread,o->width,o->height);
}
static int source_live_arm_flight(SourceLiveOwner *o,EyeFlightOwner *owner,EyePosePool *poses,
    EyeFlightToken token,unsigned long thread) {
    if(!o || o->pair_count!=EFO_CAPACITY || !o->ready || o->busy || o->retiring || thread!=o->thread)
        return EFO_REJECTED;
    return source_render_flight_arm(&o->flights,&o->pool,owner,poses,token,thread,
        &o->reader,&o->anchor,&o->math,&o->batch,o->pipeline,o->method);
}
static int source_live_release_flight(SourceLiveOwner *o,EyeFlightOwner *owner,
    EyeFlightToken token,unsigned long thread) {
    if(!o || o->pair_count!=EFO_CAPACITY || !thread || thread!=o->thread)return EFO_REJECTED;
    return source_render_flight_release(&o->flights,owner,token,thread);
}
static int source_live_open(SourceLiveOwner *o,unsigned long thread,void *pipeline,void *method,
    XrEyeMailbox *mailbox,unsigned long long *publication,unsigned long long *clock,int width,int height,float units) {
    return source_live_open_with(o,thread,pipeline,method,mailbox,publication,clock,width,height,units,
        source_live_bind_step,pose_math_close);
}
#ifdef GF2VR_ACTIVITY_UI_LIFETIME
#include "activity_ui_transition.h"
#endif
#endif

#else
#line 1
#ifndef GF2_SOURCE_LIVE_OWNER_H
#define GF2_SOURCE_LIVE_OWNER_H
#include "source_eye_prepare.h"
#include "source_eye_pool.h"
#include "source_render_flights.h"
#include "main_test_grant.h"
#include "eye_native_targets.h"
/* Persistent, address-stable owner. Never copy after initialization: internal
   pointers and outstanding render frames refer to this storage. Unity thread
   only. XR mailbox and publication storage remain externally owned. */
typedef struct SourceLiveOwner {
    unsigned long thread;
    unsigned busy,ready,retiring,stage,pipeline_root;
    int width,height;
    void *pipeline,*method;
    ManagedEyeBinding eye;
    CameraBatchApi batch;
    CameraDataComponentApi component;
    CameraDataComponentApi second_component;
#ifdef GF2VR_CAPACITY_EXPERIMENT
    CameraDataComponentApi extra_component[EFO_CAPACITY-2];
#endif
    SourceEyePool pool;
    SourceRenderFlights flights;
    unsigned pair_count;
    CameraDataOwner data;
    ManagedEyes eyes;
    SourcePoseApi reader;
    SourceTopologyApi topology_api;
    SourceTopology topology;
    SourceGeneration generation;
    SourceEyePreparation preparation;
    SourceAnchorState anchor;
    MainTestGrant manual;
    PoseMathBinding math;
    EyeNativeApi native;
#ifdef GF2VR_ACTIVITY_STACK
    ActivitySourceGroup activity;
#endif
    XrEyeMailbox *mailbox;
    /* Tests substitute external binding steps/CRT close, not owner transitions. */
    int (*bind_step)(struct SourceLiveOwner *,unsigned);
    int (*close_math)(PoseMathBinding *);
    void (*release_pipeline)(unsigned);
} SourceLiveOwner;
static int source_live_batch(SourceLiveOwner *o) {
    HINSTANCE game=GetModuleHandleW(L"GameAssembly.dll");if(!game) return 0;
    unsigned char (*generic)(void *)=(void *)GetProcAddress(game,"il2cpp_method_is_generic");
    void *(*array_class)(void *,unsigned)=(void *)GetProcAddress(game,"il2cpp_array_class_get");
    void *(*array_new)(void *,unsigned long long)=(void *)GetProcAddress(game,"il2cpp_array_new");
    if(!generic || !array_class || !array_new || !sa.array_length) return 0;
    CameraBatchApi b={0};b.camera_class=o->eye.api.camera_class;b.array_class=array_class(b.camera_class,1);
    void *array=render_class("mscorlib.dll","System","Array");
    b.get_value=eye_binding_method(array,"GetValue","System.Int32","System.Object",0,generic);
    const char *params[]={"System.Object","System.Int32"};
    b.set_value=eye_binding_method_args(array,"SetValue",2,params,"System.Void",0,generic);
    if(!b.array_class || !b.get_value || !b.set_value) return 0;
    b.array_new=array_new;b.array_length=sa.array_length;b.object_class=o->eye.api.object_class;
    b.pin=o->eye.api.pin;b.unpin=o->eye.api.unpin;b.invoke=o->eye.api.invoke;
    o->batch=b;return 1;
}
static int source_live_bind_step(SourceLiveOwner *o,unsigned step) {
    switch(step) {
    case 0:
        if(!sa.gchandle_new || !sa.gchandle_free) return 0;
        o->release_pipeline=sa.gchandle_free;
        o->pipeline_root=sa.gchandle_new(o->pipeline,1);return o->pipeline_root!=0;
    case 1:return eye_binding_bind(&o->eye);
    case 2:return source_live_batch(o);
    case 3: {
        CameraDataSettingsApi settings={0};CameraDataReferencesApi refs={0};
        return camera_data_settings_bind(&o->eye,&settings) &&
            camera_data_references_bind(&settings,&refs) && camera_data_component_bind(&refs,&o->component);
    }
    case 4:return source_pose_bind(&o->eye,&o->reader);
    case 5:return source_topology_bind(&o->eye,&o->topology_api);
    case 6:return eye_native_bind(&o->eye,&o->native);
    case 7:return pose_math_open(&o->math);
    case 8: {
        HINSTANCE game=GetModuleHandleW(L"GameAssembly.dll");if(!game) return 0;
        unsigned char (*generic)(void *)=(void *)GetProcAddress(game,"il2cpp_method_is_generic");
        void *klass=render_class("Unity.RenderPipelines.Universal.Runtime.dll",
            "UnityEngine.Rendering.Universal","UniversalRenderPipeline");
        if(!generic || !klass || o->eye.api.object_class(o->pipeline)!=klass) return 0;
        const char *params[]={"UnityEngine.Rendering.ScriptableRenderContext","UnityEngine.Camera[]"};
        void *method=eye_binding_method_args(klass,"Render",2,params,"System.Void",0,generic);
        return method && method==o->method;
    }
#ifdef GF2VR_CAPACITY_EXPERIMENT
    case 9:return camera_data_component_bind(&o->component.refs,&o->second_component);
    case 10:return camera_data_component_bind(&o->component.refs,&o->extra_component[0]);
#else
    case 9:return camera_data_component_bind(&o->component.refs,&o->second_component);
#endif
    default:return 0;
    }
}
/* May return pending. No binding/module/root reset until actual eye/data borrows
   and failed managed destruction are reconciled. No XR session teardown here. */
static int source_live_close(SourceLiveOwner *o,unsigned long thread) {
    if(!o || !thread || thread!=o->thread) return 0;
    o->ready=0;o->retiring=1;main_test_revoke(&o->manual);
    source_generation_invalidate(&o->generation);
    if(o->busy) return 0;
    o->busy=1;
    int clean=source_generation_collect(&o->generation,thread);
    /* A failed/uninjected root may hold no eye borrow but still owns a numeric
       flight and callback transaction. Do not unload its bindings/math early. */
    for(unsigned i=0;i<EFO_CAPACITY;++i)if(o->flights.slots[i].occupied ||
        o->flights.slots[i].frame.serial || o->flights.slots[i].scope.render.active)clean=0;
    if(clean && o->pool.source) clean=source_eye_pool_unbind(&o->pool,thread);
#ifdef GF2VR_ACTIVITY_STACK
    if(clean && o->activity.binding) {
        if(o->activity.busy)clean=0;
        else {
            activity_snapshot_drop(&o->activity,&o->activity.pending);
            activity_snapshot_drop(&o->activity,&o->activity.active);
        }
    }
#endif
#ifdef GF2VR_CAPACITY_EXPERIMENT
    if(clean)for(unsigned i=0;i<EFO_CAPACITY-2;++i)if(!camera_data_component_release(&o->extra_component[i]))clean=0;
#endif
    if(clean) clean=camera_data_component_release(&o->second_component);
    if(clean) clean=camera_data_component_release(&o->component);
    if(clean && o->pipeline_root) {
        unsigned root=o->pipeline_root;o->pipeline_root=0;o->release_pipeline(root);
    }
    if(clean) clean=eye_binding_release(&o->eye,&o->eyes);
    if(clean && o->close_math) clean=o->close_math(&o->math);
    o->busy=0;
    if(clean) *o=(SourceLiveOwner){0};
    return clean;
}
static int source_live_open_mode_with(SourceLiveOwner *o,unsigned long thread,void *pipeline,void *method,
    XrEyeMailbox *mailbox,unsigned long long *publication,unsigned long long *clock,int width,int height,float units,
    int (*bind_step)(SourceLiveOwner *,unsigned),int (*close_math)(PoseMathBinding *),unsigned pairs) {
    float checked;
    if(!o || !thread || o->thread || o->busy || o->ready || o->retiring || !pipeline || !method ||
       !mailbox || !publication || !clock || clock==publication || width<=0 || height<=0 || !(units>0) || !eye_optics_float(units,&checked) ||
       !bind_step || !close_math || (pairs!=1 && pairs!=EFO_CAPACITY)) return 0;
    /* o is zero-initialized, stable storage. Construct links before any managed
       bind can reenter and ask this owner to retire. */
    o->thread=thread;o->busy=1;o->pipeline=pipeline;o->method=method;o->mailbox=mailbox;
    o->width=width;o->height=height;o->bind_step=bind_step;o->close_math=close_math;
    o->pair_count=pairs;
    o->manual.units_per_meter=units;
#ifdef GF2VR_MAIN_HANDOFF
    o->manual.handoff_enabled=1;
#endif
#ifdef GF2VR_MAIN_FLAT_TRACKING
    o->manual.flat_tracking=1;
#endif
    o->topology.api=&o->topology_api;o->topology.batch=&o->batch;
    o->topology.attribute=main_test_attribute;o->topology.context=&o->manual;
    o->generation.owner_thread=thread;o->generation.api=&o->batch;
    o->generation.data=pairs==1?&o->data:0;
    o->generation.anchor=&o->anchor;o->generation.publication=publication;
    o->generation.generation_clock=clock;
    o->generation.observe=source_topology_observe;o->generation.observe_context=&o->topology;
    if(pairs==1) {
        o->preparation=(SourceEyePreparation){&o->generation,&o->component,&o->eyes,0,0,0,0};
        o->generation.retire_prepared=source_eye_retire;o->generation.retire_context=&o->preparation;
    }
    __atomic_store_n(publication,0,__ATOMIC_RELEASE);
    for(unsigned step=0;step<8+pairs;++step) {
        o->stage=step;
        if(!bind_step(o,step) || o->retiring) {
            o->busy=0;(void)source_live_close(o,thread);return 0;
        }
    }
    /* Bind both fixed pairs BEFORE publishing ready or admitting a source.
       The pool never aliases generation.data to an active slot. */
#ifdef GF2VR_CAPACITY_EXPERIMENT
    CameraDataComponentApi *bindings[EFO_CAPACITY]={&o->component,&o->second_component,&o->extra_component[0]};
    if(pairs==EFO_CAPACITY && !source_eye_pool_bind_many(&o->pool,&o->generation,bindings,EFO_CAPACITY,thread)) {
#else
    if(pairs==2 && !source_eye_pool_bind(&o->pool,&o->generation,&o->component,&o->second_component,thread)) {
#endif
        o->busy=0;(void)source_live_close(o,thread);return 0;
    }
#ifdef GF2VR_ACTIVITY_STACK
    if(pairs==EFO_CAPACITY) {
        o->activity.source=&o->generation;o->activity.topology=&o->topology;
        o->activity.binding=&o->component;o->activity.manual=&o->manual;
        o->topology.attribute=activity_source_attribute;o->topology.context=&o->activity;
        o->generation.extension=&o->activity;o->generation.extension_collect=activity_source_collect;
        o->generation.extension_adopt=activity_source_adopt;o->pool.activity=&o->activity;
    }
#endif
    o->busy=0;o->ready=1;return 1;
}
static int source_live_open_with(SourceLiveOwner *o,unsigned long thread,void *pipeline,void *method,
    XrEyeMailbox *mailbox,unsigned long long *publication,unsigned long long *clock,int width,int height,float units,
    int (*bind_step)(SourceLiveOwner *,unsigned),int (*close_math)(PoseMathBinding *)) {
    return source_live_open_mode_with(o,thread,pipeline,method,mailbox,publication,clock,width,height,units,
        bind_step,close_math,1);
}
static int source_live_open_two_with(SourceLiveOwner *o,unsigned long thread,void *pipeline,void *method,
    XrEyeMailbox *mailbox,unsigned long long *publication,unsigned long long *clock,int width,int height,float units,
    int (*bind_step)(SourceLiveOwner *,unsigned),int (*close_math)(PoseMathBinding *)) {
    return source_live_open_mode_with(o,thread,pipeline,method,mailbox,publication,clock,width,height,units,
        bind_step,close_math,EFO_CAPACITY);
}
static int source_live_open_two(SourceLiveOwner *o,unsigned long thread,void *pipeline,void *method,
    XrEyeMailbox *mailbox,unsigned long long *publication,unsigned long long *clock,int width,int height,float units) {
    return source_live_open_two_with(o,thread,pipeline,method,mailbox,publication,clock,width,height,units,
        source_live_bind_step,pose_math_close);
}
static int source_live_prepare_slot(SourceLiveOwner *o,unsigned slot,unsigned long thread) {
    GF2VR_PREPARE_STEP(100);
    if(!o || !o->ready || o->busy || o->retiring || thread!=o->thread) return 0;
    if(o->pair_count==EFO_CAPACITY) return source_eye_pool_prepare(&o->pool,slot,thread,o->width,o->height);
    return o->pair_count==1 && !slot && source_eye_prepare(&o->preparation,thread,o->width,o->height);
}
static int source_live_arm_flight(SourceLiveOwner *o,EyeFlightOwner *owner,EyePosePool *poses,
    EyeFlightToken token,unsigned long thread) {
    if(!o || o->pair_count!=EFO_CAPACITY || !o->ready || o->busy || o->retiring || thread!=o->thread)
        return EFO_REJECTED;
    return source_render_flight_arm(&o->flights,&o->pool,owner,poses,token,thread,
        &o->reader,&o->anchor,&o->math,&o->batch,o->pipeline,o->method);
}
static int source_live_release_flight(SourceLiveOwner *o,EyeFlightOwner *owner,
    EyeFlightToken token,unsigned long thread) {
    if(!o || o->pair_count!=EFO_CAPACITY || !thread || thread!=o->thread)return EFO_REJECTED;
    return source_render_flight_release(&o->flights,owner,token,thread);
}
static int source_live_open(SourceLiveOwner *o,unsigned long thread,void *pipeline,void *method,
    XrEyeMailbox *mailbox,unsigned long long *publication,unsigned long long *clock,int width,int height,float units) {
    return source_live_open_with(o,thread,pipeline,method,mailbox,publication,clock,width,height,units,
        source_live_bind_step,pose_math_close);
}
#ifdef GF2VR_ACTIVITY_UI_LIFETIME
#include "activity_ui_transition.h"
#endif
#endif

#endif
