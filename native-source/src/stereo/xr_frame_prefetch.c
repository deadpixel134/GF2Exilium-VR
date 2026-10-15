#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "xr_frame_prefetch.h"
static int owner(XrFramePrefetch *p) {
    return p && p->session && p->owner_thread==GetCurrentThreadId();
}
static DWORD WINAPI wait_worker(void *opaque) {
    XrFramePrefetch *p=opaque;
    for(;;) {
        DWORD woke=WaitForSingleObject(p->wake,INFINITE);
        if(woke!=WAIT_OBJECT_0) {__atomic_store_n(&p->state,XFP_FAULT,__ATOMIC_RELEASE);return 1;}
        if(__atomic_load_n(&p->stopping,__ATOMIC_ACQUIRE)) {
            if(__atomic_load_n(&p->state,__ATOMIC_ACQUIRE)==XFP_QUEUED)
                __atomic_store_n(&p->state,XFP_IDLE,__ATOMIC_RELEASE); /* never called XR */
            return 0;
        }
        if(__atomic_load_n(&p->state,__ATOMIC_ACQUIRE)!=XFP_QUEUED) continue;
        __atomic_store_n(&p->state,XFP_RUNNING,__ATOMIC_RELEASE);
        XrFrameWaitInfo info={0};info.type=XR_TYPE_FRAME_WAIT_INFO;
        XrFrameState frame={0};frame.type=XR_TYPE_FRAME_STATE;
        XrResult result=p->wait_frame(p->session,&info,&frame);
        p->packet=frame;p->result=result;
        __atomic_store_n(&p->state,XFP_READY,__ATOMIC_RELEASE);
        if(__atomic_load_n(&p->stopping,__ATOMIC_ACQUIRE)) return 0;
    }
}
int xr_frame_prefetch_open(XrFramePrefetch *p,XrSession session,PFN_xrWaitFrame wait) {
    if(!p || !session || !wait || p->session || p->wake || p->thread ||
       __atomic_load_n(&p->state,__ATOMIC_ACQUIRE)!=XFP_CLOSED || p->last_requested==~0ull) return 0;
    unsigned long long clock=p->last_requested;
    *p=(XrFramePrefetch){0};p->last_requested=p->last_begun=clock;
    p->session=session;p->wait_frame=wait;p->owner_thread=GetCurrentThreadId();
    p->wake=CreateEventW(0,FALSE,FALSE,0);
    if(!p->wake) {p->session=XR_NULL_HANDLE;p->wait_frame=0;p->owner_thread=0;return 0;}
    __atomic_store_n(&p->state,XFP_IDLE,__ATOMIC_RELEASE);
    p->thread=CreateThread(0,0,wait_worker,p,0,0);
    if(!p->thread) {
        CloseHandle(p->wake);p->wake=0;p->session=XR_NULL_HANDLE;p->wait_frame=0;p->owner_thread=0;
        __atomic_store_n(&p->state,XFP_CLOSED,__ATOMIC_RELEASE);return 0;
    }
    return 1;
}
int xr_frame_prefetch_request(XrFramePrefetch *p,unsigned long long frame) {
    if(!owner(p) || !frame || frame<=p->last_requested || p->pending_frame ||
       p->last_requested>p->last_begun || __atomic_load_n(&p->stopping,__ATOMIC_ACQUIRE) ||
       __atomic_load_n(&p->state,__ATOMIC_ACQUIRE)!=XFP_IDLE) return 0;
    p->requested=frame;p->last_requested=frame;
    p->packet=(XrFrameState){0};p->result=XR_SUCCESS;
    __atomic_store_n(&p->state,XFP_QUEUED,__ATOMIC_RELEASE);
    if(!SetEvent(p->wake)) {
        /* Wake failure cannot prove the worker did not enter XR; retain state. */
        __atomic_store_n(&p->stopping,1,__ATOMIC_RELEASE);return 0;
    }
    return 1;
}
int xr_frame_prefetch_take(XrFramePrefetch *p,unsigned long long *frame,XrFrameState *packet,XrResult *result) {
    if(!frame || !packet || !result)return 0;
    *frame=0;*packet=(XrFrameState){0};*result=XR_ERROR_CALL_ORDER_INVALID;
    if(!owner(p) || p->pending_frame || __atomic_load_n(&p->state,__ATOMIC_ACQUIRE)!=XFP_READY) return 0;
    *frame=p->requested;*packet=p->packet;*result=p->result;
    /* Loss-pending is success-class: preserve its waited-frame obligation too,
       even though product code must not start new visual work from it. */
    if(XR_SUCCEEDED(p->result))p->pending_frame=p->requested;
    if(p->result!=XR_SUCCESS)__atomic_store_n(&p->stopping,1,__ATOMIC_RELEASE);
    __atomic_store_n(&p->state,XFP_IDLE,__ATOMIC_RELEASE);
    return 1;
}
int xr_frame_prefetch_can_begin(XrFramePrefetch *p,unsigned long long frame) {
    return owner(p) && frame && frame==p->pending_frame && !p->open_frame;
}
int xr_frame_prefetch_begun(XrFramePrefetch *p,unsigned long long frame) {
    if(!xr_frame_prefetch_can_begin(p,frame)) return 0;
    p->open_frame=frame;p->last_begun=frame;p->pending_frame=0;return 1;
}
int xr_frame_prefetch_ended(XrFramePrefetch *p,unsigned long long frame) {
    if(!owner(p) || !frame || frame!=p->open_frame) return 0;
    p->open_frame=0;return 1;
}
static int exited(XrFramePrefetch *p) {
    return p->thread && WaitForSingleObject(p->thread,0)==WAIT_OBJECT_0;
}
static void stop(XrFramePrefetch *p) {
    __atomic_store_n(&p->stopping,1,__ATOMIC_RELEASE);
    SetEvent(p->wake);
}
int xr_frame_prefetch_session_stopped(XrFramePrefetch *p) {
    if(!owner(p))return 0;
    stop(p);
    if(!exited(p))return 0;
    p->pending_frame=p->open_frame=0;
    p->packet=(XrFrameState){0};p->result=XR_SUCCESS;
    __atomic_store_n(&p->state,XFP_IDLE,__ATOMIC_RELEASE);return 1;
}
int xr_frame_prefetch_quiesce(XrFramePrefetch *p) {
    if(!owner(p))return 0;
    stop(p);return exited(p);
}
int xr_frame_prefetch_close(XrFramePrefetch *p) {
    if(!p)return 0;
    if(!p->session)return !p->wake && !p->thread && __atomic_load_n(&p->state,__ATOMIC_ACQUIRE)==XFP_CLOSED;
    if(!owner(p))return 0;
    stop(p);
    if(!exited(p) || p->pending_frame || p->open_frame)return 0;
    unsigned state=__atomic_load_n(&p->state,__ATOMIC_ACQUIRE);
    if(state==XFP_READY && XR_SUCCEEDED(p->result))return 0;
    unsigned long long clock=p->last_requested;
    CloseHandle(p->thread);CloseHandle(p->wake);
    *p=(XrFramePrefetch){0};p->last_requested=p->last_begun=clock;return 1;
}
