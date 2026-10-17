#ifndef GF2_CAMERA_STACK_MANAGED_H
#define GF2_CAMERA_STACK_MANAGED_H
#include "camera_stack_links.h"
/* Typed managed adapter for private, unused Base/Overlay groups. Components,
   cameras and generation remain caller-owned through all real CPU/GPU borrows.
   This operation grants neither source admission nor rendering/publication.
   Discard a partial private group on failure after its actual borrows drain. */
typedef struct {
    const CameraDataReferencesApi *refs;
    const CameraStackMember *members;
    void *components[CSL_CAPACITY][3];
    void *lists[CSL_CAPACITY][3][2];
    unsigned count;
    void *context;
    int (*valid)(void *);
    int (*bind_list)(const CameraDataReferencesApi *,void *,CameraDataListApi *);
} CameraStackManaged;
static int csm_bind_list(const CameraDataReferencesApi *a,void *list,CameraDataListApi *out) {
    if(!camera_data_list_bind(a,list,0,out))return 0;
    void *k=a->settings.calls.object_class(list);
    out->item=eye_binding_method(k,"get_Item","System.Int32","UnityEngine.Camera",0,a->generic);
    out->clear=eye_binding_method(k,"Clear",0,"System.Void",0,a->generic);
    out->add=eye_binding_method(k,"Add","UnityEngine.Camera","System.Void",0,a->generic);
    return out->item && out->clear && out->add;
}
static int csm_valid(void *context) {
    CameraStackManaged *a=context;
    return a && a->valid && a->valid(a->context);
}
static int csm_find(CameraStackManaged *a,void *camera,unsigned *member,unsigned *eye) {
    if(!csm_valid(a) || !camera || a->refs->settings.calls.object_class(camera)!=a->refs->settings.calls.camera_class)return 0;
    for(unsigned i=0;i<a->count;++i) {
        void *v[]={a->members[i].source,a->members[i].left,a->members[i].right};
        for(unsigned j=0;j<3;++j)if(camera==v[j]){*member=i;*eye=j;return 1;}
    }
    return 0;
}
static unsigned csm_pin(void *context,void *object) {
    CameraStackManaged *a=context;
    return csm_valid(a)?a->refs->settings.calls.pin(object,1):0;
}
static void csm_unpin(void *context,unsigned root) {
    CameraStackManaged *a=context;a->refs->settings.calls.unpin(root);
}
static int csm_role(void *context,void *camera,unsigned *role) {
    CameraStackManaged *a=context;unsigned i=0,j=0;
    return csm_find(a,camera,&i,&j) && camera_data_u32(a->refs,
        a->refs->settings.get[4],a->components[i][j],a->refs->settings.value_class[4],role) && csm_valid(a);
}
static int csm_list(void *context,void *camera,unsigned which,void **out) {
    CameraStackManaged *a=context;unsigned i=0,j=0;*out=0;
    if(which>1 || !csm_find(a,camera,&i,&j))return 0;
    void *method=which?a->refs->get_stack:a->refs->get_cameras;
    if(!camera_data_stack_list(a->refs,a->components[i][j],method,out) || !csm_valid(a))return 0;
    a->lists[i][j][which]=*out;return 1;
}
static int csm_methods(CameraStackManaged *a,void *list,CameraDataListApi *methods) {
    return list && csm_valid(a) && a->bind_list(a->refs,list,methods) && csm_valid(a);
}
static int csm_count(void *context,void *list,unsigned *count) {
    CameraStackManaged *a=context;CameraDataListApi methods={0};
    return csm_methods(a,list,&methods) && camera_data_u32(a->refs,methods.count,list,a->refs->int_class,count) && csm_valid(a);
}
static int csm_item(void *context,void *list,unsigned index,void **out) {
    CameraStackManaged *a=context;CameraDataListApi methods={0};int n=(int)index;void *args[]={&n};*out=0;
    if(index>=CSL_CAPACITY || !csm_methods(a,list,&methods) ||
       !managed_eye_call(&a->refs->settings.calls,methods.item,list,args,out) || !*out)return 0;
    return a->refs->settings.calls.object_class(*out)==a->refs->settings.calls.camera_class && csm_valid(a);
}
/* Defense in depth: an adapter write never targets a source list, even if a
   future caller misuses the link API. Only observed private lists are writable. */
static int csm_private_list(CameraStackManaged *a,void *list) {
    if(!list || !csm_valid(a))return 0;
    unsigned private_seen=0;
    for(unsigned i=0;i<a->count;++i)for(unsigned j=0;j<3;++j)for(unsigned k=0;k<2;++k)
        if(a->lists[i][j][k]==list){if(!j)return 0;private_seen=1;}
    return private_seen!=0;
}
static int csm_clear(void *context,void *list) {
    CameraStackManaged *a=context;CameraDataListApi methods={0};
    return csm_private_list(a,list) && csm_methods(a,list,&methods) &&
        managed_eye_call(&a->refs->settings.calls,methods.clear,list,0,0) && csm_valid(a);
}
static int csm_add(void *context,void *list,void *camera) {
    CameraStackManaged *a=context;CameraDataListApi methods={0};unsigned i=0,j=0;void *args[]={camera};
    return csm_private_list(a,list) && csm_find(a,camera,&i,&j) && j &&
        csm_methods(a,list,&methods) && managed_eye_call(&a->refs->settings.calls,methods.add,list,args,0) && csm_valid(a);
}
static int camera_stack_managed_prepare_with(CameraStackManaged *a,CameraStackScratch *scratch,unsigned refresh) {
    if(!a || !a->refs || !a->refs->bound || !a->refs->settings.bound || !a->members ||
       !a->count || a->count>CSL_CAPACITY || !a->valid || !a->bind_list ||
       !a->refs->settings.calls.pin || !a->refs->settings.calls.unpin ||
       !a->refs->settings.calls.object_class || !a->refs->settings.calls.invoke ||
       !a->refs->settings.calls.unbox || !csm_valid(a))return 0;
    unsigned roots[CSL_CAPACITY*3]={0},nr=0;int ok=0;
    for(unsigned i=0;i<a->count;++i)for(unsigned j=0;j<3;++j) {
        void *component=a->components[i][j];
        if(!component || a->refs->settings.calls.object_class(component)!=a->refs->settings.component_class)goto done;
        for(unsigned k=0;k<=i;++k)for(unsigned l=0;l<3;++l)
            if((k<i || l<j) && component==a->components[k][l])goto done;
        unsigned root=a->refs->settings.calls.pin(component,1);if(!root)goto done;roots[nr++]=root;
        for(unsigned l=0;l<2;++l)a->lists[i][j][l]=0;
    }
    CameraStackLinkApi links={a,csm_valid,csm_pin,csm_unpin,csm_role,csm_list,
                             csm_count,csm_item,csm_clear,csm_add};
    ok=camera_stack_links_prepare_with(&links,a->members,a->count,scratch,refresh);
done:
    while(nr)a->refs->settings.calls.unpin(roots[--nr]);
    for(unsigned i=0;i<a->count;++i)for(unsigned j=0;j<3;++j)for(unsigned k=0;k<2;++k)a->lists[i][j][k]=0;
    return ok;
}
static int camera_stack_managed_prepare(CameraStackManaged *a) {
    CameraStackScratch scratch={0};return camera_stack_managed_prepare_with(a,&scratch,0);
}
#endif
