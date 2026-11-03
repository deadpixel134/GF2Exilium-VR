#ifndef GF2_IL2CPP_EXPORT_AUTO_POLICY_H
#define GF2_IL2CPP_EXPORT_AUTO_POLICY_H
#include "il2cpp_loader_contract.h"
/* No execution of discovered pointers here. Caller supplies readable mapped
   image ranges and executable-in-GameAssembly address validation. All-or-none
   publication: no partially resolved API set escapes on a failed contract. */
typedef struct {
    void *context;
    const unsigned char *(*read)(void *,unsigned,unsigned);
    void *(*symbol)(void *,const char *);
    int (*executable)(void *,void *);
} Il2cppAutoApi;
static int il2cpp_auto_resolve(const Il2cppAutoApi *a,void **out){
    if(!out)return 0;
    for(unsigned i=0;i<222;++i)out[i]=0;
    if(!a || !a->read || !a->symbol || !a->executable)return 0;
    const unsigned char *code=a->read(a->context,IL2CPP_LOADER_BEGIN,sizeof(il2cpp_loader_code));
    if(!code)return 0;
    for(unsigned i=0;i<sizeof(il2cpp_loader_code);++i)if(code[i]!=il2cpp_loader_code[i])return 0;
    const char *names[222]={0};
    for(unsigned n=0;n<222;++n){
        unsigned index=il2cpp_loader_names[n].index,rva=il2cpp_loader_names[n].name_rva;
        if(index>=222 || names[index])return 0;
        const unsigned char *p=a->read(a->context,rva,97);if(!p)return 0;
        unsigned len=0;for(;len<97 && p[len];++len)
            if(!((p[len]>='a' && p[len]<='z') || (p[len]>='0' && p[len]<='9') || p[len]=='_'))return 0;
        if(!len || len>=97)return 0;
        for(unsigned j=0;j<222;++j)if(names[j]){
            unsigned k=0;while(p[k] && p[k]==(unsigned char)names[j][k])++k;
            if(p[k]==(unsigned char)names[j][k])return 0;
        }
        names[index]=(const char *)p;
    }
    for(unsigned i=0;i<222;++i){
        void *fn=a->symbol(a->context,names[i]);
        if(!fn || !a->executable(a->context,fn)){
            for(unsigned j=0;j<222;++j)out[j]=0;
            return 0;
        }
        out[i]=fn;
    }
    return 1;
}
#endif
