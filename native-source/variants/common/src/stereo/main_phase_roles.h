#ifndef GF2_MAIN_PHASE_ROLES_H
#define GF2_MAIN_PHASE_ROLES_H
/* Diagnostic only. Caller holds the current topology camera root. Values and
   list edges are evidence, never eligibility. No source setters or new renders. */
typedef struct {unsigned present,complete,count; unsigned long long cameras[8];} PhaseRoleList;
typedef struct {
    unsigned attached,complete,valid,render_type,clear_depth,renderer,output;
    PhaseRoleList lists[2];
} PhaseRoleSnapshot;
typedef int (*PhaseListBind)(const CameraDataReferencesApi *,void *,void **,void **);
static int phase_list_bind(const CameraDataReferencesApi *a,void *list,void **count,void **item) {
    CameraDataListApi l={0};
    if(!camera_data_list_bind(a,list,0,&l)) return 0;
    *count=l.count;
    *item=eye_binding_method(a->settings.calls.object_class(list),"get_Item","System.Int32",
        "UnityEngine.Camera",0,a->generic);
    return *item!=0;
}
static int phase_role_list(const CameraDataReferencesApi *a,void *component,void *getter,
    PhaseListBind bind,PhaseRoleList *out) {
    const ManagedEyeApi *c=&a->settings.calls;void *list=0,*count_method=0,*item_method=0;
    *out=(PhaseRoleList){0};
    if(!camera_data_stack_list(a,component,getter,&list)) return 0;
    if(!list) {out->complete=1;return 1;}
    out->present=1;unsigned root=c->pin(list,1);if(!root) return 0;
    int ok=0;
    if(!bind(a,list,&count_method,&item_method) ||
       !camera_data_u32(a,count_method,list,a->int_class,&out->count) || out->count>8) goto end;
    for(unsigned i=0;i<out->count;++i) {
        int index=(int)i;void *args[]={&index},*camera=0;
        if(!managed_eye_call(c,item_method,list,args,&camera) || !camera ||
           c->object_class(camera)!=c->camera_class) goto end;
        /* Copy numeric identity immediately. Never retain or dereference it. */
        out->cameras[i]=(unsigned long long)camera;
    }
    unsigned after=0;
    if(!camera_data_u32(a,count_method,list,a->int_class,&after) || after!=out->count) goto end;
    ok=1;
end:
    c->unpin(root);out->complete=ok;return ok;
}
static void phase_role_read(const CameraDataComponentApi *b,void *camera,
    PhaseListBind bind,PhaseRoleSnapshot *out) {
    *out=(PhaseRoleSnapshot){0};
    if(!b || !b->type_root || !b->refs.bound || !camera || !bind) return;
    const CameraDataReferencesApi *a=&b->refs;const ManagedEyeApi *c=&a->settings.calls;
    void *args[]={b->type},*component=0;
    if(!managed_eye_call(c,b->get_component,camera,args,&component)) return;
    if(!component) {out->complete=1;return;}
    unsigned root=c->pin(component,1);if(!root) return;
    out->attached=1;
    if(c->object_class(component)!=a->settings.component_class) goto end;
    SourcePoseApi values={0};values.calls=*c;
    unsigned indexes[]={4,10,11};unsigned *dest[]={&out->render_type,&out->clear_depth,&out->renderer};
    for(unsigned i=0;i<3;++i) {
        unsigned n=indexes[i];
        if(source_pose_value(&values,a->settings.get[n],component,a->settings.value_class[n],
            dest[i],camera_data_setting_specs[n].size)) out->valid|=1u<<i;
    }
    if(camera_data_u32(a,a->get_output,component,a->output_class,&out->output)) out->valid|=8;
    phase_role_list(a,component,a->get_cameras,bind,&out->lists[0]);
    phase_role_list(a,component,a->get_stack,bind,&out->lists[1]);
    out->complete=out->valid==15 && out->lists[0].complete && out->lists[1].complete;
end:
    c->unpin(root);
}
#endif
