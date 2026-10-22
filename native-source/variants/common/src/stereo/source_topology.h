#include "il2cpp_export.h"
#ifndef GF2_SOURCE_TOPOLOGY_H
#define GF2_SOURCE_TOPOLOGY_H
#include "source_generation.h"
#ifndef GF2VR_ADMISSION_TRANSFORM
#define GF2VR_ADMISSION_TRANSFORM(root,actual,expected) ((void)0)
#endif
#ifndef GF2VR_TOPOLOGY_INPUT_REPORT
#define GF2VR_TOPOLOGY_INPUT_REPORT(s,r,n,v,i,j,c) ((void)0)
#endif
/* Live, read-only adapter. Numeric output is evidence for a game policy, not a
   main-scene classifier. No diagnostic counters/deadlines, names or selectors. */
typedef struct {
    SourcePoseApi values;
    void *parent,*instance,*orthographic,*display,*target,*width,*height,*mask;
    void *transform_class,*int_class,*bool_class;
    void *(*class_parent)(void *);
} SourceTopologyApi;
typedef struct {
    SourceGenerationCandidate identity;
    int orthographic,display,width,height,mask;
    unsigned has_target;
} SourceTopologyEntry;
/* policy must classify the complete snapshot together. Return exactly one
   attributed index plus explicit units, or fail. It may not retain pointers.
   A mere CameraProfileData/name/pointer match is not an attribution policy. */
typedef struct {
    const SourceTopologyApi *api;
    const CameraBatchApi *batch;
    void *context;
    int (*attribute)(void *,const SourceTopologyEntry *,unsigned,unsigned *,float *);
    unsigned busy,cancelled;
    /* Owner storage, not a large no-CRT hook stack frame. Cleared on return. */
    SourceTopologyEntry entries[32];
    void *cameras[32];
    unsigned roots[33];
#ifdef GF2VR_ACTIVITY_STACK
    unsigned long long group_stamp;
#endif
} SourceTopology;
static int source_topology_bind(const ManagedEyeBinding *eye,SourceTopologyApi *out) {
    GF2VR_ADMISSION_STEP(200);
    if(!out) return 0;
    *out=(SourceTopologyApi){0};
    if(!eye || !eye->type_root) return 0;
    HINSTANCE module=GetModuleHandleW(L"GameAssembly.dll");if(!module) return 0;
    unsigned char (*generic)(void *)=(void *)il2cpp_export(module,"il2cpp_method_is_generic");
    int (*size)(void *,unsigned *)=(void *)il2cpp_export(module,"il2cpp_class_value_size");
    if(!generic || !size) return 0;
    SourceTopologyApi a={0};a.values.calls=eye->api;a.class_parent=sa.class_get_parent;
    a.transform_class=render_class("UnityEngine.CoreModule.dll","UnityEngine","Transform");
    a.int_class=render_class("mscorlib.dll","System","Int32");
    a.bool_class=render_class("mscorlib.dll","System","Boolean");
    unsigned align=0;
    if(!a.transform_class || !a.int_class || !a.bool_class || !a.class_parent ||
       size(a.int_class,&align)!=4 || !align || align>_Alignof(int) ||
       size(a.bool_class,&align)!=1 || align!=1) return 0;
    void *object=render_class("UnityEngine.CoreModule.dll","UnityEngine","Object");
    a.instance=eye_binding_method(object,"GetInstanceID",0,"System.Int32",0,generic);
    a.parent=eye_binding_method(a.transform_class,"get_parent",0,"UnityEngine.Transform",0,generic);
    void *camera=a.values.calls.camera_class;
    a.orthographic=eye_binding_method(camera,"get_orthographic",0,"System.Boolean",0,generic);
    a.display=eye_binding_method(camera,"get_targetDisplay",0,"System.Int32",0,generic);
    a.target=eye_binding_method(camera,"get_targetTexture",0,"UnityEngine.RenderTexture",0,generic);
    a.width=eye_binding_method(camera,"get_pixelWidth",0,"System.Int32",0,generic);
    a.height=eye_binding_method(camera,"get_pixelHeight",0,"System.Int32",0,generic);
    a.mask=eye_binding_method(camera,"get_cullingMask",0,"System.Int32",0,generic);
    if(!a.instance || !a.parent || !a.orthographic || !a.display || !a.target || !a.width || !a.height || !a.mask ||
       !a.values.calls.get_transform || !a.values.calls.unbox || !a.values.calls.pin ||
       !a.values.calls.unpin || !a.values.calls.invoke || !a.values.calls.object_class) return 0;
    *out=a;return 1;
}
/* Caller roots camera. Root every parent and box across subsequent managed
   operations; only copied numeric identities survive this function. */
static int source_topology_transform(const SourceTopologyApi *a,void *klass) {
    /* Component.transform/Transform.parent may return a derived transform.
       Follow metadata ancestry, never names or an unchecked object cast. */
    void *seen[8]={0};
    for(unsigned i=0;i<8 && klass;++i) {
        if(klass==a->transform_class) return 1;
        for(unsigned j=0;j<i;++j) if(seen[j]==klass) return 0;
        seen[i]=klass;if(!a->class_parent) return 0;
        klass=a->class_parent(klass);
    }
    return 0;
}
static int source_topology_entry(const SourceTopologyApi *a,void *camera,SourceTopologyEntry *out) {
    const ManagedEyeApi *c=&a->values.calls;
    SourceTopologyEntry entry={0};entry.identity.source=camera;
    unsigned roots[8]={0};unsigned char ortho=0;void *current=0,*target=0;int ok=0;
    if(!(GF2VR_ADMISSION_STEP(301),source_pose_value(&a->values,a->instance,camera,a->int_class,&entry.identity.instance_id,4)) ||
       !entry.identity.instance_id ||
       !(GF2VR_ADMISSION_STEP(302),source_pose_value(&a->values,a->orthographic,camera,a->bool_class,&ortho,1)) || ortho>1 ||
       !(GF2VR_ADMISSION_STEP(303),source_pose_value(&a->values,a->display,camera,a->int_class,&entry.display,4)) ||
       !(GF2VR_ADMISSION_STEP(304),source_pose_value(&a->values,a->width,camera,a->int_class,&entry.width,4)) ||
       !(GF2VR_ADMISSION_STEP(305),source_pose_value(&a->values,a->height,camera,a->int_class,&entry.height,4)) ||
       !(GF2VR_ADMISSION_STEP(306),source_pose_value(&a->values,a->mask,camera,a->int_class,&entry.mask,4)) ||
       !(GF2VR_ADMISSION_STEP(307),managed_eye_call(c,a->target,camera,0,&target))) goto end;
    entry.orthographic=ortho;entry.has_target=target!=0;
    GF2VR_ADMISSION_STEP(308);
    if(!managed_eye_call(c,c->get_transform,camera,0,&current) || !current) goto end;
    for(unsigned i=0;i<8 && current;++i) {
        GF2VR_ADMISSION_STEP(309);
        for(unsigned j=0;j<i;++j) if(entry.identity.owners[j]==(unsigned long long)current) goto end;
        GF2VR_ADMISSION_STEP(310);
        roots[i]=c->pin(current,1);
        void *actual=roots[i]?c->object_class(current):0;
        if(!roots[i] || !source_topology_transform(a,actual)) {
            GF2VR_ADMISSION_TRANSFORM(roots[i],actual,a->transform_class);
            goto end;
        }
        entry.identity.owners[i]=(unsigned long long)current;
        GF2VR_ADMISSION_STEP(311);
        if(!source_pose_value(&a->values,a->instance,current,a->int_class,&entry.identity.owner_ids[i],4) ||
           !entry.identity.owner_ids[i]) goto end;
        entry.identity.count=i+1;
        GF2VR_ADMISSION_STEP(312);
        if(!managed_eye_call(c,a->parent,current,0,&current)) goto end;
    }
    GF2VR_ADMISSION_STEP(313);
    if(current) goto end; /* capped/incomplete ancestry is unknown, not valid */
    ok=1;
end:
    for(unsigned i=8;i>0;--i) if(roots[i-1]) c->unpin(roots[i-1]);
    if(ok) *out=entry;
    return ok;
}
static int source_topology_observe(void *context,void *array,SourceGenerationCandidate *out) {
    SourceTopology *s=context;
    GF2VR_ADMISSION_STEP(200);
    if(!out) return 0;
    *out=(SourceGenerationCandidate){0};
    if(!s) return 0;
    if(s->busy) {s->cancelled=1;return 0;}
    if(!s->api || !s->batch || !s->attribute) return 0;
    const CameraBatchApi *b=s->batch;const SourceTopologyApi *a=s->api;
    GF2VR_ADMISSION_STEP(201);
    if(!b->pin || !b->unpin || !b->invoke || !b->get_value || !b->object_class || !b->array_length ||
       !a->values.calls.pin || !a->values.calls.unpin || !a->values.calls.invoke ||
       !a->values.calls.object_class || !a->values.calls.unbox || !a->values.calls.get_transform ||
       !a->instance || !a->parent || !a->orthographic || !a->display || !a->target || !a->width || !a->height || !a->mask ||
       !a->int_class || !a->bool_class || !a->transform_class || !b->camera_class || !b->array_class ||
       a->values.calls.camera_class!=b->camera_class ||
       !array || b->object_class(array)!=b->array_class) return 0;
    s->busy=1;s->cancelled=0;
#ifdef GF2VR_ACTIVITY_STACK
    s->group_stamp=0;
#endif
    unsigned *roots=s->roots,count=0,index=0;float units=0,checked;int ok=0;
#ifdef GF2VR_ACTIVITY_UI_FRAME_ELIGIBLE
    unsigned selected[32]={0},selected_count=0;unsigned char omitted[32]={0};
#endif
    SourceTopologyEntry *entries=s->entries;void **cameras=s->cameras;
    GF2VR_ADMISSION_STEP(202);
    roots[0]=b->pin(array,1);if(!roots[0]) goto end;
    GF2VR_ADMISSION_STEP(203);
    count=b->array_length(array);if(!count || count>32) goto end;
    /* Pin the whole input before getters may reenter Unity/mutate the array. */
    for(unsigned i=0;i<count;++i) {
        int n=(int)i;void *args[]={&n},*exception=0;
        GF2VR_ADMISSION_STEP(204);
        cameras[i]=b->invoke(b->get_value,array,args,&exception);
        if(exception || !cameras[i]) goto end;
        GF2VR_ADMISSION_STEP(205);
        roots[i+1]=b->pin(cameras[i],1);
        if(!roots[i+1] || b->object_class(cameras[i])!=b->camera_class) goto end;
        GF2VR_ADMISSION_STEP(206);
        for(unsigned j=0;j<i;++j) if(cameras[i]==cameras[j]) {
            GF2VR_TOPOLOGY_INPUT_REPORT(s,1,count,i+1,i,j,cameras);goto end;
        }
    }
#ifdef GF2VR_ACTIVITY_UI_FRAME_ELIGIBLE
    /* Whole raw array is rooted/validated above and rechecked below. Never
       remove an original game camera by a generic role/layer/name heuristic. */
    GF2VR_TOPOLOGY_REAP(cameras,count);
    if(s->cancelled){GF2VR_TOPOLOGY_INPUT_REPORT(s,2,count,count,32,32,cameras);goto end;}
    for(unsigned i=0;i<count;++i) {
        int owned=GF2VR_TOPOLOGY_PRIVATE(cameras[i]);
        if(owned<0){GF2VR_TOPOLOGY_INPUT_REPORT(s,3,count,count,i,32,cameras);goto end;}
        omitted[i]=owned!=0;if(owned)continue;
        selected[selected_count]=i;
        if(!source_topology_entry(a,cameras[i],&entries[selected_count])){
            GF2VR_TOPOLOGY_INPUT_REPORT(s,4,count,count,i,32,cameras);goto end;
        }
        ++selected_count;
    }
    if(!selected_count){GF2VR_TOPOLOGY_INPUT_REPORT(s,5,count,count,32,32,cameras);goto end;}
    GF2VR_TOPOLOGY_INPUT_REPORT(s,0,count,count,selected_count,32,cameras);
#else
    for(unsigned i=0;i<count;++i) if(!source_topology_entry(a,cameras[i],&entries[i])) goto end;
#endif
    GF2VR_ADMISSION_STEP(207);
#ifdef GF2VR_ACTIVITY_UI_FRAME_ELIGIBLE
    if(!s->attribute(s->context,entries,selected_count,&index,&units) || s->cancelled || index>=selected_count ||
#else
    if(!s->attribute(s->context,entries,count,&index,&units) || s->cancelled || index>=count ||
#endif
       !(units>0) || !eye_optics_float(units,&checked)) goto end;
    GF2VR_ADMISSION_STEP(208);
    SourceTopologyEntry current={0};
#ifdef GF2VR_ACTIVITY_UI_FRAME_ELIGIBLE
    if(!source_topology_entry(a,cameras[selected[index]],&current) ||
#else
    if(!source_topology_entry(a,cameras[index],&current) ||
#endif
       !source_generation_same(&current.identity,&entries[index].identity) ||
       current.orthographic!=entries[index].orthographic || current.display!=entries[index].display ||
       current.width!=entries[index].width || current.height!=entries[index].height ||
       current.has_target!=entries[index].has_target || current.mask!=entries[index].mask) goto end;
    /* Recheck the original input after the callback. No pre-getter array
       membership may silently authorize a replacement or removed source. */
    if(b->array_length(array)!=count) goto end;
    for(unsigned i=0;i<count;++i) {
        int n=(int)i;void *args[]={&n},*exception=0;
        void *camera=b->invoke(b->get_value,array,args,&exception);
        if(exception || camera!=cameras[i]) goto end;
#ifdef GF2VR_ACTIVITY_UI_FRAME_ELIGIBLE
        int owned=GF2VR_TOPOLOGY_PRIVATE(camera);
        if(owned<0 || (owned!=0)!=omitted[i])goto end;
#endif
    }
    *out=entries[index].identity;out->units_per_meter=units;out->attributed=1;ok=1;
#ifdef GF2VR_ACTIVITY_STACK
    out->group_stamp=s->group_stamp;
#endif
end:
    for(unsigned i=33;i>0;--i) if(roots[i-1]) b->unpin(roots[i-1]);
    for(unsigned i=0;i<33;++i) roots[i]=0;
    for(unsigned i=0;i<32;++i) {entries[i]=(SourceTopologyEntry){0};cameras[i]=0;}
    if(s->cancelled) {*out=(SourceGenerationCandidate){0};ok=0;}
    s->busy=0;return ok;
}
#endif
