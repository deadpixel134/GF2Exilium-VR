/* BSO-001: observe the actual-build ExecuteTasks dispatch. No new engine API calls. */
#define DllMain unused_bsp_dllmain
#include "native_probe.c"
#undef DllMain
typedef unsigned long long U64;
#ifndef GF2VR_OBSERVER_ID
#define GF2VR_OBSERVER_ID "BSO-001"
#endif
#ifndef GF2VR_ACTIVITY_FIELD
#define GF2VR_ACTIVITY_FIELD ",\"agent_engine_api_calls\":0}\n"
#endif
#ifndef GF2VR_AFTER_RETURN
#define GF2VR_AFTER_RETURN(n) ((void)(n))
#endif
#ifndef GF2VR_CALLBACK_WAIT_CONTINUE
#define GF2VR_CALLBACK_WAIT_CONTINUE(i) ((i) < 6000)
#endif
#ifndef GF2VR_LOG_CALLBACK
#define GF2VR_LOG_CALLBACK(n) ((n) <= 8 || ((n) <= 65536 && !((n) & ((n) - 1))))
#endif
typedef U64 (*Callback)(void);
__declspec(dllimport) HINSTANCE WINAPI GetModuleHandleW(const unsigned short *);
__declspec(dllimport) BOOL WINAPI GetModuleHandleExW(DWORD, const unsigned short *, HINSTANCE *);
__declspec(dllimport) HANDLE WINAPI CreateThread(void *, U64, DWORD (WINAPI *)(void *), void *, DWORD, DWORD *);
__declspec(dllimport) void WINAPI Sleep(DWORD);
__declspec(dllimport) U64 WINAPI GetTickCount64(void);

static Callback original;
static unsigned char *unity;
static U64 calls;
static DWORD first_tid;
static U64 other_thread_calls;

static char *hexadecimal(char *p, U64 x) {
    const char digits[] = "0123456789abcdef";
    p = literal(p, "\"0x");
    for (int shift = 60; shift >= 0; shift -= 4) *p++ = digits[(x >> shift) & 15];
    *p++ = '"'; return p;
}
#include "event_line.h"
#include "event_logger.h"
static void event(const char *kind, U64 count, DWORD tid) {
    if(!GF2VR_LOGGING_ALLOWED()) return;
    DWORD saved=GetLastError();char line[512];
    unsigned length=event_line(line,kind,count,tid,GetCurrentProcessId(),GetTickCount64(),
        __atomic_load_n(&first_tid,__ATOMIC_ACQUIRE),(U64)unity);
    (void)event_log_enqueue(line,length,0);SetLastError(saved);
}
#if defined(GF2VR_PERF_OBSERVE) && defined(GF2VR_MAIN_FRAME_OBSERVE)
#include "../stereo/main_frame_observe_runtime.h"
#endif
static U64 observe(void) {
    /* The original thunk ignores incoming args, sets RCX=0 and tail-jumps.
       Preserve its RAX result and Win32 last-error, and call it exactly once. */
#if defined(GF2VR_PERF_OBSERVE) && defined(GF2VR_MAIN_FRAME_OBSERVE)
    mfo_callback_entry();
#endif
    U64 result = original();
    DWORD saved = GetLastError(), tid = GetCurrentThreadId(), zero = 0;
    __atomic_compare_exchange_n(&first_tid, &zero, tid, 0, __ATOMIC_RELEASE, __ATOMIC_RELAXED);
#if defined(GF2VR_PERF_OBSERVE) && defined(GF2VR_MAIN_FRAME_OBSERVE)
    MFO_SCOPE(MFO_CALLBACK);
#endif
    U64 n = __atomic_add_fetch(&calls, 1, __ATOMIC_RELAXED);
    if (GF2VR_LOG_CALLBACK(n)) event("execute_tasks_return", n, tid);
    if (tid != __atomic_load_n(&first_tid, __ATOMIC_ACQUIRE) &&
        __atomic_add_fetch(&other_thread_calls, 1, __ATOMIC_RELAXED) <= 8)
        event("callback_thread_differs", n, tid);
    GF2VR_AFTER_RETURN(n);
#if defined(GF2VR_CAMERA_SPLIT_BRIDGE) && defined(GF2VR_MFO_ACTIVE)
    cs_bridge_drain(); /* existing main postprocessing/stage0, no render command */
#endif
    SetLastError(saved);
    return result;
}
static BOOL install(Callback *slot, Callback expected) {
    /* Publishing original precedes publishing observe. Never overwrite another owner. */
    original = expected;
    return __atomic_compare_exchange_n(slot, &expected, observe, 0,
                                       __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
}
static DWORD WINAPI worker(void *unused) {
    (void)unused;
    HINSTANCE pinned;
    /* Static-import lifetime starts us safely; pin before exposing callbacks. */
    if (!GetModuleHandleExW(5, (const unsigned short *)&worker, &pinned)) return 0;
    gf2vr_logging_initialize();
    unity = (unsigned char *)GetModuleHandleW(L"UnityPlayer.dll");
    event_log_initialize();
    if (!unity || !GetModuleHandleExW(5, (const unsigned short *)unity, &pinned)) return 0;
    DWORD pe = *(DWORD *)(unity + 0x3c);
    if (pe > 0x1000 || *(DWORD *)(unity + pe) != 0x4550 ||
        *(DWORD *)(unity + pe + 8) != GF2VR_UNITY_TIMESTAMP ||
        *(DWORD *)(unity + pe + 24 + 56) != GF2VR_UNITY_IMAGE_SIZE) {
        event("image_mismatch", 0, GetCurrentThreadId()); return 0;
    }
    /* Exact seven-byte thunk, not a pattern scan; mismatched builds fail open.
       Staging independently checks the complete native hashes. */
    const unsigned char signature[7] = {0x33,0xc9,0xe9,0xa9,0x98,0x8e,0xff};
    for (unsigned i = 0; i < 7; ++i)
        if (unity[0x7a9ca0 + i] != signature[i]) {
            event("signature_mismatch", 0, GetCurrentThreadId()); return 0;
        }
    Callback *slot = (Callback *)(unity + 0x1841b00);
    Callback expected = (Callback)(unity + 0x7a9ca0);
    /* Bounded waiting belongs only to this diagnostic; no engine APIs or polling calls. */
    for (unsigned i = 0; GF2VR_CALLBACK_WAIT_CONTINUE(i); ++i) {
        Callback current = __atomic_load_n(slot, __ATOMIC_ACQUIRE);
        if (current == expected) {
            event(install(slot, expected) ? "slot_installed" : "slot_conflict", 0, GetCurrentThreadId());
            return 0;
        }
        if (current) { event("slot_foreign_owner", 0, GetCurrentThreadId()); return 0; }
        Sleep(10);
    }
    event("slot_not_initialized", 0, GetCurrentThreadId()); return 0;
}
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    (void)instance; (void)reserved;
#ifdef GF2VR_PORTABLE_BINDING
    if (reason == DLL_PROCESS_ATTACH) {
        DWORD saved = GetLastError();
        gf2vr_native_initialize(instance);
        if (target_process()) {
            HANDLE thread = CreateThread(NULL, 0, worker, NULL, 0, NULL);
            if (thread) CloseHandle(thread);
        }
        SetLastError(saved);
    }
#else
    if (reason == DLL_PROCESS_ATTACH && target_process()) {
        DWORD saved = GetLastError();
        HANDLE thread = CreateThread(NULL, 0, worker, NULL, 0, NULL);
        if (thread) CloseHandle(thread);
        SetLastError(saved);
    }
#endif
    /* Pinning prevents dynamic unload. At process termination other threads are
       already gone; flush owned IO synchronously without joining any thread. */
    if(reason==0 && reserved)event_log_close_process();
    return TRUE;
}
#ifdef BOUNDARY_TEST
__declspec(dllexport) BOOL observer_test_install(Callback *slot, Callback expected) {
    return install(slot, expected);
}
#endif
