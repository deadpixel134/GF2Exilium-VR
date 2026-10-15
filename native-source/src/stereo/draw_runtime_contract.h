#ifndef GF2_DRAW_RUNTIME_CONTRACT_H
#define GF2_DRAW_RUNTIME_CONTRACT_H
#include "draw_queue_contract.h"
/* Target-specific runtime adapter shared by production ordering checks.
   No sample count/time limit and no presentation/freshness certificate. */
static int draw_runtime_read(void *owner,DrawQueueAddress address,void *out,unsigned size) {
    (void)owner;U64 bytes=0;
    return address && address<=~(U64)0-size &&
        ReadProcessMemory((HANDLE)(long long)-1,(void *)address,out,size,&bytes) && bytes==size;
}
static void *draw_runtime_graphics(void) {
    static const unsigned char code[]={0x8b,0x0d,0x8a,0xf1,0xf0,0x00,0x48,0xff,0x25,0xb3,0x3d,0xbb,0x00};
    unsigned char actual[13];
    if(!unity || !draw_runtime_read(0,(U64)unity+0x8a5af0,actual,13)) return 0;
    for(unsigned i=0;i<13;++i) if(actual[i]!=code[i]) return 0;
    return ((void *(*)(void))(unity+0x8a5af0))();
}
#ifdef BOUNDARY_TEST
static int (*draw_runtime_test_identity)(DrawQueueIdentity *);
#endif
static int draw_runtime_identity(DrawQueueIdentity *out) {
    DrawQueueAddress state=0;
    *out=(DrawQueueIdentity){0};
#ifdef BOUNDARY_TEST
    if(draw_runtime_test_identity) return draw_runtime_test_identity(out);
#endif
    return unity && draw_runtime_read(0,(U64)unity+0x17ae540,&state,8) &&
        draw_queue_read(draw_runtime_read,0,(U64)unity,(U64)draw_runtime_graphics(),state,out);
}
#endif
