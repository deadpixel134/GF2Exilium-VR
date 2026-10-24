#ifndef GF2VR_PERFORMANCE_RUNTIME_H
#define GF2VR_PERFORMANCE_RUNTIME_H
#include "performance_file.h"
#include "pc_output.h"
/* All metadata binding, boxed result reads and sampling below are owned by
   the accepted ExecuteTasks main callback. No XR-thread managed calls. */
static struct {
    PerformanceConfig config;PcOutput pc;unsigned initialized;
    void *width,*height,*mode,*set,*collection;
    long long (*used)(void);long long (*heap)(void);
    unsigned count_source;U64 next_sample,previous_used,estimated_count;
} performance_runtime;
/* XR close only publishes a scalar. The main owner consumes it after all
   render guards clear; no managed API is called on the XR closing thread. */
static unsigned performance_xr_exit_pending;
static void performance_xr_closed(void) {
    __atomic_store_n(&performance_xr_exit_pending,1,__ATOMIC_RELEASE);
}
static int performance_stopping(void) {
    static unsigned bound;
    static unsigned char (WINAPI *shutdown_in_progress)(void);
    if(!bound) {
        HINSTANCE module=GetModuleHandleW(L"ntdll.dll");
        shutdown_in_progress=module?(void *)GetProcAddress(module,"RtlDllShutdownInProgress"):0;
        bound=1;
    }
    /* Missing shutdown evidence fails closed for this optional managed path. */
    return !shutdown_in_progress||shutdown_in_progress();
}
static int performance_desktop_read(void *context,int *w,int *h) {
    (void)context;HINSTANCE module=GetModuleHandleW(L"user32.dll");
    int (WINAPI *metrics)(int)=module?(void *)GetProcAddress(module,"GetSystemMetrics"):0;
    if(!metrics)return 0;
    *w=metrics(0);*h=metrics(1); /* SM_CXSCREEN/SM_CYSCREEN: primary display. */
    return *w>0&&*h>0;
}
static int performance_int(void *method,void **args,int *out) {
    if(!method||performance_stopping())return 0;void *exception=0;
    void *value=sa.runtime_invoke(method,0,args,&exception);
    if(exception||!value)return 0;void *p=sa.object_unbox(value);
    if(!p)return 0;*out=*(int *)p;return 1;
}
static void performance_report(void *context,const char *kind,U64 count) {
    (void)context;event(kind,count,GetCurrentThreadId());
}
static int performance_screen_read(void *context,int *w,int *h,int *mode) {
    (void)context;int ok=performance_int(performance_runtime.width,0,w)&&
        performance_int(performance_runtime.height,0,h)&&performance_int(performance_runtime.mode,0,mode);
    /* Existing transition reads only: distinguish observed Screen dimensions
       from the asynchronous SetResolution request without more managed calls. */
    if(ok&&*w>0&&*h>0)performance_report(0,"pc_output_observed",((U64)(unsigned)*w<<16)|(unsigned)*h);
    return ok;
}
static int performance_screen_set(void *context,int w,int h,int mode) {
    (void)context;void *args[]={&w,&h,&mode},*exception=0;
    if(!performance_runtime.set||performance_stopping())return 0;
    (void)sa.runtime_invoke(performance_runtime.set,0,args,&exception);return !exception;
}
static void performance_bind(void) {
    performance_runtime.initialized=1;
    performance_runtime.config=performance_config_load(GF2VR_PERFORMANCE_CONFIG);
    /* One cold event, including exactly one failure diagnostic per file. */
    event("performance_config_state",performance_runtime.config.invalid,GetCurrentThreadId());
    HINSTANCE module=GetModuleHandleW(L"GameAssembly.dll");
    unsigned char (*generic)(void *)=(void *)il2cpp_export(module,"il2cpp_method_is_generic");
    if(performance_runtime.config.pc.fixed) {
        void *screen=render_class("UnityEngine.CoreModule.dll","UnityEngine","Screen");
        const char *args[]={"System.Int32","System.Int32","UnityEngine.FullScreenMode"};
        if(generic) {
            performance_runtime.width=eye_binding_method(screen,"get_width",0,"System.Int32",1,generic);
            performance_runtime.height=eye_binding_method(screen,"get_height",0,"System.Int32",1,generic);
            performance_runtime.mode=eye_binding_method(screen,"get_fullScreenMode",0,"UnityEngine.FullScreenMode",1,generic);
            performance_runtime.set=eye_binding_method_args(screen,"SetResolution",3,args,"System.Void",1,generic);
        }
    }
    performance_runtime.used=(void *)il2cpp_export(module,"il2cpp_gc_get_used_size");
    performance_runtime.heap=(void *)il2cpp_export(module,"il2cpp_gc_get_heap_size");
    /* Actual Haoplay export census has no GC-count export (see implementation
       evidence). Do not invent an Il2CppStat enumerator or a native ABI. */
    if(generic) {
        void *gc=render_class("mscorlib.dll","System","GC");const char *args[]={"System.Int32"};
        performance_runtime.collection=eye_binding_method_args(gc,"CollectionCount",1,args,"System.Int32",1,generic);
    }
    performance_runtime.count_source=performance_runtime.collection?2:3;
    if(!performance_runtime.used||!performance_runtime.heap)performance_runtime.count_source=0;
    event("gc_count_source",performance_runtime.count_source,GetCurrentThreadId());
}
static void performance_main_tick(int entered,int exited) {
#ifdef GF2VR_MFO_ACTIVE
    MFO_SCOPE(MFO_PC_GC);
#endif
    if(!render_installed||GetCurrentThreadId()!=__atomic_load_n(&first_tid,__ATOMIC_ACQUIRE))return;
    DWORD saved=GetLastError();
    if(performance_stopping()){SetLastError(saved);return;}
    if(__atomic_exchange_n(&performance_xr_exit_pending,0,__ATOMIC_ACQ_REL))exited=1;
    if(!performance_runtime.initialized)performance_bind();
    PcOutputApi api={performance_screen_read,performance_screen_set,performance_report,0,performance_desktop_read};
    if(performance_runtime.config.pc.fixed&&!performance_runtime.set)pc_output_fail(&performance_runtime.pc,&api,4);
    pc_output_tick(&performance_runtime.pc,&performance_runtime.config.pc,&api,entered,exited);
    U64 now=GetTickCount64();
    if(performance_runtime.count_source&&now>=performance_runtime.next_sample) {
        performance_runtime.next_sample=now+1000;
        long long used=performance_runtime.used(),heap=performance_runtime.heap();
        if(used>=0&&heap>=0) {
            int count=0,generation=0;void *args[]={&generation};U64 collections=performance_runtime.estimated_count;
            if(performance_runtime.count_source==2) {
                if(performance_int(performance_runtime.collection,args,&count)&&count>=0)collections=(unsigned)count;
                else {performance_runtime.count_source=3;event("gc_count_source",3,GetCurrentThreadId());}
            }
            if(performance_runtime.count_source==3) {
                U64 previous=performance_runtime.previous_used;
                if(previous&&(U64)used<=previous-previous/4)++performance_runtime.estimated_count;
                collections=performance_runtime.estimated_count;
            }
            performance_runtime.previous_used=(U64)used;
            long long qpc=0;QueryPerformanceCounter(&qpc);
            event("gc_sample_qpc",(U64)qpc,GetCurrentThreadId());
            event("gc_used_size",(U64)used,GetCurrentThreadId());event("gc_heap_size",(U64)heap,GetCurrentThreadId());
            event("gc_collection_count",collections,GetCurrentThreadId());
        }
    }
    SetLastError(saved);
}
#endif
