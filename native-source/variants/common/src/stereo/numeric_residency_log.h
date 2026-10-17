#ifndef GF2_NUMERIC_RESIDENCY_LOG_H
#define GF2_NUMERIC_RESIDENCY_LOG_H
#ifdef GF2VR_NUMERIC_RESIDENCY
#include "numeric_residency.h"
#ifdef GF2VR_NUMERIC_RESIDENCY_PERSIST
#include "numeric_residency_persist.h"
#else
#line 5
static void numeric_residency_dump(void){
 static unsigned cursor;DWORD saved=GetLastError();long long frequency=0;QueryPerformanceFrequency(&frequency);
 NumericResidencyRow r;
 while(numeric_residency_read(&cursor,&r)){
  char line[1200],*p=source_prefix(line,"numeric_residency");
#define NRF(name) do{p=literal(p,",\"" #name "\":");p=decimal(p,r.name);}while(0)
  p=literal(p,",\"schema\":1,\"frequency\":");p=decimal(p,frequency>0?(U64)frequency:0);
  NRF(owner);NRF(qpc);NRF(frame);NRF(generation);NRF(epoch);NRF(serial);
  NRF(last_frame);NRF(last_begun);NRF(type);NRF(slot);NRF(result);NRF(reason);
  NRF(free_mask);NRF(eligible_mask);NRF(flags);
#undef NRF
  p=literal(p,"}\n");source_log(line,p);
 }
 char line[500],*p=source_prefix(line,"numeric_residency_summary");
 p=literal(p,",\"rows_claimed\":");p=decimal(p,numeric_residency_count());
 p=literal(p,",\"rows_dumped\":");p=decimal(p,cursor);
 p=literal(p,",\"overflow\":");p=decimal(p,numeric_residency_overflow());
 p=literal(p,",\"offer_busy\":");p=decimal(p,numeric_residency_busy(1));
 p=literal(p,",\"reserve_busy\":");p=decimal(p,numeric_residency_busy(0));
 p=literal(p,"}\n");source_log(line,p);SetLastError(saved);
}
#endif /* persistence vs legacy */
#endif
#endif
