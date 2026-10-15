#ifndef GF2_ACTIVITY_UI_TRANSITION_GROUP_H
#define GF2_ACTIVITY_UI_TRANSITION_GROUP_H
/* Stop normal-frame eligibility after CPU return, just like world Overlay
   deactivation. Do not touch lists, matrices, leases or roots until real drain. */
static int activity_ui_pause_group(ActivityEyeGroup *g) {
    if(!g->generation)return 1;
    if(!activity_group_source_valid(g) || g->busy || !g->base || g->base->busy)return 0;
    for(unsigned i=0;i<7;++i)if(g->data[i].busy)return 0;
    for(unsigned i=0;i<activity_ui_layer_count();++i)if(activity_ui_layer(g,i)->data.busy)return 0;
    g->busy=1;int ok=activity_ui_stop_eligibility(g);g->busy=0;return ok;
}
static int activity_ui_group_drained(ActivityEyeGroup *g) {
    if(!g->generation)return !g->busy && !g->borrowed;
    if(!activity_group_source_valid(g) || g->busy || g->borrowed || !g->base ||
       !g->base_eyes || g->base->busy || g->base_eyes->borrowed)return 0;
    for(unsigned i=0;i<7;++i)if(g->data[i].busy || g->eyes[i].borrowed)return 0;
    for(unsigned i=0;i<activity_ui_layer_count();++i){ActivityUiLayer *l=activity_ui_layer(g,i);
        if(l->data.busy || l->eyes.borrowed || l->enrolled)return 0;
    }
    return 1;
}
static int activity_ui_reset_group(ActivityEyeGroup *g) {
    if(!activity_ui_group_drained(g))return 0;
    if(!g->generation)return 1;
    g->busy=1;int ok=1;
    if(activity_ui_linked_count(g) && !activity_group_ui_link(g,0)){g->busy=0;return 0;}
    for(unsigned i=0;i<activity_ui_layer_count();++i){ActivityUiLayer *l=activity_ui_layer(g,i);
        l->shown=0;
#ifdef GF2VR_ACTIVITY_TARGET_SURFACE
        if(!activity_target_surface_clear(&l->binding.refs.settings.calls,&g->surfaces[7+i],&l->eyes)){ok=0;continue;}
#endif
        if(!camera_data_owner_retire_single(&l->data)){ok=0;continue;}
        if(l->binding.type_root && (!managed_eyes_retire(&l->binding.refs.settings.calls,&l->eyes) ||
           !camera_data_component_release(&l->binding))){ok=0;continue;}
        *l=(ActivityUiLayer){0};
    }
    g->busy=0;return ok;
}
#endif
