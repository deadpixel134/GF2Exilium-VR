#ifndef GF2_NUMERIC_LOSS_OBSERVE_H
#define GF2_NUMERIC_LOSS_OBSERVE_H
#ifdef GF2VR_NUMERIC_LOSS_OBSERVE
#if !defined(GF2VR_NUMERIC_RESIDENCY) || !defined(GF2VR_NUMERIC_RESIDENCY_PERSIST)
#error Loss observation requires the existing immutable residency ring and async persistence
#endif
#include "eye_flight_owner.h"
#include "numeric_residency.h"
enum {NLO_FRAME=1,NLO_BANK,NLO_READY,NLO_ARM,NLO_ROOT,NLO_DISCARD,NLO_PROJECTION,NLO_END,NLO_FINALIZE,NLO_VALID,NLO_PUBLISH};
/* Only scalar copies cross this boundary. owner is an opaque correlation tag:
   numeric-owner for frame/main records, XR-storage owner for receipt terminal
   records. Join transferred receipts by PID/token/serial, NEVER current slot. */
static inline void nlo_record(const void *owner,EyeFlightToken token,unsigned long long serial,
 unsigned boundary,unsigned reason,int result,unsigned flags){
 numeric_residency_loss((unsigned long long)owner,token.frame,token.generation,token.epoch,serial,
  boundary,reason,result,flags);
}
static inline int nlo_test(unsigned *reason,unsigned value,int predicate){
 if(predicate)*reason=value;return predicate;
}
#define NLO_RECORD(o,t,s,b,c,r,f) nlo_record(o,t,s,b,c,r,f)
#define NLO_SNAPSHOT(o,s,b,c,r,f) nlo_record(o,(s)->token,(s)->ticket.serial,b,c,r,f)
#define NLO_DRIVER(d,b,c,r,f) nlo_record((d)->owner,(EyeFlightToken){(d)->predicted,(d)->requested_generation,(d)->requested_epoch},0,b,c,r,f)
#define NLO_TEST(reason,predicate) nlo_test(&nlo_reason,reason,(predicate))
#else
#define NLO_RECORD(o,t,s,b,c,r,f) ((void)0)
#define NLO_SNAPSHOT(o,s,b,c,r,f) ((void)0)
#define NLO_DRIVER(d,b,c,r,f) ((void)0)
#define NLO_TEST(reason,predicate) (predicate)
#endif
#endif
