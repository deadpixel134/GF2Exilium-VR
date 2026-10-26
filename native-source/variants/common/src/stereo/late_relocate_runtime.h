#ifndef GF2VR_LATE_RELOCATE_RUNTIME_H
#define GF2VR_LATE_RELOCATE_RUNTIME_H
#include "late_relocate.h"
static LateRelocate stereo_late;
static int stereo_late_requested(void){return __atomic_load_n(&stereo_late.enabled,__ATOMIC_ACQUIRE)!=0;}
static int stereo_late_close(void){return !stereo_late_requested()||lr_close(&stereo_late);}
static int stereo_late_instance_retire(void){return stereo_late_close();}
static void stereo_late_invalidate(void){lr_invalidate(&stereo_late);}
static void stereo_late_publish(void *runtime,const void *frame){
    if(!stereo_late_requested())return;
    XrEyeRuntime *r=runtime;const XrFrameState *f=frame;XrEyeOwner *o=&r->owner;
    if(!f||!f->shouldRender||o->retiring||o->changing||!o->epoch||o->head_reference_epoch!=o->epoch){lr_invalidate(&stereo_late);return;}
    LrContext c={.session=o->session,.space=o->space,.head_space=o->head_space,.instance=xr_pose_clock.instance,
        .views=o->api.locate,.head=o->api.locate_space,.convert=(LrConvert)xr_pose_clock.convert,.tangent=o->api.tangent,
        .time=f->predictedDisplayTime,.period=f->predictedDisplayPeriod,.epoch=o->epoch};
    lr_publish(&stereo_late,&c);
}
static int stereo_late_refresh(void *opaque,unsigned long long generation){
    XrEyeInput *p=opaque;LrKey key={p->reference_epoch,p->frame_id,generation};
    DWORD saved=GetLastError();LrU64 before=0,after=0;
    int ok=lr_refresh(&stereo_late,key,p,xr_pose_clock_now,&before,&after);
#ifdef GF2VR_LATENCY_OBSERVE
    if(ok)latency_relocated(key.epoch,key.frame,key.generation,before,after);
#endif
    SetLastError(saved);return ok;
}
static void stereo_late_cancel(unsigned long long epoch,unsigned long long frame,unsigned long long generation){
    lr_cancel(&stereo_late,(LrKey){epoch,frame,generation});
}
static int stereo_late_submission(unsigned long long epoch,unsigned long long frame,unsigned long long generation,void *pose){
    if(!stereo_late_requested())return 1;
    return lr_submission(&stereo_late,(LrKey){epoch,frame,generation},pose);
}
static void stereo_late_config(unsigned enabled){lr_enable(&stereo_late,enabled);event("late_relocate_mode",enabled,GetCurrentThreadId());}
#ifdef GF2VR_LATENCY_OBSERVE
static void stereo_late_report(LatU64 now){
    char line[512],*p=latency_line(line,"latency_relocate_counts",now);
#define LR_STAT(field) p=latency_field(p,#field,__atomic_load_n(&stereo_late.field,__ATOMIC_RELAXED))
    LR_STAT(attempts);LR_STAT(success);LR_STAT(failed);LR_STAT(busy);LR_STAT(discarded);
#undef LR_STAT
    p=latency_field(p,"enabled",__atomic_load_n(&stereo_late.enabled,__ATOMIC_ACQUIRE));p=literal(p,"}\n");
    (void)event_log_enqueue(line,(unsigned)(p-line),0);
}
#endif
#endif
