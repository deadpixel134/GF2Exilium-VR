#ifndef GF2VR_LATE_RELOCATE_H
#define GF2VR_LATE_RELOCATE_H
#if defined(GF2VR_LATENCY_OBSERVE) || defined(GF2VR_LATE_RELOCATE_HOST_TEST)
#define GF2VR_LR_COUNT(o,field) __atomic_fetch_add(&(o)->field,1,__ATOMIC_RELAXED)
#else
#define GF2VR_LR_COUNT(o,field) ((void)0)
#endif
#include "xr_eye_input.h"
/* Separate POD; never changes a frozen helper's structs or immutable pool. */
typedef unsigned long long LrU64;
typedef XrResult (XRAPI_PTR *LrConvert)(XrInstance,XrTime,long long *);
typedef struct {LrU64 epoch,frame,generation;} LrKey;
typedef struct {
    XrSession session;XrSpace space,head_space;XrInstance instance;
    PFN_xrLocateViews views;PFN_xrLocateSpace head;LrConvert convert;
    double (*tangent)(double);XrTime time;XrDuration period;LrU64 epoch;
} LrContext;
typedef struct {unsigned lock,used;LrKey key;XrEyeInput pose;LrU64 before,after;} LrSlot;
typedef struct {
    unsigned enabled,lock;LrU64 epoch;LrContext context;
    LrSlot slots[16];LrU64 attempts,success,failed,busy,discarded;
} LateRelocate;
static int lr_same(LrKey a,LrKey b){return a.epoch==b.epoch&&a.frame==b.frame&&a.generation==b.generation;}
static int lr_lock(unsigned *lock){unsigned zero=0;return __atomic_compare_exchange_n(lock,&zero,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED);}
static void lr_unlock(unsigned *lock){__atomic_store_n(lock,0,__ATOMIC_RELEASE);}
static void lr_enable(LateRelocate *o,unsigned enabled){__atomic_store_n(&o->enabled,enabled!=0,__ATOMIC_RELEASE);}
static void lr_invalidate(LateRelocate *o){__atomic_store_n(&o->epoch,0,__ATOMIC_RELEASE);}
/* XR only: no waiting. Close stops all acquisitions before checking the lease.
   A BUSY close quarantines that owner, never destroys a handle. The parent
   instance destroy must consult this same lease before destroying children. */
static int lr_close(LateRelocate *o){lr_invalidate(o);if(!lr_lock(&o->lock))return 0;o->context=(LrContext){0};lr_unlock(&o->lock);return 1;}
static void lr_publish(LateRelocate *o,const LrContext *c){
    if(!c->epoch||!c->session||!c->space||!c->head_space)return;
    if(!lr_lock(&o->lock))return;
    o->context=*c;__atomic_store_n(&o->epoch,c->epoch,__ATOMIC_RELEASE);lr_unlock(&o->lock);
}
static int lr_current(LateRelocate *o,LrKey k){return k.epoch&&k.frame&&k.generation&&__atomic_load_n(&o->epoch,__ATOMIC_ACQUIRE)==k.epoch;}
/* On failure packet is byte-for-byte unchanged. Clock callback returns QPC;
   runtime conversion alone maps an XR prediction to that clock domain. */
static int lr_refresh(LateRelocate *o,LrKey key,XrEyeInput *packet,LrU64 (*clock)(void),LrU64 *before,LrU64 *after){
    if(!__atomic_load_n(&o->enabled,__ATOMIC_ACQUIRE))return 0;
    GF2VR_LR_COUNT(o,attempts);
    if(!packet||!clock||!lr_current(o,key)||!xr_eye_matches(packet,key.epoch,key.frame,packet->display_time))goto failed;
    LrSlot *slot=&o->slots[key.frame&15];
    if(!lr_lock(&slot->lock)){GF2VR_LR_COUNT(o,busy);goto failed;}
    if(slot->used&&lr_same(slot->key,key)){
        *packet=slot->pose;*before=slot->before;*after=slot->after;lr_unlock(&slot->lock);return 1;
    }
    if(slot->used&&slot->key.epoch==key.epoch&&slot->key.generation==key.generation){lr_unlock(&slot->lock);goto failed;}
    if(!lr_lock(&o->lock)){lr_unlock(&slot->lock);GF2VR_LR_COUNT(o,busy);goto failed;}
    LrContext c=o->context;int ok=lr_current(o,key)&&c.epoch==key.epoch&&c.convert&&c.views&&c.head&&c.tangent;
    XrEyeInput value=*packet;LrU64 begin=clock(),finish=begin;XrTime time=c.time>packet->display_time?c.time:packet->display_time;
    long long predicted=0;
    if(ok)ok=c.convert(c.instance,time,&predicted)==XR_SUCCESS&&predicted>0;
    if(ok&&(LrU64)predicted<=begin){
        if(c.period>0&&time<=0x7fffffffffffffffll-c.period){time+=c.period;ok=c.convert(c.instance,time,&predicted)==XR_SUCCESS&&predicted>0&&(LrU64)predicted>begin;}
        else ok=0;
    }
    XrView views[2]={{.type=XR_TYPE_VIEW},{.type=XR_TYPE_VIEW}};
    XrViewState state={.type=XR_TYPE_VIEW_STATE};uint32_t count=0;
    XrViewLocateInfo info={.type=XR_TYPE_VIEW_LOCATE_INFO,.viewConfigurationType=XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,.displayTime=time,.space=c.space};
    if(ok)ok=c.views(c.session,&info,&state,2,&count,views)==XR_SUCCESS&&count==2&&
        (state.viewStateFlags&(XR_VIEW_STATE_POSITION_VALID_BIT|XR_VIEW_STATE_ORIENTATION_VALID_BIT))==(XR_VIEW_STATE_POSITION_VALID_BIT|XR_VIEW_STATE_ORIENTATION_VALID_BIT);
    XrSpaceLocation head={.type=XR_TYPE_SPACE_LOCATION};
    if(ok)ok=c.head(c.head_space,c.space,time,&head)==XR_SUCCESS&&
        (head.locationFlags&(XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT))==(XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)&&xr_eye_pose_valid(&head.pose);
    if(ok)for(unsigned i=0;i<2;++i){
        if(!xr_eye_pose_valid(&views[i].pose)||!xr_eye_fov(views[i].fov,c.tangent,&value.tangents[i])){ok=0;break;}
        value.poses[i]=views[i].pose;value.fov[i]=views[i].fov;
    }
    finish=clock();
    ok=ok&&lr_current(o,key)&&finish>=begin;
    if(ok){
        value.head=head.pose;value.flags=state.viewStateFlags;
        /* Reference, frame identity and original Wait-time metadata stay exact. */
        slot->pose=value;slot->key=key;slot->before=begin;slot->after=finish;slot->used=1;
        *packet=value;*before=begin;*after=finish;GF2VR_LR_COUNT(o,success);
    }
    lr_unlock(&o->lock);lr_unlock(&slot->lock);if(ok)return 1;
failed:GF2VR_LR_COUNT(o,failed);return 0;
}
static void lr_cancel(LateRelocate *o,LrKey key){
    LrSlot *s=&o->slots[key.frame&15];if(!lr_lock(&s->lock))return;
    if(s->used&&lr_same(s->key,key)){s->used=0;GF2VR_LR_COUNT(o,discarded);}lr_unlock(&s->lock);
}
/* XR copy before producer transfer. Retain until the receipt owns the value;
   repeated BUSY retries must keep exactly the same rendered pose. */
static int lr_submission(LateRelocate *o,LrKey key,XrEyeInput *pose){
    LrSlot *s=&o->slots[key.frame&15];if(!lr_lock(&s->lock))return 0;
    if(s->used&&lr_same(s->key,key))*pose=s->pose;
    lr_unlock(&s->lock);return 1;
}
#endif
