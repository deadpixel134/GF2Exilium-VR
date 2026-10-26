#ifndef GF2VR_CONTACT_HAPTICS_H
#define GF2VR_CONTACT_HAPTICS_H
#include "contact_haptic_config.h"
/* Dirty-only output. Unity main publishes POD; the existing XR worker owns
   actions, binding, feedback and teardown. Neither side waits on the other. */
typedef struct {
    unsigned lock, mask, route;
    unsigned long long epoch, generation, tick[2];
} ContactHapticMailbox;
static ContactHapticMailbox contact_haptic_mailbox;
/* XR writes this focus/session lease; main may only echo an unchanged lease.
   Changing eligibility invalidates events on both sides, including events
   still waiting in managed code when the XR consumer loses focus. */
static unsigned contact_haptic_route;
static void contact_haptic_eligibility(int active) {
    unsigned old=__atomic_load_n(&contact_haptic_route,__ATOMIC_RELAXED);
    if((old&1u)!=(unsigned)(active!=0))
        __atomic_store_n(&contact_haptic_route,((old+2u)&~1u)|(unsigned)(active!=0),__ATOMIC_RELEASE);
}
static int contact_haptic_lock(ContactHapticMailbox *m) {
    unsigned expected=0;
    return __atomic_compare_exchange_n(&m->lock,&expected,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED);
}
static void contact_haptic_publish(unsigned mask,unsigned long long epoch,
    unsigned long long generation,unsigned long long tick,unsigned route) {
    ContactHapticMailbox *m=&contact_haptic_mailbox;
    if(!contact_haptic_lock(m))return;
    /* Coalesce only simultaneous unconsumed edges of this exact owner. */
    if(mask && (!(route&1u) || route!=__atomic_load_n(&contact_haptic_route,__ATOMIC_ACQUIRE))) {
        __atomic_store_n(&m->lock,0,__ATOMIC_RELEASE);return;
    }
    if(m->epoch!=epoch || m->generation!=generation || m->route!=route || !mask)m->mask=0;
    m->route=route;
    m->epoch=epoch;m->generation=generation;
    for(unsigned h=0;h<2;++h)if(mask&(1u<<h))m->tick[h]=tick;
    m->mask|=mask&3;
    __atomic_store_n(&m->lock,0,__ATOMIC_RELEASE);
}
typedef struct {
    XrAction action;XrPath hands[2];XrSession session;
    PFN_xrApplyHapticFeedback apply;PFN_xrStopHapticFeedback stop;
    unsigned active,profiles;
    ContactHapticSettings settings;
} ContactHapticActions;
static void contact_haptic_cancel(ContactHapticActions *a) {
    contact_haptic_eligibility(0);
    if(a->action && a->stop)for(unsigned h=0;h<2;++h)if(a->active&(1u<<h)) {
        XrHapticActionInfo info={0};info.type=XR_TYPE_HAPTIC_ACTION_INFO;
        info.action=a->action;info.subactionPath=a->hands[h];
        (void)a->stop(a->session,&info);
    }
    a->active=0;
    /* XR consumes/discards only; do not reset the live mailbox or its lock. */
    ContactHapticMailbox *m=&contact_haptic_mailbox;
    if(contact_haptic_lock(m)){m->mask=0;__atomic_store_n(&m->lock,0,__ATOMIC_RELEASE);}
}
static void contact_haptic_open(ContactHapticActions *a,PFN_xrGetInstanceProcAddr get,
    PFN_xrCreateAction create,PFN_xrStringToPath path,XrInstance instance,XrSession session,XrActionSet set) {
    a->session=session;a->settings=contact_haptic_defaults();
    if(get(instance,"xrApplyHapticFeedback",(PFN_xrVoidFunction *)&a->apply)!=XR_SUCCESS || !a->apply ||
       get(instance,"xrStopHapticFeedback",(PFN_xrVoidFunction *)&a->stop)!=XR_SUCCESS || !a->stop)return;
    if(path(instance,"/user/hand/left",&a->hands[0])!=XR_SUCCESS ||
       path(instance,"/user/hand/right",&a->hands[1])!=XR_SUCCESS)return;
    XrActionCreateInfo info={0};info.type=XR_TYPE_ACTION_CREATE_INFO;
    info.actionType=XR_ACTION_TYPE_VIBRATION_OUTPUT;info.countSubactionPaths=2;info.subactionPaths=a->hands;
    const char *name="character_contact",*label="Character contact";
    for(unsigned n=0;name[n];++n)info.actionName[n]=name[n];
    for(unsigned n=0;label[n];++n)info.localizedActionName[n]=label[n];
    if(create(set,&info,&a->action)!=XR_SUCCESS)a->action=0;
}
static XrResult contact_haptic_suggest(ContactHapticActions *a,PFN_xrSuggestInteractionProfileBindings suggest,
    PFN_xrStringToPath path,XrInstance instance,unsigned profile,const XrInteractionProfileSuggestedBinding *base) {
    if(a->action && base->countSuggestedBindings<=24) {
        XrPath left=0,right=0;
        if(path(instance,"/user/hand/left/output/haptic",&left)==XR_SUCCESS &&
           path(instance,"/user/hand/right/output/haptic",&right)==XR_SUCCESS) {
            XrActionSuggestedBinding bindings[26];
            for(unsigned n=0;n<base->countSuggestedBindings;++n)bindings[n]=base->suggestedBindings[n];
            bindings[base->countSuggestedBindings]=(XrActionSuggestedBinding){a->action,left};
            bindings[base->countSuggestedBindings+1]=(XrActionSuggestedBinding){a->action,right};
            XrInteractionProfileSuggestedBinding combined=*base;
            combined.countSuggestedBindings+=2;combined.suggestedBindings=bindings;
            XrResult result=suggest(instance,&combined);
            if(result==XR_SUCCESS){a->profiles|=1u<<profile;return result;}
        }
    }
    /* Optional haptic failure must preserve the original input suggestions. */
    a->profiles&=~(1u<<profile);
    return suggest(instance,base);
}
static unsigned contact_haptic_take(unsigned long long epoch,unsigned long long generation,unsigned long long now) {
    ContactHapticMailbox *m=&contact_haptic_mailbox;
    if(!contact_haptic_lock(m))return 0;
    unsigned mask=m->mask;m->mask=0;
    unsigned route=__atomic_load_n(&contact_haptic_route,__ATOMIC_ACQUIRE);
    if(!(route&1u) || route!=m->route || !epoch || !generation || epoch!=m->epoch || generation!=m->generation)mask=0;
    for(unsigned h=0;h<2;++h)if(now<m->tick[h] || now-m->tick[h]>100)mask&=~(1u<<h);
    __atomic_store_n(&m->lock,0,__ATOMIC_RELEASE);return mask;
}
static void contact_haptic_frame(ContactHapticActions *a,unsigned long long epoch,unsigned long long generation,
    unsigned long long now,int eligible,unsigned tracked) {
    if(!eligible || !a->action || !a->profiles || a->settings.amplitude<=0 || a->settings.duration<=0){contact_haptic_cancel(a);return;}
    contact_haptic_eligibility(1);
    unsigned mask=contact_haptic_take(epoch,generation,now);
    for(unsigned h=0;h<2;++h) {
        XrHapticActionInfo info={0};info.type=XR_TYPE_HAPTIC_ACTION_INFO;
        info.action=a->action;info.subactionPath=a->hands[h];
        if(!(tracked&(1u<<h))) {
            if(a->active&(1u<<h))(void)a->stop(a->session,&info);
            a->active&=~(1u<<h);continue;
        }
        if(!(mask&(1u<<h)))continue;
        XrHapticVibration pulse={0};pulse.type=XR_TYPE_HAPTIC_VIBRATION;
        pulse.duration=a->settings.duration;
        pulse.frequency=XR_FREQUENCY_UNSPECIFIED;pulse.amplitude=a->settings.amplitude;
        if(a->apply(a->session,&info,(const XrHapticBaseHeader *)&pulse)==XR_SUCCESS)a->active|=1u<<h;
    }
}
#endif
