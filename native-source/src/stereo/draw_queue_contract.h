#ifndef GF2_DRAW_QUEUE_CONTRACT_H
#define GF2_DRAW_QUEUE_CONTRACT_H
/* Build-specific necessary ordering guard, NOT an eye completion certificate.
   The caller obtains current TLS graphics on its checked Unity owner. Borrowed
   identities may be compared only inside that original Render/Submit scope.
   dispatch_state must come from the checked getter7e5130;
   its +474 bypass flag redirects mode4 away from queued packet27ea.
   Mode 3 (job packet27eb) and arbitrary child implementations are unproved.
   Exact target-image production remains an independent publication prerequisite. */
typedef unsigned long long DrawQueueAddress;
typedef int (*DrawQueueRead)(void *,DrawQueueAddress,void *,unsigned);
typedef struct {
    DrawQueueAddress graphics,queue,child,dispatch_state;
    unsigned mode;
} DrawQueueIdentity;
static int draw_queue_word(DrawQueueRead read,void *owner,DrawQueueAddress base,
                           unsigned offset,void *out,unsigned size) {
    return base && base<=~(DrawQueueAddress)0-offset-size &&
        read(owner,base+offset,out,size);
}
static int draw_queue_read(DrawQueueRead read,void *owner,DrawQueueAddress unity,
                          DrawQueueAddress graphics,DrawQueueAddress dispatch_state,DrawQueueIdentity *out) {
    if(!out)return 0;
    *out=(DrawQueueIdentity){0};
    if(!read || !unity || unity>~(DrawQueueAddress)0-0x184e964) return 0;
    DrawQueueAddress table=0,child_table=0,method=0,queue=0,child=0;
    unsigned mode=0;unsigned char queued=0,bypass=0;
    /* Mode2: 4fa4d9 JA ->4fa4f0, JNE ->4fa902, synchronous8a56f0
       on the wrapper itself; ordinary draw commands enter its queue before
       the later event. Mode4 uses27ea and its checked synchronous child.
       14d5ce8+970 is4fa490; 14d5ce8+790 is4fc990 (callback+ID enqueue). */
    if(!draw_queue_word(read,owner,graphics,0,&table,8) || table!=unity+0x14d5ce8 ||
       !draw_queue_word(read,owner,table,0x970,&method,8) || method!=unity+0x4fa490 ||
       !draw_queue_word(read,owner,table,0x790,&method,8) || method!=unity+0x4fc990 ||
       !draw_queue_word(read,owner,graphics,0x2811,&queued,1) || !queued ||
       !draw_queue_word(read,owner,unity,0x184e960,&mode,4) || (mode!=2 && mode!=4) ||
       !draw_queue_word(read,owner,graphics,0x2818,&queue,8) || !queue ||
       !draw_queue_word(read,owner,graphics,0x2808,&child,8) || !child || child==graphics) return 0;
    if(mode==4 && (!draw_queue_word(read,owner,dispatch_state,0x474,&bypass,1) || bypass ||
       !draw_queue_word(read,owner,child,0,&child_table,8) ||
       !draw_queue_word(read,owner,child_table,0x970,&method,8) || method!=unity+0x8a56f0)) return 0;
    *out=(DrawQueueIdentity){graphics,queue,child,mode==4?dispatch_state:0,mode};return 1;
}
static int draw_queue_same(DrawQueueIdentity a,DrawQueueIdentity b) {
    return a.graphics && a.queue && a.child && a.graphics==b.graphics &&
        a.queue==b.queue && a.child==b.child && (a.mode==2 || a.mode==4) && a.mode==b.mode &&
        a.dispatch_state==b.dispatch_state;
}
#endif
