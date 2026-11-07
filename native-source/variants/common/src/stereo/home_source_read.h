#ifndef GF2_HOME_SOURCE_READ_H
#define GF2_HOME_SOURCE_READ_H
/* Typed read services and exact rooted current-owner read; no setters or cached object pointers. */
typedef struct {
    SourcePoseApi values;
    void *owner_class,*bool_class,*live;
    void *singleton,*camera,*active,*gacha;
    void (*static_read)(void *,void *);
    void (*instance_read)(void *,void *,void *);
    unsigned attempted,ready;
#ifdef GF2VR_HOME_OWNER_OBSERVE
    unsigned long long read_observation;
#endif
} HomeSourceOwner;
#ifdef GF2VR_HOME_OWNER_OBSERVE
#define HOME_READ_NOTE(h,s,a,g,l) ((h)->read_observation=(unsigned long long)(s)|\
    ((unsigned long long)(a)<<8)|((unsigned long long)(g)<<16)|((unsigned long long)(l)<<24))
#else
#define HOME_READ_NOTE(h,s,a,g,l) ((void)0)
#endif
static void *home_source_read(HomeSourceOwner *h) {
    if(!h || !h->ready)return 0;
    const ManagedEyeApi *c=&h->values.calls;
    void *owner=0,*camera=0,*owner_after=0,*camera_after=0,*result=0;
    unsigned owner_root=0,camera_root=0;unsigned char active=0,gacha=1,live=0;
    HOME_READ_NOTE(h,1,255,255,255);
    h->static_read(h->singleton,&owner);if(!owner)return 0;
    HOME_READ_NOTE(h,2,255,255,255);
    owner_root=c->pin(owner,1);if(!owner_root)goto done;
    HOME_READ_NOTE(h,3,255,255,255);
    if(c->object_class(owner)!=h->owner_class)goto done;
    h->static_read(h->active,&active);h->instance_read(owner,h->gacha,&gacha);
    if(active!=1 || gacha!=0 ||
       !source_pose_value(&h->values,h->live,owner,h->bool_class,&live,1) || live!=1) {
        HOME_READ_NOTE(h,4,active,gacha,live);goto done;
    }
    HOME_READ_NOTE(h,5,active,gacha,live);
    h->instance_read(owner,h->camera,&camera);if(!camera)goto done;
    HOME_READ_NOTE(h,6,active,gacha,live);
    camera_root=c->pin(camera,1);if(!camera_root || c->object_class(camera)!=c->camera_class)goto done;
    live=0;
    if(!source_pose_value(&h->values,h->live,camera,h->bool_class,&live,1) || live!=1) {
        HOME_READ_NOTE(h,7,active,gacha,live);goto done;
    }
    /* Managed liveness can reenter. Re-read exact existing fields while both
       objects remain rooted; changed/null/gacha/transition state fails closed. */
    h->static_read(h->singleton,&owner_after);h->instance_read(owner,h->camera,&camera_after);
    h->static_read(h->active,&active);h->instance_read(owner,h->gacha,&gacha);
    HOME_READ_NOTE(h,8,active,gacha,live);
    if(owner_after==owner && camera_after==camera && active==1 && !gacha)result=camera;
    if(result)HOME_READ_NOTE(h,9,active,gacha,live);
done:
    if(camera_root)c->unpin(camera_root);
    if(owner_root)c->unpin(owner_root);
    return result; /* compare only against the caller's independently rooted input */
}
#undef HOME_READ_NOTE
#endif
