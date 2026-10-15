#ifndef GF2_NUMERIC_RESIDENCY_PERSIST_H
#define GF2_NUMERIC_RESIDENCY_PERSIST_H
/* Observer only. XR submits a bounded work item; file IO belongs to that item.
   Static immutable records/module live until process exit. Never join at exit. */
#ifndef GF2VR_NUMERIC_RESIDENCY
#error Persistence requires numeric residency
#endif
#include "numeric_residency.h"
#ifndef NRP_COUNTER
__declspec(dllimport) int WINAPI QueryPerformanceCounter(long long *);
#define NRP_COUNTER QueryPerformanceCounter
#endif
#ifndef NRP_FREQUENCY
#define NRP_FREQUENCY QueryPerformanceFrequency
#endif
#ifndef NRP_SUBMIT
/* Opaque x64 Win32 callback/instance/environment ABI; no SDK structure access. */
typedef void (WINAPI *NrpCallback)(void *,void *);
__declspec(dllimport) BOOL WINAPI TrySubmitThreadpoolCallback(NrpCallback,void *,void *);
#define NRP_SUBMIT TrySubmitThreadpoolCallback
#endif
#ifndef NRP_OPEN
#define NRP_OPEN CreateFileW
#endif
#ifndef NRP_WRITE
#define NRP_WRITE WriteFile
#endif
#define NRP_PERIODIC 1u
#define NRP_PUBLICATION_OFF 2u
#define NRP_CLOSE 4u
#define NRP_START 8u
static unsigned nrp_active,nrp_pending,nrp_cursor_published;
static unsigned nrp_submit_failures,nrp_io_failures,nrp_busy;
/* Only the XR caller owns these clock/publication fields. */
static U64 nrp_due,nrp_retry_due,nrp_prior_generation;
static unsigned nrp_started;
/* Only the single work item owns cursor/checkpoint/buffer. */
static unsigned nrp_cursor,nrp_checkpoint,nrp_partial;
static char nrp_buffer[65536];
/* The legacy shared decimal formatter takes DWORD: never truncate QPC/tokens/pointers. */
static char *nrp_decimal(char *p,U64 value){char digits[20];unsigned n=0;do{digits[n++]=(char)('0'+value%10);value/=10;}while(value);while(n)*p++=digits[--n];return p;}
static char *nrp_prefix(char *p,const char *kind,unsigned index){
 p=literal(p,"{\"probe\":\"" GF2VR_OBSERVER_ID "\",\"event\":\"");p=literal(p,kind);
 p=literal(p,"\",\"pid\":");p=nrp_decimal(p,GetCurrentProcessId());
 p=literal(p,",\"tid\":");p=nrp_decimal(p,GetCurrentThreadId());
 p=literal(p,",\"sample\":");p=nrp_decimal(p,index);
 p=literal(p,",\"tick_ms\":");return hexadecimal(p,GetTickCount64());
}
static int nrp_write(HANDLE file,char *data,unsigned bytes){
 DWORD written=0;BOOL ok=NRP_WRITE(file,data,bytes,&written,NULL);
 if(!ok||written!=bytes){__atomic_fetch_add(&nrp_io_failures,1,__ATOMIC_RELAXED);if(written)nrp_partial=1;return 0;}
 return 1;
}
static void WINAPI nrp_worker(void *instance,void *opaque){
 (void)instance;(void)opaque;DWORD saved=GetLastError();
 unsigned reasons=__atomic_exchange_n(&nrp_pending,0,__ATOMIC_ACQ_REL);
 unsigned target=numeric_residency_count();long long frequency=0,anchor=0;
 NRP_FREQUENCY(&frequency);NRP_COUNTER(&anchor);U64 tick=GetTickCount64();
 HANDLE file=NRP_OPEN(GF2VR_PROBE_LOG,FILE_APPEND_DATA,7,NULL,OPEN_ALWAYS,128,NULL);
 if(file==INVALID_HANDLE_VALUE){__atomic_fetch_add(&nrp_io_failures,1,__ATOMIC_RELAXED);goto done;}
 if(nrp_partial){char newline='\n';if(!nrp_write(file,&newline,1))goto close;nrp_partial=0;}
 ++nrp_checkpoint;
 unsigned read=nrp_cursor,bytes=0;NumericResidencyRow r;int failed=0;
 while(read<target){unsigned index=read;if(!numeric_residency_read(&read,&r))break;
  char line[1200],*p=nrp_prefix(line,"numeric_residency",index);
  #ifdef GF2VR_NUMERIC_LOSS_OBSERVE
  p=literal(p,",\"schema\":2,\"frequency\":");
#else
  p=literal(p,",\"schema\":1,\"frequency\":");
#endif
  p=nrp_decimal(p,frequency>0?(U64)frequency:0);
  p=literal(p,",\"record_index\":");p=nrp_decimal(p,index);
#define NRP_FIELD(name) do{p=literal(p,",\"" #name "\":");p=nrp_decimal(p,r.name);}while(0)
  NRP_FIELD(owner);NRP_FIELD(qpc);NRP_FIELD(frame);NRP_FIELD(generation);NRP_FIELD(epoch);NRP_FIELD(serial);
  NRP_FIELD(last_frame);NRP_FIELD(last_begun);NRP_FIELD(type);NRP_FIELD(slot);NRP_FIELD(result);NRP_FIELD(reason);
  NRP_FIELD(free_mask);NRP_FIELD(eligible_mask);NRP_FIELD(flags);
#ifdef GF2VR_NUMERIC_LOSS_OBSERVE
  NRP_FIELD(loss_reason);
#endif
#undef NRP_FIELD
  p=literal(p,"}\n");unsigned length=(unsigned)(p-line);
  if(bytes+length>sizeof(nrp_buffer)){if(!nrp_write(file,nrp_buffer,bytes)){failed=1;break;}nrp_cursor=index;__atomic_store_n(&nrp_cursor_published,nrp_cursor,__ATOMIC_RELEASE);bytes=0;}
  for(unsigned i=0;i<length;++i)nrp_buffer[bytes++]=line[i];
 }
 if(!failed&&bytes){if(nrp_write(file,nrp_buffer,bytes)){nrp_cursor=read;__atomic_store_n(&nrp_cursor_published,nrp_cursor,__ATOMIC_RELEASE);}else failed=1;}
 {char line[1100],*p=nrp_prefix(line,"numeric_residency_summary",nrp_checkpoint);
#define NRP_VALUE(name,value) do{p=literal(p,",\"" name "\":");p=nrp_decimal(p,(U64)(value));}while(0)
  NRP_VALUE("rows_claimed",numeric_residency_count());NRP_VALUE("rows_dumped",nrp_cursor);
  NRP_VALUE("request_target",target);NRP_VALUE("overflow",numeric_residency_overflow());
  NRP_VALUE("offer_busy",numeric_residency_busy(1));NRP_VALUE("reserve_busy",numeric_residency_busy(0));
  NRP_VALUE("persistence_schema",1);NRP_VALUE("checkpoint",nrp_checkpoint);NRP_VALUE("trigger_mask",reasons);
  NRP_VALUE("checkpoint_qpc",anchor>0?(U64)anchor:0);NRP_VALUE("checkpoint_tick_ms",tick);
  NRP_VALUE("frequency",frequency>0?(U64)frequency:0);NRP_VALUE("capacity",262144);
  NRP_VALUE("submit_failures",__atomic_load_n(&nrp_submit_failures,__ATOMIC_ACQUIRE));
  NRP_VALUE("io_failures",__atomic_load_n(&nrp_io_failures,__ATOMIC_ACQUIRE));
  NRP_VALUE("request_busy",__atomic_load_n(&nrp_busy,__ATOMIC_ACQUIRE));NRP_VALUE("chunk_failed",failed);
#undef NRP_VALUE
  p=literal(p,"}\n");(void)nrp_write(file,line,(unsigned)(p-line));}
close: CloseHandle(file);
done: __atomic_store_n(&nrp_active,0,__ATOMIC_RELEASE);SetLastError(saved);
}
static void nrp_request(unsigned reason,U64 now){
 DWORD saved=GetLastError();__atomic_fetch_or(&nrp_pending,reason,__ATOMIC_RELEASE);
 if(now<nrp_retry_due){SetLastError(saved);return;}
 unsigned zero=0;if(!__atomic_compare_exchange_n(&nrp_active,&zero,1,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE)){
  __atomic_fetch_add(&nrp_busy,1,__ATOMIC_RELAXED);SetLastError(saved);return;}
 if(!NRP_SUBMIT(nrp_worker,NULL,NULL)){__atomic_fetch_add(&nrp_submit_failures,1,__ATOMIC_RELAXED);nrp_retry_due=now+1000;__atomic_store_n(&nrp_active,0,__ATOMIC_RELEASE);}
 else nrp_retry_due=0;
 SetLastError(saved);
}
static void numeric_residency_persist_tick(U64 generation){
 DWORD saved=GetLastError();U64 now=GetTickCount64();unsigned reason=0;
 if(!nrp_started){nrp_started=1;nrp_due=now+10000;reason|=NRP_START;}
 else if(now>=nrp_due){nrp_due=now+10000;reason|=NRP_PERIODIC;}
 if(nrp_prior_generation&&!generation)reason|=NRP_PUBLICATION_OFF;
 nrp_prior_generation=generation;
 if(reason)nrp_request(reason,now);
 else if(__atomic_load_n(&nrp_pending,__ATOMIC_ACQUIRE)&&!__atomic_load_n(&nrp_active,__ATOMIC_ACQUIRE))nrp_request(0,now);
 SetLastError(saved);
}
static void numeric_residency_dump(void){nrp_request(NRP_CLOSE,GetTickCount64());}
#endif
