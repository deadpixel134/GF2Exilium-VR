#ifndef GF2_ACTIVITY_UI_TRANSITION_H
#define GF2_ACTIVITY_UI_TRANSITION_H
/* Main only, before reserving any new numeric render ticket. Old flights must
   complete the unchanged CPU/render/copy/native/XR take/release sequence.
   XR can keep the completed same-world presentation under its existing age
   policy; this adapter never clears publication, resets pose or calls XR. */
static int source_live_ui_transition(SourceLiveOwner *o,int (*drained)(void *),void *context) {
    if(!o || !o->ready || o->busy || o->retiring || GetCurrentThreadId()!=o->thread)return 0;
    ActivitySourceGroup *a=&o->activity;
    if(!a->ui_change)return 1;
    unsigned long long generation=o->generation.selection.generation;
    if(!activity_source_current(a,generation) || o->generation.busy || a->busy ||
       !a->topology || a->topology->busy || a->topology->cancelled || !a->ui_pending_ready)return 0;
    /* CPU scopes have ended before disabling the previously enrolled UI.
       Paused original-only frames cannot enqueue those private UI cameras. */
    for(unsigned i=0;i<SEP_CAPACITY;++i){SourceRenderFlight *f=&o->flights.slots[i];
        if(f->scope.render.active || f->scope.armed || f->frame.busy ||
           (f->frame.serial && !f->frame.root_done))return 0;
    }
    for(unsigned i=0;i<SEP_CAPACITY;++i)if(!activity_ui_pause_group(&o->pool.groups[i]))return 0;
    if(!drained || !drained(context))return 0;
    for(unsigned i=0;i<SEP_CAPACITY;++i){SourceRenderFlight *f=&o->flights.slots[i];
        if(f->occupied || f->frame.serial || f->scope.render.active || f->scope.armed ||
           !activity_ui_group_drained(&o->pool.groups[i]))return 0;
    }
    if(!activity_source_recheck(a,generation) || a->ui_clock==~0ULL)return 0;
    o->busy=1;a->busy=1;int ok=0;
    /* Unlink and destroy optional private pairs only; keep every world object,
       target, native cache, group stamp, anchor and published generation. */
    for(unsigned i=0;i<SEP_CAPACITY;++i)if(!activity_ui_reset_group(&o->pool.groups[i]))goto end;
    if(!GF2VR_ACTIVITY_UI_TRANSITION_RELEASE(a) || !activity_source_current(a,generation))goto end;
    activity_ui_move(&a->reading,&a->active);activity_snapshot_drop(a,&a->reading);
    activity_ui_move(&a->active,&a->ui_pending);activity_snapshot_drop(a,&a->ui_pending);
    a->ui_serial=++a->ui_clock;
    /* Allocate replacements under the new immutable UI identity, before any
       frame can borrow them. Per-frame refresh/optics still use existing code. */
    for(unsigned i=0;i<SEP_CAPACITY;++i){ActivityEyeGroup *g=&o->pool.groups[i];
        if(!g->generation)continue;
        g->busy=1;int prepared=activity_group_ui_prepare(g,&o->component.refs.settings.calls);g->busy=0;
        if(!prepared || !activity_group_source_valid(g))goto failed;
    }
    a->ui_change=a->ui_pending_ready=0;ok=1;
    static unsigned reports;
    if(reports++<32) {
        event("activity_ui_transition",a->ui_serial,GetCurrentThreadId());
        event("activity_ui_transition_world",generation,GetCurrentThreadId());
    }
    goto end;
failed:
    /* A concrete allocation/ownership failure uses the real existing
       quarantine/retirement path; never present a half-prepared UI pair. */
    source_generation_invalidate(&o->generation);
end:
    a->busy=0;o->busy=0;return ok;
}
#endif
