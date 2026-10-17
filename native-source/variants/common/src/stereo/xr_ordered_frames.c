#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "xr_ordered_frames.h"
#ifdef GF2VR_BANK_END_OBSERVE
#include "end_boundary_observe.h"
EndBoundaryObservation end_boundary_observation;
int ebo_active(void) {return end_boundary_observation.armed && end_boundary_observation.reports<EBO_REPORT_CAP;}
EboU64 ebo_stamp(void) {
    if(!ebo_active())return 0;
    DWORD saved=GetLastError();LARGE_INTEGER at;
    int ok=QueryPerformanceCounter(&at);SetLastError(saved);
    return ok && at.QuadPart>0?(EboU64)at.QuadPart:0;
}
void ebo_arm(void) {
    if(end_boundary_observation.armed)return;
    DWORD saved=GetLastError();LARGE_INTEGER frequency;
    int ok=QueryPerformanceFrequency(&frequency);SetLastError(saved);
    if(ok && frequency.QuadPart>0)end_boundary_observation.frequency=(EboU64)frequency.QuadPart;
    end_boundary_observation.armed=1;
}
void ebo_api(EboU64 before,EboU64 after,int result) {
    if(!ebo_active())return;
    EndBoundaryObservation *s=&end_boundary_observation;++s->api_calls;
    s->api_success+=result==XR_SUCCESS;
    if(!s->frequency || !before || after<before){++s->clock_invalid;return;}
    EboU64 duration=after-before;s->api_ticks+=duration;
    if(duration>=s->api_max_ticks) {
        s->api_max_ticks=duration;s->max_api_motion_ppm=s->motion_ppm;
        s->max_api_gap_ppm=s->gap_ppm;s->max_api_context_mask=s->context_mask;
    }
}
void ebo_driver(EboU64 before,EboU64 after) {
    if(!ebo_active())return;
    if(!end_boundary_observation.frequency || !before || after<before){++end_boundary_observation.clock_invalid;return;}
    end_boundary_observation.driver_ticks+=after-before;
}
void ebo_busy(int post,int result) {
    if(!ebo_active() || result!=-1)return; /* EFO_BUSY, numeric only */
    if(post)++end_boundary_observation.post_busy;else ++end_boundary_observation.pre_busy;
}
void ebo_next_window(void) {
    EndBoundaryObservation *s=&end_boundary_observation;
    EboU64 frequency=s->frequency;unsigned armed=s->armed,reports=s->reports+1;
    *s=(EndBoundaryObservation){.frequency=frequency,.armed=armed,.reports=reports};
}
#endif
static int owner(XrOrderedFrames *f) {
    return f && f->prefetch.session && f->prefetch.owner_thread==GetCurrentThreadId();
}
int xr_ordered_frames_open(XrOrderedFrames *f,XrSession session,PFN_xrWaitFrame wait,
    PFN_xrBeginFrame begin,PFN_xrEndFrame end) {
    if(!f || !begin || !end || f->begin || f->end || f->pending_id || f->open_id)return 0;
    if(!xr_frame_prefetch_open(&f->prefetch,session,wait))return 0;
    f->begin=begin;f->end=end;f->stopping=f->quarantined=0;
    f->pending=f->open=(XrFrameState){0};f->pending_result=XR_SUCCESS;return 1;
}
int xr_ordered_frames_request(XrOrderedFrames *f,unsigned long long frame,int reserved) {
    return owner(f) && reserved && !f->stopping && !f->quarantined && !f->pending_id &&
        xr_frame_prefetch_request(&f->prefetch,frame);
}
int xr_ordered_frames_take(XrOrderedFrames *f,unsigned long long *id,XrFrameState *packet,XrResult *result) {
    if(!id || !packet || !result)return 0;
    *id=0;*packet=(XrFrameState){0};*result=XR_ERROR_CALL_ORDER_INVALID;
    if(!owner(f) || f->pending_id || f->quarantined)return 0;
    if(!xr_frame_prefetch_take(&f->prefetch,id,packet,result))return 0;
    if(XR_SUCCEEDED(*result)) {
        f->pending_id=*id;f->pending=*packet;f->pending_result=*result;
    }
    if(*result!=XR_SUCCESS)f->stopping=1;
    if(XR_FAILED(*result))f->quarantined=1;
    return 1;
}
int xr_ordered_frames_begin(XrOrderedFrames *f,unsigned long long id,XrResult *result) {
    if(!result)return 0;
    *result=XR_ERROR_CALL_ORDER_INVALID;
    if(!owner(f) || f->quarantined || !id || id!=f->pending_id || f->open_id ||
       f->pending.type!=XR_TYPE_FRAME_STATE || f->pending.predictedDisplayTime<=0 ||
       !xr_frame_prefetch_can_begin(&f->prefetch,id))return 0;
    XrFrameBeginInfo info={.type=XR_TYPE_FRAME_BEGIN_INFO};
    *result=f->begin(f->prefetch.session,&info);
    if(XR_SUCCEEDED(*result)) {
        if(!xr_frame_prefetch_begun(&f->prefetch,id)) {f->quarantined=1;return 1;}
        f->open_id=id;f->open=f->pending;f->pending_id=0;f->pending=(XrFrameState){0};
    }
    if(*result!=XR_SUCCESS)f->stopping=1;
    if(XR_FAILED(*result))f->quarantined=1;
    return 1;
}
int xr_ordered_frames_end(XrOrderedFrames *f,unsigned long long id,const XrFrameEndInfo *info,XrResult *result) {
    if(!result)return 0;
    *result=XR_ERROR_CALL_ORDER_INVALID;
    if(!owner(f) || f->quarantined || !id || id!=f->open_id || !info ||
       info->type!=XR_TYPE_FRAME_END_INFO || info->displayTime!=f->open.predictedDisplayTime ||
       (info->layerCount && (!info->layers || !xr_ordered_frames_visual(f,id))))return 0;
#ifdef GF2VR_BANK_END_OBSERVE
    EboU64 before=ebo_stamp();
#endif
    *result=f->end(f->prefetch.session,info);
#ifdef GF2VR_BANK_END_OBSERVE
    EboU64 after=ebo_stamp();ebo_api(before,after,*result);
#endif
    if(XR_SUCCEEDED(*result)) {
        if(!xr_frame_prefetch_ended(&f->prefetch,id)){f->quarantined=1;return 1;}
        f->open_id=0;f->open=(XrFrameState){0};
    }
    if(*result!=XR_SUCCESS)f->stopping=1;
    if(XR_FAILED(*result))f->quarantined=1;
    return 1;
}
int xr_ordered_frames_visual(XrOrderedFrames *f,unsigned long long id) {
    return owner(f) && id && f->open_id==id && !f->stopping && !f->quarantined && f->open.shouldRender;
}
int xr_ordered_frames_close(XrOrderedFrames *f) {
    if(!f)return 0;
    if(!f->prefetch.session)return !f->begin && !f->end && !f->pending_id && !f->open_id;
    if(!owner(f))return 0;
    f->stopping=1;
    /* Always request worker stop, even while an open frame remains. */
    int stopped=xr_frame_prefetch_close(&f->prefetch);
    if(!stopped || f->pending_id || f->open_id)return 0;
    f->begin=0;f->end=0;f->pending=f->open=(XrFrameState){0};
    f->pending_result=XR_SUCCESS;f->quarantined=0;return 1;
}
int xr_ordered_frames_session_stopped(XrOrderedFrames *f) {
    if(!owner(f))return 0;
    f->stopping=1;
    if(!xr_frame_prefetch_session_stopped(&f->prefetch))return 0;
    f->pending_id=f->open_id=0;f->pending=f->open=(XrFrameState){0};
    f->quarantined=0;return 1;
}
