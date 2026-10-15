#ifndef GF2_ASYNC_THROUGHPUT_OBSERVE_H
#define GF2_ASYNC_THROUGHPUT_OBSERVE_H
/* Diagnostic only. Main and XR have independent owner-local record/dedup budgets;
   no callback logging, shared mutable telemetry, ownership snapshot or GPU call.
   QPC is observed elapsed time, NOT GPU execution time or permission to release. */
#ifdef GF2VR_ASYNC_THROUGHPUT_OBSERVE
__declspec(dllimport) BOOL WINAPI QueryPerformanceCounter(long long *);
__declspec(dllimport) BOOL WINAPI QueryPerformanceFrequency(long long *);
static struct {
    unsigned records;
    EyeFlightToken last[10];
    U64 serial[10],detail[10];unsigned flags[10];int result[10];
} async_throughput_notes[2];
static void async_throughput_record(unsigned phase,EyeFlightToken token,U64 serial,
                                   unsigned flags,int result,U64 detail) {
    /* Phases1..5 called ONLY by main owner,6..9 ONLY by XR owner. No TLS/CRT
       runtime dependency and no cross-thread writes to either lane. */
    __typeof__(async_throughput_notes[0]) *note=&async_throughput_notes[phase>5];
    if(phase>=10 || !phase || !token.frame || note->records>=256)return;
    if(note->last[phase].frame==token.frame &&
       note->last[phase].generation==token.generation &&
       note->last[phase].epoch==token.epoch &&
       note->serial[phase]==serial && note->flags[phase]==flags &&
       note->result[phase]==result && note->detail[phase]==detail)return;
    DWORD saved=GetLastError();long long stamp,frequency;
    if(!QueryPerformanceCounter(&stamp)||!QueryPerformanceFrequency(&frequency)||frequency<=0) {
        SetLastError(saved);return;
    }
    ++note->records;note->last[phase]=token;
    note->serial[phase]=serial;note->flags[phase]=flags;
    note->result[phase]=result;note->detail[phase]=detail;
    char line[1024],*p=source_prefix(line,"async_throughput_boundary");
    p=literal(p,",\"phase\":");p=decimal(p,phase);
    p=literal(p,",\"flight\":");p=decimal(p,token.frame);
    p=literal(p,",\"generation\":");p=decimal(p,token.generation);
    p=literal(p,",\"epoch\":");p=decimal(p,token.epoch);
    p=literal(p,",\"serial\":");p=decimal(p,serial);
    p=literal(p,",\"flags\":");p=decimal(p,flags);
    p=literal(p,",\"result\":");p=hexadecimal(p,(U64)(long long)result);
    p=literal(p,",\"detail\":");p=hexadecimal(p,detail);
    p=literal(p,",\"qpc\":");p=decimal(p,(U64)stamp);
    p=literal(p,",\"frequency\":");p=decimal(p,(U64)frequency);
    p=literal(p,"}\n");source_log(line,p);SetLastError(saved);
}
#ifdef BOUNDARY_TEST
__declspec(dllexport) int async_throughput_record_test(void) {
    __typeof__(async_throughput_notes[0]) saved=async_throughput_notes[0];
    __typeof__(async_throughput_notes[0]) saved_xr=async_throughput_notes[1];
    DWORD error=GetLastError();async_throughput_notes[0]=(__typeof__(async_throughput_notes[0])){0};
    async_throughput_notes[1]=(__typeof__(async_throughput_notes[1])){0};
    SetLastError(731);
    EyeFlightToken token={17,3,9};
    async_throughput_record(1,token,21,16,EFO_OK,4);
    async_throughput_record(1,token,21,16,EFO_OK,4);
    int ok=async_throughput_notes[0].records==1 && GetLastError()==731;
    async_throughput_notes[0].records=256;
    async_throughput_record(2,token,21,17,EFO_BUSY,5);
    ok=ok && async_throughput_notes[0].records==256 && GetLastError()==731;
    async_throughput_record(6,token,21,0,EFO_OK,0);
    ok=ok && async_throughput_notes[1].records==1 && GetLastError()==731;
    async_throughput_notes[1]=saved_xr;
    async_throughput_notes[0]=saved;SetLastError(error);return ok?0:__LINE__;
}
#endif
#else
#define async_throughput_record(phase,token,serial,flags,result,detail) ((void)0)
#endif
#endif
