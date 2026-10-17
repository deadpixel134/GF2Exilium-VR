#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include "eye_target_hooks.h"
int WINAPI MH_Initialize(void);
int WINAPI MH_CreateHook(void *,void *,void **);
int WINAPI MH_EnableHook(void *);
int WINAPI MH_DisableHook(void *);
static struct {
    unsigned attempted,ready,owner,created[5],alternate_created[5];
    ID3D11DeviceContext *context;
    void *targets[5],*alternate[5];
} eth;
static EyeTargetWatch *eth_watch;
static EyeTargetHookNote eth_note_callback;
static void *eth_note_owner;
unsigned eye_target_hooks_observe(void *context,unsigned long long targets[5]) {
    for(unsigned i=0;i<5;++i) targets[i]=0;
    if(eth.owner!=GetCurrentThreadId()) return 0;
    unsigned status=16|(eth.ready?1:0)|(context==eth.context?2:0);
    EyeTargetWatch *w=__atomic_load_n(&eth_watch,__ATOMIC_ACQUIRE);
    if(w) status|=4;
    if(w && w->thread==eth.owner && w->state==ETW_ACTIVE && w->context==context) status|=8;
    for(unsigned i=0;i<5;++i) targets[i]=(unsigned long long)eth.targets[i];
    return status;
}
static void (STDMETHODCALLTYPE *eth_draw)(ID3D11DeviceContext *,UINT,UINT);
static void (STDMETHODCALLTYPE *eth_indexed)(ID3D11DeviceContext *,UINT,UINT,INT);
static void (STDMETHODCALLTYPE *eth_instanced)(ID3D11DeviceContext *,UINT,UINT,UINT,UINT);
static void (STDMETHODCALLTYPE *eth_indexed_instanced)(ID3D11DeviceContext *,UINT,UINT,UINT,INT,UINT);
static void (STDMETHODCALLTYPE *eth_clear)(ID3D11DeviceContext *,ID3D11RenderTargetView *,const FLOAT *);
static void eth_note(ID3D11DeviceContext *c,unsigned nonempty) {
    EyeTargetWatch *w=__atomic_load_n(&eth_watch,__ATOMIC_ACQUIRE);
    if(w && w->thread==GetCurrentThreadId() && eth.ready) eye_target_watch_draw(w,c,nonempty);
    EyeTargetHookNote note=__atomic_load_n(&eth_note_callback,__ATOMIC_ACQUIRE);
    if(note && eth.owner==GetCurrentThreadId() && eth.ready && c==eth.context)
        note(eth_note_owner,c,ETH_NOTE_DRAW,0,nonempty);
}
static void eth_note_clear(ID3D11DeviceContext *c,ID3D11RenderTargetView *view){
    EyeTargetWatch *w=__atomic_load_n(&eth_watch,__ATOMIC_ACQUIRE);
    if(w && w->thread==GetCurrentThreadId() && eth.ready) eye_target_watch_clear(w,c,view);
    EyeTargetHookNote note=__atomic_load_n(&eth_note_callback,__ATOMIC_ACQUIRE);
    if(note && eth.owner==GetCurrentThreadId() && eth.ready && c==eth.context)
        note(eth_note_owner,c,ETH_NOTE_CLEAR,view,1);
}
static void STDMETHODCALLTYPE eth_draw_hook(ID3D11DeviceContext *c,UINT count,UINT start) {
    eth_draw(c,count,start);DWORD saved=GetLastError();eth_note(c,count!=0);SetLastError(saved);
}
static void STDMETHODCALLTYPE eth_indexed_hook(ID3D11DeviceContext *c,UINT count,UINT start,INT base) {
    eth_indexed(c,count,start,base);DWORD saved=GetLastError();eth_note(c,count!=0);SetLastError(saved);
}
static void STDMETHODCALLTYPE eth_instanced_hook(ID3D11DeviceContext *c,UINT vertices,UINT instances,UINT start,UINT first) {
    eth_instanced(c,vertices,instances,start,first);DWORD saved=GetLastError();
    eth_note(c,vertices && instances);SetLastError(saved);
}
static void STDMETHODCALLTYPE eth_indexed_instanced_hook(ID3D11DeviceContext *c,UINT indices,UINT instances,UINT start,INT base,UINT first) {
    eth_indexed_instanced(c,indices,instances,start,base,first);DWORD saved=GetLastError();
    eth_note(c,indices && instances);SetLastError(saved);
}
static void STDMETHODCALLTYPE eth_clear_hook(ID3D11DeviceContext *c,ID3D11RenderTargetView *view,const FLOAT *color) {
    eth_clear(c,view,color);DWORD saved=GetLastError();
    eth_note_clear(c,view);SetLastError(saved);
}
static void (STDMETHODCALLTYPE *eth_alt_draw)(ID3D11DeviceContext *,UINT,UINT);
static void (STDMETHODCALLTYPE *eth_alt_indexed)(ID3D11DeviceContext *,UINT,UINT,INT);
static void (STDMETHODCALLTYPE *eth_alt_instanced)(ID3D11DeviceContext *,UINT,UINT,UINT,UINT);
static void (STDMETHODCALLTYPE *eth_alt_indexed_instanced)(ID3D11DeviceContext *,UINT,UINT,UINT,INT,UINT);
static void (STDMETHODCALLTYPE *eth_alt_clear)(ID3D11DeviceContext *,ID3D11RenderTargetView *,const FLOAT *);
static void STDMETHODCALLTYPE eth_alt_draw_hook(ID3D11DeviceContext *c,UINT n,UINT start) {
    eth_alt_draw(c,n,start);DWORD saved=GetLastError();eth_note(c,n!=0);SetLastError(saved);
}
static void STDMETHODCALLTYPE eth_alt_indexed_hook(ID3D11DeviceContext *c,UINT n,UINT start,INT base) {
    eth_alt_indexed(c,n,start,base);DWORD saved=GetLastError();eth_note(c,n!=0);SetLastError(saved);
}
static void STDMETHODCALLTYPE eth_alt_instanced_hook(ID3D11DeviceContext *c,UINT n,UINT instances,UINT start,UINT first) {
    eth_alt_instanced(c,n,instances,start,first);DWORD saved=GetLastError();eth_note(c,n && instances);SetLastError(saved);
}
static void STDMETHODCALLTYPE eth_alt_indexed_instanced_hook(ID3D11DeviceContext *c,UINT n,UINT instances,UINT start,INT base,UINT first) {
    eth_alt_indexed_instanced(c,n,instances,start,base,first);DWORD saved=GetLastError();eth_note(c,n && instances);SetLastError(saved);
}
static void STDMETHODCALLTYPE eth_alt_clear_hook(ID3D11DeviceContext *c,ID3D11RenderTargetView *view,const FLOAT *color) {
    eth_alt_clear(c,view,color);DWORD saved=GetLastError();
    eth_note_clear(c,view);
    SetLastError(saved);
}
int eye_target_hooks_refresh(void *context) {
    if(eth.owner!=GetCurrentThreadId() || !eth.ready || context!=eth.context) return 0;
    DWORD saved=GetLastError();ID3D11DeviceContext *c=context;
    void *targets[]={(void *)c->lpVtbl->Draw,(void *)c->lpVtbl->DrawIndexed,
        (void *)c->lpVtbl->DrawInstanced,(void *)c->lpVtbl->DrawIndexedInstanced,(void *)c->lpVtbl->ClearRenderTargetView};
    void *hooks[]={(void *)eth_alt_draw_hook,(void *)eth_alt_indexed_hook,(void *)eth_alt_instanced_hook,
        (void *)eth_alt_indexed_instanced_hook,(void *)eth_alt_clear_hook};
    void **originals[]={(void **)&eth_alt_draw,(void **)&eth_alt_indexed,(void **)&eth_alt_instanced,
        (void **)&eth_alt_indexed_instanced,(void **)&eth_alt_clear};
    for(unsigned i=0;i<5;++i) {
        if(!targets[i]) goto failed;
        if(targets[i]==eth.targets[i] || targets[i]==eth.alternate[i]) continue;
        if(eth.alternate[i]) goto failed;
        for(unsigned j=0;j<5;++j) if(j!=i &&
            (targets[i]==targets[j] || targets[i]==eth.targets[j] || targets[i]==eth.alternate[j])) goto failed;
    }
    for(unsigned i=0;i<5;++i) if(targets[i]!=eth.targets[i] && targets[i]!=eth.alternate[i]) {
        if(MH_CreateHook(targets[i],hooks[i],originals[i])) goto failed;
        eth.alternate[i]=targets[i];eth.alternate_created[i]=1;
        if(MH_EnableHook(targets[i])) goto failed;
    }
    SetLastError(saved);return 1;
failed:
    /* Partial hooks remain forwarding-only and process-pinned. No partial
       observation can publish this or a later interval after a refresh fault. */
    eth.ready=0;
    EyeTargetWatch *w=__atomic_load_n(&eth_watch,__ATOMIC_ACQUIRE);
    if(w && w->thread==eth.owner) w->draw_mask=w->clear_mask=0;
    SetLastError(saved);return 0;
}
int eye_target_hooks_coverage(void *context){
    if(!context || eth.owner!=GetCurrentThreadId() || !eth.ready || context!=eth.context)return 0;
    ID3D11DeviceContext *c=context;
    void *targets[]={(void *)c->lpVtbl->Draw,(void *)c->lpVtbl->DrawIndexed,
        (void *)c->lpVtbl->DrawInstanced,(void *)c->lpVtbl->DrawIndexedInstanced,
        (void *)c->lpVtbl->ClearRenderTargetView};
    for(unsigned i=0;i<5;++i)
        if(!targets[i] || (targets[i]!=eth.targets[i] && targets[i]!=eth.alternate[i]))return 0;
    return 1;
}
int eye_target_hooks_disable(void) {
    if(!eth.attempted || eth.owner!=GetCurrentThreadId() ||
       __atomic_load_n(&eth_watch,__ATOMIC_ACQUIRE) ||
       __atomic_load_n(&eth_note_callback,__ATOMIC_ACQUIRE)) return 0;
    DWORD saved=GetLastError();eth.ready=0;int ok=1;
    for(unsigned i=0;i<5;++i) if(eth.created[i]) {
        int status=MH_DisableHook(eth.targets[i]);
        if(status!=0 && status!=6) ok=0;
    }
    for(unsigned i=0;i<5;++i) if(eth.alternate_created[i]) {
        int status=MH_DisableHook(eth.alternate[i]);
        if(status!=0 && status!=6) ok=0;
    }
    /* Published trampolines are never removed or freed. */
    if(ok && eth.context) {ID3D11DeviceContext_Release(eth.context);eth.context=0;}
    SetLastError(saved);return ok;
}
int eye_target_hooks_install(void *context) {
    if(!context) return 0;
    if(eth.attempted)
        return eth.ready && eth.owner==GetCurrentThreadId() && eth.context==context;
    DWORD saved=GetLastError();ID3D11DeviceContext *c=context;
    if(ID3D11DeviceContext_GetType(c)!=D3D11_DEVICE_CONTEXT_IMMEDIATE){SetLastError(saved);return 0;}
    void *targets[]={(void *)c->lpVtbl->Draw,(void *)c->lpVtbl->DrawIndexed,
        (void *)c->lpVtbl->DrawInstanced,(void *)c->lpVtbl->DrawIndexedInstanced,(void *)c->lpVtbl->ClearRenderTargetView};
    void *hooks[]={(void *)eth_draw_hook,(void *)eth_indexed_hook,(void *)eth_instanced_hook,
        (void *)eth_indexed_instanced_hook,(void *)eth_clear_hook};
    void **originals[]={(void **)&eth_draw,(void **)&eth_indexed,(void **)&eth_instanced,
        (void **)&eth_indexed_instanced,(void **)&eth_clear};
    for(unsigned i=0;i<5;++i) {
        if(!targets[i]){SetLastError(saved);return 0;}
        for(unsigned j=0;j<i;++j) if(targets[i]==targets[j]){SetLastError(saved);return 0;}
    }
    eth.attempted=1;eth.owner=GetCurrentThreadId();
    ID3D11DeviceContext_AddRef(c);eth.context=c;
    int status=MH_Initialize();if(status!=0 && status!=1) goto failed;
    for(unsigned i=0;i<5;++i) {
        eth.targets[i]=targets[i];
        if(MH_CreateHook(targets[i],hooks[i],originals[i])) goto failed;
        eth.created[i]=1;
    }
    for(unsigned i=0;i<5;++i) if(MH_EnableHook(targets[i])) goto failed;
    eth.ready=1;SetLastError(saved);return 1;
failed:
    (void)eye_target_hooks_disable();SetLastError(saved);return 0;
}
int eye_target_hooks_attach(EyeTargetWatch *w) {
    if(eth.owner!=GetCurrentThreadId() || !eth.ready || !w || w->thread!=eth.owner ||
       w->state!=ETW_ACTIVE || w->context!=eth.context) return 0;
    EyeTargetWatch *empty=0;
    return __atomic_compare_exchange_n(&eth_watch,&empty,w,0,__ATOMIC_RELEASE,__ATOMIC_RELAXED);
}
int eye_target_hooks_detach(EyeTargetWatch *w) {
    if(eth.owner!=GetCurrentThreadId() || !w || w->thread!=eth.owner) return 0;
    return __atomic_compare_exchange_n(&eth_watch,&w,0,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE);
}
int eye_target_hooks_attach_note(void *context,EyeTargetHookNote note,void *owner){
    if(!context || !note || !owner || !eth.ready || eth.owner!=GetCurrentThreadId() ||
       eth.context!=context)return 0;
    if(__atomic_load_n(&eth_note_callback,__ATOMIC_ACQUIRE))return 0;
    eth_note_owner=owner;
    __atomic_store_n(&eth_note_callback,note,__ATOMIC_RELEASE);
    return 1;
}
int eye_target_hooks_detach_note(EyeTargetHookNote note,void *owner){
    if(!note || !owner || eth.owner!=GetCurrentThreadId() ||
       eth_note_owner!=owner ||
       __atomic_load_n(&eth_note_callback,__ATOMIC_ACQUIRE)!=note)return 0;
    __atomic_store_n(&eth_note_callback,0,__ATOMIC_RELEASE);
    eth_note_owner=0;return 1;
}
