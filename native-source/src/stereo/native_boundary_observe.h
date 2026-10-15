#ifndef GF2_NATIVE_BOUNDARY_OBSERVE_H
#define GF2_NATIVE_BOUNDARY_OBSERVE_H
/* Unity owner only. No engine/GPU/XR query, no extra current() evaluation.
   Timestamps bracket existing preparation and typed native reads. Logging is
   failure/slow-only and its budget never participates in production policy. */
typedef struct {
    U64 start,phase[4],call_start[6],call_end[6];
    unsigned active,started,completed;
#ifdef GF2VR_NATIVE_TARGET_CACHE
    unsigned cache_hit,cache_sample;
#endif
} NativeBoundaryNote;
static NativeBoundaryNote native_boundary_note;
static unsigned native_boundary_records;
static U64 native_boundary_frequency;
__declspec(dllimport) int __stdcall QueryPerformanceFrequency(long long *);
__declspec(dllimport) int __stdcall QueryPerformanceCounter(long long *);
#ifdef BOUNDARY_TEST
static U64 (*native_boundary_test_clock)(void);
#endif
static U64 native_boundary_stamp(void) {
    DWORD saved=GetLastError();long long stamp=0;
#ifdef BOUNDARY_TEST
    if(native_boundary_test_clock)return native_boundary_test_clock();
#endif
    if(!native_boundary_frequency) {
        long long frequency=0;
        if(QueryPerformanceFrequency(&frequency) && frequency>0)
            native_boundary_frequency=(U64)frequency;
    }
    int ok=QueryPerformanceCounter(&stamp);
    SetLastError(saved);return ok && stamp>0?(U64)stamp:0;
}
static void native_boundary_begin(void) {
    volatile unsigned char *p=(void *)&native_boundary_note;
    for(unsigned i=0;i<sizeof(native_boundary_note);++i)p[i]=0;
    native_boundary_note.active=1;native_boundary_note.start=native_boundary_stamp();
}
static void native_boundary_phase(unsigned phase) {
    if(native_boundary_note.active && phase<4)
        native_boundary_note.phase[phase]=native_boundary_stamp();
}
static void native_boundary_call(unsigned step,int finished) {
    if(!native_boundary_note.active || step>=6)return;
    if(finished) {
        native_boundary_note.call_end[step]=native_boundary_stamp();
        native_boundary_note.completed|=1u<<step;
    } else {
        native_boundary_note.call_start[step]=native_boundary_stamp();
        native_boundary_note.started|=1u<<step;
    }
}
static U64 native_boundary_elapsed(U64 start,U64 end) {
    return start && end>=start?end-start:0;
}
static int native_boundary_wanted(int native_ok,U64 end) {
    U64 elapsed=native_boundary_elapsed(native_boundary_note.start,end);
    return native_boundary_note.active && native_boundary_records<128 &&
        (
#ifdef GF2VR_NATIVE_TARGET_CACHE
         native_boundary_note.cache_sample ||
#endif
         !native_ok || (native_boundary_frequency && elapsed>=native_boundary_frequency*32/1000));
}
static void native_boundary_record(U64 frame,U64 generation,U64 epoch,U64 serial,
    int native_ok,unsigned native_reason,unsigned current_reason,int snapshot_result,
    unsigned flags,unsigned frame_reason,unsigned pose_reason) {
    U64 end=native_boundary_stamp();int wanted=native_boundary_wanted(native_ok,end);
    native_boundary_note.active=0;
    if(!wanted)return;
    ++native_boundary_records;
    DWORD saved=GetLastError();char line[2200],*p=source_prefix(line,"native_boundary");
#define NB_FIELD(name,value) do {p=literal(p,",\"" name "\":");p=decimal(p,(value));} while(0)
#ifdef GF2VR_NATIVE_TARGET_CACHE
#define NB_WIDE(p,value) hexadecimal(p,(U64)(value))
#undef NB_FIELD
#define NB_FIELD(name,value) do {p=literal(p,",\"" name "\":");p=NB_WIDE(p,(value));} while(0)
    NB_FIELD("schema",2);NB_FIELD("cache_hit",native_boundary_note.cache_hit);
#else
#define NB_WIDE(p,value) decimal(p,(value))
    NB_FIELD("schema",1);
#endif
    NB_FIELD("frequency",native_boundary_frequency);
    NB_FIELD("frame",frame);NB_FIELD("generation",generation);NB_FIELD("epoch",epoch);
    NB_FIELD("serial",serial);NB_FIELD("start_qpc",native_boundary_note.start);NB_FIELD("end_qpc",end);
    NB_FIELD("prepare_qpc",native_boundary_note.phase[0]);NB_FIELD("arm_qpc",native_boundary_note.phase[1]);
    NB_FIELD("native_begin_qpc",native_boundary_note.phase[2]);NB_FIELD("native_end_qpc",native_boundary_note.phase[3]);
    NB_FIELD("native_ok",native_ok!=0);NB_FIELD("native_reason",native_reason);
    NB_FIELD("current_reason",current_reason);NB_FIELD("snapshot_result",snapshot_result);
    NB_FIELD("current_flags",flags);NB_FIELD("frame_reason",frame_reason);NB_FIELD("pose_reason",pose_reason);
    NB_FIELD("started_mask",native_boundary_note.started);NB_FIELD("completed_mask",native_boundary_note.completed);
    for(unsigned i=0;i<6;++i) {
        p=literal(p,",\"call");p=decimal(p,i);p=literal(p,"_start_qpc\":");p=NB_WIDE(p,native_boundary_note.call_start[i]);
        p=literal(p,",\"call");p=decimal(p,i);p=literal(p,"_end_qpc\":");p=NB_WIDE(p,native_boundary_note.call_end[i]);
    }
    p=literal(p,"}\n");source_log(line,p);SetLastError(saved);
#undef NB_FIELD
#undef NB_WIDE
}
#define GF2VR_NATIVE_CALL(step,finished) native_boundary_call(step,finished)
#endif
