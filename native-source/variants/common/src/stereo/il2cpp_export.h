#ifndef GF2_IL2CPP_EXPORT_H
#define GF2_IL2CPP_EXPORT_H
#include "il2cpp_export_profiles.h"
#ifndef GF2VR_IL2CPP_EXPORT_EVENT
#define GF2VR_IL2CPP_EXPORT_EVENT(kind,count) event(kind,count,GetCurrentThreadId())
#endif
typedef void *(*Il2cppSymbolFn)(void *module,const char *name);
enum {IL2CPP_PROFILE_UNKNOWN=-1,IL2CPP_PROFILE_PLAIN=0,IL2CPP_PROFILE_STEAM=1};
/* Binding is serialized by the attributed owner/main callback, never concurrent
   loader/worker calls. Modules are process-pinned. Atomic publication does not
   authorize concurrent binding or reuse of an unloaded module handle. */
static struct {void *module;int profile;} il2cpp_profile_cache;
static int il2cpp_export_name_valid(const char *s) {
    if(!s)return 0;
    for(unsigned i=0;i<=96;++i)if(!s[i])return 1;
    return 0;
}
static int il2cpp_export_compare(const char *a,const char *b) {
    for(unsigned i=0;i<=96;++i) {
        unsigned char x=(unsigned char)a[i],y=(unsigned char)b[i];
        if(x!=y)return x<y?-1:1;
        if(!x)return 0;
    }
    return 0;
}
static int il2cpp_export_index(const char *plain) {
    unsigned lo=0,hi=IL2CPP_EXPORT_NAME_COUNT;
    while(lo<hi) {
        unsigned mid=lo+(hi-lo)/2;
        int c=il2cpp_export_compare(plain,il2cpp_export_names[mid].plain);
        if(!c)return (int)mid;
        if(c<0)hi=mid;else lo=mid+1;
    }
    return -1;
}
static int il2cpp_export_profile_select(Il2cppSymbolFn sym,void *module) {
    if(!sym || !module)return IL2CPP_PROFILE_UNKNOWN;
    if(__atomic_load_n(&il2cpp_profile_cache.module,__ATOMIC_ACQUIRE)==module)
        return __atomic_load_n(&il2cpp_profile_cache.profile,__ATOMIC_RELAXED);
    int profile=IL2CPP_PROFILE_UNKNOWN;
    if(sym(module,"il2cpp_init"))profile=IL2CPP_PROFILE_PLAIN;
    else {
        int i=il2cpp_export_index("il2cpp_init");
        if(i>=0 && sym(module,il2cpp_export_names[i].steam))profile=IL2CPP_PROFILE_STEAM;
    }
    __atomic_store_n(&il2cpp_profile_cache.profile,profile,__ATOMIC_RELAXED);
    __atomic_store_n(&il2cpp_profile_cache.module,module,__ATOMIC_RELEASE);
    GF2VR_IL2CPP_EXPORT_EVENT("il2cpp_export_profile",(unsigned long long)(long long)profile);
    return profile;
}
static void *il2cpp_export_with(Il2cppSymbolFn sym,void *module,const char *plain) {
    /* Telemetry only, not a symbol/profile cache. Retain distinct unknown names
       without borrowing caller storage. On saturation suppress further unknown
       diagnostics rather than allocating or flooding the owner callback. */
    static unsigned char missing_logged[IL2CPP_EXPORT_NAME_COUNT];
    static char unknown_names[32][97];
    static unsigned unknown_count;
    if(!sym || !module || !il2cpp_export_name_valid(plain))return 0;
    void *found=sym(module,plain);if(found)return found;
    if(il2cpp_export_profile_select(sym,module)!=IL2CPP_PROFILE_STEAM)return 0;
    int index=il2cpp_export_index(plain);
    found=index<0?0:sym(module,il2cpp_export_names[index].steam);
    if(!found) {
        if(index>=0) {
            if(!missing_logged[index]) {
                missing_logged[index]=1;
                GF2VR_IL2CPP_EXPORT_EVENT("il2cpp_export_missing",(unsigned)index);
            }
        } else {
            for(unsigned i=0;i<unknown_count;++i)
                if(!il2cpp_export_compare(plain,unknown_names[i]))return 0;
            if(unknown_count<32) {
                for(unsigned i=0;i<=96;++i) {
                    unknown_names[unknown_count][i]=plain[i];if(!plain[i])break;
                }
                ++unknown_count;
                GF2VR_IL2CPP_EXPORT_EVENT("il2cpp_export_missing",0xffffu);
            }
        }
    }
    return found;
}
static inline void *il2cpp_export(HINSTANCE module,const char *plain) {
    if(!module || !il2cpp_export_name_valid(plain))return 0;
    /* Also report profile 0 on the all-plain product path; the injectable
       resolver itself retains immediate plain-hit/no-table semantics. */
    il2cpp_export_profile_select((Il2cppSymbolFn)GetProcAddress,module);
    return il2cpp_export_with((Il2cppSymbolFn)GetProcAddress,module,plain);
}
#endif
