#ifndef GF2_ACTIVITY_PRIVATE_BATCH_H
#define GF2_ACTIVITY_PRIVATE_BATCH_H
/* Independent private output can enter the same engine Render method. It is
   neither an original world observation nor an opportunity to inject eyes. */
typedef struct {unsigned busy,private_seen,other_seen;} ActivityPrivateBatch;
typedef struct {void *group;U64 generation;unsigned root,retired;int instance;} ActivityPrivateBatchIdentity;
static int activity_private_batch_identity(void *camera,ActivityPrivateBatchIdentity *out) {
    if(!camera || !out || !uicr_thread() || GetCurrentThreadId()!=uicr_thread())return 0;
    for(unsigned i=0;i<UICR_CAPACITY;++i)if(uicr.rows[i].root && uicr.rows[i].camera==camera) {
        *out=(ActivityPrivateBatchIdentity){uicr.rows[i].group,uicr.rows[i].generation,
            uicr.rows[i].root,uicr.rows[i].retired,uicr.rows[i].instance};
        return out->group && out->generation && out->instance;
    }
    return 0;
}
static int activity_private_batch_same(ActivityPrivateBatchIdentity a,ActivityPrivateBatchIdentity b) {
    return a.group==b.group && a.generation==b.generation && a.root==b.root &&
        a.retired==b.retired && a.instance==b.instance;
}
static void activity_private_batch_begin(ActivityPrivateBatch *s) {
    s->private_seen=s->other_seen=0; /* never clear an in-progress reader */
}
static int activity_private_batch_independent(const ActivityPrivateBatch *s,unsigned failed) {
    return !failed && s->private_seen && !s->other_seen;
}
static int activity_private_batch_only(ActivityPrivateBatch *s,const CameraBatchApi *a,void *array) {
    if(!s || s->busy || !a || !array || !uicr_thread() || GetCurrentThreadId()!=uicr_thread() ||
       !a->pin || !a->unpin || !a->object_class || !a->array_length || !a->invoke || !a->get_value ||
       !a->camera_class || !a->array_class || a->object_class(array)!=a->array_class)return 0;
    s->busy=1;unsigned roots[33]={0},count=0;void *cameras[32]={0};int ok=0;
    ActivityPrivateBatchIdentity identities[32];
    roots[0]=a->pin(array,1);if(!roots[0])goto end;
    count=a->array_length(array);if(!count || count>32)goto end;
    for(unsigned i=0;i<count;++i) {
        int index=(int)i;void *args[]={&index},*exception=0;
        cameras[i]=a->invoke(a->get_value,array,args,&exception);
        if(exception || !cameras[i])goto end;
        roots[i+1]=a->pin(cameras[i],1);
        if(!roots[i+1] || a->object_class(cameras[i])!=a->camera_class)goto end;
        for(unsigned j=0;j<i;++j)if(cameras[i]==cameras[j])goto end;
        if(!activity_private_batch_identity(cameras[i],&identities[i]))goto end;
    }
    if(a->array_length(array)!=count)goto end;
    for(unsigned i=0;i<count;++i) {
        int index=(int)i;void *args[]={&index},*exception=0;
        void *camera=a->invoke(a->get_value,array,args,&exception);ActivityPrivateBatchIdentity check;
        if(exception || camera!=cameras[i] || !activity_private_batch_identity(camera,&check) ||
           !activity_private_batch_same(identities[i],check))goto end;
    }
    if(a->array_length(array)!=count)goto end;
    ok=1;
end:
    for(unsigned i=33;i>0;--i)if(roots[i-1])a->unpin(roots[i-1]);
    /* Registry roots are independent of these temporary pins. Unknown or
       changed ownership at cleanup cannot authorize a private-only bypass. */
    for(unsigned i=0;ok && i<count;++i){ActivityPrivateBatchIdentity check;
        if(!activity_private_batch_identity(cameras[i],&check) ||
           !activity_private_batch_same(identities[i],check))ok=0;}
    s->busy=0;return ok;
}
#endif
