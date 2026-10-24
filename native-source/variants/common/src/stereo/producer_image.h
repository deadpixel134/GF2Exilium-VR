#ifndef GF2_PRODUCER_IMAGE_H
#define GF2_PRODUCER_IMAGE_H
#include "composition_bridge_image_binding.h"
#ifdef GF2VR_PRODUCER_ENGINE_OBSERVE
#ifdef GF2VR_ACTIVITY_STACK
#ifdef GF2VR_CAMERA_SPLIT_BRIDGE
#include "activity_producer_engine_bridge_package.h"
/* Exact rebuilt observation package. Do not weaken on a mismatch. */
_Static_assert(GF2_ACTIVITY_PRODUCER_BRIDGE_METHODS==24,"split bridge body count");
_Static_assert(GF2_ACTIVITY_PRODUCER_BRIDGE_BYTES==9216,"split bridge package size");
#else
#include "activity_producer_engine_bridge_baseline_package.h"
_Static_assert(GF2_ACTIVITY_PRODUCER_BRIDGE_METHODS==23,"product bridge body count");
_Static_assert(GF2_ACTIVITY_PRODUCER_BRIDGE_BYTES==8704,"product bridge package size");
#endif
#else
#include "producer_engine_bridge_package.h"
#endif
#else
#include "producer_bridge_package.h"
#endif
/* Own exact embedded package. Transfer reuses the established public metadata APIs; process-lifetime assembly root is never released by the probe. */
static int producer_image_load(const CompositionImageApi *a,CompositionImage *s,
                                  const uint8_t *encoded,size_t size,int package_verified) {
    if(!a || !s || s->attempted || s->assembly_root || s->bridge_class ||
       !package_verified || !encoded || size!=sizeof(gf2_producer_bridge_package) || !a->byte_class ||
       !a->byte_array_class || !a->assembly_class || !a->set_value ||
       !a->get_value || !a->load || !a->array_new || !a->value_box ||
       !a->object_class || !a->assignable || !a->array_length ||
       !a->pin || !a->unpin || !a->invoke || !a->unbox ||
       !a->image_absent || !a->resolve_class)return 0;
#if defined(GF2VR_PRODUCER_ENGINE_OBSERVE) && defined(GF2VR_ACTIVITY_STACK)
    unsigned exact_hash=2166136261u;
    for(size_t i=0;i<size;++i)exact_hash=(exact_hash^encoded[i])*16777619u;
#ifdef GF2VR_CAMERA_SPLIT_BRIDGE
    if(size!=9216||exact_hash!=1975618552u)return 0;
#else
    if(size!=8704||exact_hash!=627510109u)return 0;
#endif
#endif
    s->attempted=1;
    if(!a->image_absent(a->identity_context))return 0;
    void *array=a->array_new(a->byte_class,size);
    if(!array)return 0;
    unsigned array_root=a->pin(array,1);
    if(!array_root)return 0;
    int okay=a->object_class(array)==a->byte_array_class &&
             a->array_length(array)==size;
    for(size_t i=0;okay && i<size;++i){
        uint8_t value=encoded[i];int32_t index=(int32_t)i;
        void *box=a->value_box(a->byte_class,&value);
        if(!box || a->object_class(box)!=a->byte_class){okay=0;break;}
        void *args[2]={box,&index},*exception=0;
        a->invoke(a->set_value,array,args,&exception);
        if(exception){okay=0;break;}
        void *get_args[1]={&index};exception=0;
        void *read=a->invoke(a->get_value,array,get_args,&exception);
        if(exception || !read || a->object_class(read)!=a->byte_class){okay=0;break;}
        const uint8_t *actual=a->unbox(read);
        if(!actual || *actual!=value){okay=0;break;}
    }
    if(okay){
        void *args[1]={array},*exception=0;
        void *assembly=a->invoke(a->load,0,args,&exception);
        if(!exception && assembly &&
           a->assignable(a->assembly_class,a->object_class(assembly))){
            s->assembly_root=a->pin(assembly,1);
            if(s->assembly_root)s->bridge_class=a->resolve_class(a->identity_context);
        }
    }
    a->unpin(array_root);
    return s->assembly_root && s->bridge_class;
}

#endif
