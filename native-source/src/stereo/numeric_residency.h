#ifndef GF2_NUMERIC_RESIDENCY_H
#define GF2_NUMERIC_RESIDENCY_H
#ifdef GF2VR_NUMERIC_RESIDENCY
enum { NR_OFFER=1,NR_OFFER_REJECT,NR_RESERVE,NR_RESERVE_REJECT,NR_CPU,
 NR_RENDER_DRAIN,NR_COPY_DRAIN,NR_TRANSFER,NR_TAKE,NR_RETIRE,NR_CANCEL,NR_STOP,NR_BEGIN,NR_ISSUE };
enum { NR_REASON_NONE,NR_REASON_TOKEN,NR_REASON_GENERATION,NR_REASON_FRAME,
 NR_REASON_TIME,NR_REASON_ONE_AHEAD,NR_REASON_FULL,NR_REASON_NO_ELIGIBLE,
 NR_REASON_SERIAL,NR_REASON_EVENT_LIMIT,NR_REASON_BUSY };
typedef struct {
 unsigned long long owner,qpc,frame,generation,epoch,serial,last_frame,last_begun;
 unsigned type,slot,result,reason,free_mask,eligible_mask,flags;
#ifdef GF2VR_NUMERIC_LOSS_OBSERVE
 unsigned loss_reason; /* fills original row tail padding; no product capacity change */
#endif
} NumericResidencyRow;
int numeric_residency_read(unsigned *,NumericResidencyRow *);
unsigned numeric_residency_count(void);
unsigned numeric_residency_overflow(void);
unsigned numeric_residency_busy(unsigned);
#ifdef GF2VR_NUMERIC_LOSS_OBSERVE
 enum { NR_LOSS=15 };
 void numeric_residency_loss(unsigned long long,unsigned long long,unsigned long long,
  unsigned long long,unsigned long long,unsigned,unsigned,int,unsigned);
#endif
#endif
#endif
