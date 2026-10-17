#ifndef GF2_XR_EYE_MAILBOX_H
#define GF2_XR_EYE_MAILBOX_H
#include "xr_eye_input.h"
/* Single XR writer, one Unity reader. Nonblocking lock protects the struct
   copy; independent atomic epoch/frame invalidation defeats an old slot even
   when the reader temporarily owns the copy lock. No handle/pointer ownership
   is transferred. Final eye publication must still recheck these tokens. */
typedef struct {
    unsigned locked;
    unsigned long long epoch,frame;
    XrEyeInput packet;
} XrEyeMailbox;
static int xr_eye_mailbox_lock(XrEyeMailbox *m) {
    unsigned unlocked=0;
    return __atomic_compare_exchange_n(&m->locked,&unlocked,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED);
}
static void xr_eye_mailbox_invalidate(XrEyeMailbox *m) {
    __atomic_store_n(&m->epoch,0,__ATOMIC_RELEASE);
    __atomic_store_n(&m->frame,0,__ATOMIC_RELEASE);
}
static int xr_eye_mailbox_publish(XrEyeMailbox *m,const XrEyeInput *packet) {
    xr_eye_mailbox_invalidate(m);
    if(!packet || !xr_eye_matches(packet,packet->reference_epoch,packet->frame_id,packet->display_time) ||
       !xr_eye_mailbox_lock(m)) return 0;
    m->packet=*packet;
    __atomic_store_n(&m->frame,packet->frame_id,__ATOMIC_RELEASE);
    __atomic_store_n(&m->epoch,packet->reference_epoch,__ATOMIC_RELEASE);
    __atomic_store_n(&m->locked,0,__ATOMIC_RELEASE);return 1;
}
static int xr_eye_mailbox_current(const XrEyeMailbox *m,const XrEyeInput *packet) {
    return packet && packet->valid && packet->reference_epoch && packet->frame_id &&
        __atomic_load_n(&m->epoch,__ATOMIC_ACQUIRE)==packet->reference_epoch &&
        __atomic_load_n(&m->frame,__ATOMIC_ACQUIRE)==packet->frame_id;
}
static int xr_eye_mailbox_read(XrEyeMailbox *m,XrEyeInput *out) {
    if(!out) return 0;
    *out=(XrEyeInput){0};
    if(!xr_eye_mailbox_lock(m)) return 0;
    XrEyeInput packet=m->packet;
    int current=xr_eye_mailbox_current(m,&packet);
    __atomic_store_n(&m->locked,0,__ATOMIC_RELEASE);
    if(!current) return 0;
    *out=packet;return 1;
}
#endif
