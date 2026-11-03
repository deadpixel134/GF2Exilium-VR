#ifndef GF2_HOME_COMMANDER_READ_H
#define GF2_HOME_COMMANDER_READ_H
/* Current game adapter: exact registered Commander primary Camera. The
   result is compare-only against independently rooted current render inputs. */
typedef struct {
    SourcePoseApi values;
    void *game_class,*registry_class,*main_class,*commander_class,*bool_class,*live;
    void *game_singleton,*registry_singleton,*current,*current_kind,*background_kind,*scene_kind,*primary,*lookup;
    void (*static_read)(void *,void *);
    void (*instance_read)(void *,void *,void *);
    unsigned ready,stage,owns_current;
#ifdef GF2VR_HOME_TIMELINE_SOURCE
    int (*select_timeline)(void *,void **,unsigned *);
#endif
#ifdef GF2VR_HOME_PREVIEW_OBSERVE
    void *observed_secondary;
#endif
} HomeCommanderApi;
static void *home_commander_read(HomeCommanderApi *h) {
    if(!h)return 0;h->owns_current=0;if(!h->ready)return 0;
    const ManagedEyeApi *c=&h->values.calls;
    unsigned roots[5]={0};
    void *game=0,*registry=0,*main=0,*commander=0,*camera=0,*result=0;
    int current=-1,background=-1,kind=-1;unsigned char live=0;
    h->stage=1;h->static_read(h->game_singleton,&game);h->static_read(h->registry_singleton,&registry);
    if(!game || !registry)goto done;
    h->stage=2;roots[0]=c->pin(game,1);roots[1]=c->pin(registry,1);
    if(!roots[0] || !roots[1] || c->object_class(game)!=h->game_class || c->object_class(registry)!=h->registry_class)goto done;
    h->instance_read(game,h->current_kind,&current);h->instance_read(game,h->current,&main);
    h->stage=3;if(!main)goto done;roots[2]=c->pin(main,1);if(!roots[2])goto done;
    if((current!=3 && current!=41) || c->object_class(main)!=h->main_class)goto done;
    h->instance_read(main,h->background_kind,&background);
    h->stage=4;if(background!=10)goto done;
    h->owns_current=1;
    void *args[]={&background};
    if(!managed_eye_call(c,h->lookup,registry,args,&commander) || !commander)goto done;
    h->stage=5;roots[3]=c->pin(commander,1);
    if(!roots[3] || c->object_class(commander)!=h->commander_class)goto done;
    h->instance_read(commander,h->scene_kind,&kind);if(kind!=10)goto done;
    /* Exact field proven by the game's initializer; never use its second,
       independently controlled VirtualCamera field or execute initialization. */
    int timeline=0;
#ifdef GF2VR_HOME_TIMELINE_SOURCE
    if(!h->select_timeline || (timeline=h->select_timeline(commander,&camera,&roots[4]))<0)goto done;
#endif
    if(!timeline)h->instance_read(commander,h->primary,&camera);
    h->stage=6;if(!camera)goto done;if(!roots[4])roots[4]=c->pin(camera,1);
    if(!roots[4] || c->object_class(camera)!=c->camera_class)goto done;
    h->stage=7;
    if(!source_pose_value(&h->values,h->live,camera,h->bool_class,&live,1) || live!=1)goto done;
    void *game_after=0,*registry_after=0,*main_after=0,*commander_after=0,*camera_after=0;
    int current_after=-1,background_after=-1,kind_after=-1;
    h->stage=8;
    if(!managed_eye_call(c,h->lookup,registry,args,&commander_after) || commander_after!=commander)goto done;
    h->static_read(h->game_singleton,&game_after);h->static_read(h->registry_singleton,&registry_after);
    h->instance_read(game,h->current,&main_after);h->instance_read(game,h->current_kind,&current_after);
    h->instance_read(main,h->background_kind,&background_after);
    h->instance_read(commander,h->scene_kind,&kind_after);
#ifdef GF2VR_HOME_TIMELINE_SOURCE
    {unsigned held=0;int next=h->select_timeline(commander,&camera_after,&held);
     if(held)c->unpin(held);if(next!=timeline)goto done;}
#endif
    if(!timeline)h->instance_read(commander,h->primary,&camera_after);
    if(game_after!=game || registry_after!=registry || main_after!=main || current_after!=current ||
       background_after!=10 || kind_after!=10 || camera_after!=camera)goto done;
    h->stage=9;result=camera;
#ifdef GF2VR_HOME_PREVIEW_OBSERVE
    /* Numeric attribution while the current owner is rooted. Never consume
       this secondary field as a production camera or dereference its value. */
    {static unsigned samples;static unsigned long long next;
     unsigned long long now=GetTickCount64();
     if(samples<180 && now>=next){++samples;next=now+1000;
        void *secondary=0;if(h->observed_secondary)h->instance_read(commander,h->observed_secondary,&secondary);
        event("home_preview_current_kind",(unsigned)current,GetCurrentThreadId());
        event("home_preview_primary",(unsigned long long)camera,GetCurrentThreadId());
        event("home_preview_secondary",(unsigned long long)secondary,GetCurrentThreadId());
     }}
#endif
done:
    for(unsigned n=5;n>0;--n)if(roots[n-1])c->unpin(roots[n-1]);
    return result;
}
#endif
