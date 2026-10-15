#ifndef GF2_SOURCE_GENERATION_H
#define GF2_SOURCE_GENERATION_H
#include "source_render_frame.h"
#ifndef GF2VR_HANDOFF_BEGIN
#define GF2VR_HANDOFF_BEGIN(g) ((void)0)
#define GF2VR_HANDOFF_CANCEL() ((void)0)
#endif
#ifndef GF2VR_RETIRE_BEGIN
#define GF2VR_RETIRE_BEGIN() ((void)0)
#define GF2VR_RETIRE_END(source,clean) ((void)0)
#endif
/* Attribution is supplied by the game adapter, never inferred from a name or
   membership alone. Identity numbers are compared, never dereferenced later.
   observe runs with the original Camera[] rooted; its selected camera must
   remain reachable from that array until it returns. */
typedef struct {
    void *source;
    int instance_id;
    unsigned count,attributed;
    unsigned long long owners[8];
    int owner_ids[8];
    float units_per_meter;
#ifdef GF2VR_ACTIVITY_STACK
    unsigned long long group_stamp; /* Complete contributing-set identity, not an anchor. */
#endif
} SourceGenerationCandidate;
typedef struct {
    SourceRenderSelection selection;
    SourceGenerationCandidate identity;
    unsigned long long last_generation;
    unsigned long long *generation_clock; /* optional stable owner-thread clock across binding reopen */
    unsigned long owner_thread;
    unsigned source_root,busy,cancelled;
    unsigned replacement_pending; /* current candidate valid; old ownership not yet retired */
    const CameraBatchApi *api;
    CameraDataOwner *data;
    SourceAnchorState *anchor;
    unsigned long long *publication;
    void *observe_context;
    int (*observe)(void *,void *,SourceGenerationCandidate *);
    /* Optional whole producer cleanup also covers partially created eyes that
       have not yet attached a CameraDataOwner. Same Unity owner thread. */
    void *retire_context;
    int (*retire_prepared)(void *);
    /* Optional multi-pair owner: revoke every pair before managed cleanup.
       Readiness/token changes only; no destruction or reference reset. */
    void (*invalidate_prepared)(void *);
#ifdef GF2VR_ACTIVITY_STACK
    void *extension;
    int (*extension_collect)(void *);
    int (*extension_adopt)(void *,unsigned long long);
#endif
} SourceGeneration;
/* No destruction or XR-reference reset here. Publication is stopped before any
   managed cleanup that could reenter the render wrapper. */
static void source_generation_clear(SourceGeneration *s) {
#ifdef GF2VR_SOURCE_CLEAR_TRACE
    GF2VR_SOURCE_CLEAR_TRACE(s);
#endif
    s->replacement_pending=0;
    if(s->publication) __atomic_store_n(s->publication,0,__ATOMIC_RELEASE);
    s->selection=(SourceRenderSelection){0};
    if(s->anchor) source_anchor_retire(s->anchor);
    if(s->busy) s->cancelled=1;
    if(s->invalidate_prepared) s->invalidate_prepared(s->retire_context);
    if(s->data && s->data->binding) {
        s->data->retiring=1;s->data->ready=0;
        s->data->eyes->ready=0;s->data->eyes->matrices_ready=0;
    }
}
static void source_generation_invalidate(SourceGeneration *s) {
    GF2VR_HANDOFF_CANCEL();source_generation_clear(s);
}
/* Owner-thread drain only. A pending eye borrow or failed Destroy quarantines
   both old data and source; a new generation cannot reuse their storage. */
static int source_generation_collect(SourceGeneration *s,unsigned long thread) {
    if(!s || thread!=s->owner_thread || !thread || s->busy || s->selection.eligible) {
#ifdef GF2VR_SOURCE_RETIRE_OBSERVE
        GF2VR_SOURCE_RETIRE_GUARD(s,thread);
#endif
        return 0;
    }
    s->busy=1;
    GF2VR_RETIRE_BEGIN();
    int clean=(!s->retire_prepared || s->retire_prepared(s->retire_context)) &&
#ifdef GF2VR_SOURCE_RETIRE_OBSERVE
        (GF2VR_SOURCE_RETIRE_LEGACY(s),
         (!s->data || camera_data_owner_retire(s->data)));
#else
        (!s->data || camera_data_owner_retire(s->data));
#endif
#ifdef GF2VR_ACTIVITY_STACK
    if(clean && s->extension_collect)clean=s->extension_collect(s->extension);
#endif
    if(clean && s->source_root) {
        unsigned root=s->source_root;s->source_root=0;s->api->unpin(root);
    }
    if(clean) s->identity=(SourceGenerationCandidate){0};
    s->busy=0;GF2VR_RETIRE_END(s,clean);return clean;
}
static int source_generation_same(const SourceGenerationCandidate *a,const SourceGenerationCandidate *b) {
#ifdef GF2VR_ACTIVITY_STACK
    if(a->group_stamp!=b->group_stamp)return 0;
#endif
    if(a->source!=b->source || a->instance_id!=b->instance_id || a->count!=b->count ||
       a->units_per_meter!=b->units_per_meter) return 0;
    for(unsigned i=0;i<a->count;++i)
        if(a->owners[i]!=b->owners[i] || a->owner_ids[i]!=b->owner_ids[i]) return 0;
    return 1;
}
/* Called on EVERY selected wrapper input, including while an old GPU frame is
   pending. No diagnostic sample/time limit. Returns source validity, not proof
   of fresh eyes or permission to submit a projection layer. */
static int source_generation_update(SourceGeneration *s,unsigned long thread,void *array) {
    GF2VR_ADMISSION_STEP(100);
    if(!s || !thread || thread!=s->owner_thread) return 0;
    s->replacement_pending=0;
    GF2VR_ADMISSION_STEP(101);
    if(s->busy) {source_generation_invalidate(s);return 0;}
    const CameraBatchApi *a=s->api;
    GF2VR_ADMISSION_STEP(102);
    if(!a || !a->pin || !a->unpin || !a->object_class || !a->array_length ||
       !a->invoke || !a->get_value || !a->camera_class || !a->array_class || !s->observe) {
        source_generation_invalidate(s);return 0;
    }
    s->busy=1;s->cancelled=0;
    unsigned array_root=0,source_root=0,matches=0;int valid=0;float checked;
    SourceGenerationCandidate c={0};
    GF2VR_ADMISSION_STEP(103);
    if(!array || a->object_class(array)!=a->array_class) goto end;
    GF2VR_ADMISSION_STEP(104);
    array_root=a->pin(array,1);if(!array_root) goto end;
    GF2VR_ADMISSION_STEP(105);
    unsigned count=a->array_length(array);if(!count || count>32) goto end;
    GF2VR_ADMISSION_STEP(106);
    if(!s->observe(s->observe_context,array,&c) || !c.attributed || !c.source || !c.instance_id || !c.count || c.count>8 ||
       !(c.units_per_meter>0) || !eye_optics_float(c.units_per_meter,&checked)) goto end;
    for(unsigned i=0;i<c.count;++i) {
        if(!c.owners[i] || !c.owner_ids[i]) goto end;
        for(unsigned j=0;j<i;++j) if(c.owners[i]==c.owners[j]) goto end;
    }
    source_root=a->pin(c.source,1);if(!source_root) goto end;
    if(a->object_class(c.source)!=a->camera_class) goto end;
    for(unsigned i=0;i<count;++i) {
        int index=(int)i;void *args[]={&index},*exception=0;
        void *item=a->invoke(a->get_value,array,args,&exception);
        if(exception || !item || a->object_class(item)!=a->camera_class) goto end;
        if(item==c.source) ++matches;
    }
    if(matches!=1 || s->cancelled) goto end;
    valid=1;
end:
    /* Drop busy only after temporary roots are no longer needed. On replacement
       transfer the candidate pin across old-owner cleanup (which may reenter). */
    if(array_root) a->unpin(array_root);
    if(s->cancelled) valid=0;
    if(valid && s->selection.eligible && (!s->data || !s->data->retiring) &&
       source_generation_same(&s->identity,&c)) {
        a->unpin(source_root);s->busy=0;
        return !s->cancelled && s->selection.eligible;
    }
    if(valid && s->selection.eligible && !source_generation_same(&s->identity,&c))
        GF2VR_HANDOFF_BEGIN(s->selection.generation);
    if(!valid)GF2VR_HANDOFF_CANCEL();
    /* A validated replacement retains its transition token across pending
       retirement; all external invalidation paths cancel it. */
    source_generation_clear(s);
    if(!valid) {if(source_root) a->unpin(source_root);s->busy=0;return 0;}
    s->busy=0;
    s->cancelled=0;
    if(s->generation_clock && *s->generation_clock>s->last_generation) s->last_generation=*s->generation_clock;
    if(!source_generation_collect(s,thread) || s->cancelled || s->last_generation==~0ULL) {
        s->busy=1;a->unpin(source_root);s->busy=0;
        s->replacement_pending=!s->cancelled && s->last_generation!=~0ULL;
        return 0;
    }
#ifdef GF2VR_ACTIVITY_STACK
    if(s->extension_adopt && !s->extension_adopt(s->extension,c.group_stamp)) {
        a->unpin(source_root);return 0;
    }
#endif
    s->source_root=source_root;s->identity=c;
    s->selection=(SourceRenderSelection){c.source,++s->last_generation,c.units_per_meter,1};
    if(s->generation_clock) *s->generation_clock=s->last_generation;
    if(s->publication) __atomic_store_n(s->publication,s->selection.generation,__ATOMIC_RELEASE);
    return 1;
}
#endif
