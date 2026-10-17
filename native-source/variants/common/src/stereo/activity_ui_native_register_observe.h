#ifndef GF2_ACTIVITY_UI_NATIVE_REGISTER_OBSERVE_H
#define GF2_ACTIVITY_UI_NATIVE_REGISTER_OBSERVE_H
#ifdef GF2VR_ACTIVITY_UI_CANVAS_MEMBERSHIP_OBSERVE
#include "activity_ui_canvas_membership.h"
#endif
/* Exact FB17 UnityPlayer28b1b0: void(Canvas*,Camera*,uint16_t*,bool).
   Common registration boundary used by camera and global preparation paths.
   Synchronous borrowed native Camera argument. No batch/job pointer access,
   no explicit native registration invocation, no product return dependency. */
static struct {
    void (*original)(void *,void *,void *,unsigned char);
    unsigned attempted,ready,stage,selected,active,owner,member_begin,member_end;
    int ids[4]; /* original world, original UI, private UI LEFT/RIGHT */
    U64 first_tick,serial,generation,frame,global_begin,global_end;
    unsigned entered[4],returned[4],foreign,unmatched,read_fail,null_camera;
    U64 global_entered,global_returned;
} acr;
static void acr_forward(void *canvas,void *camera,void *order,unsigned char force) {
    DWORD incoming=GetLastError();
    __atomic_add_fetch(&acr.global_entered,1,__ATOMIC_RELAXED);
    unsigned active=__atomic_load_n(&acr.active,__ATOMIC_ACQUIRE);
    unsigned thread=GetCurrentThreadId(),match=4;
#ifdef GF2VR_ACTIVITY_UI_FRAME_ELIGIBLE
    unsigned enrolled=UICR_CAPACITY;U64 enrolled_generation=0;
    if(thread==uicr_thread() && uicr.live && camera && (U64)camera<=~0ULL-12) {
        int id=0;U64 read=0;
        if(ReadProcessMemory((HANDLE)(long long)-1,(const unsigned char *)camera+8,&id,4,&read) && read==4)
            enrolled=uicr_native_begin(id,&enrolled_generation);
    }
#endif
#ifdef GF2VR_ACTIVITY_UI_CANVAS_MEMBERSHIP_OBSERVE
    AcmCall canvas_call=acm_register_begin(enrolled,enrolled_generation,canvas,order);
#endif
    if(active && thread==acr.owner) {
        int id=0;U64 read=0;
        /* Do not retain the native argument. Safe bounded identity read only
           while the proved native caller lends it on the selected owner. */
        if(!camera)++acr.null_camera;
        else if((U64)camera>~0ULL-12 ||
           !ReadProcessMemory((HANDLE)(long long)-1,(const unsigned char *)camera+8,&id,4,&read) || read!=4)
            ++acr.read_fail;
        else {
            for(unsigned i=0;i<4;++i)if(id==acr.ids[i]){match=i;break;}
            if(match<4)++acr.entered[match];else ++acr.unmatched;
        }
    }else if(active)__atomic_add_fetch(&acr.foreign,1,__ATOMIC_RELAXED);
    SetLastError(incoming);
    acr.original(canvas,camera,order,force); /* exactly once, identical arguments */
    DWORD outgoing=GetLastError();
    __atomic_add_fetch(&acr.global_returned,1,__ATOMIC_RELAXED);
#ifdef GF2VR_ACTIVITY_UI_CANVAS_MEMBERSHIP_OBSERVE
    acm_register_end(&canvas_call,order);
#endif
    if(match<4)++acr.returned[match];
#ifdef GF2VR_ACTIVITY_UI_FRAME_ELIGIBLE
    if(enrolled<UICR_CAPACITY && uicr.rows[enrolled].root && !uicr.rows[enrolled].retired &&
       uicr.rows[enrolled].generation==enrolled_generation) {
        if(uicr.rows[enrolled].native_returned<~0u)++uicr.rows[enrolled].native_returned;
        if(!uicr.rows[enrolled].native_reported) {
            uicr.rows[enrolled].native_reported=1;
            char line[900],*p=source_prefix(line,"activity_ui_enrolled_native_register");
            p=literal(p,",\"generation\":");p=hexadecimal(p,enrolled_generation);
            p=literal(p,",\"camera\":");p=hexadecimal(p,(unsigned)uicr.rows[enrolled].instance);
            p=literal(p,",\"entered\":");p=decimal(p,uicr.rows[enrolled].native_entered);
            p=literal(p,",\"returned\":");p=decimal(p,uicr.rows[enrolled].native_returned);
            p=literal(p,",\"canvas_lease\":");p=decimal(p,primary_ui_canvas.scope.active &&
                !primary_ui_canvas.scope.failed && primary_ui_generation_id==enrolled_generation);
            p=literal(p,"}\n");source_log(line,p);
        }
    }
#endif
    SetLastError(outgoing);
}
static int acr_hook_target(void *target) {
    static const unsigned char prefix[]={0x48,0x89,0x5c,0x24,0x18,0x48,0x89,0x6c,0x24,0x20,0x57,0x41,0x56,0x41,0x57,0x48,0x81,0xec,0xb0,0,0,0};
    acr.stage=1;if(!stereo_hook_bytes(target,prefix,sizeof(prefix)))return 0;
    int status=MH_Initialize();acr.stage=2;if(status!=0 && status!=1)return 0;
    acr.stage=3;if(MH_CreateHook(target,acr_forward,(void **)&acr.original))return 0;
    acr.stage=4;if(MH_EnableHook(target)){(void)MH_DisableHook(target);return 0;}
    acr.stage=0;return acr.ready=1;
}
static void acr_install(void) {
    if(acr.attempted)return;acr.attempted=1;
    /* Bootstrap already pins itself and UnityPlayer for published adapters.
       Never remove/free a trampoline or enable unrelated MinHook targets. */
    static const unsigned char site[]={0xe8,0x4c,0xf8,0xff,0xff};
    acr.stage=5;
    if(unity && stereo_hook_bytes(unity+0x28b30f,site,sizeof(site)))
        (void)acr_hook_target(unity+0x28b1b0);
    event("activity_ui_native_register_install",((U64)acr.stage<<32)|acr.ready,GetCurrentThreadId());
}
static void acr_report(unsigned stage) {
    char line[1800],*p=source_prefix(line,"activity_ui_native_register");
    p=literal(p,",\"stage\":");p=decimal(p,stage);
    p=literal(p,",\"serial\":");p=hexadecimal(p,acr.serial);
    p=literal(p,",\"generation\":");p=hexadecimal(p,acr.generation);
    p=literal(p,",\"frame\":");p=hexadecimal(p,acr.frame);
    p=literal(p,",\"owner\":");p=decimal(p,acr.owner);
    p=literal(p,",\"member_begin\":");p=decimal(p,acr.member_begin);
    p=literal(p,",\"member_end\":");p=decimal(p,acr.member_end);
    p=literal(p,",\"ids\":[");
    for(unsigned i=0;i<4;++i){if(i)p=literal(p,",");p=hexadecimal(p,(unsigned)acr.ids[i]);}
    p=literal(p,"],\"entered\":[");
    for(unsigned i=0;i<4;++i){if(i)p=literal(p,",");p=decimal(p,acr.entered[i]);}
    p=literal(p,"],\"returned\":[");
    for(unsigned i=0;i<4;++i){if(i)p=literal(p,",");p=decimal(p,acr.returned[i]);}
    p=literal(p,"],\"global_before\":");p=hexadecimal(p,acr.global_begin);
    p=literal(p,",\"global_entered\":");p=hexadecimal(p,__atomic_load_n(&acr.global_entered,__ATOMIC_ACQUIRE));
    p=literal(p,",\"global_returned\":");p=hexadecimal(p,__atomic_load_n(&acr.global_returned,__ATOMIC_ACQUIRE));
    p=literal(p,",\"foreign\":");p=decimal(p,__atomic_load_n(&acr.foreign,__ATOMIC_ACQUIRE));
    p=literal(p,",\"unmatched\":");p=decimal(p,acr.unmatched);
    p=literal(p,",\"read_fail\":");p=decimal(p,acr.read_fail);
    p=literal(p,",\"null_camera\":");p=decimal(p,acr.null_camera);
    p=literal(p,"}\n");source_log(line,p);
}
static void acr_prepare(U64 serial) {
    DWORD saved=GetLastError();acr_install();
    if(!acr.ready || acr.selected || !stereo_live || stereo_two.root_slot>=EFO_CAPACITY ||
       stereo_two.root_serial!=serial || !serial)goto end;
    ActivityEyeGroup *g=&stereo_live->pool.groups[stereo_two.root_slot];
    if(!g->ui_shown || !g->borrowed || !g->ui_eyes.borrowed || !activity_group_source_valid(g) ||
       !primary_ui_current(g->source))goto end;
    U64 now=GetTickCount64();
    if(!acr.first_tick){acr.first_tick=now;goto end;}
    if(now<acr.first_tick || now-acr.first_tick<1500)goto end;
    acr.selected=1;acr.serial=serial;acr.generation=g->generation;acr.frame=g->frame;acr.owner=GetCurrentThreadId();
    acr.ids[0]=g->source->active.entries[0].identity.instance_id;
    acr.ids[1]=g->source->active.primary.entry.identity.instance_id;
    SourcePoseApi read={0};read.calls=g->ui_binding.refs.settings.calls;
    for(unsigned eye=0;eye<2;++eye)
        if(!source_pose_value(&read,stereo_live->topology_api.instance,g->ui_eyes.eyes[eye].camera,
            stereo_live->topology_api.int_class,&acr.ids[eye+2],4) || !activity_group_source_valid(g))goto incomplete;
    for(unsigned i=0;i<4;++i){if(!acr.ids[i])goto incomplete;
        for(unsigned j=0;j<i;++j)if(acr.ids[i]==acr.ids[j])goto incomplete;}
    if(!primary_ui_current(g->source))goto incomplete;
    acr.global_begin=__atomic_load_n(&acr.global_entered,__ATOMIC_ACQUIRE);
    __atomic_store_n(&acr.active,1,__ATOMIC_RELEASE);goto end;
incomplete:acr_report(10);
end:SetLastError(saved);
}
static void acr_notice(U64 serial,int operation,int camera,int result) {
    if(!result || !__atomic_load_n(&acr.active,__ATOMIC_ACQUIRE) || serial!=acr.serial ||
       GetCurrentThreadId()!=acr.owner)return;
    for(unsigned i=0;i<2;++i)if(camera==acr.ids[i+2]) {
        if(operation==5)acr.member_begin|=1u<<i;
        if(operation==6)acr.member_end|=1u<<i;
    }
}
static void acr_finish(U64 serial) {
    if(!__atomic_load_n(&acr.active,__ATOMIC_ACQUIRE) || serial!=acr.serial || GetCurrentThreadId()!=acr.owner)return;
    DWORD saved=GetLastError();__atomic_store_n(&acr.active,0,__ATOMIC_RELEASE);
    unsigned stage=acr.member_begin==3 && acr.member_end==3?0:11;
    for(unsigned i=0;i<4;++i)if(acr.entered[i]!=acr.returned[i])stage=12;
    if(!__atomic_load_n(&acr.global_returned,__ATOMIC_ACQUIRE))stage=13;
    acr_report(stage);SetLastError(saved);
}
#endif
