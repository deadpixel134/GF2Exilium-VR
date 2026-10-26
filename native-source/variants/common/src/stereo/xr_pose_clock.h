#ifndef GF2VR_XR_POSE_CLOCK_H
#define GF2VR_XR_POSE_CLOCK_H
/* Cold OpenXR clock capability shared by late-pose policy and optional
   observation. No sample rings, counters, logs or EndFrame wrapper here. */
#ifndef _WINDOWS_
__declspec(dllimport) void * WINAPI GetProcAddress(HINSTANCE,const char *);
__declspec(dllimport) BOOL WINAPI QueryPerformanceCounter(long long *);
__declspec(dllimport) BOOL WINAPI QueryPerformanceFrequency(long long *);
#endif
typedef XrResult (XRAPI_PTR *XrPoseClockConvert)(XrInstance,XrTime,long long *);
static struct {
    unsigned extension;
    unsigned long long frequency;
    XrResult enumeration_result;
    PFN_xrGetInstanceProcAddr get;
    XrInstance instance;
    XrPoseClockConvert convert;
} xr_pose_clock;
static unsigned long long xr_pose_clock_now(void){
    DWORD saved=GetLastError();long long q=0;QueryPerformanceCounter((void *)&q);
    SetLastError(saved);return q>0?(unsigned long long)q:0;
}
static int xr_pose_clock_name(const char *a,const char *b){while(*a&&*a==*b){++a;++b;}return *a==*b;}
static void xr_pose_clock_extensions(HINSTANCE module){
    DWORD saved=GetLastError();long long frequency=0;QueryPerformanceFrequency((void *)&frequency);
    xr_pose_clock.frequency=frequency>0&&frequency<=1000000000000ll?(unsigned long long)frequency:0;
    PFN_xrEnumerateInstanceExtensionProperties enumerate=(PFN_xrEnumerateInstanceExtensionProperties)
        GetProcAddress(module,"xrEnumerateInstanceExtensionProperties");
    xr_pose_clock.get=(PFN_xrGetInstanceProcAddr)GetProcAddress(module,"xrGetInstanceProcAddr");
    static XrExtensionProperties properties[512];uint32_t count=0;
    XrResult result=XR_ERROR_FUNCTION_UNSUPPORTED;xr_pose_clock.extension=0;
    if(enumerate){
        result=enumerate(0,0,&count,0);
        if(result==XR_SUCCESS&&count>512)result=XR_ERROR_SIZE_INSUFFICIENT;
        if(result==XR_SUCCESS&&count<=512){
            for(unsigned i=0;i<count;++i)properties[i].type=XR_TYPE_EXTENSION_PROPERTIES;
            result=enumerate(0,512,&count,properties);
            if(result==XR_SUCCESS&&count<=512)for(unsigned i=0;i<count;++i)
                if(xr_pose_clock_name(properties[i].extensionName,"XR_KHR_win32_convert_performance_counter_time"))xr_pose_clock.extension=1;
        }
    }
    xr_pose_clock.enumeration_result=result;SetLastError(saved);
}
static void xr_pose_clock_created(XrInstance instance){
    DWORD saved=GetLastError();xr_pose_clock.instance=instance;xr_pose_clock.convert=0;
    PFN_xrVoidFunction function=0;
    if(xr_pose_clock.extension&&xr_pose_clock.get&&
       xr_pose_clock.get(instance,"xrConvertTimeToWin32PerformanceCounterKHR",&function)==XR_SUCCESS)
        xr_pose_clock.convert=(XrPoseClockConvert)function;
    SetLastError(saved);
}
#endif
