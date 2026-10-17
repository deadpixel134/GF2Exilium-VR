#ifndef GF2_PRODUCER_ENGINE_TRANSPORT_H
#define GF2_PRODUCER_ENGINE_TRANSPORT_H
#include <stdint.h>
#include <stddef.h>
#define PEN_EVENT_ID 0x65315031
#define PEN_MAGIC 0x50534231
enum {PEN_SERIAL,PEN_OPERATION,PEN_CAMERA,PEN_NONCE,PEN_RESULT,PEN_ACK,PEN_FIELDS};
typedef struct {
    uint64_t owner,cookie,last_nonce;
    unsigned attempted,ready,busy,poison;
} ProducerEngineTransport;
typedef struct {
    void *context;
    int (*get)(void *,unsigned,void *,size_t);
    int (*set)(void *,unsigned,const void *,size_t);
} PenFields;
typedef int (*PenNotify)(void *,int64_t,int,int);
static inline size_t pen_size(unsigned id) {
    return id<PEN_FIELDS?(id==PEN_OPERATION || id==PEN_CAMERA || id==PEN_RESULT?4:8):0;
}
static int pen_initialize(ProducerEngineTransport *s,uint64_t owner,uint64_t cookie) {
    if(!s || !owner || !cookie || __atomic_load_n(&s->owner,__ATOMIC_ACQUIRE))return 0;
    s->cookie=cookie;__atomic_store_n(&s->owner,owner,__ATOMIC_RELEASE);return 1;
}
/* Only the exact reserved engine callback/ID may enter this core. Foreign
   callers inspect owner only. Fields are owned primitive statics accessed on
   that owner; no raw managed-object/static-storage layout is inferred. */
static int pen_request(ProducerEngineTransport *s,uint64_t thread,const PenFields *api,
                       PenNotify notify,void *context) {
    if(!s || !thread || __atomic_load_n(&s->owner,__ATOMIC_ACQUIRE)!=thread)return 0;
    if(s->busy){s->poison=1;return 0;}
    if(!api || !api->get || !api->set || !notify)return 0;
    s->busy=1;s->poison=0;
    int64_t serial=0,nonce=0,again=0,zero=0;
    int32_t operation=0,camera=0,result=0;
    int answered=0;
    if(!api->get(api->context,PEN_NONCE,&nonce,8) || nonce<=0 ||
       (uint64_t)nonce<=s->last_nonce ||
       !api->get(api->context,PEN_SERIAL,&serial,8) ||
       !api->get(api->context,PEN_OPERATION,&operation,4) ||
       !api->get(api->context,PEN_CAMERA,&camera,4) ||
       !api->get(api->context,PEN_NONCE,&again,8) || nonce!=again || s->poison)goto done;
    s->last_nonce=(uint64_t)nonce;
    if(operation==0) {
        if(s->attempted)goto reply;
        s->attempted=1;
        if((uint64_t)serial==s->cookie && camera==PEN_MAGIC) {s->ready=1;result=PEN_MAGIC;}
    } else if(s->ready)result=notify(context,serial,operation,camera);
reply:
    if(s->poison)result=0;
    if(!api->set(api->context,PEN_RESULT,&result,4) ||
       !api->set(api->context,PEN_ACK,&nonce,8) || s->poison) {
        (void)api->set(api->context,PEN_ACK,&zero,8);
        result=0;(void)api->set(api->context,PEN_RESULT,&result,4);
    } else answered=1;
done:
    s->busy=0;
    return answered;
}
#endif
