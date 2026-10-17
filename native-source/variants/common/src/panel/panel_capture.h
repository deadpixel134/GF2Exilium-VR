/* Single-slot XR image handoff. Only game Present issues context commands. */
typedef struct { unsigned a; unsigned short b,c; unsigned char d[8]; } PanelGuid;
typedef struct { unsigned width,height,mips,array,format,samples,quality,usage,bind,cpu,misc; } PanelTextureDesc;
typedef struct { unsigned short description[128]; unsigned vendor,device,subsystem,revision; U64 video,system,shared; unsigned low; int high; } PanelAdapterDesc;
_Static_assert(sizeof(PanelTextureDesc)==44 && sizeof(PanelAdapterDesc)==304,"D3D11 ABI");
static const PanelGuid panel_texture_iid={0x6f15aaf2,0xd208,0x4e89,{0x9a,0xb4,0x48,0x95,0x35,0xd3,0x4f,0x9c}};
static const PanelGuid panel_device_iid={0xdb6f6ddb,0xac77,0x4e88,{0x82,0x53,0x81,0x9d,0xf9,0xbb,0xf1,0x40}};
static const PanelGuid panel_mt_iid={0x9b7e4e00,0x342c,0x4106,{0xa1,0x9f,0x4f,0x27,0x04,0xf6,0x89,0xf0}};
static const PanelGuid panel_dxgi_iid={0x54ec77fa,0x1377,0x44e6,{0x8c,0x32,0x88,0xfd,0x5f,0x44,0xc8,0x4c}};
static void **panel_vt(void *p) { return *(void ***)p; }
static void panel_release(void *p) { if(p)((unsigned (*)(void *))panel_vt(p)[2])(p); }
static long panel_qi(void *p,const PanelGuid *id,void **out) { return ((long (*)(void *,const PanelGuid *,void **))panel_vt(p)[0])(p,id,out); }
#include "panel_device_identity.h"
static unsigned panel_format(unsigned source) { return source==28||source==29 ? 29 : source==87||source==91 ? 91 : 0; }
static BOOL panel_destination_format(unsigned actual,unsigned requested) {
    return requested==29 ? (actual==27 || actual==28 || actual==29) :
           requested==91 && (actual==87 || actual==90 || actual==91);
}
/* IDLE -> REQUEST -> COPYING -> INFLIGHT -> DONE; SKIPPED has no GPU work.
   FAULT is quarantined: no XR image destruction/reuse while completion unknown. */
enum { PC_IDLE,PC_REQUEST,PC_COPYING,PC_INFLIGHT,PC_DONE,PC_SKIPPED,PC_FAULT };
static unsigned panel_copy_state,panel_device_ready,panel_device_changed;
static void *panel_device,*panel_context,*panel_query;
static void *panel_destination;
static unsigned panel_request_width,panel_request_height,panel_request_format;
static unsigned panel_source_seq,panel_width,panel_height,panel_source_format;
static U64 panel_copy_tick,panel_source_tick;
static DWORD panel_render_tid;
static void *panel_selected_chain;
typedef long (*PanelPresent)(void *,unsigned,unsigned);
static PanelPresent panel_original;
static void **panel_hook_slot;
static unsigned panel_present_busy;
static U64 panel_completed_copies;
static unsigned panel_present_seen,panel_install_rejection;
#ifdef GF2VR_PERF_BOUNDARY
/* Render-owner-only aggregate: distinguish PC Present cadence from query
   completion timing without changing copy/query ownership or XR waiting. */
static U64 panel_perf_start,panel_perf_presents,panel_perf_present_ms;
static U64 panel_perf_pre_inflight,panel_perf_pre_done;
static U64 panel_perf_post_inflight,panel_perf_post_done,panel_perf_post_pending;
static unsigned panel_perf_reports;
static void panel_perf_boundary_record(U64 started,unsigned before,unsigned after_pre,
                                       unsigned before_post,unsigned after_post) {
    if(panel_perf_reports>=40)return;
    U64 now=GetTickCount64();
    if(!panel_perf_start)panel_perf_start=started;
    ++panel_perf_presents;
    panel_perf_present_ms+=now-started;
    panel_perf_pre_inflight+=before==PC_INFLIGHT;
    panel_perf_pre_done+=before==PC_INFLIGHT&&after_pre==PC_DONE;
    panel_perf_post_inflight+=before_post==PC_INFLIGHT;
    panel_perf_post_done+=before_post==PC_INFLIGHT&&after_post==PC_DONE;
    panel_perf_post_pending+=before_post==PC_INFLIGHT&&after_post==PC_INFLIGHT;
    if(now-panel_perf_start<2000)return;
    event("perf_pc_present_cadence",(panel_perf_presents<<32)|(now-panel_perf_start),GetCurrentThreadId());
    event("perf_pc_pre_query",(panel_perf_pre_inflight<<32)|panel_perf_pre_done,GetCurrentThreadId());
    event("perf_pc_post_query",(panel_perf_post_inflight<<32)|panel_perf_post_done,GetCurrentThreadId());
    event("perf_pc_present_pending",(panel_perf_present_ms<<32)|panel_perf_post_pending,GetCurrentThreadId());
    ++panel_perf_reports;
    panel_perf_start=now;
    panel_perf_presents=panel_perf_present_ms=0;
    panel_perf_pre_inflight=panel_perf_pre_done=0;
    panel_perf_post_inflight=panel_perf_post_done=panel_perf_post_pending=0;
}
#endif
__declspec(dllimport) BOOL WINAPI VirtualProtect(void *,U64,DWORD,DWORD *);
static BOOL panel_install_fail(unsigned reason) {
    if(panel_install_rejection!=reason) { panel_install_rejection=reason; event("panel_install_rejected",reason,GetCurrentThreadId()); }
    return 0;
}

static void panel_dimensions(unsigned *w,unsigned *h,unsigned *format) {
    unsigned a,b;
    do {
        a=__atomic_load_n(&panel_source_seq,__ATOMIC_ACQUIRE);
        *w=__atomic_load_n(&panel_width,__ATOMIC_RELAXED);
        *h=__atomic_load_n(&panel_height,__ATOMIC_RELAXED);
        *format=__atomic_load_n(&panel_source_format,__ATOMIC_RELAXED);
        b=__atomic_load_n(&panel_source_seq,__ATOMIC_ACQUIRE);
    } while((a&1) || a!=b);
}
#ifdef GF2VR_CONTROLLER_INPUT
#include "../stereo/controller_cursor_gpu.h"
#endif
#ifdef GF2VR_AUX_OBSERVE
#include "panel_observe_capture.h"
#endif
static void panel_copy_poll(void) {
    unsigned state=__atomic_load_n(&panel_copy_state,__ATOMIC_ACQUIRE);
    if(state==PC_INFLIGHT) {
        BOOL done=0;
        long hr=((long (*)(void *,void *,void *,unsigned,unsigned))panel_vt(panel_context)[29])(panel_context,panel_query,&done,sizeof(done),1);
        if(hr==0 && done) {
            panel_release(panel_query); panel_query=NULL;
#ifdef GF2VR_AUX_OBSERVE
            panel_observe_done();
#endif
            ++panel_completed_copies;
            if(panel_completed_copies<=3 || !(panel_completed_copies&(panel_completed_copies-1))) event("panel_copy_complete",panel_completed_copies,GetCurrentThreadId());
            __atomic_store_n(&panel_copy_state,PC_DONE,__ATOMIC_RELEASE);
        } else if(hr<0) {
            event("copy_gpu_fault",(unsigned)hr,GetCurrentThreadId());
            __atomic_store_n(&panel_copy_state,PC_FAULT,__ATOMIC_RELEASE);
        }
    }
}
static void panel_capture(void *chain) {
    panel_copy_poll();
    void *buffer=NULL,*device=NULL;
    long hr=((long (*)(void *,unsigned,const PanelGuid *,void **))panel_vt(chain)[9])(chain,0,&panel_texture_iid,&buffer);
    if(hr<0 || !buffer) return;
    PanelTextureDesc desc; bs_zero(&desc,sizeof(desc));
    ((void (*)(void *,PanelTextureDesc *))panel_vt(buffer)[10])(buffer,&desc);
    ((void (*)(void *,void **))panel_vt(buffer)[3])(buffer,&device);
    if(!panel_same_device(device,panel_device)) {
        __atomic_store_n(&panel_device_changed,1,__ATOMIC_RELEASE);
        panel_release(device); panel_release(buffer); return;
    }
    panel_release(device);
    BOOL extent_changed=desc.width!=__atomic_load_n(&panel_width,__ATOMIC_RELAXED) || desc.height!=__atomic_load_n(&panel_height,__ATOMIC_RELAXED);
    __atomic_add_fetch(&panel_source_seq,1,__ATOMIC_ACQ_REL);
    __atomic_store_n(&panel_width,desc.width,__ATOMIC_RELAXED);
    __atomic_store_n(&panel_height,desc.height,__ATOMIC_RELAXED);
    __atomic_store_n(&panel_source_format,desc.format,__ATOMIC_RELAXED);
    __atomic_store_n(&panel_source_tick,GetTickCount64(),__ATOMIC_RELAXED);
    __atomic_add_fetch(&panel_source_seq,1,__ATOMIC_RELEASE);
    if(extent_changed) {
        event("panel_source_extent",((U64)desc.width<<32)|desc.height,GetCurrentThreadId());
        event("panel_source_format",((U64)desc.format<<32)|desc.samples,GetCurrentThreadId());
    }
    unsigned expected=PC_REQUEST;
    if(__atomic_compare_exchange_n(&panel_copy_state,&expected,PC_COPYING,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE)) {
        PanelTextureDesc destination; bs_zero(&destination,sizeof(destination));
        ((void (*)(void *,PanelTextureDesc *))panel_vt(panel_destination)[10])(panel_destination,&destination);
        void *destination_device=NULL;
        ((void (*)(void *,void **))panel_vt(panel_destination)[3])(panel_destination,&destination_device);
        BOOL device_match=panel_same_device(destination_device,panel_device); panel_release(destination_device);
        if(desc.width!=panel_request_width || desc.height!=panel_request_height ||
           panel_format(desc.format)!=panel_request_format || desc.samples!=1 || desc.mips!=1 || desc.array!=1 ||
           destination.width!=desc.width || destination.height!=desc.height || destination.samples!=1 ||
           destination.mips!=1 || destination.array!=1 || !panel_destination_format(destination.format,panel_request_format) || !device_match) {
            __atomic_store_n(&panel_copy_state,PC_SKIPPED,__ATOMIC_RELEASE);
        } else {
            unsigned query_desc[2]={0,0};
            hr=((long (*)(void *,const void *,void **))panel_vt(panel_device)[24])(panel_device,query_desc,&panel_query);
            if(hr<0 || !panel_query) {
                __atomic_store_n(&panel_copy_state,PC_SKIPPED,__ATOMIC_RELEASE);
            } else {
                ((void (*)(void *,void *,void *))panel_vt(panel_context)[47])(panel_context,panel_destination,buffer);
#ifdef GF2VR_AUX_OBSERVE
                panel_observe_copy(panel_destination);
#endif
                ((void (*)(void *,void *))panel_vt(panel_context)[28])(panel_context,panel_query);
                panel_copy_tick=GetTickCount64();
                __atomic_store_n(&panel_copy_state,PC_INFLIGHT,__ATOMIC_RELEASE);
            }
        }
    }
    /* Never hold a game backbuffer across original Present/ResizeBuffers. */
    panel_release(buffer);
}
static long panel_present(void *chain,unsigned interval,unsigned flags) {
    DWORD saved=GetLastError(); unsigned zero=0,owned=0;
#ifdef GF2VR_PERF_BOUNDARY
    U64 perf_started=0;
    unsigned perf_before=PC_IDLE,perf_after_pre=PC_IDLE,perf_before_post=PC_IDLE,perf_after_post=PC_IDLE;
#endif
    if(chain==__atomic_load_n(&panel_selected_chain,__ATOMIC_ACQUIRE) &&
       !__atomic_exchange_n(&panel_present_seen,1,__ATOMIC_ACQ_REL))
        event("panel_present_enter",((U64)panel_render_tid<<32)|GetCurrentThreadId(),GetCurrentThreadId());
    if(chain==__atomic_load_n(&panel_selected_chain,__ATOMIC_ACQUIRE) &&
       GetCurrentThreadId()==panel_render_tid && !(flags&1) &&
       !__atomic_load_n(&panel_device_changed,__ATOMIC_ACQUIRE) &&
       __atomic_compare_exchange_n(&panel_present_busy,&zero,1,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE)) {
        owned=1;
#ifdef GF2VR_COPY_PRESENT_OBSERVE
        copy_present_observe_enter(panel_device,panel_context);
#endif
#ifdef GF2VR_PERF_BOUNDARY
        perf_started=GetTickCount64();
        perf_before=__atomic_load_n(&panel_copy_state,__ATOMIC_ACQUIRE);
#endif
#ifdef GF2VR_RENDER_OWNER_TICK
        /* Publish any completed eye lease before panel completion wakes the
           shared XR worker for this prediction's EndFrame. */
        GF2VR_RENDER_OWNER_TICK();
#endif
        panel_capture(chain);
#ifdef GF2VR_PERF_BOUNDARY
        perf_after_pre=__atomic_load_n(&panel_copy_state,__ATOMIC_ACQUIRE);
#endif
#ifdef GF2VR_CONTROLLER_INPUT
        controller_cursor_render(chain);
#endif
#ifndef GF2VR_PERF_POLL
        __atomic_store_n(&panel_present_busy,0,__ATOMIC_RELEASE);
#endif
    }
    SetLastError(saved);
#ifdef GF2VR_COPY_PRESENT_OBSERVE
    if(owned)copy_present_observe_mark(CPO_ORIGINAL_ENTER,0);
#endif
    long result=__atomic_load_n(&panel_original,__ATOMIC_ACQUIRE)(chain,interval,flags);
#ifdef GF2VR_PERF_POLL
    DWORD returned_error=GetLastError();
#ifdef GF2VR_COPY_PRESENT_OBSERVE
    if(owned)copy_present_observe_mark(CPO_ORIGINAL_RETURN,(unsigned long)result);
#endif
    if(owned) {
        /* Original Present has submitted preceding copy work. Poll once on
           the same context owner; pending remains owned for the next Present.
           Busy stays held through original/reentrant Present and this tail. */
        if(result>=0&&panel_context&&chain==__atomic_load_n(&panel_selected_chain,__ATOMIC_ACQUIRE)&&
           GetCurrentThreadId()==panel_render_tid&&
           !__atomic_load_n(&panel_device_changed,__ATOMIC_ACQUIRE)) {
#ifdef GF2VR_RENDER_OWNER_TAIL
            GF2VR_RENDER_OWNER_TAIL();
#elif defined(GF2VR_RENDER_OWNER_TICK)
            GF2VR_RENDER_OWNER_TICK(); /* eyes must publish before panel wake */
#endif
#ifdef GF2VR_PERF_BOUNDARY
            perf_before_post=__atomic_load_n(&panel_copy_state,__ATOMIC_ACQUIRE);
#endif
            panel_copy_poll();
#ifdef GF2VR_PERF_BOUNDARY
            perf_after_post=__atomic_load_n(&panel_copy_state,__ATOMIC_ACQUIRE);
#endif
        }
#ifdef GF2VR_PERF_BOUNDARY
        panel_perf_boundary_record(perf_started,perf_before,perf_after_pre,perf_before_post,perf_after_post);
#endif
#ifdef GF2VR_COPY_PRESENT_OBSERVE
        copy_present_observe_mark(CPO_POST_RETURN,0);
#endif
        __atomic_store_n(&panel_present_busy,0,__ATOMIC_RELEASE);
    }
    SetLastError(returned_error);
#else
    (void)owned;
#endif
    return result;
}
static BOOL panel_install(void *chain,void *device) {
    if(__atomic_load_n(&panel_device_ready,__ATOMIC_ACQUIRE)) {
        if(!panel_same_device(device,panel_device) || &panel_vt(chain)[8]!=panel_hook_slot) {
            __atomic_store_n(&panel_device_changed,1,__ATOMIC_RELEASE); return panel_install_fail(8);
        }
        __atomic_store_n(&panel_selected_chain,chain,__ATOMIC_RELEASE); return 1;
    }
    /* Unity may retain 3DMigoto's device wrapper while the swapchain owns the
       native device. Keep game rendering on that wrapper, but give XR and our
       private copy work the native device/context. Use the default wrapper-only
       mode: native objects with function hooks still dispatch through 3DMigoto. */
    void *native_device=NULL;
    long hr=((long (*)(void *,const PanelGuid *,void **))panel_vt(chain)[7])(chain,&panel_device_iid,&native_device);
    if(hr<0 || !native_device) {
        panel_release(native_device);native_device=NULL;
        void *buffer=NULL;
        hr=((long (*)(void *,unsigned,const PanelGuid *,void **))panel_vt(chain)[9])(chain,0,&panel_texture_iid,&buffer);
        if(hr>=0 && buffer)
            ((void (*)(void *,void **))panel_vt(buffer)[3])(buffer,&native_device);
        panel_release(buffer);
    }
    if(!native_device) return panel_install_fail(9);
    if(!panel_same_device(native_device,device)) {panel_release(native_device);return panel_install_fail(10);}
    if(panel_has_migoto_interface(native_device,&panel_migoto_device_iid)) {
        event("migoto_native_device_requires_hook_0",0,GetCurrentThreadId());
        panel_release(native_device);return panel_install_fail(11);
    }
    if(((unsigned (*)(void *))panel_vt(native_device)[38])(native_device)&1) {panel_release(native_device);return panel_install_fail(1);}
    void *context=NULL,*mt=NULL;
    ((void (*)(void *,void **))panel_vt(native_device)[40])(native_device,&context);
    if(context && panel_has_migoto_interface(context,&panel_migoto_context_iid)) {
        event("migoto_native_context_requires_hook_0",0,GetCurrentThreadId());
        panel_release(context);panel_release(native_device);return panel_install_fail(12);
    }
    if(!context || panel_qi(context,&panel_mt_iid,&mt)<0 || !mt) {panel_release(mt);panel_release(context);panel_release(native_device);return panel_install_fail(2);}
    ((BOOL (*)(void *,BOOL))panel_vt(mt)[5])(mt,TRUE);
    BOOL protected=((BOOL (*)(void *))panel_vt(mt)[6])(mt); panel_release(mt);
    if(!protected) {panel_release(context);panel_release(native_device);return panel_install_fail(3);}
    void **slot=&panel_vt(chain)[8]; DWORD protection,ignored;
    if(!VirtualProtect(slot,sizeof(void *),4,&protection)) {panel_release(context);panel_release(native_device);return panel_install_fail(4);}
    PanelPresent prior=(PanelPresent)__atomic_load_n(slot,__ATOMIC_ACQUIRE);
    if(prior==panel_present) {VirtualProtect(slot,sizeof(void *),protection,&ignored);panel_release(context);panel_release(native_device);return 0;}
    panel_device=native_device; panel_context=context; panel_render_tid=GetCurrentThreadId();
    panel_hook_slot=slot;
    __atomic_store_n(&panel_selected_chain,chain,__ATOMIC_RELEASE);
    __atomic_store_n(&panel_original,prior,__ATOMIC_RELEASE);
    void *expected=(void *)prior;
    BOOL installed=__atomic_compare_exchange_n(slot,&expected,(void *)panel_present,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE);
    BOOL restored=VirtualProtect(slot,sizeof(void *),protection,&ignored);
    if(!installed) {panel_release(context);panel_release(native_device);panel_device=NULL;panel_context=NULL;return panel_install_fail(5);}
    __atomic_store_n(&panel_device_ready,1,__ATOMIC_RELEASE);
    event("native_graphics_isolated",native_device!=device,GetCurrentThreadId());
    event(restored ? "present_hook_installed" : "present_hook_protection_restore_failed",0,GetCurrentThreadId());
    return 1;
}
static void panel_render_setup(int id) {
    if(id!=0x504e4c) return;
    DWORD saved=GetLastError();
    typedef void *(*Lookup)(U64,U64);
    Lookup lookup=*(Lookup *)(unity+0x17e95a8);
    void **d3d=(void **)lookup(0xAAB37EF87A87D748ULL,0xBF76967F07EFB177ULL);
    void *device=d3d && d3d[0] ? ((void *(*)(void))d3d[0])() : NULL;
    const unsigned char getter[8]={0x48,0x8b,0x05,0xb1,0xff,0x38,0x01,0xc3};
    BOOL valid=TRUE;
    for(unsigned i=0;i<8;++i) if(unity[0x45a6a0+i]!=getter[i]) valid=0;
    unsigned char *owner=valid ? *(unsigned char **)(unity+0x17ea658) : NULL;
    void *chain=owner ? *(void **)(owner+0x3e0) : NULL;
    if(device && chain) panel_install(chain,device);
    else panel_install_fail(valid ? 6 : 7);
    SetLastError(saved);
}
static void panel_main_tick(U64 n) {
    if(n!=2 && n%120) return;
    if(!unity || GetCurrentThreadId()!=__atomic_load_n(&first_tid,__ATOMIC_ACQUIRE)) return;
    typedef void *(*Lookup)(U64,U64);
    Lookup lookup=*(Lookup *)(unity+0x17e95a8);
    if((void *)lookup!=unity+0x424560 || unity[0x94d840]!=0xe9 || *(unsigned *)(unity+0x94d841)!=0xffd2cc4bU) return;
    void **graphics=(void **)lookup(0x7CBA0A9CA4DDB544ULL,0x8C5AD4926EB17B11ULL);
    if(!graphics || graphics[0]!=unity+0x424580 || ((int (*)(void))graphics[0])()!=2) return;
    ((void (*)(void (*)(int),int))(unity+0x94d840))(panel_render_setup,0x504e4c);
}
