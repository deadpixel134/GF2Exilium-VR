#ifndef GF2_PRODUCER_PENDING_H
#define GF2_PRODUCER_PENDING_H
#include <stdint.h>
#include <stddef.h>
/* Exact current Unity Submit context layout (UnityPlayer bytes are bound by
   the adapter). Read twice while borrowed on its owner. Never touch entries,
   mutate the context, flush it, or retain it past the current root. */
typedef struct {uint64_t entries,reserved,count,capacity;} PsnPendingHeader;
typedef int (*PsnRead)(void *owner,uint64_t address,void *bytes,size_t size);
static int psn_pending_empty(uint64_t context,PsnRead read,void *owner) {
    PsnPendingHeader a,b;
    if(!context || context>UINT64_MAX-0x10030u-sizeof(a) || !read)return 0;
    uint64_t address=context+0x10030u;
    if(!read(owner,address,&a,sizeof(a)) || a.count ||
       !read(owner,address,&b,sizeof(b)))return 0;
    return b.count==0 && a.entries==b.entries && a.reserved==b.reserved && a.capacity==b.capacity;
}
#endif
