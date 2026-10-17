#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include "eye_target_watch.h"
#ifdef GF2VR_BANK_EYE_WRITE_OBSERVE
#include "bank_eye_write_observe.h"
#endif
#ifdef GF2VR_FOCUS_GPU_DIR
#include "focus_gpu_observe.h"
#endif
static const GUID etw_iunknown={0,0,0,{0xc0,0,0,0,0,0,0,0x46}};
static void etw_release(void *p) {if(p) IUnknown_Release((IUnknown *)p);}
static void *etw_identity(void *p) {
    void *result=0;
    if(!p || FAILED(IUnknown_QueryInterface((IUnknown *)p,&etw_iunknown,&result))) return 0;
    return result;
}
static int etw_ticket(EyeGpuTicket a,EyeGpuTicket b) {
    return a.serial && a.generation && a.epoch && a.serial==b.serial &&
        a.generation==b.generation && a.epoch==b.epoch;
}
static int etw_owner(EyeTargetWatch *w,void *context) {
    return w && w->thread==GetCurrentThreadId() && w->state==ETW_ACTIVE && w->context==context;
}
int eye_target_watch_begin(EyeTargetWatch *w,void *context,void *left,void *right,EyeGpuTicket ticket) {
    if(!w || (w->thread && w->thread!=GetCurrentThreadId()) ||
       w->state!=ETW_IDLE || w->context || w->identity[0] || w->identity[1] ||
       !context || !left || !right || !etw_ticket(ticket,ticket) ||
       (w->thread && w->thread!=GetCurrentThreadId())) return 0;
    DWORD saved=GetLastError();int ok=0;ID3D11Device *device=0;
    if(!w->thread) w->thread=GetCurrentThreadId();w->state=ETW_BUSY;
    ID3D11DeviceContext *c=context;
    if(ID3D11DeviceContext_GetType(c)!=D3D11_DEVICE_CONTEXT_IMMEDIATE) goto done;
    ID3D11DeviceContext_GetDevice(c,&device);
    EyeGpuDescription description={0};
    if(!device || !eye_gpu_describe(device,left,right,&description)) goto done;
    void *a=etw_identity(left),*b=etw_identity(right);
    if(!a || !b || a==b) {etw_release(a);etw_release(b);goto done;}
    ID3D11DeviceContext_AddRef(c);
    w->ticket=ticket;w->context=c;w->identity[0]=a;w->identity[1]=b;
    w->draw_mask=w->clear_mask=0;w->draw_calls=w->clear_calls=w->predicated=0;ok=1;
    w->raw_draw_calls=w->empty_draw_calls=0;
done:
    etw_release(device);w->state=ok?ETW_ACTIVE:ETW_IDLE;SetLastError(saved);return ok;
}
static unsigned etw_view(EyeTargetWatch *w,ID3D11RenderTargetView *view) {
    if(!view) return 0;
    D3D11_RENDER_TARGET_VIEW_DESC d;
    ID3D11RenderTargetView_GetDesc(view,&d);
    if(d.ViewDimension!=D3D11_RTV_DIMENSION_TEXTURE2D || d.Texture2D.MipSlice) return 0;
    ID3D11Resource *resource=0;ID3D11RenderTargetView_GetResource(view,&resource);
    void *identity=etw_identity(resource);unsigned mask=0;
    if(identity==w->identity[0]) mask|=1;
    if(identity==w->identity[1]) mask|=2;
    etw_release(identity);etw_release(resource);return mask;
}
#ifdef GF2VR_FOCUS_LINEAGE_DIR
#include "focus_lineage.h"
#endif
void eye_target_watch_clear(EyeTargetWatch *w,void *context,void *view) {
    if(!etw_owner(w,context) || !view) return;
    DWORD saved=GetLastError();w->state=ETW_BUSY;
    if(w->observe && w->clear_calls!=~0u) ++w->clear_calls;
    unsigned mask=etw_view(w,view);
#ifdef GF2VR_BANK_EYE_WRITE_OBSERVE
    bank_eye_write_observe_clear(context,w->ticket,mask);
#endif
#ifdef GF2VR_FOCUS_LINEAGE_DIR
    focus_lineage_clear(w,view,mask);
#endif
    w->clear_mask|=mask;
    /* A later clear invalidates earlier draw coverage of that eye. */
    w->draw_mask&=~mask;w->state=ETW_ACTIVE;SetLastError(saved);
}
void eye_target_watch_draw(EyeTargetWatch *w,void *context,unsigned nonempty) {
    if(!etw_owner(w,context)) return;
    if(w->observe) {
        if(w->raw_draw_calls!=~0u) ++w->raw_draw_calls;
        if(!nonempty && w->empty_draw_calls!=~0u) ++w->empty_draw_calls;
    }
    if(!nonempty) return;
    DWORD saved=GetLastError();ID3D11DeviceContext *c=context;
    w->state=ETW_BUSY;
#ifdef GF2VR_FOCUS_LINEAGE_DIR
    focus_lineage_draw(w,c);
#endif
    if(w->observe && w->draw_calls!=~0u) ++w->draw_calls;
    ID3D11Predicate *predicate=0;BOOL value=0;
    ID3D11DeviceContext_GetPredication(c,&predicate,&value);
    if(predicate) {
        if(w->observe && w->predicated!=~0u) ++w->predicated;
        etw_release(predicate);w->state=ETW_ACTIVE;SetLastError(saved);return;
    }
    ID3D11RenderTargetView *views[8]={0};ID3D11BlendState *blend=0;
    FLOAT factors[4];UINT sample_mask=0;D3D11_BLEND_DESC desc={0};
    ID3D11DeviceContext_OMGetBlendState(c,&blend,factors,&sample_mask);
    if(blend) ID3D11BlendState_GetDesc(blend,&desc);
    ID3D11DeviceContext_OMGetRenderTargets(c,8,views,0);
    unsigned current_mask=0;
    for(unsigned i=0;i<8;++i) {
        if(sample_mask && (!blend || desc.RenderTarget[desc.IndependentBlendEnable?i:0].RenderTargetWriteMask))
            current_mask|=etw_view(w,views[i]);
        etw_release(views[i]);
    }
    w->draw_mask|=current_mask;
#ifdef GF2VR_BANK_EYE_WRITE_OBSERVE
    bank_eye_write_observe_draw(context,w->ticket,current_mask);
#endif
#if defined(GF2VR_FOCUS_GPU_DIR) && !defined(GF2VR_FOCUS_MRT)
    focus_gpu_draw(w,c,current_mask,0);
#endif
    etw_release(blend);w->state=ETW_ACTIVE;SetLastError(saved);
}
int eye_target_watch_end(EyeTargetWatch *w,EyeGpuTicket ticket,unsigned *draw,unsigned *clear) {
    if(draw) *draw=0;if(clear) *clear=0;
    if(!w || !draw || !clear || w->thread!=GetCurrentThreadId() || w->state!=ETW_ACTIVE ||
       !etw_ticket(w->ticket,ticket)) return 0;
    w->state=ETW_ENDED;*draw=w->draw_mask;*clear=w->clear_mask;return 1;
}
int eye_target_watch_release(EyeTargetWatch *w,EyeGpuTicket ticket,int drained) {
    if(!w || w->thread!=GetCurrentThreadId() || w->state!=ETW_ENDED || !drained ||
       !etw_ticket(w->ticket,ticket)) return 0;
    DWORD saved=GetLastError();w->state=ETW_BUSY;
#ifdef GF2VR_FOCUS_GPU_DIR
    focus_gpu_end(w);
#endif
#ifdef GF2VR_FOCUS_LINEAGE_DIR
    focus_lineage_end(w);
#endif
    etw_release(w->identity[0]);etw_release(w->identity[1]);etw_release(w->context);
    w->context=w->identity[0]=w->identity[1]=0;w->ticket=(EyeGpuTicket){0};
    w->draw_mask=w->clear_mask=0;w->state=ETW_IDLE;SetLastError(saved);return 1;
}
#ifdef GF2VR_PERF_FENCE_DELIVERY
#if defined(GF2VR_FOCUS_GPU_DIR) || defined(GF2VR_FOCUS_LINEAGE_DIR)
#error Fence delivery excludes context-dependent watch capture diagnostics
#endif
int eye_target_watch_release_completed(EyeTargetWatch *w,EyeGpuTicket ticket) {
    if(!w || w->state!=ETW_ENDED || !etw_ticket(w->ticket,ticket)) return 0;
    DWORD saved=GetLastError();w->state=ETW_BUSY;
    etw_release(w->identity[0]);etw_release(w->identity[1]);etw_release(w->context);
    w->context=w->identity[0]=w->identity[1]=0;w->ticket=(EyeGpuTicket){0};
    w->draw_mask=w->clear_mask=0;w->state=ETW_IDLE;SetLastError(saved);return 1;
}
#endif
