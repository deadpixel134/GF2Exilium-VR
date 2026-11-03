#ifndef GF2_IL2CPP_EXPORT_AUTO_RUNTIME_H
#define GF2_IL2CPP_EXPORT_AUTO_RUNTIME_H
#include "il2cpp_export_auto_policy.h"
/* Win64 MEMORY_BASIC_INFORMATION ABI; keep this owner independent of the CRT
   and do not load a second GameAssembly/UnityPlayer for inspection. */
typedef struct {
    void *base,*allocation;
    unsigned allocation_protect;unsigned short partition,pad;
    unsigned long long size;
    unsigned state,protect,type,pad2;
} Il2cppAutoMemory;
_Static_assert(sizeof(Il2cppAutoMemory)==48,"Win64 VirtualQuery layout");
typedef unsigned long long (*Il2cppAutoQuery)(const void *,void *,unsigned long long);
typedef struct {void *unity,*game;Il2cppSymbolFn symbol;Il2cppAutoQuery query;} Il2cppAutoContext;
static int il2cpp_auto_region(Il2cppAutoContext *c,const void *ptr,void *module,Il2cppAutoMemory *m){
    return c->query(ptr,m,sizeof(*m))==sizeof(*m) && m->allocation==module &&
        m->state==0x1000u && m->type==0x1000000u && !(m->protect&0x101u);
}
static const unsigned char *il2cpp_auto_read(void *context,unsigned rva,unsigned bytes){
    Il2cppAutoContext *c=context;
    if(!bytes || rva>=IL2CPP_LOADER_IMAGE_SIZE || bytes>IL2CPP_LOADER_IMAGE_SIZE-rva)return 0;
    const unsigned char *p=(const unsigned char *)c->unity+rva;unsigned left=bytes;
    while(left){
        Il2cppAutoMemory m={0};
        if(!il2cpp_auto_region(c,p,c->unity,&m) || !(m.protect&0xeeu))return 0;
        unsigned long long base=(unsigned long long)m.base,at=(unsigned long long)p;
        if(at<base || at-base>=m.size)return 0;
        unsigned long long avail=m.size-(at-base);unsigned step=avail<left?(unsigned)avail:left;
        p+=step;left-=step;
    }
    return (const unsigned char *)c->unity+rva;
}
static void *il2cpp_auto_symbol(void *context,const char *name){
    Il2cppAutoContext *c=context;return c->symbol(c->game,name);
}
static int il2cpp_auto_executable(void *context,void *address){
    Il2cppAutoContext *c=context;Il2cppAutoMemory m={0};
    return il2cpp_auto_region(c,address,c->game,&m) && (m.protect&0xf0u)!=0;
}
static unsigned il2cpp_auto_u32(const unsigned char *p){
    return (unsigned)p[0]|((unsigned)p[1]<<8)|((unsigned)p[2]<<16)|((unsigned)p[3]<<24);
}
static void *il2cpp_export_auto(Il2cppSymbolFn sym,void *game,const char *plain){
    int index=il2cpp_export_index(plain);if(index<0 || IL2CPP_EXPORT_NAME_COUNT!=222)return 0;
    void *unity=GetModuleHandleW(L"UnityPlayer.dll");if(!unity)return 0;
    /* Same serialized/process-pinned module contract as the existing resolver. */
    static struct {void *game,*unity;unsigned ready,busy;void *functions[222];} cache;
    if(cache.busy)return 0;
    if(cache.game==game && cache.unity==unity)return cache.ready?cache.functions[index]:0;
    cache.busy=1;cache.ready=0;
    Il2cppAutoContext c={unity,game,sym,0};
    c.query=(Il2cppAutoQuery)GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"VirtualQuery");
    if(c.query){
        const unsigned char *dos=il2cpp_auto_read(&c,0,64),*pe=0;
        if(dos && dos[0]=='M' && dos[1]=='Z')pe=il2cpp_auto_read(&c,il2cpp_auto_u32(dos+60),84);
        if(pe && il2cpp_auto_u32(pe)==0x4550u && pe[4]==0x64 && pe[5]==0x86 &&
           pe[24]==0x0b && pe[25]==0x02 && il2cpp_auto_u32(pe+80)==IL2CPP_LOADER_IMAGE_SIZE){
            Il2cppAutoApi api={&c,il2cpp_auto_read,il2cpp_auto_symbol,il2cpp_auto_executable};
            cache.ready=il2cpp_auto_resolve(&api,cache.functions);
        }
    }
    cache.game=game;cache.unity=unity;cache.busy=0;
    GF2VR_IL2CPP_EXPORT_EVENT("il2cpp_export_loader_contract",cache.ready?222:0);
    return cache.ready?cache.functions[index]:0;
}
#endif
