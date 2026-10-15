/* BSP-001: loader observation only. No engine calls, hooks, threads, or rendering. */
/* Opaque Win32 ABI only: this no-CRT DLL does not require MSVC CRT headers. */
#if !defined(_WIN64)
#error This probe requires Windows x64.
#endif
#define WINAPI __stdcall
typedef int BOOL;
typedef unsigned long DWORD;
typedef void *HANDLE;
typedef void *HINSTANCE;
typedef void *LPVOID;
#define NULL ((void *)0)
#define TRUE 1
#define DLL_PROCESS_ATTACH 1
#define FILE_APPEND_DATA 4
#define FILE_SHARE_READ 1
#define FILE_SHARE_WRITE 2
#define FILE_SHARE_DELETE 4
#define OPEN_ALWAYS 4
#define FILE_ATTRIBUTE_NORMAL 128
#define INVALID_HANDLE_VALUE ((HANDLE)(long long)-1)
__declspec(dllimport) DWORD WINAPI GetLastError(void);
__declspec(dllimport) void WINAPI SetLastError(DWORD);
__declspec(dllimport) DWORD WINAPI GetCurrentProcessId(void);
__declspec(dllimport) DWORD WINAPI GetCurrentThreadId(void);
__declspec(dllimport) DWORD WINAPI GetModuleFileNameW(HINSTANCE, unsigned short *, DWORD);
__declspec(dllimport) HANDLE WINAPI CreateFileW(const unsigned short *, DWORD, DWORD, void *, DWORD, DWORD, HANDLE);
__declspec(dllimport) BOOL WINAPI WriteFile(HANDLE, const void *, DWORD, DWORD *, void *);
__declspec(dllimport) BOOL WINAPI CloseHandle(HANDLE);
_Static_assert(sizeof(DWORD) == 4 && sizeof(void *) == 8, "Win32 x64 ABI");
#include "probe_config.h"
#ifdef GF2VR_PORTABLE_BINDING
#include "portable_binding.h"
#else
#define gf2vr_native_initialize(instance) ((void)(instance))
#endif

static char *decimal(char *p, DWORD value) {
    char digits[10]; unsigned n = 0;
    do { digits[n++] = (char)('0' + value % 10); value /= 10; } while (value);
    while (n) *p++ = digits[--n];
    return p;
}
static char *literal(char *p, const char *s) {
    while (*s) *p++ = *s++;
    return p;
}
static BOOL target_process(void) {
#ifdef GF2VR_PORTABLE_BINDING
    return gf2vr_paths.ready != 0;
#else
    unsigned short path[512];
    const unsigned short *expected = GF2VR_GAME_EXE;
    DWORD n = GetModuleFileNameW(NULL, path, 512);
    if (!n || n >= 512) return 0;
    for (DWORD i = 0; i <= n; ++i) {
        unsigned short a = path[i], b = expected[i];
        if (a >= 'A' && a <= 'Z') a += 'a' - 'A';
        if (b >= 'A' && b <= 'Z') b += 'a' - 'A';
        if (a != b) return 0;
        if (!a) return TRUE;
    }
    return 0;
#endif
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    (void)instance; (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        DWORD previous_error = GetLastError();
        gf2vr_native_initialize(instance);
        /* Kernel32 file I/O only; missing log access must never reject DLL load. */
        HANDLE file = CreateFileW(GF2VR_PROBE_LOG, FILE_APPEND_DATA,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
            OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (file != INVALID_HANDLE_VALUE) {
            char line[240]; char *p = line; DWORD written = 0;
            p = literal(p, "{\"probe\":\"BSP-001\",\"event\":\"native_attach\",\"pid\":");
            p = decimal(p, GetCurrentProcessId());
            p = literal(p, ",\"loader_thread_id\":");
            p = decimal(p, GetCurrentThreadId());
            p = literal(p, target_process() ? ",\"target_exe\":true" : ",\"target_exe\":false");
            p = literal(p, ",\"engine_ready\":\"UNKNOWN\",\"engine_main_thread\":\"UNKNOWN\",\"engine_api_calls\":0}\n");
            WriteFile(file, line, (DWORD)(p - line), &written, NULL);
            CloseHandle(file);
        }
        SetLastError(previous_error);
    }
    return TRUE;
}
