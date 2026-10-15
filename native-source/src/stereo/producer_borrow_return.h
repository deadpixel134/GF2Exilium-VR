#ifndef GF2_PRODUCER_BORROW_RETURN_H
#define GF2_PRODUCER_BORROW_RETURN_H
#include "producer_scope_native.h"
/* One scalar request, valid only inside the existing selected outer borrow.
   No resource, completion, queue, or publication authority lives here. */
typedef struct {
    PsnToken token;
    uint64_t context;
    int camera;
    unsigned pending,consumed,cancelled,request_depth,reported;
} ProducerBorrowReturn;
static int pbr_defer(ProducerBorrowReturn *s,PsnToken t,uint64_t context,int camera,
                     unsigned depth,unsigned submit,unsigned ambiguous,int complete) {
    if(!s || s->pending || s->consumed || s->cancelled || s->request_depth ||
       !psn_same(t,t) || !context || !camera || depth<=1 || submit || ambiguous || !complete)return 0;
    *s=(ProducerBorrowReturn){.token=t,.context=context,.camera=camera,.pending=1,.request_depth=depth};
    return 1;
}
static void pbr_cancel(ProducerBorrowReturn *s) {
    if(s && s->pending){s->pending=0;s->cancelled=1;}
}
static int pbr_take(ProducerBorrowReturn *s,PsnToken t,uint64_t context,unsigned depth,
                    unsigned submit,unsigned ambiguous,int valid,int *camera) {
    if(!s || !s->pending)return 0;
    if(depth!=1){if(!depth)pbr_cancel(s);return 0;}
    s->pending=0;s->consumed=1;
    if(!valid || !psn_same(s->token,t) || s->context!=context || submit || ambiguous){s->cancelled=1;return 0;}
    *camera=s->camera;return 1;
}
#endif
