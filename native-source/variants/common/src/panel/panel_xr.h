typedef struct ID3D11Device ID3D11Device;
typedef struct ID3D11Texture2D ID3D11Texture2D;
typedef struct { unsigned LowPart; long HighPart; } LUID;
typedef unsigned D3D_FEATURE_LEVEL;
#define XR_USE_GRAPHICS_API_D3D11
#include <openxr/openxr_platform.h>
#ifndef GF2VR_PACING_RECORD
#define GF2VR_PACING_RECORD(start,period,wait,copy,end,render) ((void)0)
#endif
#ifndef GF2VR_OUTPUT_SESSION_STATE
#define GF2VR_OUTPUT_SESSION_STATE(state) ((void)0)
#endif

/* Optional stereo consumer; the ordinary panel has no extra XR dependency. */
#ifndef GF2VR_EYE_OPEN
#define GF2VR_EYE_OPEN(instance,session,module) ((void)0)
#define GF2VR_EYE_FRAME(number,frame) ((void)0)
#define GF2VR_EYE_CHANGE(change) ((void)0)
#define GF2VR_EYE_INVALIDATE() ((void)0)
#define GF2VR_EYE_CLOSE() 1
#endif
#ifndef GF2VR_OUTPUT_FRAME
#define GF2VR_OUTPUT_FRAME(panel,number) ((void)0)
#define GF2VR_OUTPUT_INVALIDATE(panel) ((void)0)
#define GF2VR_OUTPUT_CLOSE(panel) 1
#endif
#ifndef GF2VR_OUTPUT_END_FRAME
#define GF2VR_OUTPUT_END_FRAME(panel,end,visible) ((panel)->api.EndFrame((panel)->session,end))
#endif
#ifndef GF2VR_OUTPUT_STOP_SESSION
#define GF2VR_OUTPUT_STOP_SESSION(panel) GF2VR_OUTPUT_CLOSE(panel)
#define GF2VR_OUTPUT_SESSION_STOPPED(panel) 1
#endif
#ifndef GF2VR_FRAME_ABORT_REQUIRED
#define GF2VR_FRAME_ABORT_REQUIRED(panel) 0
#endif
#ifndef GF2VR_FRAME_ABORT_INPUT
#define GF2VR_FRAME_ABORT_INPUT() ((void)0)
#endif
#ifndef GF2VR_OUTPUT_OPEN
#define GF2VR_OUTPUT_OPEN(state,module) ((void)0)
#endif
#ifndef GF2VR_PANEL_WANTED
#define GF2VR_PANEL_WANTED(panel,frame) 1
#define GF2VR_PANEL_CLOSE() ((void)0)
#endif

#define PANEL_XR_CORE(F) \
 F(CreateSession) F(DestroySession) F(BeginSession) F(EndSession) F(RequestExitSession) \
 F(CreateReferenceSpace) F(DestroySpace) F(EnumerateSwapchainFormats) F(CreateSwapchain) \
 F(DestroySwapchain) F(EnumerateSwapchainImages) F(AcquireSwapchainImage) \
 F(WaitSwapchainImage) F(ReleaseSwapchainImage) F(WaitFrame) F(BeginFrame) F(EndFrame) \
 F(EnumerateEnvironmentBlendModes)
typedef struct {
#define DECLARE_FN(name) PFN_xr##name name;
    PANEL_XR_CORE(DECLARE_FN)
#undef DECLARE_FN
    PFN_xrGetD3D11GraphicsRequirementsKHR requirements;
} PanelXrApi;
#ifdef BOUNDARY_TEST
static PanelXrApi panel_test_api;
static BOOL panel_test_api_enabled;
#endif
typedef struct {
    PanelXrApi api;
    XrSession session;
    XrSpace space;
    XrSwapchain swapchain;
    XrSwapchainImageD3D11KHR images[16];
    unsigned image_count,width,height,format,index,released_index;
    BOOL running,acquired,waited,have_image;
    U64 latest_tick,frames;
} PanelXr;
static BOOL panel_xr_check(XrResult result,const char *stage) {
    if(XR_SUCCEEDED(result)) return TRUE;
    event(stage,(unsigned)result,GetCurrentThreadId()); return 0;
}
/* Optional sole frame owner. Defaults preserve the existing panel path. */
#ifndef GF2VR_FRAME_WAIT
#define GF2VR_FRAME_WAIT(panel,wait,frame) ((panel)->api.WaitFrame((panel)->session,wait,frame))
#endif
static XrResult panel_frame_begin_legacy(PanelXr *p,const XrFrameState *frame,int exiting) {
    XrFrameBeginInfo begin={.type=XR_TYPE_FRAME_BEGIN_INFO};
    XrResult result=p->api.BeginFrame(p->session,&begin);
    if(XR_SUCCEEDED(result)) {
        if(!exiting) GF2VR_EYE_FRAME(p->frames+1,frame);
        else GF2VR_EYE_INVALIDATE();
    }
    return result;
}
#ifndef GF2VR_FRAME_BEGIN
#define GF2VR_FRAME_BEGIN(panel,frame,exiting) panel_frame_begin_legacy(panel,frame,exiting)
#endif
static BOOL panel_xr_device(PanelXrApi *api,BootstrapState *state) {
    XrGraphicsRequirementsD3D11KHR req; bs_zero(&req,sizeof(req)); req.type=XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR;
    if(!panel_xr_check(api->requirements(state->instance,state->system,&req),"xr_graphics_requirements_failed")) return 0;
    void *dxgi=NULL,*adapter=NULL; PanelAdapterDesc desc; bs_zero(&desc,sizeof(desc));
    long hr=panel_qi(panel_device,&panel_dxgi_iid,&dxgi);
    if(hr>=0 && dxgi) hr=((long (*)(void *,void **))panel_vt(dxgi)[7])(dxgi,&adapter);
    if(hr>=0 && adapter) hr=((long (*)(void *,PanelAdapterDesc *))panel_vt(adapter)[8])(adapter,&desc);
    unsigned level=((unsigned (*)(void *))panel_vt(panel_device)[37])(panel_device);
    BOOL match=hr>=0 && adapter && desc.low==req.adapterLuid.LowPart && desc.high==req.adapterLuid.HighPart && level>=req.minFeatureLevel;
    panel_release(adapter); panel_release(dxgi);
    event(match ? "xr_adapter_match" : "xr_adapter_mismatch",((U64)(unsigned)req.adapterLuid.HighPart<<32)|req.adapterLuid.LowPart,GetCurrentThreadId());
    return match;
}
static BOOL panel_swapchain_create(PanelXr *p,unsigned w,unsigned h,unsigned format) {
    int64_t formats[64]; uint32_t count=0;
    if(!panel_xr_check(p->api.EnumerateSwapchainFormats(p->session,0,&count,NULL),"xr_formats_failed") || count>64) return 0;
    if(!panel_xr_check(p->api.EnumerateSwapchainFormats(p->session,64,&count,formats),"xr_formats_failed")) return 0;
    BOOL found=0;
    for(unsigned i=0;i<count;++i) if(formats[i]==format) found=1;
    if(!found) { event("xr_srgb_format_unavailable",format,GetCurrentThreadId()); return 0; }
    XrSwapchainCreateInfo info; bs_zero(&info,sizeof(info)); info.type=XR_TYPE_SWAPCHAIN_CREATE_INFO;
    info.usageFlags=XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT|XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
    info.format=format; info.sampleCount=1; info.width=w; info.height=h; info.faceCount=1; info.arraySize=1; info.mipCount=1;
    if(!panel_xr_check(p->api.CreateSwapchain(p->session,&info,&p->swapchain),"xr_swapchain_create_failed")) return 0;
    if(!panel_xr_check(p->api.EnumerateSwapchainImages(p->swapchain,0,&count,NULL),"xr_images_failed") || count>16 || !count) return 0;
    bs_zero(p->images,sizeof(p->images));
    for(unsigned i=0;i<count;++i) p->images[i].type=XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR;
    if(!panel_xr_check(p->api.EnumerateSwapchainImages(p->swapchain,count,&count,(XrSwapchainImageBaseHeader *)p->images),"xr_images_failed")) return 0;
    p->image_count=count; p->width=w; p->height=h; p->format=format; p->have_image=0;
    event("panel_extent",((U64)w<<32)|h,GetCurrentThreadId()); return TRUE;
}
static BOOL panel_image_release(PanelXr *p) {
    XrSwapchainImageReleaseInfo info; bs_zero(&info,sizeof(info)); info.type=XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO;
    BOOL ok=panel_xr_check(p->api.ReleaseSwapchainImage(p->swapchain,&info),"xr_image_release_failed");
    /* A failed release is fatal to this session; cleanup must not repeat it. */
    p->acquired=0; p->waited=0;
    return ok;
}
#ifdef GF2VR_PERF_ASYNC_PANEL
/* xrEndFrame uses the most recently released image. Never offer that image
   while a new copy has reacquired the same index for writing. */
static BOOL panel_released_image_usable(const PanelXr *p) {
    return p->have_image && !__atomic_load_n(&panel_device_changed,__ATOMIC_ACQUIRE) &&
           __atomic_load_n(&panel_copy_state,__ATOMIC_ACQUIRE)!=PC_FAULT &&
           (!p->acquired || p->index!=p->released_index);
}
#endif
static void panel_drain(PanelXr *p) {
    /* Never destroy a destination while the render thread/GPU can still use it.
       Quarantine on opaque GPU failure; no game-thread join and no timeout-free. */
    unsigned expected=PC_REQUEST;
    __atomic_compare_exchange_n(&panel_copy_state,&expected,PC_SKIPPED,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE);
    for(;;) {
        unsigned state=__atomic_load_n(&panel_copy_state,__ATOMIC_ACQUIRE);
        if(state==PC_IDLE || state==PC_DONE || state==PC_SKIPPED) break;
        Sleep(10);
    }
    if(p->acquired && p->waited) panel_image_release(p);
    __atomic_store_n(&panel_copy_state,PC_IDLE,__ATOMIC_RELEASE);
}
static int panel_end_session(PanelXr *p) {
    if(!GF2VR_OUTPUT_STOP_SESSION(p))return 0;
    panel_drain(p);p->have_image=0;
    XrResult result=p->api.EndSession(p->session);p->running=0;
    if(!panel_xr_check(result,"xr_end_session_failed"))return 0;
    return GF2VR_OUTPUT_SESSION_STOPPED(p);
}
/* A selected frame driver with a hard API error cannot balance by inventing
   another Begin/End. Stop the loop, request exit, and wait for actual STOPPING.
   This is not a timer-based permission to destroy a session or its loader. */
static int panel_stop_failed_loop(PanelXr *p,BootstrapState *state) {
    if(!p->running || !GF2VR_FRAME_ABORT_REQUIRED(p))return 1;
    GF2VR_FRAME_ABORT_INPUT();
    if(!panel_xr_check(p->api.RequestExitSession(p->session),"xr_failed_loop_exit_failed"))return 0;
    for(;;) {
        XrEventDataBuffer *ev=&state->event_buffer;
        bs_zero(ev,sizeof(*ev));ev->type=XR_TYPE_EVENT_DATA_BUFFER;
        XrResult result=state->api.poll(state->instance,ev);
        if(result==XR_EVENT_UNAVAILABLE){Sleep(10);continue;}
        if(result!=XR_SUCCESS || ev->type==XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING)return 0;
        if(ev->type==XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING)
            GF2VR_EYE_CHANGE((const XrEventDataReferenceSpaceChangePending *)ev);
        if(ev->type!=XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED)continue;
        XrEventDataSessionStateChanged *change=(XrEventDataSessionStateChanged *)ev;
        if(change->session!=p->session)continue;
        GF2VR_OUTPUT_SESSION_STATE(change->state);
        if(change->state==XR_SESSION_STATE_STOPPING){GF2VR_EYE_INVALIDATE();return panel_end_session(p);}
        if(change->state==XR_SESSION_STATE_EXITING || change->state==XR_SESSION_STATE_LOSS_PENDING)return 0;
    }
}
static void panel_ready_run(void *opaque,void *module) {
    static unsigned disabled;
    static PanelXr panel;
    if(disabled || !__atomic_load_n(&panel_device_ready,__ATOMIC_ACQUIRE) || __atomic_load_n(&panel_device_changed,__ATOMIC_ACQUIRE)) return;
    unsigned w,h,source_format; panel_dimensions(&w,&h,&source_format);
    unsigned format=panel_format(source_format);
    if(!w || !h || !format) return;
    BootstrapState *state=(BootstrapState *)opaque; PanelXr *p=&panel; bs_zero(p,sizeof(*p));
#ifdef BOUNDARY_TEST
    if(panel_test_api_enabled) {
        for(unsigned i=0;i<sizeof(p->api);++i) ((volatile unsigned char *)&p->api)[i]=((unsigned char *)&panel_test_api)[i];
    }
    else
#endif
    {
#define LOAD_FN(name) p->api.name=(PFN_xr##name)GetProcAddress(module,"xr" #name); if(!p->api.name) { event("xr_panel_api_missing",0,GetCurrentThreadId()); disabled=1; return; }
    PANEL_XR_CORE(LOAD_FN)
#undef LOAD_FN
    PFN_xrGetInstanceProcAddr get=(PFN_xrGetInstanceProcAddr)GetProcAddress(module,"xrGetInstanceProcAddr");
    if(!get || get(state->instance,"xrGetD3D11GraphicsRequirementsKHR",(PFN_xrVoidFunction *)&p->api.requirements)!=XR_SUCCESS || !p->api.requirements) { disabled=1; return; }
    }
    if(!panel_xr_device(&p->api,state)) { disabled=1; return; }
    XrEnvironmentBlendMode blends[8]; uint32_t count=0; BOOL opaque_blend=0;
    if(p->api.EnumerateEnvironmentBlendModes(state->instance,state->system,XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,8,&count,blends)!=XR_SUCCESS) { disabled=1; return; }
    for(unsigned i=0;i<count;++i) if(blends[i]==XR_ENVIRONMENT_BLEND_MODE_OPAQUE) opaque_blend=1;
    if(!opaque_blend) { disabled=1; event("xr_opaque_blend_unavailable",0,GetCurrentThreadId()); return; }
    XrGraphicsBindingD3D11KHR binding; bs_zero(&binding,sizeof(binding));
    binding.type=XR_TYPE_GRAPHICS_BINDING_D3D11_KHR; binding.device=(ID3D11Device *)panel_device;
    XrSessionCreateInfo create; bs_zero(&create,sizeof(create)); create.type=XR_TYPE_SESSION_CREATE_INFO; create.next=&binding; create.systemId=state->system;
    if(!panel_xr_check(p->api.CreateSession(state->instance,&create,&p->session),"xr_session_create_failed")) { disabled=1; return; }
    GF2VR_EYE_OPEN(state->instance,p->session,module);
    GF2VR_OUTPUT_OPEN(state,module);
    XrReferenceSpaceCreateInfo space; bs_zero(&space,sizeof(space)); space.type=XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
    space.referenceSpaceType=XR_REFERENCE_SPACE_TYPE_VIEW; space.poseInReferenceSpace.orientation.w=1.0f;
    BOOL good=panel_xr_check(p->api.CreateReferenceSpace(p->session,&space,&p->space),"xr_view_space_failed");
    BOOL exit_requested=0;
    while(good) {
        XrEventDataBuffer *ev=&state->event_buffer;
        for(unsigned i=0;i<32;++i) {
            bs_zero(ev,sizeof(*ev)); ev->type=XR_TYPE_EVENT_DATA_BUFFER;
            XrResult result=state->api.poll(state->instance,ev);
            if(result==XR_EVENT_UNAVAILABLE) break;
            if(result!=XR_SUCCESS || ev->type==XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING) { good=0; break; }
            if(ev->type==XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING)
                GF2VR_EYE_CHANGE((const XrEventDataReferenceSpaceChangePending *)ev);
            if(ev->type==XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
                XrEventDataSessionStateChanged *change=(XrEventDataSessionStateChanged *)ev;
                if(change->session!=p->session) continue;
                event("xr_session_state",change->state,GetCurrentThreadId());
                GF2VR_OUTPUT_SESSION_STATE(change->state);
                if(change->state==XR_SESSION_STATE_READY) {
                    XrSessionBeginInfo begin; bs_zero(&begin,sizeof(begin)); begin.type=XR_TYPE_SESSION_BEGIN_INFO; begin.primaryViewConfigurationType=XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
                    good=panel_xr_check(p->api.BeginSession(p->session,&begin),"xr_begin_session_failed"); p->running=good;
                } else if(change->state==XR_SESSION_STATE_STOPPING) {
                    GF2VR_EYE_INVALIDATE();
                    good=panel_end_session(p);
                    if(exit_requested) good=0;
                } else if(change->state==XR_SESSION_STATE_EXITING || change->state==XR_SESSION_STATE_LOSS_PENDING) { good=0; }
            }
        }
        if(!good || __atomic_load_n(&panel_device_changed,__ATOMIC_ACQUIRE)) break;
        if(__atomic_load_n(&stop_requested,__ATOMIC_ACQUIRE) && !exit_requested) {
            GF2VR_EYE_INVALIDATE();
            if(!p->running) break;
            if(!panel_xr_check(p->api.RequestExitSession(p->session),"xr_request_exit_failed")) break;
            exit_requested=1;
        }
        if(!p->running) { GF2VR_EYE_INVALIDATE(); Sleep(20); continue; }
        unsigned copy=__atomic_load_n(&panel_copy_state,__ATOMIC_ACQUIRE);
        if(copy==PC_DONE || copy==PC_SKIPPED) {
            unsigned released=p->index;
            if(!panel_image_release(p)) break;
            p->have_image=copy==PC_DONE;
            if(p->have_image) { p->released_index=released; p->latest_tick=panel_copy_tick; }
            __atomic_store_n(&panel_copy_state,PC_IDLE,__ATOMIC_RELEASE);
        }
        panel_dimensions(&w,&h,&source_format); format=panel_format(source_format);
        if(!p->acquired && (!p->swapchain || w!=p->width || h!=p->height || format!=p->format)) {
            if(p->swapchain) { p->api.DestroySwapchain(p->swapchain); p->swapchain=XR_NULL_HANDLE; p->have_image=0; }
            if(!w || !h || !format || !panel_swapchain_create(p,w,h,format)) break;
        }
        XrFrameWaitInfo wait; bs_zero(&wait,sizeof(wait)); wait.type=XR_TYPE_FRAME_WAIT_INFO;
#ifdef GF2VR_PERF_OBSERVE
        U64 pacing_start=GetTickCount64();
#endif
        XrFrameState frame; bs_zero(&frame,sizeof(frame)); frame.type=XR_TYPE_FRAME_STATE;
        if(!panel_xr_check(GF2VR_FRAME_WAIT(p,&wait,&frame),"xr_wait_frame_failed")) break;
#ifdef GF2VR_PERF_OBSERVE
        U64 pacing_wait=GetTickCount64()-pacing_start;
#endif
        if(!panel_xr_check(GF2VR_FRAME_BEGIN(p,&frame,exit_requested),"xr_begin_frame_failed")) break;
        GF2VR_OUTPUT_FRAME(p,p->frames+1);
        int panel_wanted=GF2VR_PANEL_WANTED(p,&frame);
        if(panel_wanted && frame.shouldRender && !exit_requested && !p->acquired && copy!=PC_FAULT) {
            XrSwapchainImageAcquireInfo acquire; bs_zero(&acquire,sizeof(acquire)); acquire.type=XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO;
            good=panel_xr_check(p->api.AcquireSwapchainImage(p->swapchain,&acquire,&p->index),"xr_image_acquire_failed");
            if(good) { p->acquired=1; p->waited=0; if(p->index>=p->image_count) good=0; }
        }
        if(good && p->acquired && !p->waited) {
            XrSwapchainImageWaitInfo image_wait; bs_zero(&image_wait,sizeof(image_wait)); image_wait.type=XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO; image_wait.timeout=0;
            XrResult result=p->api.WaitSwapchainImage(p->swapchain,&image_wait);
            if(result==XR_SUCCESS) {
                p->waited=1; panel_destination=p->images[p->index].texture;
                panel_request_width=p->width; panel_request_height=p->height; panel_request_format=p->format;
                __atomic_store_n(&panel_copy_state,PC_REQUEST,__ATOMIC_RELEASE);
            } else if(result!=XR_TIMEOUT_EXPIRED) good=panel_xr_check(result,"xr_image_wait_failed");
        }
        /* Only the XR worker waits. Game Present continues, polls GPU completion
           and publishes DONE. A slow/missing producer ends with zero layers and
           keeps the outstanding image alive for the next iteration. */
#ifdef GF2VR_PERF_OBSERVE
        U64 pacing_copy_start=GetTickCount64();
#endif
#ifdef GF2VR_PERF_ASYNC_PANEL
        if(good && p->acquired && p->waited && panel_released_image_usable(p)) {
            static U64 bypasses;
            ++bypasses;
            if(bypasses<=3 || !(bypasses&(bypasses-1)))
                event("panel_async_copy_bypass",bypasses,GetCurrentThreadId());
        }
#endif
        if(good && p->acquired && p->waited
#ifdef GF2VR_PERF_ASYNC_PANEL
           && !panel_released_image_usable(p)
#endif
           ) {
            U64 deadline=GetTickCount64()+100;
            unsigned completion;
            do {
                completion=__atomic_load_n(&panel_copy_state,__ATOMIC_ACQUIRE);
                if(completion==PC_DONE || completion==PC_SKIPPED || completion==PC_FAULT) break;
                Sleep(1);
            } while(GetTickCount64()<deadline);
            if(completion==PC_DONE || completion==PC_SKIPPED) {
                unsigned released=p->index;
                good=panel_image_release(p);
                if(good) {
                    p->have_image=completion==PC_DONE;
                    if(p->have_image) { p->released_index=released; p->latest_tick=panel_copy_tick; }
                }
                __atomic_store_n(&panel_copy_state,PC_IDLE,__ATOMIC_RELEASE);
            }
        }
#ifdef GF2VR_PERF_OBSERVE
        U64 pacing_copy=GetTickCount64()-pacing_copy_start;
#endif
        XrCompositionLayerQuad quad; bs_zero(&quad,sizeof(quad)); quad.type=XR_TYPE_COMPOSITION_LAYER_QUAD;
        quad.space=p->space; quad.eyeVisibility=XR_EYE_VISIBILITY_BOTH; quad.subImage.swapchain=p->swapchain;
        quad.subImage.imageRect.extent.width=(int)p->width; quad.subImage.imageRect.extent.height=(int)p->height;
        quad.pose.orientation.w=1.0f; quad.pose.position.z=-2.0f;
        quad.size.width=2.0f; quad.size.height=2.0f*(float)p->height/(float)p->width;
        const XrCompositionLayerBaseHeader *layer=(const XrCompositionLayerBaseHeader *)&quad;
        XrFrameEndInfo end; bs_zero(&end,sizeof(end)); end.type=XR_TYPE_FRAME_END_INFO;
        end.displayTime=frame.predictedDisplayTime; end.environmentBlendMode=XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
        if(good && frame.shouldRender && !exit_requested &&
#ifdef GF2VR_PERF_ASYNC_PANEL
           panel_released_image_usable(p) &&
#else
           !p->acquired && p->have_image &&
#endif
           GetTickCount64()-p->latest_tick<=500) { end.layerCount=1; end.layers=&layer; }
#ifdef GF2VR_PERF_OBSERVE
        U64 pacing_end_start=GetTickCount64();
#endif
        if(!panel_xr_check(GF2VR_OUTPUT_END_FRAME(p,&end,good && frame.shouldRender && !exit_requested),"xr_end_frame_failed")) good=0;
        GF2VR_PACING_RECORD(pacing_start,frame.predictedDisplayPeriod,pacing_wait,pacing_copy,
            GetTickCount64()-pacing_end_start,frame.shouldRender&&good&&!exit_requested);
        GF2VR_EYE_INVALIDATE(); /* Prediction is no longer this open XR frame. */
        GF2VR_OUTPUT_INVALIDATE(p);
        ++p->frames;
        if(p->frames<=3 || !(p->frames&(p->frames-1))) event(end.layerCount ? "panel_frame" : "panel_empty_frame",p->frames,GetCurrentThreadId());
    }
    GF2VR_EYE_INVALIDATE();
    if(!panel_stop_failed_loop(p,state))event("xr_failed_loop_quarantined",p->frames,GetCurrentThreadId());
    panel_drain(p);
    if(p->swapchain) p->api.DestroySwapchain(p->swapchain);
    if(p->space) p->api.DestroySpace(p->space);
    BOOL eye_closed=GF2VR_OUTPUT_CLOSE(p) && GF2VR_EYE_CLOSE();
    GF2VR_PANEL_CLOSE();
    if(p->session && eye_closed) p->api.DestroySession(p->session);
    else if(p->session) event("eye_session_quarantined",p->frames,GetCurrentThreadId());
    disabled=1; event("panel_session_retired",p->frames,GetCurrentThreadId());
}
