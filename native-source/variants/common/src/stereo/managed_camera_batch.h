#ifndef GF2_MANAGED_CAMERA_BATCH_H
#define GF2_MANAGED_CAMERA_BATCH_H
/* Public IL2CPP APIs plus runtime-resolved System.Array methods. No managed
   object-layout writes. Owner-thread only; batch is scoped to one invocation. */
typedef struct {
    void *camera_class,*array_class,*get_value,*set_value;
    void *(*object_class)(void *);
    unsigned (*array_length)(void *);
    void *(*array_new)(void *,unsigned long long);
    unsigned (*pin)(void *,int);
    void (*unpin)(unsigned);
    void *(*invoke)(void *,void *,void **,void **);
} CameraBatchApi;
typedef struct { void *array; unsigned roots[4]; } CameraBatch;
static void camera_batch_release(const CameraBatchApi *api,CameraBatch *batch) {
    for(unsigned i=4;i>0;--i) if(batch->roots[i-1]) {
        api->unpin(batch->roots[i-1]);batch->roots[i-1]=0;
    }
    batch->array=0;
}
/* Destination must be empty. On any error, caller forwards the original array.
   The producer owns the disabled eye-camera configuration and generation; this
   function only constructs/roots the replacement array. */
static int camera_batch_build_source(const CameraBatchApi *api,CameraBatch *batch,
                              void *original,void *left,void *right,void *required_source) {
    if(batch->array || batch->roots[0] || batch->roots[1] || batch->roots[2] || batch->roots[3]) return 0;
    if(!api->camera_class || !api->array_class || !api->get_value || !api->set_value ||
       !api->object_class || !api->array_length || !api->array_new || !api->pin ||
       !api->unpin || !api->invoke || !original || !left || !right || left==right) return 0;
    if(api->object_class(original)!=api->array_class ||
       api->object_class(left)!=api->camera_class || api->object_class(right)!=api->camera_class) return 0;
    batch->roots[0]=api->pin(original,1);if(!batch->roots[0]) goto failed;
    batch->roots[1]=api->pin(left,1);if(!batch->roots[1]) goto failed;
    batch->roots[2]=api->pin(right,1);if(!batch->roots[2]) goto failed;
    unsigned count=api->array_length(original);
    if(!count || count>32) goto failed;
    void *output=api->array_new(api->camera_class,(unsigned long long)count+2);
    if(!output) goto failed;
    batch->roots[3]=api->pin(output,1);if(!batch->roots[3]) goto failed;
    if(api->object_class(output)!=api->array_class || api->array_length(output)!=count+2) goto failed;
    unsigned source_count=0;
    for(unsigned i=0;i<count+2;++i) {
        void *item;void *exception=0;
        if(i<2) item=i?right:left;
        else {
            int index=(int)i-2;void *args[]={&index};
            item=api->invoke(api->get_value,original,args,&exception);
            if(exception || !item || item==left || item==right ||
               api->object_class(item)!=api->camera_class) goto failed;
            if(required_source && item==required_source) ++source_count;
        }
        /* IL2CPP reference parameters are the object pointer itself; scalar
           Int32 parameters are addresses. Runtime SetValue performs the GC barrier. */
        int index=(int)i;void *args[]={item,&index};
        exception=0;api->invoke(api->set_value,output,args,&exception);
        if(exception) goto failed;
    }
    /* Count the original references actually copied into the private array,
       not an earlier allCameras snapshot. Missing/ambiguous source is fallback.
       The producer retains its source root through this operation. */
    if(required_source && source_count!=1) goto failed;
    batch->array=output;return 1;
failed:
    camera_batch_release(api,batch);return 0;
}
static int camera_batch_build(const CameraBatchApi *api,CameraBatch *batch,
                              void *original,void *left,void *right) {
    return camera_batch_build_source(api,batch,original,left,right,0);
}
#endif
