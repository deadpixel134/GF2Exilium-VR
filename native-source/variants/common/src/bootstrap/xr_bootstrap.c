/* BSX-001 product bootstrap: serial worker owns all OpenXR instance calls. */
static void bootstrap_after_return(unsigned long long n);
#ifndef GF2VR_OBSERVER_ID
#define GF2VR_OBSERVER_ID "BSX-001"
#endif
#ifndef GF2VR_MAIN_TICK
#define GF2VR_MAIN_TICK(n) ((void)(n))
#endif
#ifndef GF2VR_READY_RUN
#define GF2VR_READY_RUN(s,module) ((void)(s),(void)(module))
#endif
#define GF2VR_ACTIVITY_FIELD ",\"activity\":\"bootstrap\"}\n"
#define GF2VR_AFTER_RETURN(n) bootstrap_after_return(n)
#define GF2VR_CALLBACK_WAIT_CONTINUE(i) ((void)(i),1)
#define GF2VR_LOG_CALLBACK(n) ((n)==1)
#include "callback_observer.c"
#include "xr_bootstrap_state.h"

__declspec(dllimport) HINSTANCE WINAPI LoadLibraryExW(const unsigned short *,HANDLE,DWORD);
__declspec(dllimport) void * WINAPI GetProcAddress(HINSTANCE,const char *);
static unsigned bootstrap_started,stop_requested,published_phase;
#ifdef BOUNDARY_TEST
static BootstrapApi worker_test_api;
static unsigned worker_test_enabled;
#endif

static void bootstrap_report(const BootstrapState *s, DWORD loader_error) {
    if(!GF2VR_LOGGING_ALLOWED()) return;
    DWORD written; HANDLE f=CreateFileW(GF2VR_PROBE_LOG,FILE_APPEND_DATA,7,NULL,OPEN_ALWAYS,128,NULL);
    if(f!=INVALID_HANDLE_VALUE) {
        char line[768],*p=line;
        p=literal(p,"{\"build\":\"" GF2VR_OBSERVER_ID "\",\"event\":\"xr_bootstrap_state\",\"pid\":");
        p=decimal(p,GetCurrentProcessId()); p=literal(p,",\"tid\":"); p=decimal(p,GetCurrentThreadId());
        p=literal(p,",\"tick_ms\":"); p=hexadecimal(p,GetTickCount64());
        p=literal(p,",\"phase\":"); p=decimal(p,s->phase);
        p=literal(p,",\"xr_result\":"); p=hexadecimal(p,(unsigned)s->result);
        p=literal(p,",\"loader_error\":"); p=decimal(p,loader_error);
        p=literal(p,",\"generation\":"); p=decimal(p,s->generation);
        p=literal(p,",\"instance\":"); p=hexadecimal(p,(U64)s->instance);
        p=literal(p,",\"system\":"); p=hexadecimal(p,s->system);
        p=literal(p,",\"desktop_preserved\":true,\"xr_session_created\":false}\n");
        WriteFile(f,line,(DWORD)(p-line),&written,NULL); CloseHandle(f);
    }
}
static DWORD WINAPI bootstrap_worker(void *unused) {
    (void)unused;
    static BootstrapState state; bs_zero(&state,sizeof(state));
    HINSTANCE module=NULL;
#ifdef BOUNDARY_TEST
    if(worker_test_enabled) { state.api=worker_test_api; module=(HINSTANCE)1; }
#endif
    unsigned last_phase=~0u,last_generation=~0u; XrResult last_result=XR_SUCCESS;
    DWORD last_loader_error=~0u;
    while(!__atomic_load_n(&stop_requested,__ATOMIC_ACQUIRE)) {
        DWORD loader_error=0;
        if(!module) {
            /* Exact owned path. Dependencies come only from that directory/System32. */
            module=LoadLibraryExW(GF2VR_XR_LOADER,NULL,0x1100);
            if(!module) {
                loader_error=GetLastError(); state.phase=BS_LOADER_UNAVAILABLE;
                state.result=XR_ERROR_RUNTIME_UNAVAILABLE;
            } else {
                state.api.create=(PFN_xrCreateInstance)GetProcAddress(module,"xrCreateInstance");
                state.api.destroy=(PFN_xrDestroyInstance)GetProcAddress(module,"xrDestroyInstance");
                state.api.get_system=(PFN_xrGetSystem)GetProcAddress(module,"xrGetSystem");
                state.api.poll=(PFN_xrPollEvent)GetProcAddress(module,"xrPollEvent");
                /* Keep loader mapped until process exit; never unload callbacks. */
            }
        }
        if(module) bs_step(&state);
        __atomic_store_n(&published_phase,state.phase,__ATOMIC_RELEASE);
        if(last_phase!=(unsigned)state.phase || last_result!=state.result ||
           last_generation!=state.generation || last_loader_error!=loader_error) {
            bootstrap_report(&state,loader_error);
            last_phase=state.phase; last_result=state.result;
            last_generation=state.generation; last_loader_error=loader_error;
        }
        if(state.phase==BS_INSTANCE_SYSTEM_READY) GF2VR_READY_RUN(&state,module);
        /* Interval throttles retries, not eligibility; there is no attempt/time cap. */
        for(unsigned i=0;i<5 && !__atomic_load_n(&stop_requested,__ATOMIC_ACQUIRE);++i) Sleep(200);
    }
    bs_stop(&state); __atomic_store_n(&published_phase,state.phase,__ATOMIC_RELEASE);
    bootstrap_report(&state,0); return 0;
}
static void bootstrap_after_return(U64 n) {
    GF2VR_MAIN_TICK(n);
    if(n<2 || GetCurrentThreadId()!=__atomic_load_n(&first_tid,__ATOMIC_ACQUIRE)) return;
    unsigned zero=0;
    if(!__atomic_compare_exchange_n(&bootstrap_started,&zero,1,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE)) return;
    HANDLE thread=CreateThread(NULL,0,bootstrap_worker,NULL,0,NULL);
    if(thread) { CloseHandle(thread); event("xr_worker_started",0,GetCurrentThreadId()); }
    else { __atomic_store_n(&published_phase,BS_WORKER_FAILED,__ATOMIC_RELEASE); event("xr_worker_failed",GetLastError(),GetCurrentThreadId()); }
}
/* Nonblocking stop: never join or invoke runtime code from DllMain. Future XR
   consumers must marshal work to this owner, not borrow its private handles. */
__declspec(dllexport) void gf2vr_bootstrap_request_stop(void) {
    __atomic_store_n(&stop_requested,1,__ATOMIC_RELEASE);
}
__declspec(dllexport) unsigned gf2vr_bootstrap_status(void) {
    return __atomic_load_n(&published_phase,__ATOMIC_ACQUIRE);
}
#ifdef BOUNDARY_TEST
static BootstrapState test_state;
__declspec(dllexport) void xr_test_set_worker_api(const BootstrapApi *api) {
    worker_test_api=*api; worker_test_enabled=1;
}
__declspec(dllexport) void xr_test_reset(const BootstrapApi *api) {
    bs_stop(&test_state); bs_zero(&test_state,sizeof(test_state)); test_state.api=*api;
}
__declspec(dllexport) unsigned xr_test_step(void) { bs_step(&test_state); return test_state.phase; }
__declspec(dllexport) void xr_test_stop(void) { bs_stop(&test_state); }
__declspec(dllexport) unsigned xr_test_generation(void) { return test_state.generation; }
#endif
