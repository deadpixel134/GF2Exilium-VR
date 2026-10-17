#ifndef GF2_PRODUCER_INSTALL_DIAGNOSTIC_H
#define GF2_PRODUCER_INSTALL_DIAGNOSTIC_H
#include "producer_bridge_invoke.h"
#define PID_TEXT_CAPACITY 256
typedef struct {void *progress,*failure;} ProducerInstallMethods;
static int pid_bind(ProducerInstallMethods *out,void *klass,
    unsigned char (*generic)(void *),unsigned char (*inflated)(void *),void *(*declaring)(void *)) {
    if(!out)return 0;*out=(ProducerInstallMethods){0};
    if(!klass || !generic || !inflated || !declaring)return 0;
    ProducerInstallMethods b={0};
    b.progress=eye_binding_method_args(klass,"InstallProgress",0,0,"System.Int32",1,generic);
    b.failure=eye_binding_method_args(klass,"InstallFailure",0,0,"System.String",1,generic);
    if(!b.progress || !b.failure || generic(b.progress) || generic(b.failure) ||
       inflated(b.progress) || inflated(b.failure) || declaring(b.progress)!=klass || declaring(b.failure)!=klass)return 0;
    *out=b;return 1;
}
typedef struct {
    unsigned progress_read,text_read,truncated;
    int progress,length;
    unsigned copied;
    uint16_t units[PID_TEXT_CAPACITY];
} ProducerInstallDiagnostic;
static void pid_capture(ProducerBridgeInvoker *a,ProducerInstallMethods methods,void *string_class,
    int (*length)(void *),const uint16_t *(*chars)(void *),ProducerInstallDiagnostic *out) {
    if(!out)return;*out=(ProducerInstallDiagnostic){0};
    if(!a || !a->owner || !a->thread || a->thread()!=a->owner || !methods.progress || !methods.failure)return;
    /* Reuse the verified typed zero-argument scalar contract. No guessed
       MethodInfo/code-pointer ABI; the copied invoker cannot detach anything. */
    ProducerBridgeInvoker local=*a;local.methods.method[PBI_DETACH-1]=methods.progress;
    out->progress_read=pbi_invoke(&local,PBI_DETACH,0,0,0,0,0,0,0,&out->progress);
    if(!string_class || !length || !chars || !a->invoke || !a->object_class ||
       !a->pin || !a->unpin || !a->read)return;
    void *exception=0,*value=a->invoke(methods.failure,0,0,&exception);
    if(exception || !value)return;
    unsigned root=a->pin(value,1);if(!root)return;
    if(a->object_class(value)==string_class) {
        int count=length(value);
        if(count>=0) {
            unsigned copied=(unsigned)count;
            if(copied>PID_TEXT_CAPACITY)copied=PID_TEXT_CAPACITY;
            const uint16_t *data=copied?chars(value):0;
            if(!copied || (data && a->read((uintptr_t)data,out->units,copied*sizeof(uint16_t)))) {
                out->length=count;out->copied=copied;out->truncated=(unsigned)count>copied;out->text_read=1;
            }
        }
    }
    a->unpin(root);
}
#endif
