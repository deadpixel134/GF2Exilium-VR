#ifndef GF2_COMPOSITION_BRIDGE_IMAGE_H
#define GF2_COMPOSITION_BRIDGE_IMAGE_H
#include <stddef.h>
#include <stdint.h>
/* One-shot package transfer. The caller first verifies the exact encoded-file
   SHA256 and Unity owner. Failure after Assembly.Load must never retry loading
   this image in the process (the domain may already have registered it). */
typedef struct {
    void *byte_class,*byte_array_class,*assembly_class;
    void *set_value,*get_value,*load;
    void *(*array_new)(void *,unsigned long long);
    void *(*value_box)(void *,void *);
    void *(*object_class)(void *);
    int (*assignable)(void *,void *);
    unsigned (*array_length)(void *);
    unsigned (*pin)(void *,int);
    void (*unpin)(unsigned);
    void *(*invoke)(void *,void *,void **,void **);
    void *(*unbox)(void *);
    /* Resolves one newly registered, exact-name image and its bridge class.
       A prior image of that name must cause rejection before Load. */
    int (*image_absent)(void *);
    void *(*resolve_class)(void *);
    void *identity_context;
} CompositionImageApi;
typedef struct {unsigned attempted,assembly_root;void *bridge_class;} CompositionImage;
static int composition_image_load(const CompositionImageApi *a,CompositionImage *s,
                                  const uint8_t *encoded,size_t size,int package_verified) {
    if(!a || !s || s->attempted || s->assembly_root || s->bridge_class ||
       !package_verified || !encoded || size!=5120 || !a->byte_class ||
       !a->byte_array_class || !a->assembly_class || !a->set_value ||
       !a->get_value || !a->load || !a->array_new || !a->value_box ||
       !a->object_class || !a->assignable || !a->array_length ||
       !a->pin || !a->unpin || !a->invoke || !a->unbox ||
       !a->image_absent || !a->resolve_class)return 0;
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
/* Only after all bridge callbacks/delegates and GPU users have retired. */
static void composition_image_release(const CompositionImageApi *a,CompositionImage *s){
    if(!a || !s)return;
    if(s->assembly_root){a->unpin(s->assembly_root);s->assembly_root=0;}
    s->bridge_class=0;
}
#endif
