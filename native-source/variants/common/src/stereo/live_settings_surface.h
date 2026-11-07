#ifndef GF2VR_LIVE_SETTINGS_SURFACE_H
#define GF2VR_LIVE_SETTINGS_SURFACE_H
#include <stddef.h>
enum { LSS_WIDTH=1280,LSS_HEIGHT=960,LSS_PIXELS=1280*960*4,LSS_HEADER=4096,LSS_EVENTS=64 };
enum { LSS_FREE,LSS_WRITING,LSS_READY,LSS_READING };
typedef struct {unsigned type;int x,y;unsigned value;unsigned long long session,reserved;} LiveSettingsInput;
typedef struct {
    unsigned magic,version,pid,width,height,stride;
    unsigned long long session,instance,heartbeat;
    unsigned reserved[4],slots[2];
    unsigned long long sequences[2],sequence;
    unsigned write,read,flags,mode;
    unsigned input_reset,toggle,redraw,reserved_end;
    LiveSettingsInput events[LSS_EVENTS];
    unsigned char tail[LSS_HEADER-128-LSS_EVENTS*sizeof(LiveSettingsInput)];
    unsigned char pixels[2][LSS_PIXELS];
} LiveSettingsSurface;
_Static_assert(offsetof(LiveSettingsSurface,slots)==64,"settings slot ABI");
_Static_assert(offsetof(LiveSettingsSurface,write)==96,"settings input ABI");
_Static_assert(offsetof(LiveSettingsSurface,events)==128,"settings ring ABI");
_Static_assert(offsetof(LiveSettingsSurface,pixels)==4096,"settings pixel ABI");
/* One XR writer, one host reader. Overflow drops the event and invalidates the
   pointer through an epoch/session change at the caller; never overwrite input. */
static int live_settings_surface_input(LiveSettingsSurface *s,unsigned type,int x,int y,unsigned value) {
    unsigned write=__atomic_load_n(&s->write,__ATOMIC_RELAXED),read=__atomic_load_n(&s->read,__ATOMIC_ACQUIRE);
    if(write-read>=LSS_EVENTS){__atomic_add_fetch(&s->session,1,__ATOMIC_ACQ_REL);return 0;}
    s->events[write%LSS_EVENTS]=(LiveSettingsInput){type,x,y,value,s->session,0};
    __atomic_store_n(&s->write,write+1,__ATOMIC_RELEASE);return 1;
}
static int live_settings_surface_claim(LiveSettingsSurface *s) {
    unsigned long long best=0;int selected=-1;
    for(unsigned i=0;i<2;++i)if(__atomic_load_n(&s->slots[i],__ATOMIC_ACQUIRE)==LSS_READY) {
        unsigned long long seq=__atomic_load_n(&s->sequences[i],__ATOMIC_ACQUIRE);
        if(seq>best){best=seq;selected=(int)i;}
    }
    if(selected<0)return -1;
    unsigned expected=LSS_READY;
    if(!__atomic_compare_exchange_n(&s->slots[selected],&expected,LSS_READING,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE))return -1;
    // Discard an older queued frame only by claiming its READY ownership.
    unsigned other=(unsigned)selected^1u;expected=LSS_READY;
    if(__atomic_compare_exchange_n(&s->slots[other],&expected,LSS_READING,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE))
        __atomic_store_n(&s->slots[other],s->sequences[other]<best?LSS_FREE:LSS_READY,__ATOMIC_RELEASE);
    return selected;
}
static void live_settings_surface_release(LiveSettingsSurface *s,unsigned slot) {
    if(slot<2)__atomic_store_n(&s->slots[slot],LSS_FREE,__ATOMIC_RELEASE);
}
#endif
