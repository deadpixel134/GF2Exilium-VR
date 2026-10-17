#ifndef GF2_SOURCE_RETIRE_OBSERVE_H
#define GF2_SOURCE_RETIRE_OBSERVE_H
/* Main-owner scalar observation only. No new Unity/current/GPU/XR call;
   cleanup still executes exactly once with its original short circuits. */
typedef struct {
    U64 preparation_generation,native_generation;
    unsigned data,eyes,binding,roots,native;
} SrSlotState;
typedef struct {
    U64 root;
    unsigned active,current_slot,slot_seen,slot_retired,cache_dropped;
    unsigned reasons[SEP_CAPACITY+1],eyes_failed[SEP_CAPACITY+1];
    SrSlotState before[SEP_CAPACITY+1],after[SEP_CAPACITY+1];
} SourceRetireNote;
static SourceRetireNote sr_note;
static unsigned sr_failure_records,sr_success_records,sr_guard_records;
static SrSlotState sr_state(SourceEyePreparation *p,EyeNativeCache *native) {
    SrSlotState out={0};if(!p)return out;
    CameraDataOwner *d=p->data?p->data:(p->source?p->source->data:0);ManagedEyes *e=p->eyes;
    out.preparation_generation=p->generation;
    if(d)out.data=(d->binding!=0)|(d->ready<<1)|(d->busy<<2)|(d->retiring<<3)|
        ((d->source_root!=0)<<4)|((d->roots[0]!=0)<<5)|((d->roots[1]!=0)<<6)|((d->roots[2]!=0)<<7);
    if(e){out.eyes=e->borrowed|(e->ready<<1)|(e->targets_ready<<2)|(e->matrices_ready<<3);
        for(unsigned i=0;i<2;++i){ManagedEyeSlot *s=&e->eyes[i];
            out.roots|=((s->object_root!=0)|((s->camera_root!=0)<<1)|((s->target_root!=0)<<2)|
                (s->target_constructed<<3))<<(i*4);}}
    if(p->binding)out.binding=(p->binding->type_root!=0)|(p->binding->in_use<<1);
    if(native){out.native=native->valid|(native->busy<<1)|((native->native.resource[0]!=0)<<2)|
        ((native->native.resource[1]!=0)<<3);out.native_generation=native->generation;}
    return out;
}
static void sr_begin(void) {
    sr_note=(SourceRetireNote){0};sr_note.active=1;sr_note.current_slot=SEP_CAPACITY;
    sr_note.root=stereo_pipeline.render.serial;
}
static void sr_failure(unsigned reason,unsigned eye) {
    if(!sr_note.active || reason>10 || sr_note.current_slot>SEP_CAPACITY)return;
    sr_note.reasons[sr_note.current_slot]|=1u<<reason;
    sr_note.eyes_failed[sr_note.current_slot]|=eye<2?1u<<eye:4u;
}
static void sr_slot_before(void *pool,unsigned slot) {
    SourceEyePool *p=pool;if(!sr_note.active || !p || slot>=SEP_CAPACITY)return;
    sr_note.current_slot=slot;sr_note.slot_seen|=1u<<slot;
    sr_note.before[slot]=sr_state(&p->preparation[slot],&p->native[slot]);
}
static void sr_slot_after(void *pool,unsigned slot,int retired,int dropped) {
    SourceEyePool *p=pool;if(!sr_note.active || !p || slot>=SEP_CAPACITY)return;
    if(retired)sr_note.slot_retired|=1u<<slot;
    if(dropped)sr_note.cache_dropped|=1u<<slot;
    sr_note.after[slot]=sr_state(&p->preparation[slot],&p->native[slot]);
}
static SrSlotState sr_legacy_state(SourceGeneration *s) {
    if(!s || !s->data)return (SrSlotState){0};
    SourceEyePreparation view={.source=s,.binding=s->data->binding,.eyes=s->data->eyes,
        .generation=s->data->generation,.data=s->data};return sr_state(&view,0);
}
static void sr_legacy_before(void *source) {
    SourceGeneration *s=source;if(!sr_note.active)return;
    sr_note.current_slot=SEP_CAPACITY;sr_note.before[SEP_CAPACITY]=sr_legacy_state(s);
}
static int sr_wanted(int clean) {
    return clean?sr_success_records<8:sr_failure_records<64;
}
static void sr_end(void *source,int clean) {
    SourceGeneration *s=source;if(!sr_note.active)return;
    sr_note.active=0;sr_note.after[SEP_CAPACITY]=sr_legacy_state(s);
    if(!sr_wanted(clean))return;
    if(clean)++sr_success_records;else ++sr_failure_records;
    DWORD saved=GetLastError();char line[4000],*q=source_prefix(line,"source_retire_boundary");
#define SR_NUM(name,value) do{q=literal(q,",\"" name "\":");q=decimal(q,(value));}while(0)
#define SR_WIDE(name,value) do{q=literal(q,",\"" name "\":");q=hexadecimal(q,(U64)(value));}while(0)
    SR_NUM("schema",1);SR_WIDE("root",sr_note.root);SR_WIDE("owner",(U64)s);
    SR_WIDE("last_generation",s?s->last_generation:0);SR_NUM("clean",clean!=0);
    SR_NUM("cancelled",s?s->cancelled:0);SR_NUM("source_root",s?s->source_root:0);
    SR_NUM("source_busy",s?s->busy:0);SR_NUM("source_eligible",s?s->selection.eligible:0);
    SR_NUM("slots_seen",sr_note.slot_seen);SR_NUM("eyes_retired",sr_note.slot_retired);SR_NUM("native_dropped",sr_note.cache_dropped);
    for(unsigned i=0;i<=SEP_CAPACITY;++i) {
        q=literal(q,",\"slot");q=decimal(q,i);q=literal(q,"\":[");
        q=decimal(q,sr_note.reasons[i]);q=literal(q,",");q=decimal(q,sr_note.eyes_failed[i]);
        for(unsigned n=0;n<2;++n){SrSlotState *v=n?&sr_note.after[i]:&sr_note.before[i];
            q=literal(q,",");q=hexadecimal(q,v->preparation_generation);q=literal(q,",");q=hexadecimal(q,v->native_generation);
            q=literal(q,",");q=decimal(q,v->data);q=literal(q,",");q=decimal(q,v->eyes);
            q=literal(q,",");q=decimal(q,v->binding);q=literal(q,",");q=decimal(q,v->roots);
            q=literal(q,",");q=decimal(q,v->native);}
        q=literal(q,"]");
    }
    q=literal(q,"}\n");source_log(line,q);SetLastError(saved);
#undef SR_NUM
#undef SR_WIDE
}
static void sr_guard(void *source,unsigned long thread) {
    /* Immutable owner-thread metadata only on foreign callers. No mutable
       source fields are read before that check; count is main-only. */
    SourceGeneration *s=source;if(!s || thread!=s->owner_thread || GetCurrentThreadId()!=s->owner_thread)return;
    if(s->selection.eligible || sr_guard_records>=8)return;
    DWORD saved=GetLastError();char line[600],*q=source_prefix(line,"source_retire_guard");
    q=literal(q,",\"last_generation\":");q=hexadecimal(q,s->last_generation);
    q=literal(q,",\"source_busy\":");q=decimal(q,s->busy);
    q=literal(q,",\"eligible\":");q=decimal(q,s->selection.eligible);
    q=literal(q,"}\n");source_log(line,q);++sr_guard_records;SetLastError(saved);
}
#endif
