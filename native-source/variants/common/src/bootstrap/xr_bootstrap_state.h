/* Worker-owned OpenXR instance state. No game, rendering or timing dependencies. */
#define XR_NO_PROTOTYPES
#define XR_NO_STDINT_H
typedef __INT8_TYPE__ int8_t;
typedef __UINT8_TYPE__ uint8_t;
typedef __INT16_TYPE__ int16_t;
typedef __UINT16_TYPE__ uint16_t;
typedef __INT32_TYPE__ int32_t;
typedef __UINT32_TYPE__ uint32_t;
typedef __INT64_TYPE__ int64_t;
typedef __UINT64_TYPE__ uint64_t;
#include <openxr/openxr.h>
#ifdef GF2VR_LATE_RELOCATE
static int stereo_late_instance_retire(void);
#endif
#ifdef GF2VR_LATENCY_OBSERVE
#include "../stereo/latency_observe_runtime.h"
#elif defined(GF2VR_LATE_RELOCATE)
#include "../stereo/xr_pose_clock.h"
#endif

enum BootstrapPhase { BS_COLD, BS_WAIT_RUNTIME, BS_WAIT_SYSTEM, BS_INSTANCE_SYSTEM_READY,
                     BS_RESTART_REQUIRED, BS_STOPPED, BS_LOADER_UNAVAILABLE, BS_WORKER_FAILED };
typedef struct {
    PFN_xrCreateInstance create;
    PFN_xrDestroyInstance destroy;
    PFN_xrGetSystem get_system;
    PFN_xrPollEvent poll;
} BootstrapApi;
typedef struct {
    BootstrapApi api;
    XrInstance instance;
    XrSystemId system;
    enum BootstrapPhase phase;
    XrResult result;
    unsigned generation;
    XrEventDataBuffer event_buffer;
} BootstrapState;
static void bs_zero(void *p, unsigned size) {
    for(unsigned i=0;i<size;++i) ((volatile unsigned char *)p)[i]=0;
}
static void bs_retire(BootstrapState *s) {
    XrInstance instance=s->instance;
#ifdef GF2VR_LATE_RELOCATE
    /* Parent destruction also owns every child handle. Quarantine on an
       outstanding main locate lease; never destroy underneath that call. */
    if(instance && !stereo_late_instance_retire())return;
#endif
    s->instance=XR_NULL_HANDLE; s->system=XR_NULL_SYSTEM_ID;
    if(instance) s->api.destroy(instance);
}
static void bs_stop(BootstrapState *s) {
    bs_retire(s); s->phase=BS_STOPPED;
}
static void bs_step(BootstrapState *s) {
    if(s->phase==BS_STOPPED || s->phase==BS_RESTART_REQUIRED) return;
    if(!s->api.create || !s->api.destroy || !s->api.get_system || !s->api.poll) {
        s->phase=BS_LOADER_UNAVAILABLE; s->result=XR_ERROR_FUNCTION_UNSUPPORTED; return;
    }
    if(!s->instance) {
        const char *extension="XR_KHR_D3D11_enable";
        XrInstanceCreateInfo info; bs_zero(&info,sizeof(info));
        info.type=XR_TYPE_INSTANCE_CREATE_INFO;
        const char app[]="GF2 VR"; const char engine[]="GF2VR Native Bootstrap";
        for(unsigned i=0;i<sizeof(app);++i) info.applicationInfo.applicationName[i]=app[i];
        for(unsigned i=0;i<sizeof(engine);++i) info.applicationInfo.engineName[i]=engine[i];
        info.applicationInfo.applicationVersion=1;
        info.applicationInfo.engineVersion=1;
        info.applicationInfo.apiVersion=XR_MAKE_VERSION(1,0,0);
        info.enabledExtensionCount=1; info.enabledExtensionNames=&extension;
#if defined(GF2VR_LATENCY_OBSERVE) || defined(GF2VR_LATE_RELOCATE)
        const char *extensions[2]={extension,"XR_KHR_win32_convert_performance_counter_time"};
        if(xr_pose_clock.extension){info.enabledExtensionCount=2;info.enabledExtensionNames=extensions;}
#endif
        XrInstance instance=XR_NULL_HANDLE;
        s->result=s->api.create(&info,&instance);
#if defined(GF2VR_LATENCY_OBSERVE) || defined(GF2VR_LATE_RELOCATE)
        if((s->result==XR_ERROR_EXTENSION_NOT_PRESENT || s->result==XR_ERROR_FEATURE_UNSUPPORTED) && xr_pose_clock.extension){
            xr_pose_clock.extension=0;xr_pose_clock.convert=0;
#ifdef GF2VR_LATENCY_OBSERVE
            event("latency_optional_extension_create_rejected",(U64)(long long)s->result,GetCurrentThreadId());
#endif
            /* Keep the original one-create-per-step/retry cadence. The next
               existing retry uses the original D3D11-only extension list. */
        }
#endif
        if(s->result!=XR_SUCCESS || !instance) {
            if(s->result==XR_SUCCESS) s->result=XR_ERROR_RUNTIME_FAILURE;
            s->phase=BS_WAIT_RUNTIME; return;
        }
        s->instance=instance; ++s->generation;
#ifdef GF2VR_LATENCY_OBSERVE
        latency_created(instance);
#elif defined(GF2VR_LATE_RELOCATE)
        xr_pose_clock_created(instance);
#endif
    }
    /* Limit per-tick event work, never the lifetime of the bootstrap. */
    for(unsigned i=0;i<32;++i) {
        XrEventDataBuffer *event=&s->event_buffer;
        bs_zero(event,sizeof(*event)); event->type=XR_TYPE_EVENT_DATA_BUFFER;
        XrResult result=s->api.poll(s->instance,event);
        if(result==XR_EVENT_UNAVAILABLE) break;
        if(result!=XR_SUCCESS || event->type==XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING) {
            bs_retire(s);
            s->result=result==XR_SUCCESS ? XR_ERROR_INSTANCE_LOST : result;
            /* A lossTime event needs XR-time conversion before safe recreation.
               Keep desktop usable; explicit game restart is the supported recovery. */
            s->phase=event->type==XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING ? BS_RESTART_REQUIRED : BS_WAIT_RUNTIME;
            return;
        }
    }
    if(s->system) { s->phase=BS_INSTANCE_SYSTEM_READY; s->result=XR_SUCCESS; return; }
    XrSystemGetInfo info; bs_zero(&info,sizeof(info));
    info.type=XR_TYPE_SYSTEM_GET_INFO; info.formFactor=XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    XrSystemId system=XR_NULL_SYSTEM_ID;
    s->result=s->api.get_system(s->instance,&info,&system);
    if(s->result==XR_SUCCESS && system) {
        s->system=system; s->phase=BS_INSTANCE_SYSTEM_READY;
    } else if(s->result==XR_ERROR_FORM_FACTOR_UNAVAILABLE) {
        s->phase=BS_WAIT_SYSTEM;
    } else {
        if(s->result==XR_SUCCESS) s->result=XR_ERROR_RUNTIME_FAILURE;
        bs_retire(s); s->phase=BS_WAIT_RUNTIME;
    }
}
