#ifndef GF2_HOME_TIMELINE_READ_H
#define GF2_HOME_TIMELINE_READ_H
/* Exact current Commander owns this root; names/order/renderer IDs never select. */
typedef struct {
    SourcePoseApi values;
    void *root_field,*gameobject_class,*bool_class,*active,*live,*components,*get_value;
    void *(*class_type)(void *);
    void *(*type_object)(void *);
    void (*read)(void *,void *,void *);
    unsigned long long (*length)(void *);
} HomeTimelineApi;
/* -1 unsupported/ambiguous, 0 no live timeline camera, 1 unique rooted camera.
   A successful camera root is transferred to the caller. */
static int home_timeline_read(HomeTimelineApi *h,void *owner,void **out,unsigned *out_root){
    *out=0;*out_root=0;
    if(!h || !owner || !h->root_field || !h->class_type || !h->type_object || !h->components || !h->get_value ||
       !h->active || !h->live || !h->length)return -1;
    const ManagedEyeApi *c=&h->values.calls;
    void *root=0,*items=0,*chosen=0;unsigned roots[3]={0},chosen_root=0;int result=-1;
    h->read(owner,h->root_field,&root);if(!root)return 0;
    roots[0]=c->pin(root,1);if(!roots[0] || c->object_class(root)!=h->gameobject_class)goto done;
    unsigned char active=0;
    if(!source_pose_value(&h->values,h->active,root,h->bool_class,&active,1) || active>1)goto done;
    if(!active){result=0;goto done;}
    void *camera_type=h->type_object(h->class_type(c->camera_class));
    if(!camera_type || !(roots[1]=c->pin(camera_type,1)))goto done;
    void *args[]={camera_type};
    if(!managed_eye_call(c,h->components,root,args,&items) || !items || !(roots[2]=c->pin(items,1)))goto done;
    unsigned long long count=h->length(items);if(count>8)goto done;
    for(int i=0;i<(int)count;++i){
        void *camera=0;unsigned held=0;void *index[]={&i};
        if(!managed_eye_call(c,h->get_value,items,index,&camera) || !camera || !(held=c->pin(camera,1)))goto done;
        unsigned char live=0;
        int valid=c->object_class(camera)==c->camera_class &&
            source_pose_value(&h->values,h->live,camera,h->bool_class,&live,1) && live<=1;
        if(!valid || (live && chosen)){c->unpin(held);goto done;}
        if(live){chosen=camera;chosen_root=held;}else c->unpin(held);
    }
    void *after=0;h->read(owner,h->root_field,&after);if(after!=root)goto done;
    result=chosen?1:0;
    if(chosen){*out=chosen;*out_root=chosen_root;chosen_root=0;}
done:
    if(chosen_root)c->unpin(chosen_root);
    for(unsigned i=3;i>0;--i)if(roots[i-1])c->unpin(roots[i-1]);
    return result;
}
#endif
