#ifndef GF2_ACTIVITY_UI_CAMERA_REGISTRY_H
#define GF2_ACTIVITY_UI_CAMERA_REGISTRY_H
/* Exact script-free private UI objects only. Independent pins outlive group
   retirement/deferred Destroy; a dead object still in an original raw input
   remains excluded. No native pointer, name/role/layer-based exclusion. */
enum {UICR_CAPACITY=64};
static struct {
    unsigned owner,live;
    struct {
        void *camera,*group;U64 generation;unsigned root,retired;
        int instance;ManagedEyeApi calls;
        unsigned native_entered,native_returned,native_reported;
    } rows[UICR_CAPACITY];
} uicr;
static unsigned uicr_thread(void){return __atomic_load_n(&uicr.owner,__ATOMIC_ACQUIRE);}
static int uicr_owned(void *camera) {
    unsigned owner=uicr_thread();if(!owner)return 0;
    if(GetCurrentThreadId()!=owner)return -1;
    for(unsigned i=0;i<UICR_CAPACITY;++i)
        if(uicr.rows[i].root && uicr.rows[i].camera==camera)return 1;
    return 0;
}
/* Active eligibility belongs to this exact camera/group/generation, while
   uicr_owned also deliberately includes retired input-exclusion tombstones. */
static int uicr_current(void *camera,void *group,U64 generation) {
    if(!camera || !group || !generation || GetCurrentThreadId()!=uicr_thread())return 0;
    for(unsigned i=0;i<UICR_CAPACITY;++i)
        if(uicr.rows[i].root && !uicr.rows[i].retired && uicr.rows[i].camera==camera &&
           uicr.rows[i].group==group && uicr.rows[i].generation==generation)return 1;
    return 0;
}
static int uicr_add(void *group,U64 generation,const ManagedEyeApi *a,void *camera,int instance) {
    if(!group || !generation || !a || !camera || !instance || !a->pin || !a->unpin ||
       !a->object_class || a->object_class(camera)!=a->camera_class)return 0;
    unsigned thread=GetCurrentThreadId();
    unsigned owner=uicr_thread();if(!thread || (owner && owner!=thread))return 0;
    unsigned free_slot=UICR_CAPACITY;
    for(unsigned i=0;i<UICR_CAPACITY;++i) {
        if(uicr.rows[i].root && uicr.rows[i].camera==camera)
            return !uicr.rows[i].retired && uicr.rows[i].group==group &&
                uicr.rows[i].generation==generation && uicr.rows[i].instance==instance;
        if(!uicr.rows[i].root && free_slot==UICR_CAPACITY)free_slot=i;
    }
    if(free_slot==UICR_CAPACITY)return 0;
    unsigned root=a->pin(camera,1);if(!root)return 0;
    __atomic_store_n(&uicr.owner,thread,__ATOMIC_RELEASE);uicr.rows[free_slot].camera=camera;uicr.rows[free_slot].group=group;
    uicr.rows[free_slot].generation=generation;uicr.rows[free_slot].root=root;
    uicr.rows[free_slot].instance=instance;uicr.rows[free_slot].calls=*a;++uicr.live;return 1;
}
static void uicr_retire(void *group,U64 generation) {
    if(GetCurrentThreadId()!=uicr_thread())return;
    for(unsigned i=0;i<UICR_CAPACITY;++i)
        if(uicr.rows[i].root && !uicr.rows[i].retired && uicr.rows[i].group==group && uicr.rows[i].generation==generation)
            {uicr.rows[i].retired=1;--uicr.live;}
}
static void uicr_reap(void **input,unsigned count) {
    if(GetCurrentThreadId()!=uicr_thread())return;
    for(unsigned i=0;i<UICR_CAPACITY;++i) {
        if(!uicr.rows[i].root || !uicr.rows[i].retired)continue;
        unsigned present=0;for(unsigned n=0;n<count;++n)if(input[n]==uicr.rows[i].camera){present=1;break;}
        if(present || !managed_eye_verified_dead(&uicr.rows[i].calls,uicr.rows[i].camera))continue;
        uicr.rows[i].calls.unpin(uicr.rows[i].root);
        /* Anonymous row type: zero values individually without type punning. */
        uicr.rows[i].root=0;uicr.rows[i].camera=uicr.rows[i].group=0;uicr.rows[i].generation=0;
        uicr.rows[i].retired=uicr.rows[i].native_entered=uicr.rows[i].native_returned=uicr.rows[i].native_reported=0;
        uicr.rows[i].instance=0;uicr.rows[i].calls=(ManagedEyeApi){0};
    }
}
static unsigned uicr_native_begin(int instance,U64 *generation) {
    if(GetCurrentThreadId()!=uicr_thread() || !uicr.live)return UICR_CAPACITY;
    for(unsigned i=0;i<UICR_CAPACITY;++i)if(uicr.rows[i].root && !uicr.rows[i].retired &&
        uicr.rows[i].instance==instance) {
        if(uicr.rows[i].native_entered<~0u)++uicr.rows[i].native_entered;
        *generation=uicr.rows[i].generation;return i;
    }
    return UICR_CAPACITY;
}
#endif
