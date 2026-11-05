#ifndef GF2_HOME_ROOT_BINDING_H
#define GF2_HOME_ROOT_BINDING_H
/* Semantic identity is independently established by the game's initializer
   and actual camera ancestry. No field name, offset, order, or lazy getter. */
typedef struct {
    void *owner_class,*root_class;
    void *(*fields)(void *,void **);
    unsigned (*flags)(void *);
    int (*root_field_type)(void *);
    void (*read)(void *,void *,void *);
    void *(*object_class)(void *);
    unsigned (*pin)(void *,int);
    void (*unpin)(unsigned);
    int (*semantic_root)(void *); /* -1 invalid/unavailable, 0 other, 1 match */
} HomeRootBinding;
/* Transfer a rooted unique object. Repeated aliases to the same object are
   harmless; two distinct matching objects or any opaque candidate reject. */
static int home_root_binding_read(const HomeRootBinding *a,void *owner,void **out,unsigned *held){
    *out=0;*held=0;
    if(!a || !owner || !a->owner_class || !a->root_class || !a->fields || !a->flags ||
       !a->root_field_type || !a->read || !a->object_class || !a->pin || !a->unpin || !a->semantic_root ||
       a->object_class(owner)!=a->owner_class)return -1;
    void *it=0,*chosen=0;unsigned chosen_root=0;int complete=0;
    for(unsigned n=0;n<128;++n){
        void *f=a->fields(a->owner_class,&it);if(!f){complete=1;break;}
        if((a->flags(f)&0x50u) || !a->root_field_type(f))continue;
        void *value=0;a->read(owner,f,&value);if(!value)continue;
        unsigned root=a->pin(value,1);if(!root)goto reject;
        int match=a->object_class(value)==a->root_class?a->semantic_root(value):-1;
        if(match<0 || (match==1 && chosen && chosen!=value)){a->unpin(root);goto reject;}
        if(match==1 && !chosen){chosen=value;chosen_root=root;}else a->unpin(root);
    }
    if(!complete || !chosen)goto reject;
    *out=chosen;*held=chosen_root;return 1;
reject:
    if(chosen_root)a->unpin(chosen_root);
    return -1;
}
#endif
