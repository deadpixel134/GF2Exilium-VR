#include "il2cpp_export.h"
#ifndef GF2_EYE_NATIVE_TARGETS_H
#define GF2_EYE_NATIVE_TARGETS_H
#include "managed_eye_binding.h"
#ifndef GF2VR_NATIVE_CALL
#define GF2VR_NATIVE_CALL(step,finished) ((void)0)
#endif
typedef struct {
    ManagedEyeApi calls;
    void *get_native,*intptr_class,*bool_class,*get_srgb;
} EyeNativeApi;
/* Borrowed snapshot, NOT an owning COM reference or render completion token.
   Unity owner thread only. Keep eyes borrowed until the render backend has
   acquired its own references and all engine/GPU use drains. Never cache across
   target recreation or pass this to another thread without a lifetime handoff. */
typedef struct {void *resource[2];unsigned long long frame;unsigned srgb,encoding_valid;} EyeNativeTargets;
static int eye_native_format(const EyeNativeTargets *n,unsigned *format) {
    if(!n || !n->encoding_valid || n->srgb>1 || !format) return 0;
    if(*format==27) {*format=n->srgb?29:28;return 1;}
    if(*format==28 || *format==87) return !n->srgb;
    if(*format==29 || *format==91) return n->srgb;
    return *format==10 && !n->srgb;
}
static int eye_native_bind_contract(const ManagedEyeBinding *eye,EyeNativeApi *out,
    void *(*find)(const char *,const char *,const char *),
    int (*size)(void *,unsigned *),unsigned char (*generic)(void *)) {
    if(!out) return 0;
    *out=(EyeNativeApi){0};
    if(!eye || !eye->type_root || !find || !size || !generic) return 0;
    EyeNativeApi a={0};a.calls=eye->api;
    if(!a.calls.target_is_created || !a.calls.invoke || !a.calls.pin ||
       !a.calls.unpin || !a.calls.unbox || !a.calls.object_class) return 0;
    a.intptr_class=find("mscorlib.dll","System","IntPtr");
    a.bool_class=find("mscorlib.dll","System","Boolean");
    unsigned alignment=0;
    if(!a.intptr_class || size(a.intptr_class,&alignment)!=(int)sizeof(void *) ||
       !alignment || alignment>_Alignof(void *) ||
       !a.bool_class || size(a.bool_class,&alignment)!=1 || alignment!=1) return 0;
    void *texture=find("UnityEngine.CoreModule.dll","UnityEngine","Texture");
    a.get_native=eye_binding_method(texture,"GetNativeTexturePtr",0,"System.IntPtr",0,generic);
    a.get_srgb=eye_binding_method(a.calls.target_class,"get_sRGB",0,"System.Boolean",0,generic);
    if(!a.get_native || !a.get_srgb) return 0;
    *out=a;return 1;
}
static int eye_native_bind(const ManagedEyeBinding *eye,EyeNativeApi *out) {
    if(!out) return 0;
    *out=(EyeNativeApi){0};
    HINSTANCE module=GetModuleHandleW(L"GameAssembly.dll");if(!module) return 0;
    int (*size)(void *,unsigned *)=(void *)il2cpp_export(module,"il2cpp_class_value_size");
    unsigned char (*generic)(void *)=(void *)il2cpp_export(module,"il2cpp_method_is_generic");
    return eye_native_bind_contract(eye,out,render_class,size,generic);
}
static int eye_native_value(const EyeNativeApi *a,void *target,void *method,
                             void *klass,void *out,unsigned bytes) {
    void *box=0;
    if(!managed_eye_call(&a->calls,method,target,0,&box) || !box) return 0;
    unsigned root=a->calls.pin(box,1);if(!root) return 0;
    int ok=0;
    if(a->calls.object_class(box)==klass) {
        const unsigned char *data=a->calls.unbox(box);
        if(data) {
            for(unsigned i=0;i<bytes;++i) ((unsigned char *)out)[i]=data[i];
            ok=1;
        }
    }
    a->calls.unpin(root);return ok;
}
static int eye_native_current_detail(const ManagedEyes *eyes,unsigned long long frame,
        int (*current)(void *),void *owner,unsigned *reason,unsigned base) {
    if(!current || !current(owner)) {if(reason)*reason=base+1;return 0;}
    if(!eyes || !eyes->borrowed || !eyes->ready || !eyes->targets_ready) {
        if(reason)*reason=base+2;return 0;
    }
    if(!frame || eyes->matrix_frame!=frame) {if(reason)*reason=base+3;return 0;}
    return 1;
}
static int eye_native_read_detail(const EyeNativeApi *a,const ManagedEyes *eyes,
    unsigned long long frame,int (*current)(void *),void *owner,EyeNativeTargets *out,unsigned *reason) {
    if(reason)*reason=0;
    if(!out){if(reason)*reason=1;return 0;}
    *out=(EyeNativeTargets){0};
    if(!a || !a->get_native || !a->get_srgb || !a->intptr_class || !a->bool_class ||
       !a->calls.target_is_created || !a->calls.invoke || !a->calls.pin ||
       !a->calls.unpin || !a->calls.object_class || !a->calls.unbox) {
        if(reason)*reason=2;return 0;
    }
    if(!eye_native_current_detail(eyes,frame,current,owner,reason,10))return 0;
    EyeNativeTargets next={0};void *targets[2]={eyes->eyes[0].target,eyes->eyes[1].target};
    if(!targets[0] || !targets[1] || targets[0]==targets[1] ||
       !eyes->eyes[0].target_root || !eyes->eyes[1].target_root) {
        if(reason)*reason=4;return 0;
    }
    for(unsigned i=0;i<2;++i) {
        unsigned char srgb=0;
        GF2VR_NATIVE_CALL(i*3,0);
        int srgb_ok=eye_native_value(a,targets[i],a->get_srgb,a->bool_class,&srgb,1);
        GF2VR_NATIVE_CALL(i*3,1);
        if(!srgb_ok) {
            if(reason)*reason=20+i*10;return 0;
        }
        if(srgb>1 || (i && srgb!=next.srgb)) {if(reason)*reason=21+i*10;return 0;}
        next.srgb=srgb;
        unsigned char created=0;
        if(!eye_native_current_detail(eyes,frame,current,owner,reason,40+i*10))return 0;
        GF2VR_NATIVE_CALL(i*3+1,0);
        int created_ok=eye_native_value(a,targets[i],a->calls.target_is_created,a->bool_class,&created,1);
        GF2VR_NATIVE_CALL(i*3+1,1);
        if(!created_ok) {
            if(reason)*reason=44+i*10;return 0;
        }
        if(!created) {if(reason)*reason=45+i*10;return 0;}
        if(!eye_native_current_detail(eyes,frame,current,owner,reason,60+i*10))return 0;
        GF2VR_NATIVE_CALL(i*3+2,0);
        int pointer_ok=eye_native_value(a,targets[i],a->get_native,a->intptr_class,&next.resource[i],sizeof(void *));
        GF2VR_NATIVE_CALL(i*3+2,1);
        if(!pointer_ok) {
            if(reason)*reason=64+i*10;return 0;
        }
        if(!next.resource[i]) {if(reason)*reason=65+i*10;return 0;}
    }
    if(!eye_native_current_detail(eyes,frame,current,owner,reason,80))return 0;
    if(targets[0]!=eyes->eyes[0].target || targets[1]!=eyes->eyes[1].target) {
        if(reason)*reason=84;return 0;
    }
    if(next.resource[0]==next.resource[1]) {if(reason)*reason=85;return 0;}
    next.frame=frame;next.encoding_valid=1;*out=next;return 1;
}
static int eye_native_read(const EyeNativeApi *a,const ManagedEyes *eyes,
    unsigned long long frame,int (*current)(void *),void *owner,EyeNativeTargets *out) {
    return eye_native_read_detail(a,eyes,frame,current,owner,out,0);
}
#endif
