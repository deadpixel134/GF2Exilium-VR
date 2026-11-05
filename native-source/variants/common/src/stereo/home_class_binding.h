#ifndef GF2_HOME_CLASS_BINDING_H
#define GF2_HOME_CLASS_BINDING_H
/* Native metadata only: no game method execution, managed object layout,
   name/ordinal role selection, or persistent class-name cache. */
typedef struct {unsigned flags,fields,methods;const char *signature;} HomeClassContract;
#include "home_class_contract.generated.h"
typedef struct {
    unsigned (*class_flags)(void *);
    void *(*fields)(void *,void **);
    unsigned (*field_flags)(void *);
    void *(*field_type)(void *);
    void (*constant)(void *,void *);
    void *(*methods)(void *,void **);
    unsigned (*method_flags)(void *,unsigned *);
    unsigned (*param_count)(void *);
    void *(*param)(void *,unsigned);
    void *(*result)(void *);
    unsigned char (*generic)(void *);
    const char *(*method_name)(void *);
    int (*type_code)(void *);
    unsigned char (*byref)(void *);
    void *(*from_type)(void *);
    void *(*parent)(void *);
    void *(*class_image)(void *);
    const char *(*image_name)(void *);
    const char *(*class_name)(void *);
    const char *(*class_namespace)(void *);
    unsigned char (*value_type)(void *);
    unsigned char (*is_enum)(void *);
    void *(*enum_base)(void *);
} HomeClassApi;
typedef struct {char *data;unsigned size,used,ok;} HcsText;
typedef struct {
    char field[128][512],method[256][2048],signature[131072];
    char value[128][16];
    char *fs[128],*ms[256],*vs[128];
    void *fields[128],*methods[256];
    unsigned busy;
} HomeClassScratch;
static int hcs_equal(const char *a,const char *b){
    if(!a || !b)return 0;
    for(unsigned i=0;i<131072;++i){if(a[i]!=b[i])return 0;if(!a[i])return 1;}return 0;
}
static void hcs_add(HcsText *t,const char *s){
    if(!t->ok || !s){t->ok=0;return;}
    for(unsigned i=0;i<131072;++i){if(!s[i])return;
        if(t->used+1>=t->size){t->ok=0;return;}t->data[t->used++]=s[i];t->data[t->used]=0;}
    t->ok=0;
}
static void hcs_number(HcsText *t,unsigned n){
    char text[16],rev[16];unsigned count=0;
    do{rev[count++]=(char)('0'+n%10);n/=10;}while(n);
    for(unsigned i=0;i<count;++i)text[i]=rev[count-i-1];text[count]=0;hcs_add(t,text);
}
static int hcs_compare(const char *a,const char *b){
    for(unsigned n=0;n<131072;++n){unsigned x=(unsigned char)a[n],y=(unsigned char)b[n];
        if(x!=y)return x<y?-1:1;if(!x)return 0;}return 0;
}
static void hcs_sort(char **rows,unsigned count){
    for(unsigned i=1;i<count;++i){char *row=rows[i];unsigned j=i;
        while(j && hcs_compare(rows[j-1],row)>0){rows[j]=rows[j-1];--j;}rows[j]=row;}
}
static int hcs_game_image(const char *s){
    return hcs_equal(s,"Assembly-CSharp.dll") || hcs_equal(s,"Assembly-CSharp-firstpass.dll") ||
        hcs_equal(s,"Common.dll") || hcs_equal(s,"CommonUtils.dll") ||
        hcs_equal(s,"NetworkProtocol.dll") || hcs_equal(s,"StaticTable.dll");
}
static void hcs_kind(const HomeClassApi *a,void *klass,HcsText *t){
    if(!klass){t->ok=0;return;}
    void *image=a->class_image(klass);const char *name=image?a->image_name(image):0;
    if(!name){t->ok=0;return;}
    if(hcs_game_image(name)){hcs_add(t,a->value_type(klass)?"V":"G");return;}
    const char *ns=a->class_namespace(klass),*short_name=a->class_name(klass);
    if(!ns || !short_name){t->ok=0;return;}
    hcs_add(t,ns);hcs_add(t,".");hcs_add(t,short_name);
}
static void hcs_type(const HomeClassApi *a,void *type,HcsText *t){
    if(!type){t->ok=0;return;}
    if(a->byref(type))hcs_add(t,"&");
    int code=a->type_code(type);if(code<1 || code>30 || code==16){t->ok=0;return;}
    hcs_number(t,(unsigned)code);
    if(code==17 || code==18 || code==21){hcs_add(t,":");hcs_kind(a,a->from_type(type),t);}
}
/* Complete declared-member enumeration is required. Unrelated oversized types
   cannot match these exact bounded contracts and are skipped. */
static int hcs_describe(const HomeClassApi *a,HomeClassScratch *s,void *klass,
                        unsigned nf,unsigned nm){
    for(unsigned i=0;i<nf;++i){HcsText t={s->field[i],512,0,1};t.data[0]=0;
        hcs_number(&t,a->field_flags(s->fields[i]));hcs_add(&t,":");hcs_type(a,a->field_type(s->fields[i]),&t);
        if(!t.ok)return 0;s->fs[i]=t.data;}
    for(unsigned i=0;i<nm;++i){void *m=s->methods[i];unsigned impl=0,n=a->param_count(m);
        if(n>16)return 0;HcsText t={s->method[i],2048,0,1};t.data[0]=0;
        hcs_number(&t,a->method_flags(m,&impl));hcs_add(&t,":");hcs_number(&t,a->generic(m)?1:0);hcs_add(&t,":");
        const char *name=a->method_name(m);if(!name)return 0;
        hcs_number(&t,hcs_equal(name,".ctor") || hcs_equal(name,".cctor"));hcs_add(&t,":");
        hcs_type(a,a->result(m),&t);hcs_add(&t,"(");
        for(unsigned j=0;j<n;++j){if(j)hcs_add(&t,",");hcs_type(a,a->param(m,j),&t);}hcs_add(&t,")");
        if(!t.ok)return 0;s->ms[i]=t.data;}
    hcs_sort(s->fs,nf);hcs_sort(s->ms,nm);
    unsigned nv=0;
    if(a->is_enum(klass)){
        void *base=a->enum_base(klass);if(!base || a->type_code(base)!=8)return 0;
        for(unsigned i=0;i<nf;++i)if(a->field_flags(s->fields[i])&0x40u){
            int value=0;a->constant(s->fields[i],&value);
            HcsText t={s->value[nv],16,0,1};t.data[0]=0;
            if(value<0){hcs_add(&t,"-");hcs_number(&t,0u-(unsigned)value);}else hcs_number(&t,(unsigned)value);
            if(!t.ok)return 0;s->vs[nv++]=t.data;
        }
        hcs_sort(s->vs,nv);
    }
    HcsText t={s->signature,131072,0,1};t.data[0]=0;
    hcs_number(&t,a->class_flags(klass));hcs_add(&t,"|");
    void *parent=a->parent(klass);if(parent)hcs_kind(a,parent,&t);else hcs_add(&t,"null");
    hcs_add(&t,"|F");for(unsigned i=0;i<nf;++i){if(i)hcs_add(&t,"|");hcs_add(&t,s->fs[i]);}
    hcs_add(&t,"|M");for(unsigned i=0;i<nm;++i){if(i)hcs_add(&t,"|");hcs_add(&t,s->ms[i]);}
    hcs_add(&t,"|E");for(unsigned i=0;i<nv;++i){if(i)hcs_add(&t,",");hcs_add(&t,s->vs[i]);}
    return t.ok;
}
static unsigned hcs_counts(void *(*next)(void *,void **),void *klass,void **out,unsigned cap){
    void *it=0;for(unsigned n=0;n<=cap;++n){void *v=next(klass,&it);if(!v)return n;
        if(n==cap)return cap+1;out[n]=v;}return cap+1;
}
static int home_class_binding(const HomeClassApi *a,HomeClassScratch *s,void *image,
    unsigned long long count,void *(*get_class)(void *,unsigned long long),void **out,unsigned *mask){
    for(unsigned r=0;r<7;++r)out[r]=0;*mask=0;
    if(!a || !s || s->busy || !image || !get_class || !count || count>32768)return 0;
#define HCS_REQUIRE(f) if(!a->f)return 0
    HCS_REQUIRE(class_flags);HCS_REQUIRE(fields);HCS_REQUIRE(field_flags);HCS_REQUIRE(field_type);HCS_REQUIRE(constant);
    HCS_REQUIRE(methods);HCS_REQUIRE(method_flags);HCS_REQUIRE(param_count);HCS_REQUIRE(param);HCS_REQUIRE(result);
    HCS_REQUIRE(generic);HCS_REQUIRE(method_name);HCS_REQUIRE(type_code);HCS_REQUIRE(byref);HCS_REQUIRE(from_type);
    HCS_REQUIRE(parent);HCS_REQUIRE(class_image);HCS_REQUIRE(image_name);HCS_REQUIRE(class_name);HCS_REQUIRE(class_namespace);
    HCS_REQUIRE(value_type);HCS_REQUIRE(is_enum);HCS_REQUIRE(enum_base);
#undef HCS_REQUIRE
    s->busy=1;void *found[7]={0};unsigned matches[7]={0};int ok=0;
    for(unsigned long long i=0;i<count;++i){
        void *klass=get_class(image,i);if(!klass)goto done;
        unsigned flags=a->class_flags(klass),possible=0;
        for(unsigned r=0;r<7;++r)if(flags==home_class_contracts[r].flags)possible|=1u<<r;
        if(!possible)continue;
        unsigned nf=hcs_counts(a->fields,klass,s->fields,128);if(nf>128)continue;
        unsigned nm=hcs_counts(a->methods,klass,s->methods,256);if(nm>256)continue;
        unsigned filtered=0;
        for(unsigned r=0;r<7;++r)if((possible&(1u<<r)) && nf==home_class_contracts[r].fields && nm==home_class_contracts[r].methods)filtered|=1u<<r;
        if(!filtered || !hcs_describe(a,s,klass,nf,nm))continue;
        for(unsigned r=0;r<7;++r)if(filtered&(1u<<r)){
            if(hcs_equal(s->signature,home_class_contracts[r].signature)){
                if(++matches[r]>1)goto done;found[r]=klass;*mask|=1u<<r;
            }
#ifdef GF2VR_HOME_CLASS_MISMATCH
            else GF2VR_HOME_CLASS_MISMATCH(r,klass,s->signature,home_class_contracts[r].signature);
#endif
        }
    }
    if(*mask!=127u)goto done;
    for(unsigned r=0;r<7;++r)out[r]=found[r];ok=1;
done:
    s->busy=0;return ok;
}
#endif
