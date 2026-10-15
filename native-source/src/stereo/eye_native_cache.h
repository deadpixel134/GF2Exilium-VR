#ifndef GF2_EYE_NATIVE_CACHE_H
#define GF2_EYE_NATIVE_CACHE_H
#include "eye_native_targets.h"
/* Fixed private allocation ownership, Unity owner thread only. Native identity
   is not frame content or a completion proof. A borrow quarantines invalid refs
   until the existing CPU/render/copy drain releases that same pool slot. */
typedef struct {
    EyeNativeTargets native;
    const ManagedEyes *eyes;
    unsigned long long generation;
    void *target[2];unsigned root[2];
    int width,height;
    unsigned valid,busy;
} EyeNativeCache;
void eye_native_resource_retain(void *resource);
void eye_native_resource_release(void *resource);
typedef struct {void (*retain)(void *);void (*release)(void *);} EyeNativeRefApi;
static const EyeNativeRefApi eye_native_refs={eye_native_resource_retain,eye_native_resource_release};
static int eye_native_cache_key(const EyeNativeCache *c,const SourceEyePreparation *p) {
    return c && p && c->valid && c->eyes==p->eyes && p->generation &&
        c->generation==p->generation && p->source &&
        p->generation==p->source->selection.generation && c->width==p->width && c->height==p->height &&
        p->eyes && c->target[0]==p->eyes->eyes[0].target && c->target[1]==p->eyes->eyes[1].target &&
        c->root[0]==p->eyes->eyes[0].target_root && c->root[1]==p->eyes->eyes[1].target_root;
}
static void eye_native_cache_invalidate(EyeNativeCache *c) {if(c)c->valid=0;}
static int eye_native_cache_drop(EyeNativeCache *c,const ManagedEyes *eyes,const EyeNativeRefApi *refs) {
    if(!c || c->busy || (eyes && eyes->borrowed) || !refs || !refs->release)return 0;
    for(unsigned i=0;i<2;++i)if(c->native.resource[i])refs->release(c->native.resource[i]);
    *c=(EyeNativeCache){0};return 1;
}
/* Called BEFORE preparation may recreate a target, including same-generation
   CameraDataOwner reconciliation. An address/root ABA cannot preserve the cache. */
static int eye_native_cache_prepare(EyeNativeCache *c,const SourceEyePreparation *p,const EyeNativeRefApi *refs) {
    if(!c || !p || !p->eyes || c->busy)return 0;
    if(eye_native_cache_key(c,p) && p->eyes->ready && p->eyes->targets_ready)return 1;
    eye_native_cache_invalidate(c);
    return eye_native_cache_drop(c,p->eyes,refs);
}
static int eye_native_cache_read(EyeNativeCache *c,const SourceEyePreparation *p,const EyeNativeApi *a,
    unsigned long long frame,int (*current)(void *),void *owner,const EyeNativeRefApi *refs,
    EyeNativeTargets *out,unsigned *reason,unsigned *hit) {
    if(reason)*reason=0;if(hit)*hit=0;if(!out)return 0;*out=(EyeNativeTargets){0};
    if(!c || !p || !p->eyes || !p->generation || !refs || !refs->retain || !refs->release || c->busy) {
        if(reason)*reason=86;return 0;
    }
    const ManagedEyes *eyes=p->eyes;int ok=0;c->busy=1;
    unsigned long long generation=p->generation;
    void *target[2]={eyes->eyes[0].target,eyes->eyes[1].target};
    unsigned root[2]={eyes->eyes[0].target_root,eyes->eyes[1].target_root};
    int width=p->width,height=p->height;
    EyeNativeTargets next={0};
    if(!c->native.resource[0] && !c->native.resource[1]) {
        if(eye_native_read_detail(a,eyes,frame,current,owner,&next,reason) &&
           generation && p->source && generation==p->generation &&
           generation==p->source->selection.generation && p->eyes==eyes &&
           width==p->width && height==p->height && width>0 && height>0 &&
           target[0]==eyes->eyes[0].target && target[1]==eyes->eyes[1].target &&
           root[0]==eyes->eyes[0].target_root && root[1]==eyes->eyes[1].target_root) {
            /* No managed invocation between the last current check and AddRef. */
            refs->retain(next.resource[0]);refs->retain(next.resource[1]);
            c->native=next;c->native.frame=0;c->eyes=eyes;c->generation=p->generation;
            c->width=p->width;c->height=p->height;
            for(unsigned i=0;i<2;++i){c->target[i]=eyes->eyes[i].target;c->root[i]=eyes->eyes[i].target_root;}
            c->valid=1;ok=1;
        }
    } else if(!eye_native_cache_key(c,p)) {if(reason)*reason=87;}
    else if(!a || !a->get_srgb || !a->bool_class || !a->calls.target_is_created ||
        !a->calls.invoke || !a->calls.pin || !a->calls.unpin || !a->calls.object_class || !a->calls.unbox) {
        if(reason)*reason=2;
    } else if(eye_native_current_detail(eyes,frame,current,owner,reason,10)) {
        next=c->native;next.frame=frame;ok=1;
        for(unsigned i=0;i<2 && ok;++i) {
            unsigned char srgb=0,created=0;
            GF2VR_NATIVE_CALL(i*3,0);
            int typed=eye_native_value(a,c->target[i],a->get_srgb,a->bool_class,&srgb,1);
            GF2VR_NATIVE_CALL(i*3,1);
            if(!typed){if(reason)*reason=20+i*10;ok=0;break;}
            if(srgb>1 || srgb!=c->native.srgb){if(reason)*reason=21+i*10;ok=0;break;}
            if(!eye_native_current_detail(eyes,frame,current,owner,reason,40+i*10)){ok=0;break;}
            GF2VR_NATIVE_CALL(i*3+1,0);
            typed=eye_native_value(a,c->target[i],a->calls.target_is_created,a->bool_class,&created,1);
            GF2VR_NATIVE_CALL(i*3+1,1);
            if(!typed){if(reason)*reason=44+i*10;ok=0;break;}
            if(!created){if(reason)*reason=45+i*10;ok=0;break;}
            if(!eye_native_current_detail(eyes,frame,current,owner,reason,60+i*10)){ok=0;break;}
        }
        if(ok)ok=eye_native_current_detail(eyes,frame,current,owner,reason,80);
        if(ok && !eye_native_cache_key(c,p)){if(reason)*reason=87;ok=0;}
        if(ok && hit)*hit=1;
    }
    /* Current/pose/cancellation failure rejects this frame, without confusing
       allocation identity with frame eligibility. Changed/uncreated/ill-typed
       targets invalidate identity; an owner invalidation also remains sticky. */
    if(!ok && reason && (*reason==20 || *reason==21 || *reason==30 || *reason==31 ||
        *reason==44 || *reason==45 || *reason==54 || *reason==55 ||
        *reason==84 || *reason==85 || *reason==87))eye_native_cache_invalidate(c);
    if(!ok && next.frame && reason && !*reason)*reason=87;
    c->busy=0;if(ok)*out=next;return ok;
}
/* Only completed native coverage/CPU facts; never acquires another snapshot. */
static int eye_native_cache_failed_output(unsigned rendered,unsigned draw,unsigned clear,
                                         unsigned injected,unsigned cpu_ok) {
    return (rendered && (draw!=3 || clear!=3)) || (injected && !cpu_ok);
}
#endif
