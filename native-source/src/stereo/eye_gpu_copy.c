#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#ifdef GF2VR_PERF_FENCE_DELIVERY
#include <d3d11_4.h>
#if defined(GF2VR_FOCUS_CAPTURE_DIR)
#error Fence delivery excludes render-thread capture diagnostics
#endif
static const GUID egc_device5={0x8ffde202,0xa0e7,0x45df,{0x9e,1,0xe8,0x37,0x80,0x1b,0x5e,0xa0}};
static const GUID egc_context4={0x917600da,0xf58c,0x4c33,{0x98,0xd8,0x3e,0x15,0xb3,0x90,0xfa,0x24}};
static const GUID egc_fence_iid={0xaffde9d1,0x1df7,0x4bb7,{0x8a,0x34,0x0f,0x46,0x25,0x1d,0xab,0x80}};
#endif
#include "eye_gpu_copy.h"
#ifdef GF2VR_NATIVE_TARGET_CACHE
void eye_native_resource_retain(void *resource){IUnknown_AddRef((IUnknown *)resource);}
void eye_native_resource_release(void *resource){IUnknown_Release((IUnknown *)resource);}
#endif
#ifdef GF2VR_COPY_PRESENT_OBSERVE
#if defined(GF2VR_COPY_GPU_OBSERVE) || defined(GF2VR_BANK_COPY_DELIVERY_OBSERVE) || defined(GF2VR_FOCUS_CAPTURE_DIR)
#error Copy Present correlation excludes GPU/copy capture diagnostics
#endif
#define GF2_COPY_PRESENT_IMPLEMENTATION
#include "copy_present_observe.h"
#endif
#ifdef GF2VR_COPY_GPU_OBSERVE
#if defined(GF2VR_BANK_GPU_OBSERVE) || defined(GF2VR_BANK_EYE_WRITE_OBSERVE) || defined(GF2VR_BANK_COPY_DELIVERY_OBSERVE) || defined(GF2VR_FOCUS_CAPTURE_DIR)
#error Copy-only GPU observation excludes other render/copy capture diagnostics
#endif
#define GF2_COPY_GPU_IMPLEMENTATION
#include "copy_gpu_observe.h"
#endif
#ifdef GF2VR_BANK_FENCE_OBSERVE
#define GF2_BANK_FENCE_OBSERVE_IMPLEMENTATION
#include "bank_fence_observe.h"
static void egc_observe(EyeGpuCopy *p,unsigned phase,unsigned tried,const unsigned *hr,UINT64 value) {
    DWORD saved=GetLastError();LARGE_INTEGER at={0};QueryPerformanceCounter(&at);
    BankFenceObservation r={0};
    r.serial=p->ticket.serial;r.generation=p->ticket.generation;r.epoch=p->ticket.epoch;
    r.qpc=(unsigned long long)at.QuadPart;r.value=value;r.thread=GetCurrentThreadId();
    r.phase=phase;r.drain_only=p->drain_only;r.tried=tried;
    if(hr)for(unsigned i=0;i<4;++i)r.hr[i]=hr[i];
    bank_fence_observe_record(&r);SetLastError(saved);
}
#endif
#ifdef GF2VR_FOCUS_CAPTURE_DIR
#include "focus_capture.h"
#endif
static const GUID egc_texture_iid={0x6f15aaf2,0xd208,0x4e89,{0x9a,0xb4,0x48,0x95,0x35,0xd3,0x4f,0x9c}};
static const GUID egc_identity_iid={0,0,0,{0xc0,0,0,0,0,0,0,0x46}};
static void egc_release(void *p) { if(p) IUnknown_Release((IUnknown *)p); }
#ifdef GF2VR_UPRIGHT_TRANSFER
#include "eye_upright.h"
#endif
static int egc_same(void *a,void *b) {
    IUnknown *x=0,*y=0;
    HRESULT hx=IUnknown_QueryInterface((IUnknown *)a,&egc_identity_iid,(void **)&x);
    HRESULT hy=IUnknown_QueryInterface((IUnknown *)b,&egc_identity_iid,(void **)&y);
    int same=SUCCEEDED(hx) && SUCCEEDED(hy) && x && y && x==y;
    egc_release(x);egc_release(y);return same;
}
static void egc_clear(EyeGpuCopy *p) {
#ifdef GF2VR_PERF_FENCE_DELIVERY
    egc_release(p->fence);
#endif
    egc_release(p->transfer);
    for(unsigned i=0;i<4;++i) egc_release(p->textures[i]);
    egc_release(p->query);egc_release(p->context);egc_release(p->device);
    *p=(EyeGpuCopy){0};
}
static int egc_format(DXGI_FORMAT f) {
    /* Storage validation only. Typeless RGBA8 interpretation is supplied by
       Unity's retained RenderTexture.sRGB before typed XR configuration. */
    return f==DXGI_FORMAT_R8G8B8A8_TYPELESS || f==DXGI_FORMAT_R8G8B8A8_UNORM || f==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB ||
           f==DXGI_FORMAT_B8G8R8A8_UNORM || f==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB ||
           f==DXGI_FORMAT_R16G16B16A16_FLOAT;
}
static int egc_resources(EyeGpuCopy *candidate,ID3D11Device *d,void **inputs,unsigned count,EyeGpuDescription *out,EyeGpuObservation *obs) {
    D3D11_TEXTURE2D_DESC first={0};DXGI_FORMAT destination=DXGI_FORMAT_UNKNOWN;
    for(unsigned i=0;i<count;++i) {
        ID3D11Texture2D *t=0;
        if(obs) {obs->stage=4;obs->eye=i;}
        HRESULT hr=IUnknown_QueryInterface((IUnknown *)inputs[i],&egc_texture_iid,(void **)&t);
        if(obs) obs->result=(unsigned)hr;
        if(FAILED(hr) || !t) {
            egc_release(t);return 0;
        }
        candidate->textures[i]=t;
        if(obs) obs->stage=5;
        for(unsigned j=0;j<i;++j) if(egc_same(t,candidate->textures[j])) return 0;
        if(obs) obs->stage=6;
        ID3D11Device *td=0;ID3D11Texture2D_GetDevice(t,&td);
        int match=td && egc_same(td,d);egc_release(td);if(!match) return 0;
        D3D11_TEXTURE2D_DESC desc;ID3D11Texture2D_GetDesc(t,&desc);
        /* XR owns shared destination synchronization through acquire/wait and
           release after our GPU drain. This does NOT admit keyed-mutex storage
           or shared Unity producers. The caller holds the claimed XR lease. */
        unsigned unsupported_misc=desc.MiscFlags & ~(i>=2?(unsigned)D3D11_RESOURCE_MISC_SHARED:0u);
        if(obs) {
            obs->stage=7;
            if(i<4) {
                obs->valid_mask|=1u<<i;
                obs->textures[i]=(EyeGpuObservedTexture){desc.Width,desc.Height,desc.MipLevels,desc.ArraySize,
                    desc.Format,desc.SampleDesc.Count,desc.SampleDesc.Quality,desc.Usage,desc.BindFlags,desc.CPUAccessFlags,desc.MiscFlags};
            }
            obs->reject_mask=(!desc.Width || !desc.Height?1u:0u)|(desc.MipLevels!=1?2u:0u)|
                (desc.ArraySize!=1?4u:0u)|(desc.SampleDesc.Count!=1 || desc.SampleDesc.Quality?8u:0u)|
                (desc.Usage!=D3D11_USAGE_DEFAULT?16u:0u)|(desc.CPUAccessFlags?32u:0u)|
                (unsupported_misc?64u:0u)|((desc.BindFlags&D3D11_BIND_DEPTH_STENCIL)?128u:0u)|
                (!egc_format(desc.Format)?256u:0u);
        }
        if(!desc.Width || !desc.Height || desc.MipLevels!=1 || desc.ArraySize!=1 ||
           desc.SampleDesc.Count!=1 || desc.SampleDesc.Quality ||
           desc.Usage!=D3D11_USAGE_DEFAULT || desc.CPUAccessFlags || unsupported_misc ||
           (desc.BindFlags&D3D11_BIND_DEPTH_STENCIL) || !egc_format(desc.Format)) return 0;
        if(obs) obs->stage=8;
        if(i && (desc.Width!=first.Width || desc.Height!=first.Height)) return 0;
        if(i==1 && desc.Format!=first.Format) return 0;
        if(i>=2) {
            int compatible=desc.Format==first.Format && desc.Format!=DXGI_FORMAT_R8G8B8A8_TYPELESS;
            if(first.Format==DXGI_FORMAT_R8G8B8A8_TYPELESS)
                /* D3D OpenXR images use typeless storage even for a typed
                   swapchain. Encoding stays in the typed XR configuration. */
                compatible=desc.Format==DXGI_FORMAT_R8G8B8A8_TYPELESS ||
                    desc.Format==DXGI_FORMAT_R8G8B8A8_UNORM || desc.Format==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            if(!compatible || (i==3 && desc.Format!=destination)) return 0;
            destination=desc.Format;
        }
        if(!i) first=desc;
    }
    if(out) *out=(EyeGpuDescription){first.Width,first.Height,(unsigned)first.Format};
    return 1;
}
int eye_gpu_describe(void *device,void *left,void *right,EyeGpuDescription *out) {
    return eye_gpu_describe_observed(device,left,right,out,0);
}
int eye_gpu_describe_observed(void *device,void *left,void *right,EyeGpuDescription *out,EyeGpuObservation *obs) {
    if(obs) {*obs=(EyeGpuObservation){0};obs->stage=1;}
    if(!out) return 0;
    *out=(EyeGpuDescription){0};
    if(!device || !left || !right) return 0;
    DWORD saved=GetLastError();EyeGpuCopy candidate={0};void *inputs[]={left,right};
    unsigned flags=ID3D11Device_GetCreationFlags((ID3D11Device *)device);
    if(obs) {obs->stage=2;obs->device_flags=flags;}
    int ok=0;
    if(!(flags&D3D11_CREATE_DEVICE_SINGLETHREADED)) {
        HRESULT hr=ID3D11Device_GetDeviceRemovedReason((ID3D11Device *)device);
        if(obs) {obs->stage=3;obs->result=(unsigned)hr;}
        if(SUCCEEDED(hr)) ok=egc_resources(&candidate,device,inputs,2,out,obs);
    }
    if(obs && ok) obs->stage=9;
    egc_clear(&candidate);SetLastError(saved);return ok;
}
int eye_gpu_preflight(void *device,void *left,void *right,void *dl,void *dr) {
    return eye_gpu_preflight_observed(device,left,right,dl,dr,0);
}
int eye_gpu_preflight_observed(void *device,void *left,void *right,void *dl,void *dr,EyeGpuObservation *obs) {
    if(obs) {*obs=(EyeGpuObservation){0};obs->stage=1;}
    if(!device || !left || !right || !dl || !dr) return 0;
    DWORD saved=GetLastError();EyeGpuCopy candidate={0};void *inputs[]={left,right,dl,dr};
    unsigned flags=ID3D11Device_GetCreationFlags((ID3D11Device *)device);
    if(obs) {obs->stage=2;obs->device_flags=flags;}
    int ok=0;
    if(!(flags&D3D11_CREATE_DEVICE_SINGLETHREADED)) {
        HRESULT hr=ID3D11Device_GetDeviceRemovedReason((ID3D11Device *)device);
        if(obs) {obs->stage=3;obs->result=(unsigned)hr;}
        if(SUCCEEDED(hr)) ok=egc_resources(&candidate,device,inputs,4,0,obs);
    }
    if(obs && ok) obs->stage=9;
    egc_clear(&candidate);SetLastError(saved);return ok;
}
static int egc_submit(EyeGpuCopy *p,void *device,void *context,
                   void *left,void *right,void *dl,void *dr,EyeGpuTicket ticket,int drain_only) {
    if(!p || p->state!=EGC_IDLE || p->device || p->context || p->query || p->transfer ||
       !device || !context || !left || !right || (!drain_only && (!dl || !dr)) ||
       !ticket.serial || !ticket.generation || !ticket.epoch) return 0;
    for(unsigned i=0;i<4;++i) if(p->textures[i]) return 0;
    ID3D11Device *d=device;ID3D11DeviceContext *c=context;
    if(ID3D11DeviceContext_GetType(c)!=D3D11_DEVICE_CONTEXT_IMMEDIATE ||
       FAILED(ID3D11Device_GetDeviceRemovedReason(d))) return 0;
    ID3D11Predicate *predicate=0;BOOL predicate_value=FALSE;
    ID3D11DeviceContext_GetPredication(c,&predicate,&predicate_value);
    if(predicate) {egc_release(predicate);return 0;} /* Copy/End must not be predicated. */
    ID3D11Device *cd=0;ID3D11DeviceContext_GetDevice(c,&cd);
    int match=cd && egc_same(cd,d);egc_release(cd);if(!match) return 0;
    EyeGpuCopy candidate={0};void *inputs[]={left,right,dl,dr};
#ifdef GF2VR_COPY_GPU_OBSERVE
    EyeGpuDescription copy_desc={0};
    if(!egc_resources(&candidate,d,inputs,drain_only?2:4,&copy_desc,0)) goto failed;
#else
    if(!egc_resources(&candidate,d,inputs,drain_only?2:4,0,0)) goto failed;
#endif
    D3D11_QUERY_DESC qd={D3D11_QUERY_EVENT,0};
    if(FAILED(ID3D11Device_CreateQuery(d,&qd,(ID3D11Query **)&candidate.query)) || !candidate.query) goto failed;
#ifdef GF2VR_UPRIGHT_TRANSFER
    if(!drain_only) {
        candidate.transfer=egc_upright_prepare(d,candidate.textures);
        if(!candidate.transfer) goto failed;
    }
#endif
    ID3D11Device_AddRef(d);ID3D11DeviceContext_AddRef(c);
    candidate.device=d;candidate.context=c;candidate.ticket=ticket;
    candidate.thread=GetCurrentThreadId();candidate.state=EGC_PENDING;
    candidate.drain_only=drain_only;
    *p=candidate; /* Ownership established before any GPU command. */
    if(!drain_only) {
#ifdef GF2VR_COPY_PRESENT_OBSERVE
        copy_present_observe_copy(ticket,d,c,CPO_COPY_BEGIN,0);
#endif
#ifdef GF2VR_COPY_GPU_OBSERVE
        copy_gpu_observe_begin(d,c,ticket,copy_desc,1);
#endif
#ifdef GF2VR_UPRIGHT_TRANSFER
        ID3D11DeviceContext_ExecuteCommandList(c,(ID3D11CommandList *)p->transfer,TRUE);
#else
        ID3D11DeviceContext_CopyResource(c,(ID3D11Resource *)p->textures[2],(ID3D11Resource *)p->textures[0]);
        ID3D11DeviceContext_CopyResource(c,(ID3D11Resource *)p->textures[3],(ID3D11Resource *)p->textures[1]);
#endif
#ifdef GF2VR_COPY_GPU_OBSERVE
#ifdef GF2VR_UPRIGHT_TRANSFER
        copy_gpu_observe_end(c,ticket,3,1);
#else
        copy_gpu_observe_end(c,ticket,3,0);
#endif
#endif
#ifdef GF2VR_FOCUS_CAPTURE_DIR
        focus_capture_begin(p);
#endif
    }
    ID3D11DeviceContext_End(c,(ID3D11Asynchronous *)p->query);
#ifdef GF2VR_PERF_FENCE_DELIVERY
    /* Render owner only. Optional capability; the existing EVENT query remains
       the fallback if creation/Signal fails. No CPU signal or Flush. */
    ID3D11Device5 *d5=0;ID3D11DeviceContext4 *c4=0;ID3D11Fence *fence=0;
#ifdef GF2VR_BANK_FENCE_OBSERVE
    unsigned tried=0,hrs[4]={0};
#define EGC_ARM_CALL(i,call) (tried|=1u<<(i),hrs[i]=(unsigned)(call),(HRESULT)hrs[i])
#else
#define EGC_ARM_CALL(i,call) (call)
#endif
    if(SUCCEEDED(EGC_ARM_CALL(0,ID3D11Device_QueryInterface(d,&egc_device5,(void **)&d5))) && d5 &&
       SUCCEEDED(EGC_ARM_CALL(1,ID3D11DeviceContext_QueryInterface(c,&egc_context4,(void **)&c4))) && c4 &&
       SUCCEEDED(EGC_ARM_CALL(2,ID3D11Device5_CreateFence(d5,0,D3D11_FENCE_FLAG_NONE,&egc_fence_iid,(void **)&fence))) && fence) {
        p->fence=fence;fence=0;
        p->fence_armed=SUCCEEDED(EGC_ARM_CALL(3,ID3D11DeviceContext4_Signal(c4,(ID3D11Fence *)p->fence,1)));
#ifdef GF2VR_COPY_PRESENT_OBSERVE
        if(!drain_only)copy_present_observe_copy(ticket,d,c,CPO_COPY_SIGNAL,p->fence_armed);
#endif
#ifdef GF2VR_PERF_FENCE_TIMING
        if(p->fence_armed) {
            DWORD saved=GetLastError();
            LARGE_INTEGER stamp={0};
            if(QueryPerformanceCounter(&stamp)) p->timing.signal=(unsigned long long)stamp.QuadPart;
            SetLastError(saved);
        }
#endif
    }
    egc_release(fence);egc_release(c4);egc_release(d5);
#ifdef GF2VR_BANK_FENCE_OBSERVE
    egc_observe(p,1,tried,hrs,p->fence_armed);
#endif
#undef EGC_ARM_CALL
#endif
    return 1;
failed:
    egc_clear(&candidate);return 0;
}
int eye_gpu_submit(EyeGpuCopy *p,void *d,void *c,void *l,void *r,void *dl,void *dr,EyeGpuTicket t) {
    return egc_submit(p,d,c,l,r,dl,dr,t,0);
}
int eye_gpu_drain(EyeGpuCopy *p,void *d,void *c,void *l,void *r,EyeGpuTicket t) {
    return egc_submit(p,d,c,l,r,0,0,t,1);
}
int eye_gpu_poll(EyeGpuCopy *p) {
    if(!p) return EGC_FAULT;
    if(p->state==EGC_IDLE) return EGC_IDLE;
    if(p->thread!=GetCurrentThreadId()) return EGC_FAULT; /* no state mutation */
    if(p->state!=EGC_PENDING) return (int)p->state;
#ifdef GF2VR_PERF_FENCE_DELIVERY
    /* Once Signal succeeds, wait for that marker itself before releasing it. */
    if(p->fence_armed) return eye_gpu_poll_fenced(p);
#endif
    ID3D11Device *d=p->device;ID3D11DeviceContext *c=p->context;
    if(FAILED(ID3D11Device_GetDeviceRemovedReason(d))) {p->state=EGC_FAULT;return EGC_FAULT;}
    BOOL done=FALSE;
    HRESULT hr=ID3D11DeviceContext_GetData(c,(ID3D11Asynchronous *)p->query,&done,sizeof(done),D3D11_ASYNC_GETDATA_DONOTFLUSH);
    if(FAILED(hr)) p->state=EGC_FAULT;
    else if(hr==S_OK && done) {
#ifdef GF2VR_BANK_FENCE_OBSERVE
        egc_observe(p,4,0,0,1);
#endif
        p->state=EGC_DRAINED;
#ifdef GF2VR_FOCUS_CAPTURE_DIR
        focus_capture_end(p);
#endif
    }
    return (int)p->state;
}
int eye_gpu_current(const EyeGpuCopy *p,EyeGpuTicket t) {
    return p && !p->drain_only && p->thread==GetCurrentThreadId() && p->state==EGC_DRAINED &&
        SUCCEEDED(ID3D11Device_GetDeviceRemovedReason((ID3D11Device *)p->device)) &&
        t.serial==p->ticket.serial && t.generation==p->ticket.generation && t.epoch==p->ticket.epoch;
}
int eye_gpu_dispose(EyeGpuCopy *p) {
    if(!p) return 0;
    if(p->state==EGC_IDLE) return 1;
    if(p->thread!=GetCurrentThreadId() || p->state!=EGC_DRAINED) return 0;
    egc_clear(p);return 1;
}
#ifdef GF2VR_PERF_FENCE_DELIVERY
int eye_gpu_poll_fenced(EyeGpuCopy *p) {
    if(!p) return EGC_FAULT;
    if(p->state!=EGC_PENDING) return (int)p->state;
    if(!p->fence_armed || !p->fence) return EGC_PENDING;
    UINT64 value=ID3D11Fence_GetCompletedValue((ID3D11Fence *)p->fence);
#ifdef GF2VR_COPY_PRESENT_OBSERVE
    if(!p->drain_only)copy_present_observe_copy(p->ticket,p->device,p->context,CPO_FENCE_POLL,value);
#endif
#ifdef GF2VR_BANK_FENCE_OBSERVE
    if(value<1 && !p->observe_pending) {p->observe_pending=1;egc_observe(p,2,0,0,value);}
    if(value>=1)egc_observe(p,3,0,0,value); /* actual observed value, including device loss */
#endif
#ifdef GF2VR_PERF_FENCE_TIMING
    DWORD saved=GetLastError();
    LARGE_INTEGER stamp={0};
    if(p->timing.signal && QueryPerformanceCounter(&stamp)) {
        unsigned long long at=(unsigned long long)stamp.QuadPart;
        ++p->timing.polls;
        if(value<1) {
            ++p->timing.pending;
            if(!p->timing.first_pending)p->timing.first_pending=at;
            p->timing.last_pending=at;
        } else if(value!=~(UINT64)0 && !p->timing.first_complete)p->timing.first_complete=at;
    }
    SetLastError(saved);
#endif
    if(value==~(UINT64)0 || FAILED(ID3D11Device_GetDeviceRemovedReason((ID3D11Device *)p->device)))
        p->state=EGC_FAULT;
    else if(value>=1) {p->fence_completed=1;p->state=EGC_DRAINED;}
    return (int)p->state;
}
int eye_gpu_dispose_fenced(EyeGpuCopy *p) {
    if(!p || p->state!=EGC_DRAINED || !p->fence_armed || !p->fence_completed) return 0;
    egc_clear(p);return 1;
}
#endif

