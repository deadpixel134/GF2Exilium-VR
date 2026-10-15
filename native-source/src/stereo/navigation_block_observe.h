#ifndef GF2_NAVIGATION_BLOCK_OBSERVE_H
#define GF2_NAVIGATION_BLOCK_OBSERVE_H
/* Read-only predicates against the actual policy call. Flags may overlap;
   counts are pose invocations, not independent XR frames or GPU evidence. */
static U64 navigation_block_flags(const NavigationState *before,const NavigationState *after,
    const NavigationSample *s,NavigationSettings cfg,U64 epoch,U64 generation,U64 now,
    long long pose_time,const NavigationOutput *out) {
    unsigned move=cfg.locomotion_hand<2?cfg.locomotion_hand:0,turn=1u-move;U64 flags=0;
    if(!navigation_settings_valid(cfg))flags|=1ull<<0;
    if(!s->eligible)flags|=1ull<<1;
    if(s->epoch!=epoch)flags|=1ull<<2;
    if(s->generation!=generation)flags|=1ull<<3;
    if(!s->tick_ms||now<s->tick_ms||now-s->tick_ms>250)flags|=1ull<<4;
    if(s->display_time<=0||pose_time<=0)flags|=1ull<<5;
    if(pose_time<s->display_time)flags|=1ull<<6;
    if(pose_time>=s->display_time&&pose_time-s->display_time>250000000ll)flags|=1ull<<7;
    if(!s->sequence||s->sequence<=before->sequence)flags|=1ull<<8;
    if(s->cancel!=before->cancel)flags|=1ull<<9;
    if(s->hand_cancel[move]!=before->hand_cancel[move])flags|=1ull<<10;
    if(s->hand_cancel[turn]!=before->hand_cancel[turn])flags|=1ull<<11;
    if(before->last_time>0&&s->display_time<=before->last_time)flags|=1ull<<12;
    if(before->last_time>0&&s->display_time>before->last_time&&s->display_time-before->last_time>100000000ll)flags|=1ull<<13;
    if(!s->active[move])flags|=1ull<<14;
    if(!s->active[turn])flags|=1ull<<15;
    if(!s->tracked[move])flags|=1ull<<16;
    if(!s->tracked[turn])flags|=1ull<<17;
    if(!navigation_axis_valid(s->x[move],s->y[move]))flags|=1ull<<18;
    if(!navigation_axis_valid(s->x[turn],s->y[turn]))flags|=1ull<<19;
    if(!before->move_armed)flags|=1ull<<20;
    if(!before->turn_armed)flags|=1ull<<21;
    if(out->rejected)flags|=1ull<<22;
    if(out->moved)flags|=1ull<<23;
    if(out->turned)flags|=1ull<<24;
    if(!after->move_armed)flags|=1ull<<25;
    if(!after->turn_armed)flags|=1ull<<26;
    if(before->epoch!=epoch||before->generation!=generation)flags|=1ull<<27;
    return flags;
}
static struct {
    U64 epoch,generation,last_ms,calls,moves,turns,rejected,counts[28];
    U64 pending_epoch,pending_generation,pending_frame,pending_sample;
    unsigned reports;
} navigation_block_observer;
static struct {
    NavigationState before,after;
    NavigationSample sample;
    NavigationSettings settings;
    NavigationOutput output;
    UnityTrackingInput input;
    PoseSourceAnchor anchor;
    PoseAppliedTicket ticket;
    ManagedEyePose eyes[2];
    U64 captured_ms,flags,calls,moves,turns,rejected,counts[28];
    unsigned active,success,applied;
} navigation_block_note;
static void navigation_block_observe(const NavigationState *before,const NavigationState *after,
    const NavigationSample *s,NavigationSettings cfg,const UnityTrackingInput *i,
    const PoseSourceAnchor *a,U64 now,const NavigationOutput *out,int success) {
    U64 flags=navigation_block_flags(before,after,s,cfg,i->reference_epoch,a->source_generation,now,i->display_time,out);
    __typeof__(navigation_block_observer) *o=&navigation_block_observer;
    int scope=o->epoch!=i->reference_epoch||o->generation!=a->source_generation;
    if(scope){o->calls=o->moves=o->turns=o->rejected=0;for(unsigned n=0;n<28;++n)o->counts[n]=0;}
    ++o->calls;o->moves+=out->moved;o->turns+=out->turned;o->rejected+=out->rejected;
    for(unsigned n=0;n<28;++n)if(flags&(1ull<<n))++o->counts[n];
    if(o->reports>=2048||(!scope&&now>=o->last_ms&&now-o->last_ms<200))return;
    ++o->reports;o->epoch=i->reference_epoch;o->generation=a->source_generation;o->last_ms=now;
    o->pending_epoch=i->reference_epoch;o->pending_generation=a->source_generation;
    o->pending_frame=i->frame_id;o->pending_sample=s->sequence;
    /* Values only. No formatting/file IO between pose composition and the
       current-mailbox/setter/ticket boundary being observed. */
    navigation_block_note.applied=0;navigation_block_note.ticket=(PoseAppliedTicket){0};
    navigation_block_note.before=*before;navigation_block_note.after=*after;navigation_block_note.sample=*s;
    navigation_block_note.settings=cfg;navigation_block_note.output=*out;navigation_block_note.input=*i;navigation_block_note.anchor=*a;
    navigation_block_note.captured_ms=now;navigation_block_note.flags=flags;navigation_block_note.active=1;navigation_block_note.success=success!=0;
    navigation_block_note.calls=o->calls;navigation_block_note.moves=o->moves;navigation_block_note.turns=o->turns;navigation_block_note.rejected=o->rejected;
    for(unsigned n=0;n<28;++n)navigation_block_note.counts[n]=o->counts[n];
}
static void navigation_block_flush(int failed) {
    __typeof__(navigation_block_note) *note=&navigation_block_note;
    if(!note->active)return;
    note->active=0;
    const NavigationState *before=&note->before,*after=&note->after;
    const NavigationSample *s=&note->sample;NavigationSettings cfg=note->settings;
    const UnityTrackingInput *i=&note->input;const PoseSourceAnchor *a=&note->anchor;
    const NavigationOutput *out=&note->output;U64 flags=note->flags;int success=note->success;
    /* Main-owner scratch, no reentrant engine/API work in this serializer. */
    DWORD saved=GetLastError();static char line[4000];char *p=source_prefix(line,"navigation_block_snapshot");
    p=literal(p,",\"captured_ms\":");p=hexadecimal(p,note->captured_ms);
    p=literal(p,",\"root_failed\":");p=decimal(p,failed!=0);p=literal(p,",\"setter_applied\":");p=decimal(p,note->applied);
    p=literal(p,",\"epoch\":");p=hexadecimal(p,i->reference_epoch);p=literal(p,",\"generation\":");p=hexadecimal(p,a->source_generation);
    p=literal(p,",\"frame\":");p=hexadecimal(p,i->frame_id);p=literal(p,",\"sample\":");p=hexadecimal(p,s->sequence);
    p=literal(p,",\"raw_epoch\":");p=hexadecimal(p,s->epoch);p=literal(p,",\"raw_generation\":");p=hexadecimal(p,s->generation);
    p=literal(p,",\"flags\":");p=hexadecimal(p,flags);p=literal(p,",\"policy_success\":");p=decimal(p,success!=0);
    p=literal(p,",\"pose_time\":");p=hexadecimal(p,(U64)i->display_time);p=literal(p,",\"raw_time\":");p=hexadecimal(p,(U64)s->display_time);
    p=literal(p,",\"prior_time\":");p=hexadecimal(p,(U64)before->last_time);p=literal(p,",\"raw_tick\":");p=hexadecimal(p,s->tick_ms);
    p=literal(p,",\"scope_cancels\":[");p=hexadecimal(p,before->cancel);p=literal(p,",");p=hexadecimal(p,s->cancel);
    for(unsigned h=0;h<2;++h){p=literal(p,",");p=hexadecimal(p,before->hand_cancel[h]);p=literal(p,",");p=hexadecimal(p,s->hand_cancel[h]);}
    p=literal(p,"],\"raw_gate\":[");p=decimal(p,s->eligible);p=literal(p,",");p=decimal(p,s->observed_eligible);
    p=literal(p,",");p=decimal(p,s->observed_inhibited);p=literal(p,",");p=decimal(p,s->observed_drag_owner);
    for(unsigned h=0;h<2;++h){p=literal(p,",");p=decimal(p,s->active[h]);p=literal(p,",");p=decimal(p,s->tracked[h]);}
    p=literal(p,"],\"roles_latches\":[");p=decimal(p,cfg.locomotion_hand);p=literal(p,",");p=decimal(p,cfg.smooth);
    p=literal(p,",");p=decimal(p,before->move_armed);p=literal(p,",");p=decimal(p,after->move_armed);
    p=literal(p,",");p=decimal(p,before->turn_armed);p=literal(p,",");p=decimal(p,after->turn_armed);
    p=literal(p,"],\"axes_f32\":[");float axes[]={s->x[0],s->y[0],s->x[1],s->y[1]};
    for(unsigned n=0;n<4;++n){union{float value;unsigned bits;}v={.value=axes[n]};if(n)p=literal(p,",");p=hexadecimal(p,v.bits);}
    p=literal(p,"],\"motion_f64\":[");double motion[]={out->dt,out->effective_speed,after->yaw,after->pitch,after->offset[0],after->offset[1],after->offset[2]};
    for(unsigned n=0;n<7;++n){union{double value;U64 bits;}v={.value=motion[n]};if(n)p=literal(p,",");p=hexadecimal(p,v.bits);}
    p=literal(p,"],\"scope_calls_moves_turns_rejects\":[");U64 totals[]={note->calls,note->moves,note->turns,note->rejected};
    for(unsigned n=0;n<4;++n){if(n)p=literal(p,",");p=hexadecimal(p,totals[n]);}
    p=literal(p,"],\"guard_counts\":[");for(unsigned n=0;n<28;++n){if(n)p=literal(p,",");p=hexadecimal(p,note->counts[n]);}
    p=literal(p,"]}\n");source_log(line,p);SetLastError(saved);
    if(!note->applied)return;
    const PoseAppliedTicket *t=&note->ticket;const ManagedEyePose *eyes=note->eyes;
    p=source_prefix(line,"navigation_block_pose_applied");
    p=literal(p,",\"captured_ms\":");p=hexadecimal(p,note->captured_ms);p=literal(p,",\"root_failed\":");p=decimal(p,failed!=0);
    p=literal(p,",\"epoch\":");p=hexadecimal(p,t->reference_epoch);p=literal(p,",\"generation\":");p=hexadecimal(p,t->source_generation);
    p=literal(p,",\"frame\":");p=hexadecimal(p,t->frame_id);p=literal(p,",\"sample\":");p=hexadecimal(p,s->sequence);
    p=literal(p,",\"anchor_yaw_pitch_units_f32\":[");float values[]={a->position[0],a->position[1],a->position[2],a->yaw,a->pitch,a->units_per_meter};
    for(unsigned n=0;n<6;++n){union{float value;unsigned bits;}v={.value=values[n]};if(n)p=literal(p,",");p=hexadecimal(p,v.bits);}
    p=literal(p,"],\"eye_positions_f32\":[");
    for(unsigned eye=0;eye<2;++eye)for(unsigned n=0;n<3;++n){union{float value;unsigned bits;}v={.value=eyes[eye].position[n]};if(eye||n)p=literal(p,",");p=hexadecimal(p,v.bits);}
    p=literal(p,"]}\n");source_log(line,p);SetLastError(saved);
}
static void navigation_block_applied(const void *input,const void *anchor,const void *poses,const void *ticket) {
    const UnityTrackingInput *i=input;const PoseSourceAnchor *a=anchor;const ManagedEyePose *eyes=poses;const PoseAppliedTicket *t=ticket;
    __typeof__(navigation_block_observer) *o=&navigation_block_observer;
    if(!i||!a||!eyes||!t||!navigation_block_note.active||!o->pending_frame||t->frame_id!=o->pending_frame||t->reference_epoch!=o->pending_epoch||
       t->source_generation!=o->pending_generation)return;
    if(t->frame_id!=i->frame_id||t->reference_epoch!=i->reference_epoch||t->source_generation!=a->source_generation)return;
    navigation_block_note.ticket=*t;navigation_block_note.eyes[0]=eyes[0];navigation_block_note.eyes[1]=eyes[1];
    navigation_block_note.applied=1;o->pending_frame=0;
}
#endif
