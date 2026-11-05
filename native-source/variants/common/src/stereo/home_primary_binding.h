#ifndef GF2_HOME_PRIMARY_BINDING_H
#define GF2_HOME_PRIMARY_BINDING_H
#include "home_root_binding.h"
/* Commander owns a unique declared Transform named SceneCameraRoot.
   Only a Camera named MainCamera directly beneath that root may be primary.
   All semantic getters are read-only; no Find/create/enable/transform writes. */
typedef struct {
    HomeRootBinding root;
    void *camera_class;
    int (*camera_field_type)(void *);
    int (*camera_matches)(void *,void *);
} HomePrimaryBinding;
static int home_primary_binding_read(const HomePrimaryBinding *a,void *owner,void **out,unsigned *held){
    *out=0;*held=0;
    if(!a || !a->camera_class || !a->camera_field_type || !a->camera_matches)return -1;
    void *root=0,*chosen=0;unsigned root_hold=0,chosen_hold=0;int complete=0;
    const HomeRootBinding *b=&a->root;
    if(home_root_binding_read(b,owner,&root,&root_hold)!=1)return -1;
    void *it=0;
    for(unsigned n=0;n<128;++n){
        void *f=b->fields(b->owner_class,&it);if(!f){complete=1;break;}
        if((b->flags(f)&0x50u) || !a->camera_field_type(f))continue;
        void *camera=0;b->read(owner,f,&camera);if(!camera)continue;
        unsigned h=b->pin(camera,1);if(!h)goto done;
        int match=b->object_class(camera)==a->camera_class?a->camera_matches(camera,root):-1;
        if(match<0 || (match==1 && chosen && chosen!=camera)){b->unpin(h);goto done;}
        if(match==1 && !chosen){chosen=camera;chosen_hold=h;}else b->unpin(h);
    }
    if(complete && chosen){
        void *after=0;unsigned h=0;int status=home_root_binding_read(b,owner,&after,&h);
        if(h)b->unpin(h);
        if(status==1 && after==root){*out=chosen;*held=chosen_hold;chosen_hold=0;}
    }
done:
    if(chosen_hold)b->unpin(chosen_hold);
    if(root_hold)b->unpin(root_hold);
    return *held?1:-1;
}
#endif
