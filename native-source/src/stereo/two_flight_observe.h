#ifndef GF2_TWO_FLIGHT_OBSERVE_H
#define GF2_TWO_FLIGHT_OBSERVE_H
/* Diagnostic only. Caller supplies already observed numeric facts, never
   query GPU/XR/Unity or take another owner snapshot to populate this record.
   XR and main counters have separate sole writers. No callback/lock logging. */
#ifdef GF2VR_TWO_FLIGHT_OBSERVE
typedef struct {
    unsigned phase,flags,cpu_ok,projection_reason;
    int result;
} TwoFlightWaitNote;
static TwoFlightWaitNote stereo_two_wait_note;
static unsigned two_observe_bad[2],two_observe_good[2];
static U64 two_observe_main_frame[EFO_CAPACITY];
static void two_observe_record(unsigned lane,const char *name,EyeFlightToken token,
        U64 current,U64 serial,unsigned phase,int result,unsigned flags,U64 detail,int bad) {
    unsigned *count=bad?&two_observe_bad[lane]:&two_observe_good[lane];
    if(*count>=(bad?64u:16u) || (!bad && current%100>=2))return;
    ++*count;DWORD saved=GetLastError();char line[1024];char *p=source_prefix(line,name);
    p=literal(p,",\"current\":");p=hexadecimal(p,current);
    p=literal(p,",\"flight\":");p=hexadecimal(p,token.frame);
    p=literal(p,",\"generation\":");p=hexadecimal(p,token.generation);
    p=literal(p,",\"epoch\":");p=hexadecimal(p,token.epoch);
    p=literal(p,",\"serial\":");p=hexadecimal(p,serial);
    p=literal(p,",\"phase\":");p=hexadecimal(p,phase);
    p=literal(p,",\"result\":");p=hexadecimal(p,(U64)(long long)result);
    p=literal(p,",\"flags\":");p=hexadecimal(p,flags);
    p=literal(p,",\"detail\":");p=hexadecimal(p,detail);
    p=literal(p,"}\n");source_log(line,p);SetLastError(saved);
}
#define TWO_WAIT_NOTE(p,r,s) do {stereo_two_wait_note.phase=(p);stereo_two_wait_note.result=(r); \
    stereo_two_wait_note.flags=(s).flags;stereo_two_wait_note.cpu_ok=(s).cpu_ok;} while(0)
#ifdef BOUNDARY_TEST
__declspec(dllexport) int two_flight_observe_test(void) {
    unsigned bad0=two_observe_bad[0],good0=two_observe_good[0],bad1=two_observe_bad[1];
    DWORD saved=GetLastError();EyeFlightToken token={200,3,9};
    two_observe_bad[0]=63;two_observe_good[0]=15;two_observe_bad[1]=63;
    SetLastError(773);
    two_observe_record(0,"two_flight_observe_fixture",token,200,300,6,EFO_REJECTED,31,0x303,1);
    two_observe_record(0,"two_flight_observe_fixture",token,200,300,6,EFO_REJECTED,31,0x303,1);
    if(two_observe_bad[0]!=64 || GetLastError()!=773)return __LINE__;
    two_observe_record(0,"two_flight_observe_fixture",token,202,300,6,EFO_OK,31,0x303,0);
    if(two_observe_good[0]!=15 || GetLastError()!=773)return __LINE__;
    two_observe_record(0,"two_flight_observe_fixture",token,200,300,6,EFO_OK,31,0x303,0);
    two_observe_record(0,"two_flight_observe_fixture",token,200,300,6,EFO_OK,31,0x303,0);
    if(two_observe_good[0]!=16 || GetLastError()!=773)return __LINE__;
    two_observe_record(1,"two_flight_observe_fixture",token,200,300,7,EFO_OK,0,0x303,1);
    two_observe_record(1,"two_flight_observe_fixture",token,200,300,7,EFO_OK,0,0x303,1);
    if(two_observe_bad[1]!=64 || GetLastError()!=773)return __LINE__;
    two_observe_bad[0]=bad0;two_observe_good[0]=good0;two_observe_bad[1]=bad1;
    SetLastError(saved);return 0;
}
#endif
#else
#define TWO_WAIT_NOTE(p,r,s) ((void)0)
#endif
#endif
