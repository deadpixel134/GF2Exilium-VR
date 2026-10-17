#ifndef GF2_COMPOSITION_CAMERA_PLAN_H
#define GF2_COMPOSITION_CAMERA_PLAN_H
#include <stdint.h>
/* Original Render Camera[] only. Copy numeric identities; do not retain the
   borrowed camera objects or array after the scoped caller returns. */
typedef struct {
    void *camera_class,*array_class,*int_class,*get_value,*get_instance_id;
    void *(*object_class)(void *);
    unsigned (*array_length)(void *);
    unsigned (*pin)(void *,int);
    void (*unpin)(unsigned);
    void *(*invoke)(void *,void *,void **,void **);
    void *(*unbox)(void *);
} CompositionCameraApi;
typedef struct {unsigned count;int32_t camera[8],begin[8],end[8];} CompositionCameraPlan;
static int composition_camera_plan(const CompositionCameraApi *a,void *original,
                                   CompositionCameraPlan *out) {
    if(!out)return 0;*out=(CompositionCameraPlan){0};
    if(!a || !original || !a->camera_class || !a->array_class ||
       !a->int_class || !a->get_value || !a->get_instance_id ||
       !a->object_class || !a->array_length || !a->pin || !a->unpin ||
       !a->invoke || !a->unbox || a->object_class(original)!=a->array_class)return 0;
    unsigned root=a->pin(original,1);if(!root)return 0;
    int good=0;unsigned count=a->array_length(original);
    if(!count || count>8)goto end;
    CompositionCameraPlan plan={0};
    for(unsigned i=0;i<count;++i){
        int32_t index=(int32_t)i;void *args[1]={&index},*exception=0;
        void *camera=a->invoke(a->get_value,original,args,&exception);
        if(exception || !camera || a->object_class(camera)!=a->camera_class)goto end;
        unsigned camera_root=a->pin(camera,1);
        if(!camera_root)goto end;
        exception=0;
        void *boxed=a->invoke(a->get_instance_id,camera,0,&exception);
        int32_t id=0;
        if(!exception && boxed && a->object_class(boxed)==a->int_class){
            const int32_t *value=a->unbox(boxed);
            if(value)id=*value;
        }
        a->unpin(camera_root);
        if(!id)goto end;
        for(unsigned j=0;j<i;++j)if(plan.camera[j]==id)goto end;
        plan.camera[i]=id;
        plan.begin[i]=(int32_t)(2*i+1);
        plan.end[i]=(int32_t)(2*i+2);
    }
    if(a->array_length(original)!=count)goto end;
    plan.count=count;*out=plan;good=1;
end:
    a->unpin(root);return good;
}
#endif
