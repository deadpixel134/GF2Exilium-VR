#ifndef GF2_SOURCE_EYE_PREPARE_H
#define GF2_SOURCE_EYE_PREPARE_H
#include "source_generation.h"
/* One source generation owns the whole private camera/target/data pair.
   Bindings and extent config must remain alive/immutable through retirement. */
typedef struct {
    SourceGeneration *source;
    CameraDataComponentApi *binding;
    ManagedEyes *eyes;
    unsigned long long generation;
    int width,height;
    CameraDataOwner *data; /* Optional fixed pair; never swap source->data. */
} SourceEyePreparation;
static CameraDataOwner *source_eye_data(SourceEyePreparation *p) {
    return p->data?p->data:p->source->data;
}
static int source_eye_retire(void *context) {
    SourceEyePreparation *p=context;
    if(!p || !p->source || !p->binding || !p->eyes) return 0;
    CameraDataOwner *d=source_eye_data(p);
    if(d && d->binding && !camera_data_owner_retire(d)) return 0;
    /* Before data attachment, failed constructors/targets still own roots.
       Those must drain/clean before the source owner can free its own root. */
    if(!managed_eyes_retire(&p->binding->refs.settings.calls,p->eyes)) return 0;
    *p->eyes=(ManagedEyes){0};
    p->generation=0;p->width=p->height=0;return 1;
}
static int source_eye_current(void *context,unsigned long long generation) {
    SourceEyePreparation *p=context;SourceGeneration *s=p->source;
    CameraDataOwner *d=source_eye_data(p);
    return generation && s->selection.eligible && !s->cancelled && s->source_root &&
        s->selection.source && generation==s->selection.generation &&
        d && (!d->binding || !d->retiring);
}
/* Successful CameraDataOwner retirement clears the private pair but cannot
   clear its separate preparation owner. Reconcile metadata only, after proving
   there is no remaining engine/GPU borrow or resource to recycle. This does not
   restore readiness, authorize old eyes or modify the physical pose reference. */
static int source_eye_reconcile_retired(SourceEyePreparation *p) {
    SourceGeneration *s=p->source;CameraDataOwner *d=source_eye_data(p);ManagedEyes *e=p->eyes;
    if(!source_eye_current(p,s->selection.generation) || !p->generation ||
       p->generation!=s->selection.generation || p->binding->in_use ||
       d->binding || d->eyes || d->source_camera || d->source_root || d->generation ||
       d->ready || d->busy || d->retiring || e->borrowed || e->ready ||
       e->targets_ready || e->matrices_ready) return 0;
    for(unsigned i=0;i<3;++i) if(d->components[i] || d->roots[i]) return 0;
    for(unsigned i=0;i<2;++i) {
        ManagedEyeSlot *slot=&e->eyes[i];
        if(slot->object || slot->camera || slot->object_root || slot->camera_root ||
           slot->target || slot->target_root || slot->target_constructed) return 0;
    }
#ifdef GF2VR_PREPARATION_RECONCILED
    GF2VR_PREPARATION_RECONCILED(p);
#endif
    e->matrix_frame=0;p->generation=0;p->width=p->height=0;return 1;
}
static int source_eye_prepare_owned(SourceEyePreparation *p,unsigned long thread,int width,int height,
    int (*retire)(void *),void *retire_context) {
    GF2VR_PREPARE_STEP(10);
    if(!p || !p->source || !p->binding || !p->eyes) return 0;
    SourceGeneration *s=p->source;CameraDataOwner *d=source_eye_data(p);
    GF2VR_PREPARE_STEP(11);
    if(!thread || thread!=s->owner_thread || s->busy || !d || !p->binding->type_root ||
       !retire || s->retire_prepared!=retire || s->retire_context!=retire_context) return 0;
    GF2VR_PREPARE_STEP(12);
    if(!source_eye_current(p,s->selection.generation)) return 0;
    GF2VR_PREPARE_STEP(13);
    if(width<=0 || height<=0) {source_generation_invalidate(s);return 0;}
    if(d->binding) {
        GF2VR_PREPARE_STEP(14);
        if(d->binding==p->binding && d->eyes==p->eyes && !d->retiring &&
           d->generation==s->selection.generation && p->generation==s->selection.generation &&
           p->width==width && p->height==height && p->eyes->borrowed) return 0;
        if(d->binding==p->binding && d->eyes==p->eyes && d->ready && !d->busy && !d->retiring &&
           p->eyes->ready && p->eyes->targets_ready && !p->eyes->borrowed &&
           d->source_camera==s->selection.source && d->generation==s->selection.generation &&
           p->generation==s->selection.generation && p->width==width && p->height==height) return 1;
        /* A resize/replacement is retirement, never in-place GPU resource reuse. */
        source_generation_invalidate(s);return 0;
    }
    GF2VR_PREPARE_STEP(15);
    (void)source_eye_reconcile_retired(p);
    if(p->binding->in_use || p->generation || p->eyes->borrowed || p->eyes->ready ||
       p->eyes->eyes[0].object_root || p->eyes->eyes[1].object_root ||
       p->eyes->eyes[0].target_root || p->eyes->eyes[1].target_root) {
        source_generation_invalidate(s);return 0;
    }
    s->busy=1;s->cancelled=0;
    unsigned long long generation=s->selection.generation;
    const ManagedEyeApi *calls=&p->binding->refs.settings.calls;
    GF2VR_PREPARE_STEP(20);
    int ok=managed_eyes_create(calls,p->eyes,s->selection.source) &&
        (GF2VR_PREPARE_STEP(21),source_eye_current(p,generation)) &&
        (GF2VR_PREPARE_STEP(22),camera_data_owner_prepare(p->binding,d,p->eyes,s->selection.source,generation,source_eye_current,p)) &&
        (GF2VR_PREPARE_STEP(23),managed_eyes_targets(calls,p->eyes,width,height)) &&
        (GF2VR_PREPARE_STEP(24),source_eye_current(p,generation));
    if(ok) {p->generation=generation;p->width=width;p->height=height;}
    else source_generation_invalidate(s);
    s->busy=0;
    if(!ok) (void)source_generation_collect(s,thread);
    return ok;
}
static int source_eye_prepare(SourceEyePreparation *p,unsigned long thread,int width,int height) {
    return source_eye_prepare_owned(p,thread,width,height,source_eye_retire,p);
}
#endif
