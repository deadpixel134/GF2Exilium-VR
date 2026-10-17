#ifndef GF2_TWO_FLIGHT_ROOT_OBSERVE_H
#define GF2_TWO_FLIGHT_ROOT_OBSERVE_H
/* Diagnostic only: sole main-owner counters and already-owned numeric slot
   transitions. No new Unity, XR, GPU, fence or resource-owner operation. */
#ifdef GF2VR_TWO_FLIGHT_ROOT_OBSERVE
static struct {
    U64 start,root_count,arm_attempts,end_pending_block,reserve_ok,reserve_busy,reserve_none;
    U64 arm_ready,root_finished,render_issued,preissue_failed;
    U64 render_drained,copy_drained,retired;
    U64 reserve_to_root_ms,reserve_to_render_ms,reserve_to_copy_ms,reserve_to_retire_ms;
    U64 occupied_roots[3];
    unsigned reports;
} two_root_note;
static U64 two_root_flight[EFO_CAPACITY],two_root_reserved_at[EFO_CAPACITY];
static unsigned two_root_render_seen[EFO_CAPACITY],two_root_copy_seen[EFO_CAPACITY];
#ifdef GF2VR_RESERVE_STATE_OBSERVE
#include "reserve_state_observe.h"
static void two_root_reserve_report(void) {
    DWORD saved=GetLastError();unsigned long long values[RSO_COUNT];
    eye_reserve_observe_take(values);
    char line[1000],*p=source_prefix(line,"main_reserve_state");
    p=literal(p,",\"schema\":1,\"counts\":[");
    for(unsigned i=0;i<RSO_COUNT;++i){if(i)p=literal(p,",");p=decimal(p,values[i]);}
    p=literal(p,"]}\n");source_log(line,p);SetLastError(saved);
}
#endif
static void two_root_report(U64 now) {
    if(!two_root_note.start || now-two_root_note.start<2000 || two_root_note.reports>=24)return;
    DWORD saved=GetLastError();char line[1300],*p=source_prefix(line,"two_root_lifetime");
#define TWO_ROOT_FIELD(name) do {p=literal(p,",\"" #name "\":");p=decimal(p,two_root_note.name);} while(0)
    p=literal(p,",\"elapsed_ms\":");p=decimal(p,now-two_root_note.start);
    TWO_ROOT_FIELD(root_count);TWO_ROOT_FIELD(arm_attempts);TWO_ROOT_FIELD(end_pending_block);
    TWO_ROOT_FIELD(reserve_ok);TWO_ROOT_FIELD(reserve_busy);TWO_ROOT_FIELD(reserve_none);
    TWO_ROOT_FIELD(arm_ready);TWO_ROOT_FIELD(root_finished);
    TWO_ROOT_FIELD(render_issued);TWO_ROOT_FIELD(preissue_failed);
    TWO_ROOT_FIELD(render_drained);TWO_ROOT_FIELD(copy_drained);TWO_ROOT_FIELD(retired);
    TWO_ROOT_FIELD(reserve_to_root_ms);TWO_ROOT_FIELD(reserve_to_render_ms);
    TWO_ROOT_FIELD(reserve_to_copy_ms);TWO_ROOT_FIELD(reserve_to_retire_ms);
#undef TWO_ROOT_FIELD
    p=literal(p,",\"occupied_roots\":[");
    for(unsigned i=0;i<3;++i){if(i)p=literal(p,",");p=decimal(p,two_root_note.occupied_roots[i]);}
    p=literal(p,"]}\n");source_log(line,p);SetLastError(saved);
#ifdef GF2VR_RESERVE_STATE_OBSERVE
    two_root_reserve_report();
#endif
    unsigned reports=two_root_note.reports+1;
    two_root_note=(__typeof__(two_root_note)){0};
    two_root_note.start=now;two_root_note.reports=reports;
}
static void two_root_begin(void) {
    U64 now=GetTickCount64();if(!two_root_note.start)two_root_note.start=now;
    unsigned occupied=0;
    for(unsigned i=0;i<EFO_CAPACITY;++i)occupied+=stereo_two.slots[i].admitted.token.frame!=0;
    if(occupied>2)occupied=2;
    ++two_root_note.root_count;++two_root_note.occupied_roots[occupied];
    two_root_report(now);
}
static void two_root_reserved(unsigned index,U64 frame) {
    two_root_flight[index]=frame;two_root_reserved_at[index]=GetTickCount64();
    two_root_render_seen[index]=two_root_copy_seen[index]=0;
}
static void two_root_snapshot(unsigned index,EyeFlightToken token,const EyeFlightSnapshot *s) {
    if(!s || !token.frame || two_root_flight[index]!=token.frame)return;
    U64 now=GetTickCount64(),start=two_root_reserved_at[index];
    if((s->flags&EFF_RENDER_DRAINED) && !two_root_render_seen[index]) {
        two_root_render_seen[index]=1;++two_root_note.render_drained;
        if(now>=start)two_root_note.reserve_to_render_ms+=now-start;
    }
    if((s->flags&EFF_COPY_DRAINED) && !two_root_copy_seen[index]) {
        two_root_copy_seen[index]=1;++two_root_note.copy_drained;
        if(now>=start)two_root_note.reserve_to_copy_ms+=now-start;
    }
}
static void two_root_retired(unsigned index,U64 frame) {
    if(two_root_flight[index]!=frame)return;
    U64 now=GetTickCount64(),start=two_root_reserved_at[index];
    ++two_root_note.retired;if(now>=start)two_root_note.reserve_to_retire_ms+=now-start;
    two_root_flight[index]=two_root_reserved_at[index]=0;
    two_root_render_seen[index]=two_root_copy_seen[index]=0;
}
#ifdef BOUNDARY_TEST
__declspec(dllexport) int two_root_observe_test(void) {
    __typeof__(two_root_note) saved=two_root_note;DWORD error=GetLastError();
    U64 now=GetTickCount64();two_root_note=(__typeof__(two_root_note)){0};
    two_root_note.start=now>=2000?now-2000:1;
    two_root_note.root_count=9;two_root_note.arm_attempts=8;
    two_root_note.reserve_ok=3;two_root_note.occupied_roots[2]=5;
    SetLastError(773);two_root_report(now);
    int ok=two_root_note.reports==1 && two_root_note.root_count==0 &&
        two_root_note.occupied_roots[2]==0 && GetLastError()==773;
    two_root_note=saved;SetLastError(error);return ok?0:__LINE__;
}
#endif
#else
#define two_root_begin() ((void)0)
#define two_root_reserved(index,frame) ((void)0)
#define two_root_snapshot(index,token,snapshot) ((void)0)
#define two_root_retired(index,frame) ((void)0)
#endif
#endif
