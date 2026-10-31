from pathlib import Path
import re,subprocess,os
import argparse
p=argparse.ArgumentParser();p.add_argument('--zig',required=True);p.add_argument('--output',required=True);options=p.parse_args()
root=Path(__file__).resolve().parent;out=Path(options.output).resolve();out.mkdir(parents=True,exist_ok=True)
s=(root/'src/bootstrap/xr_bootstrap.c').read_text(encoding='utf-8-sig')
def extract(name):
 m=re.search(r'^static (?:DWORD WINAPI|void) '+name+r'\([^\n]*\) \{.*?^\}',s,re.M|re.S)
 assert m,name
 return m.group()
prefix=r'''
#include <stdio.h>
#include <string.h>
#define WINAPI
#define BOUNDARY_TEST
#define GF2VR_DEFER_CONSUMERS_UNTIL_HMD
#define GF2VR_MAIN_TICK(n) main_tick(n)
#define GF2VR_READY_RUN(s,m) ready_run(s,m)
typedef unsigned DWORD;
typedef unsigned long long U64;
typedef void *HANDLE;typedef void *HINSTANCE;
#include "xr_bootstrap_state.h"
static unsigned bootstrap_started,stop_requested,published_phase,bootstrap_consumers_ready,first_tid=7;
static BootstrapApi worker_test_api;static unsigned worker_test_enabled=1;
static U64 now,connect_at,loss_at,stop_at=500000;static unsigned ticks,early,ready_calls,creates,destroys,systems,threads,closed,reports,fail_thread,loader_missing;
static unsigned thread_id=7;
static DWORD GetCurrentThreadId(void){return thread_id;}
static DWORD GetLastError(void){return 42;}
static void event(const char *s,U64 n,DWORD t){(void)s;(void)n;(void)t;}
static void main_tick(U64 n){(void)n;++ticks;if(!bootstrap_consumers_ready)++early;}
static void ready_run(void *s,void *m){(void)s;(void)m;++ready_calls;}
static void bootstrap_after_return(U64);
static void Sleep(unsigned ms){now+=ms;bootstrap_after_return(now+2);if(now>=stop_at)stop_requested=1;}
static HINSTANCE LoadLibraryExW(const unsigned short *s,HANDLE h,DWORD f){(void)s;(void)h;(void)f;return loader_missing?0:(void*)1;}
static void *GetProcAddress(HINSTANCE m,const char *s){(void)m;(void)s;return 0;}
#define GF2VR_XR_LOADER ((const unsigned short*)0)
static void bootstrap_report(const BootstrapState *s,DWORD e){(void)s;(void)e;++reports;}
static HANDLE CreateThread(void *a,U64 b,DWORD (*f)(void*),void *d,DWORD e,void *g){(void)a;(void)b;(void)f;(void)d;(void)e;(void)g;++threads;return fail_thread?0:(void*)1;}
static void CloseHandle(HANDLE h){(void)h;++closed;}
static XrResult XRAPI_PTR create(const XrInstanceCreateInfo *i,XrInstance *o){(void)i;++creates;*o=(XrInstance)1;return XR_SUCCESS;}
static XrResult XRAPI_PTR destroy(XrInstance i){(void)i;++destroys;return XR_SUCCESS;}
static XrResult XRAPI_PTR system_get(XrInstance i,const XrSystemGetInfo *g,XrSystemId *o){(void)i;(void)g;++systems;if(now<connect_at)return XR_ERROR_FORM_FACTOR_UNAVAILABLE;*o=1;return XR_SUCCESS;}
static XrResult XRAPI_PTR poll(XrInstance i,XrEventDataBuffer *e){(void)i;if(loss_at && now>=loss_at){e->type=XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING;return XR_SUCCESS;}return XR_EVENT_UNAVAILABLE;}
'''
main=r'''
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){printf("FAIL %u line%d: %s\n",checks,__LINE__,#x);return 1;}}while(0)
static void reset(void){bootstrap_started=stop_requested=published_phase=bootstrap_consumers_ready=0;now=0;connect_at=~0ULL;loss_at=0;stop_at=500000;ticks=early=ready_calls=creates=destroys=systems=threads=closed=reports=fail_thread=loader_missing=0;thread_id=7;worker_test_enabled=1;worker_test_api=(BootstrapApi){create,destroy,system_get,poll};}
int main(void){
 reset();bootstrap_after_return(1);CHECK(!threads && !ticks);bootstrap_after_return(2);CHECK(threads==1 && closed==1 && !ticks);
 bootstrap_worker(0);CHECK(!ticks && !early && !ready_calls);CHECK(creates==1 && destroys==1 && systems==500);CHECK(published_phase==BS_STOPPED && !bootstrap_consumers_ready);CHECK(threads==1);
 reset();connect_at=300000;bootstrap_worker(0);CHECK(bootstrap_consumers_ready && ticks>0 && !early);CHECK(systems==301 && creates==1);CHECK(ready_calls==200 && destroys==1);CHECK(threads==1);
 reset();connect_at=0;loss_at=1000;stop_at=3000;bootstrap_worker(0);CHECK(ready_calls==1 && creates==1 && destroys==1);CHECK(ticks==15 && bootstrap_consumers_ready);CHECK(published_phase==BS_STOPPED);
 reset();worker_test_enabled=0;loader_missing=1;stop_at=3000;bootstrap_worker(0);CHECK(!ticks && !ready_calls && !creates && !systems);CHECK(!bootstrap_consumers_ready && reports>=1);
 reset();fail_thread=1;bootstrap_after_return(2);CHECK(threads==1 && !closed && published_phase==BS_WORKER_FAILED && !ticks);bootstrap_after_return(3);CHECK(threads==1 && !ticks);
 reset();thread_id=8;bootstrap_after_return(2);CHECK(!threads && !ticks);thread_id=7;bootstrap_after_return(3);CHECK(threads==1);
 reset();bootstrap_consumers_ready=1;published_phase=BS_WAIT_RUNTIME;bootstrap_after_return(2);CHECK(ticks==1);published_phase=BS_RESTART_REQUIRED;bootstrap_after_return(3);CHECK(ticks==2);
 printf("PASS %u actual bootstrap worker/dispatch checks; 500s no-HMD, late connect, loss cleanup, loader/thread failure. Runtime APIs modeled.\n",checks);return 0;}
'''
fixture=prefix+'\n'+extract('bootstrap_worker')+'\n'+extract('bootstrap_after_return')+'\n'+main
(out/'actual-test.c').write_text(fixture,encoding='utf-8')
env=os.environ.copy();env['ZIG_GLOBAL_CACHE_DIR']=str(out/'zig-global-cache');env['ZIG_LOCAL_CACHE_DIR']=str(out/'zig-cache')
args=[options.zig,'cc','-target','x86_64-windows-gnu','-std=c11','-Wall','-Wextra','-Werror','-Wno-unused-function','-I'+str(root/'src/bootstrap'),'-I'+str(root/'third_party/openxr/include'),str(out/'actual-test.c'),'-o',str(out/'test.exe')]
subprocess.run(args,env=env,check=True);r=subprocess.run([str(out/'test.exe')],capture_output=True,text=True);print(r.stdout,end='');(out/'result.txt').write_text(r.stdout);raise SystemExit(r.returncode)