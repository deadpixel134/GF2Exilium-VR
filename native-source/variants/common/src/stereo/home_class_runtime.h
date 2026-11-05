#ifndef GF2_HOME_CLASS_RUNTIME_H
#define GF2_HOME_CLASS_RUNTIME_H
static void hcr_mismatch(unsigned,void *,const char *,const char *);
#define GF2VR_HOME_CLASS_MISMATCH(r,k,a,e) hcr_mismatch(r,k,a,e)
#include "home_class_binding.h"
#undef GF2VR_HOME_CLASS_MISMATCH
static void hcr_mismatch(unsigned role,void *klass,const char *actual,const char *expected){
    static unsigned observations[7];
    static unsigned busy;
    if(busy || role>=7 || observations[role]>=2)return;busy=1;++observations[role];
    unsigned diff=0;while(diff<131071 && actual[diff] && expected[diff] && actual[diff]==expected[diff])++diff;
    unsigned start=diff>24?diff-24:0;
    /* Main-thread-only cold scan; avoid a CRT stack-probe dependency. */
    static char line[4096];char *p=source_prefix(line,"home_class_shape_mismatch");
    p=literal(p,",\"role\":");p=decimal(p,role);
    p=literal(p,",\"class\":");p=hexadecimal(p,(U64)klass);
    p=literal(p,",\"diff\":");p=decimal(p,diff);
    p=literal(p,",\"actual\":");p=render_ascii(p,actual+start);
    p=literal(p,",\"expected\":");p=render_ascii(p,expected+start);
    p=literal(p,"}\n");source_log(line,p);busy=0;
}
static HomeClassApi hcr_api;
static HomeClassScratch hcr_scratch;
static int home_classes_bind(HINSTANCE game,void **classes){
    HomeClassApi *a=&hcr_api;
    *a=(HomeClassApi){0};
    unsigned stage=0;
#define HCR_API(member,name) ++stage;a->member=(void *)il2cpp_export(game,name);if(!a->member){event("home_structural_bind_failure",stage,GetCurrentThreadId());return 0;}
    HCR_API(class_flags,"il2cpp_class_get_flags");HCR_API(byref,"il2cpp_type_is_byref");
    HCR_API(class_image,"il2cpp_class_get_image");HCR_API(class_name,"il2cpp_class_get_name");
    HCR_API(class_namespace,"il2cpp_class_get_namespace");HCR_API(value_type,"il2cpp_class_is_valuetype");
#undef HCR_API
    a->fields=hc_fields;a->field_flags=hc_field_flags;a->field_type=hc_field_type;
    a->constant=hc_api.static_read;a->methods=sa.class_get_methods;a->method_flags=sa.method_get_flags;
    a->param_count=sa.method_get_param_count;a->param=sa.method_get_param;a->result=sa.method_get_return_type;
    a->generic=(void *)il2cpp_export(game,"il2cpp_method_is_generic");a->method_name=sa.method_get_name;
    a->type_code=sa.type_get_type;a->from_type=hc_class_from_type;a->parent=hc_parent;
    a->image_name=sa.image_get_name;a->is_enum=hc_is_enum;a->enum_base=hc_enum_base;
    unsigned long long (*class_count)(void *)=(void *)il2cpp_export(game,"il2cpp_image_get_class_count");
    void *(*get_class)(void *,unsigned long long)=(void *)il2cpp_export(game,"il2cpp_image_get_class");
    if(!class_count || !get_class){event("home_structural_bind_failure",7,GetCurrentThreadId());return 0;}
    U64 count=0;void **assemblies=sa.domain_get_assemblies(sa.domain_get(),&count);void *target=0;
    if(!assemblies || !count || count>4096){event("home_structural_bind_failure",8,GetCurrentThreadId());return 0;}
    for(U64 i=0;i<count;++i){void *image=sa.assembly_get_image(assemblies[i]);
        if(image && source_equal(sa.image_get_name(image),"Assembly-CSharp.dll")){
            if(target){event("home_structural_bind_failure",9,GetCurrentThreadId());return 0;}target=image;
        }}
    if(!target){event("home_structural_bind_failure",10,GetCurrentThreadId());return 0;}
    unsigned mask=0;int ok=home_class_binding(a,&hcr_scratch,target,class_count(target),get_class,classes,&mask);
    event("home_structural_class_binding",((U64)ok<<32)|mask,GetCurrentThreadId());
    return ok;
}
static void *hcr_instance_field(void *klass,void *type){
    void *it=0,*found=0;if(!klass || !type)return 0;
    for(unsigned n=0;n<128;++n){void *f=hc_fields(klass,&it);if(!f)return found;
        void *t=hc_field_type(f);
        if((hc_field_flags(f)&0x50u) || !t || hcr_api.byref(t) ||
           sa.type_get_type(t)!=(hc_is_enum(type)?17:18) || hc_class_from_type(t)!=type)continue;
        if(found)return 0;found=f;
    }return 0;
}
static void *hcr_singleton(void *klass){
    void *found=0,*self=klass;
    for(unsigned depth=0;depth<4 && klass;++depth,klass=hc_parent(klass)){
        void *it=0;unsigned complete=0;
        for(unsigned n=0;n<128;++n){void *f=hc_fields(klass,&it);if(!f){complete=1;break;}
            unsigned flags=hc_field_flags(f);void *t=hc_field_type(f);
            if(!(flags&0x10u) || (flags&0x40u) || !t || hcr_api.byref(t) ||
               sa.type_get_type(t)!=18 || hc_class_from_type(t)!=self)continue;
            if(found)return 0;found=f;
        }if(!complete)return 0;
    }return klass?0:found;
}
static void *hcr_lookup(void *klass,void *param,void *result){
    void *it=0,*found=0;
    if(!klass || !param || !result)return 0;
    for(unsigned n=0;n<512;++n){void *m=sa.class_get_methods(klass,&it);if(!m)return found;
        unsigned impl=0;
        if((sa.method_get_flags(m,&impl)&0x10u) || hcr_api.generic(m) || sa.method_get_param_count(m)!=1)continue;
        void *p=sa.method_get_param(m,0),*r=sa.method_get_return_type(m);
        if(!p || !r || hcr_api.byref(p) || hcr_api.byref(r) || sa.type_get_type(p)!=17 || sa.type_get_type(r)!=18 ||
           hc_class_from_type(p)!=param || hc_class_from_type(r)!=result)continue;
        if(found)return 0;found=m;
    }return 0;
}
#endif
