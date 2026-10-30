#include <stdio.h>
#include <string.h>
#include <stdint.h>
typedef unsigned long long U64;
typedef unsigned DWORD;
#define GF2VR_ACTIVITY_PRIMARY_UI
#define GF2VR_ACTIVITY_UI_FRAME_ELIGIBLE
#define GF2VR_ACTIVITY_STACK
#define GF2VR_RETIRE_FAILURE(a,b) ((void)0)
#define GF2VR_ADMISSION_STEP(a) ((void)0)
typedef struct Obj {void *klass;int id;unsigned live,known,active,enabled,scheduled;}Obj;
static int camera_class,bool_class,array_class,alive_method,active_method,enabled_method,instance_method,item_method;
static unsigned thread=7,roots,next_root,fail_pin,set_count,fail_set,set_after_throw,value_ok=1,source_valid=1,retire_count;
static void *handles[4096];
static unsigned GetCurrentThreadId(void){return thread;}
static unsigned pin(void *p,int strong){if(!p || !strong || ++next_root==fail_pin)return 0;handles[next_root]=p;++roots;return next_root;}
static void unpin(unsigned h){if(!h || !handles[h]){roots=9999;return;}handles[h]=0;--roots;}
static void *objclass(void *p){return *(void **)p;}
static struct {void *klass;unsigned char value;}box;
static void *unbox(void *p){(void)p;return &box.value;}
typedef struct {void *klass;void *items[32];unsigned count;}Array;
static void *invoke(void *method,void *p,void **args,void **exception){
 if(method==&item_method)return ((Array *)p)->items[*(int *)args[0]];
 Obj *o=p;
 if(method==&alive_method){o=args[0];box.klass=o->known?&bool_class:&array_class;box.value=o->live;return &box;}
 if(method==&active_method || method==&enabled_method){++set_count;
  if(set_count==fail_set){*exception=&box;return 0;}
  if(method==&active_method)o->active=*(unsigned char *)args[0];else o->enabled=*(unsigned char *)args[0];
  if(set_count==set_after_throw)*exception=&box;return 0;}
 *exception=&box;return 0;
}
typedef struct {void *camera_class,*bool_class,*object_alive,*set_active,*set_enabled,*get_transform;
 unsigned (*pin)(void *,int);void (*unpin)(unsigned);void *(*object_class)(void *);void *(*unbox)(void *);
 void *(*invoke)(void *,void *,void **,void **);}ManagedEyeApi;
#include "frame_actual_managed.h"
typedef struct {ManagedEyeApi calls;}SourcePoseApi;
static int source_pose_value(SourcePoseApi *a,void *m,void *o,void *k,void *out,unsigned n){
 (void)a;(void)m;(void)k;if(!value_ok || n!=4)return 0;*(int *)out=((Obj *)o)->id;return 1;}
#include "activity_ui_camera_registry.h"
typedef struct {void *source;int instance_id;unsigned count,attributed;float units_per_meter;U64 group_stamp;}SourceGenerationCandidate;
typedef struct {SourceGenerationCandidate identity;int orthographic,display,width,height,mask;unsigned has_target;}SourceTopologyEntry;
typedef struct {SourcePoseApi values;void *instance,*parent,*orthographic,*display,*target,*width,*height,*mask;
 void *int_class,*bool_class,*transform_class;}SourceTopologyApi;
typedef struct {unsigned (*pin)(void *,int);void (*unpin)(unsigned);void *(*object_class)(void *);
 unsigned (*array_length)(void *);void *(*invoke)(void *,void *,void **,void **);void *get_value,*camera_class,*array_class;}CameraBatchApi;
typedef struct {SourceTopologyApi *api;CameraBatchApi *batch;void *context;
 int (*attribute)(void *,const SourceTopologyEntry *,unsigned,unsigned *,float *);
 unsigned busy,cancelled;SourceTopologyEntry entries[32];void *cameras[32];unsigned roots[33];U64 group_stamp;}SourceTopology;
static int source_topology_entry(const SourceTopologyApi *a,void *camera,SourceTopologyEntry *e){(void)a;
 Obj *o=camera;if(!o->live)return 0;*e=(SourceTopologyEntry){.identity={.source=camera,.instance_id=o->id,.count=1},.width=3840,.height=2160};return 1;}
static int source_generation_same(const SourceGenerationCandidate *a,const SourceGenerationCandidate *b){return a->source==b->source && a->instance_id==b->instance_id && a->count==b->count;}
static int eye_optics_float(float n,float *out){*out=n;return n>0;}
#define GF2VR_TOPOLOGY_PRIVATE(c) uicr_owned(c)
#define GF2VR_TOPOLOGY_REAP(c,n) uicr_reap(c,n)
#include "frame_actual_topology.h"
typedef struct {void *camera,*object;unsigned camera_root,object_root,target_root;}ManagedEyeSlot;
typedef struct {ManagedEyeSlot eyes[2];unsigned ready,borrowed,targets_ready,matrices_ready;U64 matrix_frame;}ManagedEyes;
typedef struct {struct {ManagedEyeApi calls;void *get[11],*value_class[11];}settings;}Refs;
typedef struct {Refs refs;unsigned type_root;}Binding;
typedef struct {Binding *binding;ManagedEyes *eyes;void *source_camera,*components[3];unsigned ready,busy,retiring;}Data;
typedef struct {unsigned owner_thread;}Source;
typedef struct {Source *source;SourceTopology *topology;}ActivitySourceGroup;
typedef struct {void *source,*left,*right;}Member;
typedef struct {ActivitySourceGroup *source;Binding binding[7],ui_binding;Data data[7],ui_data,*base;
 ManagedEyes eyes[7],ui_eyes,*base_eyes;Member members[8];
 unsigned count,ready,busy,retiring,borrowed,activated,ui_shown,ui_linked,ui_activated,ui_enrolled;U64 generation,frame;}ActivityEyeGroup;
static int activity_group_source_valid(ActivityEyeGroup *g){return g && !g->retiring && source_valid && g->generation && g->count>1;}
static int generic_source_active(ActivitySourceGroup *a,void *p,unsigned *active){(void)a;
 Obj *o=p;if(!o->known)return 0;*active=o->live && o->active && o->enabled;return 1;}
static int camera_data_u32(Refs *r,void *m,void *p,void *k,unsigned *out){(void)r;(void)m;(void)p;(void)k;*out=1;return 1;}
static void source_generation_invalidate(Source *s){(void)s;source_valid=0;}
static void activity_group_invalidate(ActivityEyeGroup *g){g->retiring=1;g->ready=0;g->ui_data.retiring=1;}
static int camera_data_owner_retire_single(Data *d){(void)d;++retire_count;return 1;}
static int managed_eyes_retire(ManagedEyeApi *a,ManagedEyes *p){if(p->borrowed)return 0;
 for(unsigned i=0;i<2;++i){if(p->eyes[i].camera_root){((Obj *)p->eyes[i].camera)->scheduled=1;a->unpin(p->eyes[i].camera_root);p->eyes[i].camera_root=0;}
  if(p->eyes[i].object_root){a->unpin(p->eyes[i].object_root);p->eyes[i].object_root=0;}}return 1;}
static int camera_data_component_release(Binding *b){b->type_root=0;return 1;}
#include "activity_ui_frame_eligibility.h"
static int activity_group_cpu_return(void *);
#include "frame_actual_group.h"
static unsigned checks,policy_count,policy_mode;
static Obj original_world,original_ui,ui[2],world_overlay[2];static Array input;
static ActivityEyeGroup group;static Source source={7};static ActivitySourceGroup source_group;
static ManagedEyeApi api;
static int policy(void *p,const SourceTopologyEntry *e,unsigned n,unsigned *index,float *units){
 (void)p;++policy_count;if(n!=2 || e[0].identity.source!=&original_world || e[1].identity.source!=&original_ui)return 0;
 *index=1;*units=1;
 if(policy_mode==1)input.items[0]=&original_ui;
 if(policy_mode==2)uicr_add(&policy_mode,3,&api,&original_world,original_world.id);
 return 1;}
static unsigned length(void *p){return ((Array *)p)->count;}
#define CHECK(x) do{++checks;if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
static void seed(void){
 memset(&uicr,0,sizeof uicr);memset(handles,0,sizeof handles);roots=next_root=fail_pin=set_count=fail_set=set_after_throw=retire_count=policy_mode=0;
 source_valid=value_ok=1;thread=7;api=(ManagedEyeApi){.camera_class=&camera_class,.bool_class=&bool_class,.object_alive=&alive_method,
  .set_active=&active_method,.set_enabled=&enabled_method,.get_transform=&active_method,.pin=pin,.unpin=unpin,.object_class=objclass,.unbox=unbox,.invoke=invoke};
 original_world=(Obj){&camera_class,11,1,1,1,1,0};original_ui=(Obj){&camera_class,12,1,1,1,1,0};
 for(unsigned i=0;i<2;++i){ui[i]=(Obj){&camera_class,101+(int)i,1,1,0,0,0};world_overlay[i]=(Obj){&camera_class,201+(int)i,1,1,0,0,0};}
 group=(ActivityEyeGroup){.source=&source_group,.count=2,.ready=1,.generation=4,.ui_shown=1,.ui_linked=1};
 group.ui_binding.refs.settings.calls=group.binding[0].refs.settings.calls=api;
 group.ui_binding.type_root=group.binding[0].type_root=1;
 group.ui_data=(Data){.binding=&group.ui_binding,.eyes=&group.ui_eyes,.ready=1,.source_camera=&original_ui};
 group.data[0]=(Data){.binding=&group.binding[0],.ready=1};
 group.members[0].source=&original_world;group.members[1].source=&original_ui;
 group.ui_eyes.ready=group.ui_eyes.matrices_ready=group.eyes[0].ready=group.eyes[0].matrices_ready=1;
 group.ui_eyes.matrix_frame=group.eyes[0].matrix_frame=5;
 static Data base;static ManagedEyes base_eyes;base=(Data){0};base_eyes=(ManagedEyes){0};group.base=&base;group.base_eyes=&base_eyes;
 for(unsigned i=0;i<2;++i){group.ui_eyes.eyes[i]=(ManagedEyeSlot){&ui[i],&ui[i],pin(&ui[i],1),pin(&ui[i],1),0};
  group.eyes[0].eyes[i]=(ManagedEyeSlot){&world_overlay[i],&world_overlay[i],0,0,0};}
 input=(Array){.klass=&array_class,.items={&ui[0],&original_world,&ui[1],&original_ui},.count=4};
}
int main(void){
 SourceTopologyApi topo={.values={api},.instance=&instance_method,.parent=&active_method,.orthographic=&active_method,
  .display=&active_method,.target=&active_method,.width=&active_method,.height=&active_method,.mask=&active_method,
  .int_class=&camera_class,.bool_class=&bool_class,.transform_class=&camera_class};
 CameraBatchApi batch={pin,unpin,objclass,length,invoke,&item_method,&camera_class,&array_class};
 SourceTopology t={.api=&topo,.batch=&batch,.attribute=policy};source_group=(ActivitySourceGroup){&source,&t};
 seed();topo.values.calls=api;
 CHECK(activity_group_borrow(&group,5));CHECK(group.ui_enrolled && roots==6 && ui[0].active && ui[1].enabled);
 CHECK(uicr_owned(&ui[0])==1 && uicr_owned(&original_ui)==0);
 CHECK(activity_group_cpu_return(&group));CHECK(ui[0].active && ui[1].enabled && !world_overlay[0].active && !world_overlay[1].enabled);
 CHECK(group.borrowed && group.ui_eyes.borrowed); /* CPU return is not a GPU drain */
 SourceGenerationCandidate out={0};CHECK(source_topology_observe(&t,&input,&out));
 CHECK(out.source==&original_ui && out.instance_id==12 && out.attributed && roots==6);
 policy_mode=1;CHECK(!source_topology_observe(&t,&input,&out) && !out.attributed);input.items[0]=&ui[0];policy_mode=0;
 policy_mode=2;CHECK(!source_topology_observe(&t,&input,&out) && !out.attributed);policy_mode=0;
 thread=8;CHECK(uicr_owned(&ui[0])==-1);unsigned before=set_count;CHECK(!activity_ui_stop_eligibility(&group) && set_count==before);thread=7;
 CHECK(!activity_group_retire(&group) && !retire_count); /* still borrowed */
 group.retiring=0;source_valid=0;CHECK(activity_group_cpu_return(&group));CHECK(!ui[0].active && !ui[1].enabled && !group.ui_enrolled);
 CHECK(uicr_owned(&ui[0])==1 && roots>=6); /* tombstone survives native-off and deferred destroy */
 group.borrowed=group.ui_eyes.borrowed=group.eyes[0].borrowed=0;
 CHECK(activity_group_retire(&group));CHECK(ui[0].scheduled && ui[1].scheduled && roots>=2);
 ui[0].live=ui[1].live=0;void *empty[1]={0};unsigned retained=roots;
 uicr_reap(input.items,input.count);CHECK(roots==retained && uicr_owned(&ui[0])==1);
 ui[0].known=0;uicr_reap(empty,0);CHECK(uicr_owned(&ui[0])==1 && uicr_owned(&ui[1])==0);
 ui[0].known=1;uicr_reap(empty,0);CHECK(uicr_owned(&ui[0])==0);
 seed();topo.values.calls=api;fail_pin=next_root+2;CHECK(!activity_group_borrow(&group,5));CHECK(!ui[0].active && !ui[1].enabled && !source_valid);
 seed();topo.values.calls=api;fail_set=7;CHECK(!activity_group_borrow(&group,5));CHECK(!ui[0].active && !ui[1].active && !source_valid);
 seed();topo.values.calls=api;CHECK(activity_group_borrow(&group,5));source_valid=0;fail_set=set_count+5;
 CHECK(!activity_group_cpu_return(&group));CHECK(group.ui_enrolled && uicr_owned(&ui[0])==1);
 fail_set=0;CHECK(activity_group_cpu_return(&group));CHECK(!group.ui_enrolled && !ui[0].active);
 seed();topo.values.calls=api;group.ui_shown=0;CHECK(activity_group_borrow(&group,5));CHECK(!group.ui_enrolled && !ui[0].active && roots==4);
 seed();topo.values.calls=api;thread=8;unsigned value_roots=roots;CHECK(!activity_ui_enroll(&group) && roots==value_roots && !uicr.owner);thread=7;
 printf("PASS %u actual registry/raw-input filtering/group borrow-CPU-return-retirement checks; modeled Unity, native UI/PCaux runtime unaccepted.\n",checks);return 0;
}
