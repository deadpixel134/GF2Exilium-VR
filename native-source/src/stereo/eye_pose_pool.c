#include "eye_pose_pool.h"
static int lock_pool(EyePosePool *p) {
    unsigned expected=0;
    return __atomic_compare_exchange_n(&p->locked,&expected,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED);
}
static int unlock_pool(EyePosePool *p,int result) {
    __atomic_store_n(&p->locked,0,__ATOMIC_RELEASE);return result;
}
static int same(EyeFlightToken a,EyeFlightToken b) {
    return a.frame && a.generation && a.epoch && a.frame==b.frame &&
        a.generation==b.generation && a.epoch==b.epoch;
}
static int pose_valid(const XrEyeInput *p,EyeFlightToken t,long long time) {
    if(!xr_eye_matches(p,t.epoch,t.frame,time) || !p->head_valid ||
       !xr_eye_pose_valid(&p->head) || !xr_eye_pose_valid(&p->reference_head))return 0;
    XrViewStateFlags required=XR_VIEW_STATE_POSITION_VALID_BIT|XR_VIEW_STATE_ORIENTATION_VALID_BIT;
    if((p->flags&required)!=required)return 0;
    for(unsigned i=0;i<2;++i) {
        float checked;EyeFrustumTangents v=p->tangents[i];
        XrFovf f=p->fov[i];
        if(!xr_eye_pose_valid(&p->poses[i]) ||
           !eye_optics_float(v.left,&checked) || !eye_optics_float(v.right,&checked) ||
           !eye_optics_float(v.down,&checked) || !eye_optics_float(v.up,&checked) ||
           !(v.right>v.left && v.up>v.down) ||
           !(f.angleLeft>-1.5707963267948966 && f.angleRight<1.5707963267948966 &&
             f.angleDown>-1.5707963267948966 && f.angleUp<1.5707963267948966 &&
             f.angleRight>f.angleLeft && f.angleUp>f.angleDown))return 0;
    }
    return 1;
}
int eye_pose_pool_offer(EyePosePool *p,EyeFlightOwner *o,EyeFlightToken t,
                       long long time,int renderable,const XrEyeInput *pose,unsigned *slot) {
    if(!slot)return EFO_REJECTED;
    *slot=EFO_CAPACITY;
    if(!p || !o || (renderable && !pose_valid(pose,t,time)) || (!renderable && pose))return EFO_REJECTED;
    if(!lock_pool(p))return EFO_BUSY;
    /* Detect bypassed retirement before allocating. Existing pose storage must
       match its numeric slot, even when cancelled/reclaiming. */
    for(unsigned i=0;i<EFO_CAPACITY;++i)if(p->slots[i].token.frame) {
        EyeFlightSnapshot s;int result=eye_flight_snapshot(o,p->slots[i].token,&s);
        if(result!=EFO_OK)return unlock_pool(p,result);
    }
    unsigned index;int result=eye_flight_offer(o,t,time,renderable,&index);
    if(result!=EFO_OK)return unlock_pool(p,result);
    p->slots[index]=(EyePoseSlot){.token=t};
    if(renderable)p->slots[index].pose=*pose;
    *slot=index;return unlock_pool(p,EFO_OK);
}
int eye_pose_pool_read(EyePosePool *p,EyeFlightOwner *o,EyeFlightToken t,
                      EyeFlightSnapshot *snapshot,XrEyeInput *pose) {
    if(snapshot)*snapshot=(EyeFlightSnapshot){0};
    if(pose)*pose=(XrEyeInput){0};
    if(!p || !o || !snapshot || !pose)return EFO_REJECTED;
    if(!lock_pool(p))return EFO_BUSY;
    for(unsigned i=0;i<EFO_CAPACITY;++i)if(same(p->slots[i].token,t)) {
        EyeFlightSnapshot s;int result=eye_flight_snapshot(o,t,&s);
        if(result!=EFO_OK)return unlock_pool(p,result);
        unsigned terminal=EFF_XR_ENDED;
#ifdef GF2VR_ASYNC_PRESENTATION
        if(s.flags&EFF_DISPLAY_DETACHED)terminal=EFF_PRESENTATION_RETIRED;
#ifdef GF2VR_BANK_TRANSFER
        terminal|=EFF_PRODUCER_TRANSFERRED;
#endif
#endif
        if(s.flags&(EFF_CANCELLED|EFF_FAULT|EFF_RECLAIMING|terminal) ||
           !s.renderable || !pose_valid(&p->slots[i].pose,t,s.display_time))
            return unlock_pool(p,EFO_REJECTED);
        *snapshot=s;*pose=p->slots[i].pose;return unlock_pool(p,EFO_OK);
    }
    return unlock_pool(p,EFO_REJECTED);
}
int eye_pose_pool_retire(EyePosePool *p,EyeFlightOwner *o,EyeFlightToken t) {
    if(!p || !o)return EFO_REJECTED;
    if(!lock_pool(p))return EFO_BUSY;
    for(unsigned i=0;i<EFO_CAPACITY;++i)if(same(p->slots[i].token,t)) {
        /* Ack checks authoritative reclaim state. Keep the packet on BUSY or
           refusal so retry never loses the exact identity. All pool readers
           remain excluded until both owners are cleared. */
        int result=eye_flight_retire_ack(o,t);
        if(result==EFO_OK)p->slots[i]=(EyePoseSlot){0};
        return unlock_pool(p,result);
    }
    return unlock_pool(p,EFO_REJECTED);
}
