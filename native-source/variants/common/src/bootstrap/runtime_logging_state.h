#ifndef GF2VR_RUNTIME_LOGGING_STATE_H
#define GF2VR_RUNTIME_LOGGING_STATE_H
/* Process-immutable after the pinned bootstrap worker's cold configuration read.
   No polling, loader-lock I/O, or diagnostic-dependent production lifetime. */
#ifdef GF2VR_OPTIONAL_LOGGING
extern unsigned gf2vr_runtime_logging_enabled;
#define GF2VR_LOGGING_ALLOWED() __atomic_load_n(&gf2vr_runtime_logging_enabled,__ATOMIC_ACQUIRE)
#else
#define GF2VR_LOGGING_ALLOWED() 1u
#endif
#endif
