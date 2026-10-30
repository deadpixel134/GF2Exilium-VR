/* Actual eligibility/group/typed-liveness functions, modeled Unity calls. */
#define GF2VR_TOPOLOGY_INPUT_REPORT(...) ((void)0)
#include "crewdeck_fixture.h"
static unsigned throw_dead;
static void *dead_invoke(void *m,void *p,void **args,void **ex) {
 if(throw_dead && (m==&active_method || m==&enabled_method) && !((Obj *)p)->live){*ex=&box;return 0;}
 return invoke(m,p,args,ex);
}
static void setup(void) {
 seed();api.invoke=dead_invoke;group.ui_binding.refs.settings.calls=api;throw_dead=1;
}
int main(void) {
 SourceTopologyApi topo={.instance=&instance_method,.int_class=&camera_class,.bool_class=&bool_class};
 SourceTopology t={.api=&topo};source_group=(ActivitySourceGroup){&source,&t};
 setup();CHECK(activity_ui_enroll(&group));ui[0].live=ui[1].live=0;
 unsigned retained=roots;CHECK(activity_ui_stop_eligibility(&group));
 CHECK(!group.ui_enrolled && roots==retained && uicr_owned(&ui[0])==1);
 CHECK(activity_group_retire(&group));CHECK(!group.generation && roots==2);
 void *raw[]={&ui[0]};uicr_reap(raw,1);CHECK(roots==1 && uicr_owned(&ui[0])==1);
 uicr_reap(raw,0);CHECK(!roots);
 /* Unknown typed liveness must retain ownership and allow a later retry. */
 setup();CHECK(activity_ui_enroll(&group));ui[0].live=0;ui[0].known=0;
 CHECK(!activity_group_retire(&group) && group.ui_enrolled && !retire_count);
 ui[0].known=1;CHECK(activity_group_retire(&group));
 /* A live setter failure is not proof of death, even if readback is off. */
 setup();CHECK(activity_ui_enroll(&group));fail_set=set_count+1;
 CHECK(!activity_ui_stop_eligibility(&group) && group.ui_enrolled);
 fail_set=0;CHECK(activity_ui_stop_eligibility(&group));
 /* Camera death cannot excuse failure on its still-live GameObject. */
 setup();CHECK(activity_ui_enroll(&group));Obj parent={&camera_class,900,1,1,1,1,0};
 group.ui_eyes.eyes[0].object=&parent;ui[0].live=0;fail_set=set_count+1;
 CHECK(!activity_ui_stop_eligibility(&group) && group.ui_enrolled);
 fail_set=0;CHECK(activity_ui_stop_eligibility(&group) && !parent.active);
 /* Borrow and owner-thread guards remain earlier than cleanup. */
 setup();CHECK(activity_ui_enroll(&group));ui[0].live=ui[1].live=0;group.ui_eyes.borrowed=1;
 CHECK(!activity_group_retire(&group) && group.ui_enrolled && !retire_count);
 group.ui_eyes.borrowed=0;thread=8;CHECK(!activity_ui_stop_eligibility(&group));thread=7;
 CHECK(activity_group_retire(&group));
 /* Ordinary live frame eligibility survives CPU return until explicit drain. */
 setup();CHECK(activity_group_borrow(&group,5));
 CHECK(activity_group_cpu_return(&group) && group.ui_enrolled && ui[0].active && ui[1].enabled);
 CHECK(!activity_group_retire(&group));
 group.borrowed=group.ui_eyes.borrowed=group.eyes[0].borrowed=0;
 CHECK(activity_group_retire(&group));
 /* Both independent private UI layers must retire, not only the first. */
 setup();Obj secondary[2];ActivityUiLayer *l=&group.dialogue_ui;
 l->binding=group.ui_binding;l->data=(Data){.binding=&l->binding,.ready=1,.source_camera=&original_ui};
 for(unsigned i=0;i<2;++i){secondary[i]=(Obj){&camera_class,301+(int)i,1,1,1,1,0};
  l->eyes.eyes[i]=(ManagedEyeSlot){&secondary[i],&secondary[i],pin(&secondary[i],1),pin(&secondary[i],1),0};}
 CHECK(activity_ui_enroll(&group) && activity_ui_enroll_layer(&group,l));
 for(unsigned i=0;i<2;++i)ui[i].live=secondary[i].live=0;
 CHECK(activity_group_retire(&group) && !group.generation && roots==4);
 uicr_reap(raw,0);CHECK(!roots);
 printf("PASS %u dead-UI retirement/live-unknown rejection/ownership/retry checks; modeled Unity only\n",checks);
 return 0;
}
