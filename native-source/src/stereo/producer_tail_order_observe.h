#ifndef GF2_PRODUCER_TAIL_ORDER_OBSERVE_H
#define GF2_PRODUCER_TAIL_ORDER_OBSERVE_H
/* Scalar trace only. PTO_RESULT evaluates the original expression exactly
   once and only when its surrounding original short-circuit reaches it. */
typedef struct {
    unsigned observed,reported,depth,submit_depth,ambiguous;
    unsigned identity_reached,queue_reached,issuer_reached;
    int identity_result,queue_result,issuer_result;
    DrawQueueIdentity expected,actual;
} ProducerTailOrderObservation;
#define PTO_RESULT(o,kind,expression) \
    ((o)->kind##_reached=1,(o)->kind##_result=(expression))
enum {PTO_UNOBSERVED=0,PTO_DEPTH=1,PTO_SUBMIT_DEPTH=2,PTO_AMBIGUOUS=3,
      PTO_IDENTITY=4,PTO_QUEUE=5,PTO_ISSUER=6,PTO_ORDERED=7};
static unsigned pto_first(const ProducerTailOrderObservation *o) {
    if(!o || !o->observed)return PTO_UNOBSERVED;
    if(o->depth!=1)return PTO_DEPTH;
    if(o->submit_depth)return PTO_SUBMIT_DEPTH;
    if(o->ambiguous)return PTO_AMBIGUOUS;
    if(!o->identity_reached)return PTO_UNOBSERVED;
    if(!o->identity_result)return PTO_IDENTITY;
    if(!o->queue_reached)return PTO_UNOBSERVED;
    if(!o->queue_result)return PTO_QUEUE;
    if(!o->issuer_reached)return PTO_UNOBSERVED;
    return o->issuer_result?PTO_ORDERED:PTO_ISSUER;
}
#endif
