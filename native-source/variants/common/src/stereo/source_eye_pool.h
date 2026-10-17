#ifndef GF2_SOURCE_EYE_POOL_H
#define GF2_SOURCE_EYE_POOL_H
#include "source_eye_prepare.h"
#ifdef GF2VR_ACTIVITY_STACK
#include "activity_eye_group.h"
#endif
#ifdef GF2VR_NATIVE_TARGET_CACHE
#include "eye_native_cache.h"
#endif
#ifdef GF2VR_CAPACITY_EXPERIMENT
#include "eye_flight_owner.h"
#define SEP_CAPACITY EFO_CAPACITY
#else
#define SEP_CAPACITY 2
#endif
/* Two independently bound private camera/data/target pairs share only the
   authoritative source generation and authored anchor. Unity owner thread
   only, never change SourceGeneration.data under a borrow. Bindings are caller
   owned, independently pinned and resident until this entire pool drains.
   Numeric flight slot i must always use pair i through render AND copy drain.
   Zero initialize once and keep address stable. This does not return a
   SourceRenderFrame borrow or attest GPU completion. */
typedef struct {
    SourceGeneration *source;
    CameraDataOwner data[SEP_CAPACITY];
    ManagedEyes eyes[SEP_CAPACITY];
    SourceEyePreparation preparation[SEP_CAPACITY];
#ifdef GF2VR_ACTIVITY_STACK
    ActivitySourceGroup *activity;
    ActivityEyeGroup groups[SEP_CAPACITY];
#endif
#ifdef GF2VR_NATIVE_TARGET_CACHE
    EyeNativeCache native[SEP_CAPACITY];
#endif
} SourceEyePool;
static void source_eye_pool_invalidate(void *context) {
    SourceEyePool *p=context;
    for(unsigned i=0;i<SEP_CAPACITY;++i) {
#ifdef GF2VR_ACTIVITY_STACK
        if(p->groups[i].generation)activity_group_invalidate(&p->groups[i]);
#endif
        CameraDataOwner *d=&p->data[i];ManagedEyes *e=&p->eyes[i];
        if(d->binding)d->retiring=1;
        d->ready=0;e->ready=e->matrices_ready=0;
#ifdef GF2VR_NATIVE_TARGET_CACHE
        eye_native_cache_invalidate(&p->native[i]);
#endif
    }
}
static int source_eye_pool_retire(void *context) {
    SourceEyePool *p=context;
    if(!p || !p->source)return 0;
    int clean=1;
#ifdef GF2VR_ACTIVITY_STACK
    for(unsigned i=0;i<SEP_CAPACITY;++i)if(p->groups[i].generation && !activity_group_retire(&p->groups[i]))clean=0;
#endif
    /* Attempt both, even if the first remains borrowed/quarantined. A drained
       sibling can clean independently but cannot admit a new generation. */
#ifdef GF2VR_NATIVE_TARGET_CACHE
    for(unsigned i=0;i<SEP_CAPACITY;++i) {
#ifdef GF2VR_SOURCE_RETIRE_OBSERVE
        GF2VR_SOURCE_RETIRE_SLOT_BEFORE(p,i);
        int retired=source_eye_retire(&p->preparation[i]),dropped=0;
        if(retired)dropped=eye_native_cache_drop(&p->native[i],&p->eyes[i],&eye_native_refs);
        GF2VR_SOURCE_RETIRE_SLOT_AFTER(p,i,retired,dropped);
        if(!retired || !dropped)clean=0;
#else
        if(!source_eye_retire(&p->preparation[i])){clean=0;continue;}
        if(!eye_native_cache_drop(&p->native[i],&p->eyes[i],&eye_native_refs))clean=0;
#endif
    }
#else
    for(unsigned i=0;i<SEP_CAPACITY;++i)if(!source_eye_retire(&p->preparation[i]))clean=0;
#endif
    return clean;
}
#ifdef GF2VR_CAPACITY_EXPERIMENT
static int source_eye_pool_bind_many(SourceEyePool *p,SourceGeneration *s,
        CameraDataComponentApi *const *bindings,unsigned count,unsigned long thread) {
    if(!p || p->source || !s || !thread || thread!=s->owner_thread || s->busy ||
       s->source_root || s->selection.eligible || s->data || s->retire_prepared ||
       s->retire_context || s->invalidate_prepared || !bindings || count!=SEP_CAPACITY)return 0;
    for(unsigned i=0;i<count;++i){
        if(!bindings[i] || !bindings[i]->type_root || bindings[i]->in_use)return 0;
        for(unsigned j=0;j<i;++j)if(bindings[i]==bindings[j] ||
            bindings[i]->type_root==bindings[j]->type_root)return 0;
    }
    *p=(SourceEyePool){0};p->source=s;
    for(unsigned i=0;i<count;++i)p->preparation[i]=(SourceEyePreparation){
        .source=s,.binding=bindings[i],.eyes=&p->eyes[i],.data=&p->data[i]};
    s->retire_context=p;s->retire_prepared=source_eye_pool_retire;
    s->invalidate_prepared=source_eye_pool_invalidate;return 1;
}
#endif
static int source_eye_pool_bind(SourceEyePool *p,SourceGeneration *s,
    CameraDataComponentApi *left,CameraDataComponentApi *right,unsigned long thread) {
#ifdef GF2VR_CAPACITY_EXPERIMENT
    CameraDataComponentApi *bindings[2]={left,right};
    return source_eye_pool_bind_many(p,s,bindings,2,thread);
#else
    if(!p || p->source || !s || !thread || thread!=s->owner_thread || s->busy ||
       s->source_root || s->selection.eligible || s->data || s->retire_prepared ||
       s->retire_context || s->invalidate_prepared || !left || !right || left==right ||
       !left->type_root || !right->type_root || left->type_root==right->type_root ||
       left->in_use || right->in_use)return 0;
    *p=(SourceEyePool){0};p->source=s;
    CameraDataComponentApi *bindings[2]={left,right};
    for(unsigned i=0;i<2;++i)p->preparation[i]=(SourceEyePreparation){
        .source=s,.binding=bindings[i],.eyes=&p->eyes[i],.data=&p->data[i]};
    s->retire_context=p;s->retire_prepared=source_eye_pool_retire;
    s->invalidate_prepared=source_eye_pool_invalidate;return 1;
#endif
}
static int source_eye_pool_prepare(SourceEyePool *p,unsigned slot,unsigned long thread,int width,int height) {
    if(!p || !p->source || slot>=SEP_CAPACITY)return 0;
    if(!thread || thread!=p->source->owner_thread || p->source->busy)return 0;
    for(unsigned i=0;i<SEP_CAPACITY;++i)if(p->preparation[i].generation &&
        (p->preparation[i].width!=width || p->preparation[i].height!=height)) {
        source_generation_invalidate(p->source);return 0;
    }
#ifdef GF2VR_NATIVE_TARGET_CACHE
    if(!eye_native_cache_prepare(&p->native[slot],&p->preparation[slot],&eye_native_refs))return 0;
#endif
#ifdef GF2VR_ACTIVITY_STACK
    if(p->source->identity.group_stamp) {
        if(!p->activity)return 0;
        return activity_group_prepare(&p->groups[slot],p->activity,&p->preparation[slot],thread,width,height);
    }
#endif
    return source_eye_prepare_owned(&p->preparation[slot],thread,width,height,source_eye_pool_retire,p);
}
static int source_eye_pool_unbind(SourceEyePool *p,unsigned long thread) {
    if(!p || !p->source)return 0;
    SourceGeneration *s=p->source;
    if(!thread || thread!=s->owner_thread || s->busy || s->selection.eligible || s->source_root ||
       s->retire_context!=p || s->retire_prepared!=source_eye_pool_retire ||
       s->invalidate_prepared!=source_eye_pool_invalidate)return 0;
    for(unsigned i=0;i<SEP_CAPACITY;++i) {
#ifdef GF2VR_ACTIVITY_STACK
        if(p->groups[i].generation || p->groups[i].busy || p->groups[i].borrowed)return 0;
#endif
        SourceEyePreparation *a=&p->preparation[i];ManagedEyes *e=&p->eyes[i];
#ifdef GF2VR_NATIVE_TARGET_CACHE
        if(p->native[i].busy || p->native[i].valid || p->native[i].native.resource[0] ||
           p->native[i].native.resource[1])return 0;
#endif
#ifdef GF2VR_CAPACITY_EXPERIMENT
        if(p->data[i].binding || p->data[i].busy || a->generation || (a->binding && a->binding->in_use) ||
#else
        if(p->data[i].binding || p->data[i].busy || a->generation || a->binding->in_use ||
#endif
           e->borrowed || e->ready || e->targets_ready)return 0;
        for(unsigned j=0;j<2;++j)if(e->eyes[j].object_root || e->eyes[j].camera_root ||
            e->eyes[j].target_root || e->eyes[j].object || e->eyes[j].camera || e->eyes[j].target)return 0;
    }
    s->retire_context=0;s->retire_prepared=0;s->invalidate_prepared=0;
    *p=(SourceEyePool){0};return 1;
}
#endif
