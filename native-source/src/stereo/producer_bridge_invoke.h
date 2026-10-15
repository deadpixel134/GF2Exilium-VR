#ifndef GF2_PRODUCER_BRIDGE_INVOKE_H
#define GF2_PRODUCER_BRIDGE_INVOKE_H
#include <stdint.h>
#include <stddef.h>
enum {PBI_INSTALL=1,PBI_ARM=2,PBI_FINISH=3,PBI_STATUS=4,PBI_DETACH=5};
typedef struct {void *method[5];} ProducerBridgeMethods;
static int pbi_bind(ProducerBridgeMethods *out,void *klass,int verified,
    unsigned char (*generic)(void *),unsigned char (*inflated)(void *),void *(*declaring)(void *)) {
    if(!out)return 0;*out=(ProducerBridgeMethods){0};
    if(!klass || !verified || !generic || !inflated || !declaring)return 0;
    const char *names[]={"Install","Arm","Finish","Status","Detach"};
    const char *install[]={"System.IntPtr"};
    const char *arm[]={"System.Int64","UnityEngine.Rendering.ScriptableRenderContext","UnityEngine.Camera","UnityEngine.Camera","UnityEngine.Camera[]"};
    const char *finish[]={"System.Int64","System.Int32"},*status[]={"System.Int64"};
    const char **args[]={install,arm,finish,status,0};const unsigned counts[]={1,5,2,1,0};
    ProducerBridgeMethods b={0};
    for(unsigned i=0;i<5;++i) {
        b.method[i]=eye_binding_method_args(klass,names[i],counts[i],args[i],"System.Int32",1,generic);
        if(!b.method[i] || generic(b.method[i]) || inflated(b.method[i]) || declaring(b.method[i])!=klass)return 0;
    }
    *out=b;return 1;
}
typedef struct {
    uint64_t owner;
    ProducerBridgeMethods methods;
    void *integer,*camera_class,*camera_array_class;
    uint64_t (*thread)(void);
    void *(*object_class)(void *);
    unsigned (*array_length)(void *);
    void *(*invoke)(void *,void *,void **,void **);
    void *(*unbox)(void *);
    unsigned (*pin)(void *,int);
    void (*unpin)(unsigned);
    int (*read)(uintptr_t,void *,size_t);
} ProducerBridgeInvoker;
/* Reference objects are direct; values point to storage. The adapter must
   verify the actual ScriptableRenderContext value layout is one IntPtr before
   passing context_value here. Never invoke an inferred native MethodInfo ABI. */
static int pbi_invoke(ProducerBridgeInvoker *a,unsigned op,uint64_t serial,uintptr_t callback,
    uintptr_t context_value,void *left,void *right,void *originals,int cpu,int *result) {
    if(result)*result=0;
    if(!a || !result || op<1 || op>5 || !a->owner || !a->thread || a->thread()!=a->owner ||
       !a->methods.method[op-1] || !a->integer || !a->invoke || !a->object_class ||
       !a->unbox || !a->pin || !a->unpin || !a->read)return 0;
    int64_t epoch=(int64_t)serial;int32_t success=cpu!=0;void *args[5]={0};
    if(op==PBI_INSTALL) {if(!callback)return 0;args[0]=&callback;}
    else if(op==PBI_ARM) {
        if(!serial || serial>INT64_MAX || !context_value || !left || !right || left==right || !originals ||
           !a->camera_class || !a->camera_array_class || !a->array_length ||
           a->object_class(left)!=a->camera_class || a->object_class(right)!=a->camera_class ||
           a->object_class(originals)!=a->camera_array_class)return 0;
        unsigned count=a->array_length(originals);if(!count || count>32)return 0;
        args[0]=&epoch;args[1]=&context_value;args[2]=left;args[3]=right;args[4]=originals;
    }
    else if(op!=PBI_DETACH) {
        if(!serial || serial>INT64_MAX)return 0;
        args[0]=&epoch;if(op==PBI_FINISH)args[1]=&success;
    }
    void *exception=0,*boxed=a->invoke(a->methods.method[op-1],0,op==PBI_DETACH?0:args,&exception);
    if(exception || !boxed)return 0;
    unsigned root=a->pin(boxed,1);if(!root)return 0;
    int32_t value=0;int okay=0;
    if(a->object_class(boxed)==a->integer) {
        void *data=a->unbox(boxed);if(data)okay=a->read((uintptr_t)data,&value,sizeof(value));
    }
    a->unpin(root);if(!okay)return 0;*result=value;return 1;
}
#endif
