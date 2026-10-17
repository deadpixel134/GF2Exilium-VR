#ifndef GF2_COMPOSITION_BRIDGE_IMAGE_BINDING_H
#define GF2_COMPOSITION_BRIDGE_IMAGE_BINDING_H
#include "composition_bridge_image.h"
#define GF2_COMPOSITION_IMAGE_NAME "GF2.Composition.CameraBridge.dll"
/* Uses the already initialized SourceApi. At most one exact image is allowed. */
static unsigned composition_image_find(void **result){
    *result=0;
    void *domain=sa.domain_get();if(!domain)return 2;
    U64 count=0;void **assemblies=sa.domain_get_assemblies(domain,&count);
    if(!assemblies || count>4096)return 2;
    unsigned matches=0;
    for(U64 i=0;i<count;++i){
        void *image=sa.assembly_get_image(assemblies[i]);
        if(image && source_equal(sa.image_get_name(image),GF2_COMPOSITION_IMAGE_NAME)){
            ++matches;*result=image;if(matches>1)return matches;
        }
    }
    return matches;
}
static int composition_image_absent(void *context){
    (void)context;void *image=0;return composition_image_find(&image)==0;
}
static void *composition_image_resolve(void *context){
    (void)context;void *image=0;
    if(composition_image_find(&image)!=1)return 0;
    return sa.class_from_name(image,"GF2.Composition","CameraScopeBridge");
}
static int composition_image_bind(CompositionImageApi *out){
    if(!out || out->byte_class || !sa.runtime_invoke || !sa.object_get_class ||
       !sa.array_length || !sa.gchandle_new || !sa.gchandle_free ||
       !sa.object_unbox || !sa.domain_get || !sa.domain_get_assemblies ||
       !sa.assembly_get_image || !sa.image_get_name || !sa.class_from_name)return 0;
    HINSTANCE module=GetModuleHandleW(L"GameAssembly.dll");if(!module)return 0;
    void *(*array_class)(void *,unsigned)=(void *)GetProcAddress(module,"il2cpp_array_class_get");
    unsigned char (*generic)(void *)=(void *)GetProcAddress(module,"il2cpp_method_is_generic");
    CompositionImageApi a={0};
    a.array_new=(void *)GetProcAddress(module,"il2cpp_array_new");
    a.value_box=(void *)GetProcAddress(module,"il2cpp_value_box");
    a.assignable=(void *)GetProcAddress(module,"il2cpp_class_is_assignable_from");
    if(!array_class || !generic || !a.array_new || !a.value_box || !a.assignable)return 0;
    a.byte_class=render_class("mscorlib.dll","System","Byte");
    void *array=render_class("mscorlib.dll","System","Array");
    a.assembly_class=render_class("mscorlib.dll","System.Reflection","Assembly");
    if(!a.byte_class || !array || !a.assembly_class)return 0;
    const char *setter[]={"System.Object","System.Int32"};
    a.set_value=eye_binding_method_args(array,"SetValue",2,setter,"System.Void",0,generic);
    a.get_value=eye_binding_method(array,"GetValue","System.Int32","System.Object",0,generic);
    a.load=eye_binding_method(a.assembly_class,"Load","System.Byte[]","System.Reflection.Assembly",1,generic);
    a.byte_array_class=array_class(a.byte_class,1);
    if(!a.set_value || !a.get_value || !a.load || !a.byte_array_class)return 0;
    a.object_class=sa.object_get_class;a.array_length=sa.array_length;
    a.pin=sa.gchandle_new;a.unpin=sa.gchandle_free;
    a.invoke=sa.runtime_invoke;a.unbox=sa.object_unbox;
    a.image_absent=composition_image_absent;a.resolve_class=composition_image_resolve;
    *out=a;return 1;
}
#endif
