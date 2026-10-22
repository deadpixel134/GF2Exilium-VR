#include "il2cpp_export.h"
#ifndef GF2VR_TOUCH_P3_RUNTIME_H
#define GF2VR_TOUCH_P3_RUNTIME_H
#include "../bootstrap/runtime_logging_state.h"
/* Included late in the single frozen integration TU, private macro only.
   XR owner publishes POD. All managed calls remain on Unity's main owner. */
#include "touch2_pose.h"
#include "touch3_alignment.h"
#include "touch2_config.h"
#include "gf2_touch_bridge_package.h"
#include "touch2_capacity.h"
#include "controller_paths.h"
static struct {
    Touch2Ring ring;
    unsigned loaded,enabled,bound,fault,prepared,writer,down,context,busy;
    unsigned ready_snapshot,length,overflow,perf_context,mode;
    CompositionImageApi api; CompositionImage image;
    void *prepare,*draw,*reset,*snapshot,*diagnostics;
    void *contact_configure,*contact_service,*contact_diagnostics;
#ifdef GF2VR_DIRTY_CONTACT
    void *contact_haptics;
    unsigned haptic_route;
#endif
    unsigned contact_configured,contact_config_length,contact_ready,contact_length;
    char contact_config[4097],contact_json[16384];
    unsigned contact_down;
    void *bone_sample,*bone_drain,*bone_reset;
    unsigned bone_bound,bone_pending;
    U64 bone_epoch,bone_generation,bone_until;
    void *bone_source; /* Borrowed identity only; compare on main owner, never dereference. */
    char shader_json[8192],bone_json[131072];
    unsigned shader_length,shader_ready,bone_length,bone_ready,diagnostic_dropped;
    U64 epoch,generation,frame,time,perf_after,next_key,last_draw,prepare_after;
    U64 xr_epoch; /* Explicit XR lifetime, never a transient frame mailbox. */
    float positions[2][3],radius,units; unsigned mask;
    Touch3Basis hand_basis;
    Touch2Common old_common;
    float old_positions[2][3];unsigned old_mask;
    double configured_head_scale,authored_pitch;
    char alignment_json[4096];unsigned alignment_length,alignment_ready;
    U64 composed_epoch,composed_generation,composed_frame; long long composed_time;
    int status,last_status;
    struct { U64 epoch,generation,frame; int status; } states[128];
    unsigned state_read,state_write,state_dropped;
    char inventory[4u*1024u*1024u];
    struct { U64 begin,end,fresh,frames,render,periods,invalid; unsigned context,enabled; } bins[128];
    unsigned bin_read,bin_write,dropped; U64 fresh;
} t2;
static void touch_p2_shader_report(void);
static void touch_p2_bone_cancel(void);
static int touch_p3_configure_contact(void);
static void touch_p3_contact_report(void);
static void touch_p2_config_load(void) {
    if(t2.loaded)return;t2.enabled=1;
    DWORD saved=GetLastError();unsigned status=1;
    HANDLE file=CreateFileW(gf2vr_controller_path(0),0x80000000u,7,0,3,128,0);
    if(file!=INVALID_HANDLE_VALUE){
        DWORD n=0;
        int ok=ReadFile(file,t2.contact_config,sizeof(t2.contact_config),&n,0);CloseHandle(file);
        if(!ok || n>=sizeof(t2.contact_config) || !touch2_config_parse(t2.contact_config,n,&t2.enabled)){t2.enabled=0;status=0;}
        else {status=2;t2.contact_config_length=n;}
    }else if(GetLastError()!=2 && GetLastError()!=3){t2.enabled=0;status=0;}
    t2.contact_config_length=0;
#ifdef GF2VR_DIRTY_CONTACT
    // Hand visibility is common. Optional deformation settings are Dirty-only.
    file=CreateFileW(gf2vr_controller_path(1),0x80000000u,7,0,3,128,0);
    if(file!=INVALID_HANDLE_VALUE){
        DWORD n=0;int ok=ReadFile(file,t2.contact_config,sizeof(t2.contact_config),&n,0);CloseHandle(file);
        if(ok && n<sizeof(t2.contact_config))t2.contact_config_length=n;
        else {t2.contact_config[0]='x';t2.contact_config_length=1;}
    }else if(GetLastError()!=2 && GetLastError()!=3){t2.contact_config[0]='x';t2.contact_config_length=1;}
#endif
    event(status?"touch_p2_config_ready":"touch_p2_config_invalid_disabled",((U64)status<<32)|t2.enabled,GetCurrentThreadId());
    __atomic_store_n(&t2.loaded,1,__ATOMIC_RELEASE);
    SetLastError(saved);
}
static unsigned touch_p2_find(void **out) {
    *out=0;void *domain=sa.domain_get();if(!domain)return 2;
    U64 count=0;void **assemblies=sa.domain_get_assemblies(domain,&count);
    if(!assemblies || count>4096)return 2;
    unsigned matches=0;
    for(U64 i=0;i<count;++i){void *image=sa.assembly_get_image(assemblies[i]);
        if(image && source_equal(sa.image_get_name(image),"GF2.Private.TouchBridge.dll")){
            *out=image;if(++matches>1)return matches;
        }
    }return matches;
}
static int touch_p2_absent(void *unused){(void)unused;void *image=0;return touch_p2_find(&image)==0;}
static void *touch_p2_resolve(void *unused){(void)unused;void *image=0;return touch_p2_find(&image)==1?sa.class_from_name(image,"GF2.Private","ControllerDisplay"):0;}
static int touch_p2_image_load(void) {
    CompositionImageApi *a=&t2.api;CompositionImage *s=&t2.image;
    if(s->attempted || !composition_image_bind(a))return 0;
    a->image_absent=touch_p2_absent;a->resolve_class=touch_p2_resolve;s->attempted=1;
    if(!a->image_absent(0))return 0;
    size_t size=sizeof(gf2_touch_bridge_package);
    if(!size || size>1048576)return 0;
    void *array=a->array_new(a->byte_class,size);if(!array)return 0;
    unsigned root=a->pin(array,1);if(!root)return 0;
    int ok=a->object_class(array)==a->byte_array_class && a->array_length(array)==size;
    for(size_t i=0;ok && i<size;++i){
        uint8_t value=gf2_touch_bridge_package[i];int32_t index=(int32_t)i;
        void *box=a->value_box(a->byte_class,&value);
        if(!box || a->object_class(box)!=a->byte_class){ok=0;break;}
        void *args[]={box,&index},*exception=0;a->invoke(a->set_value,array,args,&exception);
        if(exception){ok=0;break;}
        void *get[]={&index};void *read=a->invoke(a->get_value,array,get,&exception);
        const uint8_t *actual=read && !exception && a->object_class(read)==a->byte_class?a->unbox(read):0;
        if(!actual || *actual!=value)ok=0;
    }
    if(ok){void *args[]={array},*exception=0;void *assembly=a->invoke(a->load,0,args,&exception);
        if(!exception && assembly && a->assignable(a->assembly_class,a->object_class(assembly))){
            s->assembly_root=a->pin(assembly,1);if(s->assembly_root)s->bridge_class=a->resolve_class(0);
        }
    }
    a->unpin(root);return s->assembly_root && s->bridge_class;
}
static int touch_p2_bind(void) {
    if(t2.bound)return 1;if(t2.fault || !sa.runtime_invoke)return 0;
    if(!touch_p2_image_load()){t2.fault=1;event("touch_p2_load_failed",0,GetCurrentThreadId());return 0;}
    HINSTANCE game=GetModuleHandleW(L"GameAssembly.dll");
    unsigned char (*generic)(void *)=game?(void *)il2cpp_export(game,"il2cpp_method_is_generic"):0;
    if(!generic){t2.fault=1;return 0;}
    const char *p[]={"UnityEngine.Camera","System.Int64","System.Int64","System.Int32&"};
    const char *d[]={"UnityEngine.Camera","UnityEngine.Camera","UnityEngine.Camera","UnityEngine.Vector3","UnityEngine.Vector3","System.Single","System.Single","System.Int32","System.Int64","System.Int64","System.Int64","System.Int32&"};
    const char *r[]={"System.Int32&"},*s[]={"UnityEngine.Camera","System.Int32"};
    void *klass=t2.image.bridge_class;
    t2.prepare=eye_binding_method_args(klass,"Prepare",4,p,"System.Void",1,generic);
    t2.draw=eye_binding_method_args(klass,"Draw",12,d,"System.Void",1,generic);
    t2.reset=eye_binding_method_args(klass,"Reset",1,r,"System.Void",1,generic);
    t2.snapshot=eye_binding_method_args(klass,"Snapshot",2,s,"System.String",1,generic);
    t2.diagnostics=eye_binding_method_args(klass,"Diagnostics",0,0,"System.String",1,generic);
#ifdef GF2VR_DIRTY_CONTACT
    const char *ch[]={"System.Int64","System.Int64","System.Int32&"};
    t2.contact_haptics=eye_binding_method_args(klass,"DrainContactHaptics",3,ch,"System.Void",1,generic);
#endif
    const char *cc[]={"System.Byte[]","System.Int32&"};
    t2.contact_configure=eye_binding_method_args(klass,"ConfigureContact",2,cc,"System.Void",1,generic);
    t2.contact_service=eye_binding_method_args(klass,"ServiceContact",1,r,"System.Void",1,generic);
    t2.contact_diagnostics=eye_binding_method_args(klass,"ContactDiagnostics",0,0,"System.String",1,generic);
    void *image=0,*bone=touch_p2_find(&image)==1?sa.class_from_name(image,"GF2.Private","BoneReadProbe"):0;
    if(bone){
        t2.bone_sample=eye_binding_method_args(bone,"Sample",0,0,"System.Void",1,generic);
        t2.bone_drain=eye_binding_method_args(bone,"Drain",0,0,"System.String",1,generic);
        t2.bone_reset=eye_binding_method_args(bone,"Reset",0,0,"System.Void",1,generic);
    }
    t2.bone_bound=t2.bone_sample && t2.bone_drain && t2.bone_reset;
    t2.bound=t2.prepare && t2.draw && t2.reset && t2.snapshot && t2.diagnostics &&
        t2.contact_configure && t2.contact_service && t2.contact_diagnostics;
    if(!t2.bound)t2.fault=1;
    if(t2.bound && !touch_p3_configure_contact()){t2.enabled=0;t2.fault=1;}
    event("touch_p2_binding",t2.bound,GetCurrentThreadId());
    event("touch_p2_bone_binding",t2.bone_bound,GetCurrentThreadId());return t2.bound && !t2.fault;
}
static int touch_p2_void(void *method,void **args,int *status) {
    *status=-999;void *exception=0;
    unsigned prior=t2.busy;t2.busy=1;
    sa.runtime_invoke(method,0,args,&exception);
    t2.busy=prior;
    if(exception){*status=-998;t2.fault=1;return 0;}return 1;
}
/* One cold transfer through the same verified byte-array API as image load.
   No new IL2CPP export, string layout, per-frame allocation or file read. */
static int touch_p3_configure_contact(void){
    if(t2.contact_configured)return 1;
    CompositionImageApi *a=&t2.api;void *array=0;unsigned root=0;int ok=1;
    unsigned length=t2.contact_config_length;
    if(length){
        if(length>4096 || !a->array_new || !a->pin || !a->unpin || !a->invoke ||
           !a->value_box || !a->object_class || !a->array_length || !a->unbox ||
           !a->byte_class || !a->byte_array_class || !a->set_value || !a->get_value)return 0;
        array=a->array_new(a->byte_class,length);if(!array)return 0;
        root=a->pin(array,1);if(!root)return 0;
        ok=a->object_class(array)==a->byte_array_class && a->array_length(array)==length;
        for(unsigned i=0;ok && i<length;++i){
            uint8_t value=(uint8_t)t2.contact_config[i];int32_t index=(int32_t)i;
            void *box=a->value_box(a->byte_class,&value);
            if(!box || a->object_class(box)!=a->byte_class){ok=0;break;}
            void *args[]={box,&index},*exception=0;a->invoke(a->set_value,array,args,&exception);
            if(exception){ok=0;break;}
            void *get[]={&index};void *read=a->invoke(a->get_value,array,get,&exception);
            const uint8_t *actual=read && !exception && a->object_class(read)==a->byte_class?a->unbox(read):0;
            if(!actual || *actual!=value)ok=0;
        }
    }
    int status=-999;
    if(ok){void *args[]={array,&status};ok=touch_p2_void(t2.contact_configure,args,&status) && (status==1 || status==-70);}
    if(root)a->unpin(root);
    if(ok)t2.contact_configured=1;
    event(ok?"touch_p3_config_ready":"touch_p3_config_invalid_disabled",((U64)ok<<32)|length,GetCurrentThreadId());
    return ok;
}
static void touch_p2_status(int status) {
    if(status!=t2.last_status){
        t2.last_status=status;
        unsigned write=__atomic_load_n(&t2.state_write,__ATOMIC_RELAXED),read=__atomic_load_n(&t2.state_read,__ATOMIC_ACQUIRE);
        if(write-read<128){t2.states[write%128]=(typeof(t2.states[0])){t2.epoch,t2.generation,t2.composed_frame,status};__atomic_store_n(&t2.state_write,write+1,__ATOMIC_RELEASE);}else __atomic_fetch_add(&t2.state_dropped,1,__ATOMIC_RELAXED);
    }
}
static void touch_p2_reset(void) {
#ifdef GF2VR_DIRTY_CONTACT
    contact_haptic_publish(0,0,0,GetTickCount64(),0);
#endif
    touch_p2_bone_cancel();
    if(t2.bound && t2.prepared){int status;void *args[]={&status};(void)touch_p2_void(t2.reset,args,&status);touch_p2_status(status);touch_p3_contact_report();}
    t2.prepared=0;t2.mask=0;touch3_invalidate(&t2.hand_basis);t2.epoch=t2.generation=t2.last_draw=t2.prepare_after=0;
}
/* Main-owner cold retry only. No source scan or managed call before deadline.
   An already valid contact owner returns without a scan in managed Prepare. */
static int touch_p3_prepare_due(U64 now,U64 *after){
    if(now<*after)return 0;*after=now+1000;return 1;
}
static void touch_p2_prepare(void *camera,U64 epoch,U64 generation) {
    if(t2.busy || GetCurrentThreadId()!=__atomic_load_n(&first_tid,__ATOMIC_ACQUIRE))return;
    DWORD saved=GetLastError();t2.busy=1;touch_p2_config_load();
    if(!t2.enabled || t2.fault || !camera || !epoch || !generation)goto done;
    if(t2.prepared && t2.epoch==epoch && t2.generation==generation){
        if(!touch_p3_prepare_due(GetTickCount64(),&t2.prepare_after))goto done;
        int status;void *args[]={camera,&epoch,&generation,&status};
        (void)touch_p2_void(t2.prepare,args,&status);touch_p2_status(status);
        goto done;
    }
    if(!touch_p2_bind())goto done;
    touch_p2_reset();int status;void *args[]={camera,&epoch,&generation,&status};
    if(touch_p2_void(t2.prepare,args,&status)){
        t2.prepared=1;t2.epoch=epoch;t2.generation=generation;t2.prepare_after=GetTickCount64()+1000;
    }
    touch_p2_status(status);
    touch_p2_shader_report();
    touch_p3_contact_report();
done:t2.busy=0;SetLastError(saved);
}
static void touch_p2_lifetime(U64 epoch){__atomic_store_n(&t2.xr_epoch,epoch,__ATOMIC_RELEASE);}
static void touch_p2_capture(void *owner,const void *input) {
    /* Sole existing XR owner. No SyncActions, buttons/nav mutation or Unity. */
    XrEyeOwner *o=owner;const XrEyeInput *p=input;
    if(o && p && p->valid && p->head_valid && !o->retiring)touch_p2_lifetime(p->reference_epoch);
    if(!__atomic_load_n(&t2.loaded,__ATOMIC_ACQUIRE) || !t2.enabled || !o || !p || !p->valid || !p->head_valid || !o->space || o->retiring ||
       GetCurrentThreadId()!=stereo_aux.thread || !stereo_aux.ready || !stereo_aux.drag.ready ||
       o->session!=stereo_aux.session || o->session!=stereo_aux.drag.session)return;
    Touch2Packet packet={.epoch=p->reference_epoch,.frame=p->frame_id,.time=p->display_time};
    WorldDragActions *a=&stereo_aux.drag;
    for(unsigned h=0;h<2;++h){
        if(!a->spaces[h])continue;
        XrActionStateGetInfo get={.type=XR_TYPE_ACTION_STATE_GET_INFO,.action=a->grip_pose,.subactionPath=a->hand[h]};
        XrActionStatePose state={.type=XR_TYPE_ACTION_STATE_POSE};
        XrSpaceLocation location={.type=XR_TYPE_SPACE_LOCATION};
        XrSpaceLocationFlags required=XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_POSITION_TRACKED_BIT;
        if(a->api.pose(a->session,&get,&state)!=XR_SUCCESS || !state.isActive ||
           a->api.locate(a->spaces[h],o->space,p->display_time,&location)!=XR_SUCCESS ||
           (location.locationFlags&required)!=required)continue;
        packet.position[h][0]=location.pose.position.x;packet.position[h][1]=location.pose.position.y;packet.position[h][2]=location.pose.position.z;
        if(touch2_finite(packet.position[h][0]) && touch2_finite(packet.position[h][1]) && touch2_finite(packet.position[h][2]))packet.tracked[h]=1;
    }
    (void)touch2_publish(&t2.ring,&packet);
}
static void touch_p2_composed(const void *tracking,const void *source,const double *navigation,const float *offset,double head_scale) {
    const UnityTrackingInput *input=tracking;const PoseSourceAnchor *anchor=source;
    t2.mask=0;t2.old_mask=0;t2.composed_epoch=input->reference_epoch;t2.composed_frame=input->frame_id;
    t2.composed_generation=anchor->source_generation;t2.composed_time=input->display_time;
    if(!t2.enabled || !t2.prepared || t2.fault)return;
    Touch2Packet packet;Touch2Common common={.head_scale=head_scale,.units=anchor->units_per_meter};
    for(unsigned i=0;i<3;++i){common.anchor[i]=anchor->position[i];common.offset[i]=offset[i];common.reference_head[i]=input->reference_head.position[i];}
    for(unsigned i=0;i<4;++i)common.navigation[i]=navigation[i];
    t2.old_common=common;t2.configured_head_scale=head_scale;t2.authored_pitch=anchor->pitch;
    if(!touch2_read(&t2.ring,input->reference_epoch,input->frame_id,input->display_time,&packet))return;
    for(unsigned h=0;h<2;++h)if(packet.tracked[h] && touch2_transform(&common,packet.position[h],t2.old_positions[h]))t2.old_mask|=1u<<h;
}
/* Start invalidation precedes every possible pose_compose failure. Final capture
   is read-only and runs only after the actual Stereo Depth output is complete. */
static void touch_p3_begin(void){t2.mask=0;t2.old_mask=0;touch3_invalidate(&t2.hand_basis);}
static void touch_p3_final(const void *math,const void *tracking,const void *source,
    const void *rendered,const double *final,const double *inverse_head) {
    const PoseMath *m=math;const UnityTrackingInput *input=tracking;
    const PoseSourceAnchor *anchor=source;const ManagedEyePose *eyes=rendered;
    t2.mask=0;
    if(!m || !input || !anchor || !eyes || !final || !inverse_head ||
       !t2.enabled || !t2.prepared || t2.fault)return;
    t2.composed_epoch=input->reference_epoch;t2.composed_generation=anchor->source_generation;
    t2.composed_frame=input->frame_id;t2.composed_time=input->display_time;
    double rotation[4];pose_multiply(final,inverse_head,rotation);
    float raw[2][3],output[2][3];
    for(unsigned eye=0;eye<2;++eye)for(unsigned axis=0;axis<3;++axis){raw[eye][axis]=input->eyes[eye].position[axis];output[eye][axis]=eyes[eye].position[axis];}
    if(!touch3_capture(&t2.hand_basis,input->reference_epoch,anchor->source_generation,
        input->frame_id,input->display_time,raw,output,rotation,anchor->units_per_meter,m->sqrt))return;
    double radius=.035*t2.hand_basis.k*t2.hand_basis.units;
    if(!touch2_finite(radius) || radius<=0 || radius>1000){touch3_invalidate(&t2.hand_basis);return;}
    double character_units=t2.old_common.head_scale*t2.old_common.units;
    if(!touch2_finite(character_units) || character_units<=0 || character_units>FLT_MAX){touch3_invalidate(&t2.hand_basis);return;}
    if(!((float)radius>0) || !((float)character_units>0)){touch3_invalidate(&t2.hand_basis);return;}
    /* Keep accepted002 body/displacement/velocity scale byte-for-byte in value;
       only the hand center and physical hand radius follow actual eye IPD. */
    t2.radius=(float)radius;t2.units=(float)character_units;
    Touch2Packet packet;
    if(!touch2_read(&t2.ring,input->reference_epoch,input->frame_id,input->display_time,&packet))return;
    for(unsigned h=0;h<2;++h)if(packet.tracked[h] && touch3_transform(&t2.hand_basis,packet.position[h],t2.positions[h]))t2.mask|=1u<<h;
}
static void touch_p2_draw(void *opaque) {
    SourceRenderFlight *s=opaque;SourceRenderFrame *f=s?&s->frame:0;
    if(!f || t2.busy || GetCurrentThreadId()!=__atomic_load_n(&first_tid,__ATOMIC_ACQUIRE) || !t2.enabled || t2.fault || !t2.prepared)return;
    const PoseAppliedTicket *p=&f->check.ticket;
    if(!stereo_two_frame_current(s) || !source_render_frame_current(f) || !s->scope.left || !s->scope.right ||
       p->reference_epoch!=t2.epoch || p->source_generation!=t2.generation ||
       p->reference_epoch!=t2.composed_epoch || p->source_generation!=t2.composed_generation ||
       p->frame_id!=t2.composed_frame || p->display_time!=t2.composed_time || p->frame_id==t2.last_draw)return;
    DWORD saved=GetLastError();int mask=(int)t2.mask,status;
    U64 epoch=p->reference_epoch,generation=p->source_generation,frame=p->frame_id;
    if(!touch3_current(&t2.hand_basis,epoch,generation,frame,p->display_time))mask=0;
    void *args[]={f->data->source_camera,s->scope.left,s->scope.right,t2.positions[0],t2.positions[1],&t2.radius,&t2.units,&mask,&epoch,&generation,&frame,&status};
    if(touch_p2_void(t2.draw,args,&status))t2.last_draw=frame;
    if(status==-9 || status==-2 || status==-1)t2.fault=1;
    touch_p2_status(status);SetLastError(saved);
}
/* Writer-only disk path; main copies cold managed JSON to a bounded mailbox. */
static void touch_p2_write(const char *text,unsigned length){
    if(!GF2VR_LOGGING_ALLOWED())return;
    HANDLE file=CreateFileW(gf2vr_controller_path(2),FILE_APPEND_DATA,7,0,OPEN_ALWAYS,128,0);
    if(file==INVALID_HANDLE_VALUE)return;
    while(length){DWORD done=0;if(!WriteFile(file,text,length,&done,0)||!done)break;text+=done;length-=done;}CloseHandle(file);
}
/* Cold metadata strings -> fixed POD mailboxes. The writer alone does disk IO. */
static int touch_p2_ascii(const unsigned short *chars,int length,char *out,unsigned capacity,unsigned *written){
    if(!chars || !out || !written || length<0 || (unsigned)length+1>=capacity)return 0;
    for(int i=0;i<length;++i)if(chars[i]>127)return 0;
    for(int i=0;i<length;++i)out[i]=(char)chars[i];
    out[length]='\n';*written=(unsigned)length+1;return 1;
}
static int touch_p2_queue_string(void *method,char *out,unsigned capacity,unsigned *length,unsigned *ready){
    /* 0: not ready/backpressure (Drain not consumed), 1: queued, -1: terminal
       consumption failure. Never poll a consumed managed result indefinitely. */
    if(!method || __atomic_load_n(ready,__ATOMIC_ACQUIRE))return 0;
    void *exception=0;unsigned prior=t2.busy;t2.busy=1;
    void *value=sa.runtime_invoke(method,0,0,&exception);t2.busy=prior;
    if(exception){__atomic_fetch_add(&t2.diagnostic_dropped,1,__ATOMIC_RELAXED);return -1;}
    if(!value)return 0;
    unsigned root=sa.gchandle_new(value,1);
    if(!root){__atomic_fetch_add(&t2.diagnostic_dropped,1,__ATOMIC_RELAXED);return -1;}
    int ok=touch_p2_ascii(sa.string_chars(value),sa.string_length(value),out,capacity,length);
    sa.gchandle_free(root);
    if(ok)__atomic_store_n(ready,1,__ATOMIC_RELEASE);
    else __atomic_fetch_add(&t2.diagnostic_dropped,1,__ATOMIC_RELAXED);
    return ok?1:-1;
}
static void touch_p2_shader_report(void){
    if(!t2.bound)return;
    if(__atomic_load_n(&t2.shader_ready,__ATOMIC_ACQUIRE)){
        __atomic_fetch_add(&t2.diagnostic_dropped,1,__ATOMIC_RELAXED);return;
    }
    (void)touch_p2_queue_string(t2.diagnostics,t2.shader_json,sizeof(t2.shader_json),&t2.shader_length,&t2.shader_ready);
}
static void touch_p3_contact_report(void){
    if(t2.bound && t2.contact_diagnostics)
        (void)touch_p2_queue_string(t2.contact_diagnostics,t2.contact_json,sizeof(t2.contact_json),&t2.contact_length,&t2.contact_ready);
}
static void touch_p3_contact_service(void){
    if(!t2.bound || !t2.prepared || !t2.enabled || t2.fault || !t2.contact_service)return;
    int status;void *args[]={&status};
    if(!touch_p2_void(t2.contact_service,args,&status)){touch_p2_reset();return;}
#ifdef GF2VR_DIRTY_CONTACT
    if(t2.contact_haptics) {
        unsigned route=__atomic_load_n(&contact_haptic_route,__ATOMIC_ACQUIRE);
        int stable=route==t2.haptic_route;t2.haptic_route=route;
        int mask=0;void *feedback[]={&t2.epoch,&t2.generation,&mask};
        void *exception=0;unsigned prior=t2.busy;t2.busy=1;
        sa.runtime_invoke(t2.contact_haptics,0,feedback,&exception);t2.busy=prior;
        if(exception)t2.contact_haptics=0; /* Optional feedback cannot fault hands/contact. */
        else if(stable && (route&1u) && mask>0 && mask<=3)
            contact_haptic_publish((unsigned)mask,t2.epoch,t2.generation,GetTickCount64(),route);
    }
#endif
}
static void touch_p2_bone_cancel(void){
    if(!t2.bone_bound || !t2.bone_pending)return;
    unsigned prior=t2.busy;t2.busy=1;void *exception=0;
    sa.runtime_invoke(t2.bone_reset,0,0,&exception);t2.busy=prior;
    (void)touch_p2_queue_string(t2.bone_drain,t2.bone_json,sizeof(t2.bone_json),&t2.bone_length,&t2.bone_ready);
    t2.bone_pending=0;t2.bone_epoch=t2.bone_generation=t2.bone_until=0;t2.bone_source=0;
}
static void touch_p2_bone_tick(void){
    if(!t2.bone_bound || !t2.bone_pending)return;
    if(!t2.bone_epoch || !t2.bone_generation || GetTickCount64()>=t2.bone_until ||
       t2.fault || !stereo_source.enabled ||
       !stereo_source.generation || !stereo_source.generation->selection.eligible ||
       stereo_source.generation->selection.source!=t2.bone_source ||
       stereo_source.generation->selection.generation!=t2.bone_generation ||
       __atomic_load_n(&t2.xr_epoch,__ATOMIC_ACQUIRE)!=t2.bone_epoch){touch_p2_bone_cancel();return;}
    unsigned prior=t2.busy;t2.busy=1;void *exception=0;
    sa.runtime_invoke(t2.bone_sample,0,0,&exception);t2.busy=prior;
    if(exception){touch_p2_bone_cancel();return;}
    if(touch_p2_queue_string(t2.bone_drain,t2.bone_json,sizeof(t2.bone_json),&t2.bone_length,&t2.bone_ready)){
        t2.bone_pending=0;t2.bone_epoch=t2.bone_generation=t2.bone_until=0;t2.bone_source=0;
    }
}
/* F11-only formatting, never called from composition or Draw. Fixed decimal
   output avoids CRT allocation/imports; null denotes an unrepresentable value. */
static char *touch_p3_number(char *p,double value){
    if(!touch2_finite(value) || value>1e9 || value< -1e9)return literal(p,"null");
    if(value<0){*p++='-';value=-value;}
    U64 scaled=(U64)(value*1000000.0+.5),whole=scaled/1000000u,fraction=scaled%1000000u;
    p=decimal(p,whole);*p++='.';
    for(unsigned divisor=100000;divisor;divisor/=10)*p++=(char)('0'+(fraction/divisor)%10);
    return p;
}
static void touch_p3_alignment_report(void *camera){
    if(__atomic_load_n(&t2.alignment_ready,__ATOMIC_ACQUIRE)){++t2.diagnostic_dropped;return;}
    char *p=literal(t2.alignment_json,"{\"event\":\"touch_p3_hand_alignment\",\"pid\":");
    p=decimal(p,GetCurrentProcessId());p=literal(p,",\"context\":");p=decimal(p,t2.context);
    p=literal(p,",\"source_pointer\":");p=decimal(p,(U64)(uintptr_t)camera);
#define T3_U(k,v) do{p=literal(p,",\"" k "\":");p=decimal(p,(U64)(v));}while(0)
#define T3_D(k,v) do{p=literal(p,",\"" k "\":");p=touch_p3_number(p,(double)(v));}while(0)
    T3_U("valid",t2.hand_basis.valid);T3_U("mask",t2.mask);T3_U("old_mask",t2.old_mask);
    T3_U("epoch",t2.composed_epoch);T3_U("generation",t2.composed_generation);
    T3_U("frame",t2.composed_frame);T3_U("display_time",t2.composed_time);
    T3_U("basis_epoch",t2.hand_basis.epoch);T3_U("basis_generation",t2.hand_basis.generation);
    T3_U("basis_frame",t2.hand_basis.frame);T3_U("basis_display_time",t2.hand_basis.time);
    T3_D("k",t2.hand_basis.k);T3_D("configured_one_over_world_scale",t2.configured_head_scale);
    T3_D("authored_pitch_radians",t2.authored_pitch);T3_D("units_per_meter",t2.hand_basis.units);
    T3_D("preserved_character_unit_scale",t2.units);
    T3_D("raw_ipd_m",t2.hand_basis.raw_ipd);T3_D("rendered_ipd_units",t2.hand_basis.rendered_ipd);
    T3_D("hand_radius_units",t2.radius);
    p=literal(p,",\"new_minus_old_world_difference_m\":[");
    for(unsigned hand=0;hand<2;++hand){if(hand)*p++=',';*p++='[';
        for(unsigned axis=0;axis<3;++axis){if(axis)*p++=',';
            if(t2.hand_basis.valid && (t2.mask&t2.old_mask&(1u<<hand)) && t2.hand_basis.units>0)
                p=touch_p3_number(p,((double)t2.positions[hand][axis]-t2.old_positions[hand][axis])/t2.hand_basis.units);
            else p=literal(p,"null");
        }*p++=']';
    }
    p=literal(p,"]}\n");
#undef T3_U
#undef T3_D
    t2.alignment_length=(unsigned)(p-t2.alignment_json);
    __atomic_store_n(&t2.alignment_ready,1,__ATOMIC_RELEASE);
}
static DWORD WINAPI touch_p2_writer(void *unused){
    (void)unused;
    for(;;){
        if(__atomic_load_n(&t2.alignment_ready,__ATOMIC_ACQUIRE)){touch_p2_write(t2.alignment_json,t2.alignment_length);__atomic_store_n(&t2.alignment_ready,0,__ATOMIC_RELEASE);}
        if(__atomic_load_n(&t2.contact_ready,__ATOMIC_ACQUIRE)){touch_p2_write(t2.contact_json,t2.contact_length);__atomic_store_n(&t2.contact_ready,0,__ATOMIC_RELEASE);}
        if(__atomic_load_n(&t2.shader_ready,__ATOMIC_ACQUIRE)){touch_p2_write(t2.shader_json,t2.shader_length);__atomic_store_n(&t2.shader_ready,0,__ATOMIC_RELEASE);}
        if(__atomic_load_n(&t2.bone_ready,__ATOMIC_ACQUIRE)){touch_p2_write(t2.bone_json,t2.bone_length);__atomic_store_n(&t2.bone_ready,0,__ATOMIC_RELEASE);}
        if(__atomic_load_n(&t2.ready_snapshot,__ATOMIC_ACQUIRE)){touch_p2_write(t2.inventory,t2.length);__atomic_store_n(&t2.ready_snapshot,0,__ATOMIC_RELEASE);}
        unsigned sr=__atomic_load_n(&t2.state_read,__ATOMIC_RELAXED),sw=__atomic_load_n(&t2.state_write,__ATOMIC_ACQUIRE);
        while(sr!=sw){char line[400],*p=literal(line,"{\"event\":\"touch_p2_state\",\"pid\":");p=decimal(p,GetCurrentProcessId());
            p=literal(p,",\"epoch\":");p=decimal(p,t2.states[sr%128].epoch);p=literal(p,",\"generation\":");p=decimal(p,t2.states[sr%128].generation);
            p=literal(p,",\"frame\":");p=decimal(p,t2.states[sr%128].frame);p=literal(p,",\"status\":");int status=t2.states[sr%128].status;
            if(status<0){*p++='-';p=decimal(p,(U64)(-(long long)status));}else p=decimal(p,(U64)status);
            p=literal(p,"}\n");touch_p2_write(line,(unsigned)(p-line));++sr;__atomic_store_n(&t2.state_read,sr,__ATOMIC_RELEASE);
        }
        unsigned read=__atomic_load_n(&t2.bin_read,__ATOMIC_RELAXED),write=__atomic_load_n(&t2.bin_write,__ATOMIC_ACQUIRE);
        while(read!=write){char line[900],*p=literal(line,"{\"event\":\"touch_p2_perf_bin\",\"pid\":");p=decimal(p,GetCurrentProcessId());
#define T2_BIN(k,v) do{p=literal(p,",\"" k "\":");p=decimal(p,(U64)(v));}while(0)
            T2_BIN("context",t2.bins[read%128].context);T2_BIN("enabled",t2.bins[read%128].enabled);
            T2_BIN("begin_ms",t2.bins[read%128].begin);T2_BIN("end_ms",t2.bins[read%128].end);
            T2_BIN("fresh",t2.bins[read%128].fresh);T2_BIN("xr_frames",t2.bins[read%128].frames);T2_BIN("render_frames",t2.bins[read%128].render);
            T2_BIN("period_us_sum",t2.bins[read%128].periods);T2_BIN("invalid_mode_frames",t2.bins[read%128].invalid);
            T2_BIN("dropped",__atomic_load_n(&t2.dropped,__ATOMIC_RELAXED));T2_BIN("state_dropped",__atomic_load_n(&t2.state_dropped,__ATOMIC_RELAXED));
            T2_BIN("diagnostic_dropped",__atomic_load_n(&t2.diagnostic_dropped,__ATOMIC_RELAXED));
#undef T2_BIN
            p=literal(p,"}\n");touch_p2_write(line,(unsigned)(p-line));++read;__atomic_store_n(&t2.bin_read,read,__ATOMIC_RELEASE);
        }Sleep(100);
    }return 0;
}
static void touch_p2_fresh(void){__atomic_fetch_add(&t2.fresh,1,__ATOMIC_RELAXED);}
static void touch_p2_mode(unsigned mode){__atomic_store_n(&t2.mode,mode,__ATOMIC_RELEASE);}
static void touch_p2_xr(long long period,int render){
    static unsigned context;static U64 total,begin,prior,frames,rendered,periods,invalid;
    unsigned selected=__atomic_load_n(&t2.perf_context,__ATOMIC_ACQUIRE);if(!selected)return;
    U64 now=GetTickCount64(),fresh=__atomic_load_n(&t2.fresh,__ATOMIC_RELAXED);
    if(selected!=context){context=selected;total=begin=now;prior=fresh;frames=rendered=periods=invalid=0;return;}
    if(now-total>32000)return;++frames;rendered+=render!=0;if(period>0)periods+=(U64)period/1000;
    unsigned mode=__atomic_load_n(&t2.mode,__ATOMIC_ACQUIRE);invalid+=mode!=1&&mode!=2;
    if(now-begin<2000)return;
    unsigned write=__atomic_load_n(&t2.bin_write,__ATOMIC_RELAXED),read=__atomic_load_n(&t2.bin_read,__ATOMIC_ACQUIRE);
    if(write-read<128){t2.bins[write%128]=(typeof(t2.bins[0])){begin,now,fresh-prior,frames,rendered,periods,invalid,context,t2.enabled};__atomic_store_n(&t2.bin_write,write+1,__ATOMIC_RELEASE);}else __atomic_fetch_add(&t2.dropped,1,__ATOMIC_RELAXED);
    begin=now;prior=fresh;frames=rendered=periods=invalid=0;
}
static void touch_p2_snapshot(void){
    if(t2.busy)return;t2.busy=1;
    if(!touch_p2_bind()){t2.busy=0;return;}
    void *camera=stereo_source.generation?stereo_source.generation->selection.source:0;
    int context=(int)t2.context;void *args[]={camera,&context},*exception=0;
    void *value=sa.runtime_invoke(t2.snapshot,0,args,&exception);
    t2.busy=0;
    touch_p3_alignment_report(camera);
    if(!exception && t2.bone_bound){
        t2.bone_pending=1;t2.bone_epoch=__atomic_load_n(&t2.xr_epoch,__ATOMIC_ACQUIRE);
        t2.bone_generation=stereo_source.generation?stereo_source.generation->selection.generation:0;
        t2.bone_source=camera;
        t2.bone_until=GetTickCount64()+2500; /* Also bounds full/blocked writer polling. */
    }
    if(exception || !value){event("touch_p2_snapshot_failed",t2.context,GetCurrentThreadId());return;}
    unsigned root=sa.gchandle_new(value,1);if(!root)return;
    int length=sa.string_length(value);const unsigned short *chars=sa.string_chars(value);
    t2.length=0;t2.overflow=length<0 || length>(int)sizeof(t2.inventory)-2 || !chars;
    if(!t2.overflow){for(int i=0;i<length;++i){if(chars[i]>127){t2.overflow=1;break;}t2.inventory[t2.length++]=(char)chars[i];}}
    sa.gchandle_free(root);
    if(!t2.overflow){t2.inventory[t2.length++]='\n';__atomic_store_n(&t2.ready_snapshot,1,__ATOMIC_RELEASE);}else event("touch_p2_snapshot_overflow",length,GetCurrentThreadId());
}
static void touch_p2_tick(U64 sequence){
    (void)sequence;if(t2.busy || GetCurrentThreadId()!=__atomic_load_n(&first_tid,__ATOMIC_ACQUIRE) || !render_installed || !sa.runtime_invoke)return;
    DWORD saved=GetLastError();touch_p2_config_load();
    touch_p2_bone_tick();
    if(t2.prepared && (!t2.enabled || t2.fault || !stereo_source.enabled || !stereo_source.generation || !stereo_source.generation->selection.eligible ||
       stereo_source.generation->selection.generation!=t2.generation ||
       __atomic_load_n(&t2.xr_epoch,__ATOMIC_ACQUIRE)!=t2.epoch))touch_p2_reset();
    touch_p3_contact_service();
    if(!t2.writer){HANDLE worker=CreateThread(0,0,touch_p2_writer,0,0,0);if(worker){CloseHandle(worker);t2.writer=1;}}
    U64 now=GetTickCount64();if(now<t2.next_key){SetLastError(saved);return;}t2.next_key=now+50;
    if(t2.perf_after && now>=t2.perf_after && !__atomic_load_n(&t2.ready_snapshot,__ATOMIC_ACQUIRE)){
        __atomic_store_n(&t2.perf_context,t2.context,__ATOMIC_RELEASE);t2.perf_after=0;
    }
    HINSTANCE user=GetModuleHandleW(L"user32.dll");
    short (WINAPI *key)(int)=user?(void *)GetProcAddress(user,"GetAsyncKeyState"):0;
    void *(WINAPI *foreground)(void)=user?(void *)GetProcAddress(user,"GetForegroundWindow"):0;
    DWORD (WINAPI *process)(void *,DWORD *)=user?(void *)GetProcAddress(user,"GetWindowThreadProcessId"):0;
    DWORD pid=0;void *window=foreground?foreground():0;if(window && process)process(window,&pid);
    unsigned contact_down=key && pid==GetCurrentProcessId() && (((unsigned short)key(0x79)&0x8000u)!=0);
    if(contact_down && !t2.contact_down){
        touch_p3_contact_report();
        event("touch_p3_contact_mark",((U64)t2.context<<32)|t2.enabled,GetCurrentThreadId());
    }
    t2.contact_down=contact_down;
    unsigned down=key && pid==GetCurrentProcessId() && (((unsigned short)key(0x7a)&0x8000u)!=0);
    if(down && !t2.down && t2.context<5 && !__atomic_load_n(&t2.ready_snapshot,__ATOMIC_ACQUIRE)){
        __atomic_store_n(&t2.perf_context,0,__ATOMIC_RELEASE);++t2.context;touch_p2_snapshot();t2.perf_after=GetTickCount64()+3000;
        event("touch_p2_mark",((U64)t2.context<<32)|t2.enabled,GetCurrentThreadId());
    }t2.down=down;SetLastError(saved);
}
#ifdef GF2VR_TOUCH_P2_TEST
static unsigned t2_fixture_resets,t2_fixture_bone_samples,t2_fixture_bone_resets,t2_fixture_drain_fail;
static unsigned t3_fixture_configs,t3_fixture_services,t3_fixture_config_bad,t3_fixture_config_reject,t3_fixture_service_throw;
static void *t3_fixture_array_arg;
static unsigned t3_fixture_unpins,t3_fixture_byte_count,t3_fixture_array_fail,t3_fixture_pin_fail,t3_fixture_copy_fail;
static uint8_t t3_fixture_bytes[4096],t3_fixture_box;
static SourceGeneration t2_fixture_generation;
static void *t3_fixture_array_new(void *klass,unsigned long long length){
    if(klass!=(void *)102 || length>4096 || t3_fixture_array_fail)return 0;
    t3_fixture_byte_count=(unsigned)length;return (void *)101;
}
static void *t3_fixture_value_box(void *klass,void *value){if(klass!=(void *)102)return 0;t3_fixture_box=*(uint8_t *)value;return (void *)104;}
static void *t3_fixture_class(void *value){return value==(void *)101?(void *)103:value==(void *)104?(void *)102:0;}
static unsigned t3_fixture_length(void *array){return array==(void *)101?t3_fixture_byte_count:0;}
static unsigned t3_fixture_pin(void *array,int pinned){return array==(void *)101 && pinned && !t3_fixture_pin_fail?17:0;}
static void t3_fixture_unpin(unsigned root){if(root==17)++t3_fixture_unpins;}
static void *t3_fixture_unbox(void *value){return value==(void *)104?&t3_fixture_box:0;}
static void *t3_fixture_array_invoke(void *method,void *array,void **args,void **exception){
    *exception=0;if(array!=(void *)101){*exception=(void *)1;return 0;}
    int index=*(int32_t *)args[method==(void *)105?1:0];
    if(index<0 || (unsigned)index>=t3_fixture_byte_count){*exception=(void *)1;return 0;}
    if(method==(void *)105){if(t3_fixture_copy_fail){*exception=(void *)1;return 0;}t3_fixture_bytes[index]=t3_fixture_box;return 0;}
    if(method==(void *)106){t3_fixture_box=t3_fixture_bytes[index];return (void *)104;}
    *exception=(void *)1;return 0;
}
static unsigned t2_fixture_pin_fail(void *value,int pinned){(void)value;(void)pinned;return 0;}
static void *t2_fixture_invoke(void *method,void *object,void **args,void **exception){
    (void)object;*exception=0;
    if(method==(void *)1){++t2_fixture_resets;*(int *)args[0]=t2.busy?1:-2;touch_p2_prepare((void *)1,7,8);}
    if(method==(void *)2)++t2_fixture_bone_samples;
    if(method==(void *)3)++t2_fixture_bone_resets;
    if(method==(void *)4 && t2_fixture_drain_fail)return (void *)40;
    if(method==(void *)5){
        ++t3_fixture_configs;t3_fixture_array_arg=args[0];
        *(int *)args[1]=t3_fixture_config_reject?-70:1;
        if(!t2.busy)t3_fixture_config_bad=1;
    }
    if(method==(void *)6){++t3_fixture_services;*(int *)args[0]=1;if(t3_fixture_service_throw)*exception=(void *)1;}
    return 0;
}
/* Fixture only: keep the inert native test free of a new CRT import. Runtime
   uses the existing PoseMath.sqrt function pointer supplied by eye composition. */
static double t3_fixture_sqrt(double value){
    if(value<=0)return 0;double guess=value>1?value:1;
    for(unsigned i=0;i<64;++i)guess=(guess+value/guess)*.5;
    return guess;
}
/* Actual private integration fixture, never runs in the target game. */
__declspec(dllexport) int gf2vr_touch_p2_fixture(void){
    if(target_process())return 1;
    U64 retry_after=0;
    if(!touch_p3_prepare_due(123,&retry_after) || retry_after!=1123)return 33;
    for(U64 i=124;i<1123;++i)if(touch_p3_prepare_due(i,&retry_after))return 34;
    if(!touch_p3_prepare_due(1123,&retry_after) || retry_after!=2123)return 35;
    if(touch_p3_prepare_due(1124,&retry_after) || !touch_p3_prepare_due(5000,&retry_after) || retry_after!=6000)return 36;
    first_tid=GetCurrentThreadId();t2.enabled=1;t2.loaded=1;t2.prepared=1;t2.epoch=7;t2.generation=8;
    Touch2Packet packet={.epoch=7,.frame=101,.time=900,.tracked={1,1},.position={{1,2,3},{-1,2,3}}};
    if(!touch2_publish(&t2.ring,&packet))return 2;
    UnityTrackingInput input={.reference_epoch=7,.frame_id=101,.display_time=900};
    PoseSourceAnchor anchor={.source_generation=8,.position={10,20,30},.units_per_meter=1};
    double nav[4]={0,0,0,1};float offset[3]={4,5,6};
    input.eyes[0].position[0]=-.032f;input.eyes[1].position[0]=.032f;
    ManagedEyePose rendered[2]={{{13.984f,25,36},{0,0,0,1}},{{14.016f,25,36},{0,0,0,1}}};
    PoseMath math={.sqrt=t3_fixture_sqrt};double final[4]={0,0,0,1},inverse[4]={0,0,0,1};
    touch_p3_begin();
    touch_p2_composed(&input,&anchor,nav,offset,.5);
    touch_p3_final(&math,&input,&anchor,rendered,final,inverse);
    if(t2.mask!=3 || __builtin_fabs(t2.positions[0][0]-14.5f)>.0001 || __builtin_fabs(t2.positions[0][1]-26)>.0001 || __builtin_fabs(t2.positions[0][2]-34.5f)>.0001 ||
       __builtin_fabs(t2.positions[1][0]-13.5f)>.0001 || t2.composed_epoch!=7 || t2.composed_frame!=101 ||
       t2.composed_generation!=8 || t2.composed_time!=900 || __builtin_fabs(t2.radius-.0175f)>.000001 || t2.units!=.5f)return 3;
    input.display_time=901;touch_p3_begin();touch_p2_composed(&input,&anchor,nav,offset,.5);touch_p3_final(&math,&input,&anchor,rendered,final,inverse);if(t2.mask)return 4;
    input.display_time=900;t2.enabled=0;touch_p3_begin();touch_p2_composed(&input,&anchor,nav,offset,.5);touch_p3_final(&math,&input,&anchor,rendered,final,inverse);if(t2.mask)return 5;
    t2.enabled=1;packet.tracked[1]=0;if(!touch2_publish(&t2.ring,&packet))return 6;
    touch_p3_begin();touch_p2_composed(&input,&anchor,nav,offset,.5);touch_p3_final(&math,&input,&anchor,rendered,final,inverse);if(t2.mask!=1)return 7;
    final[3]=2;touch_p3_begin();touch_p2_composed(&input,&anchor,nav,offset,.5);touch_p3_final(&math,&input,&anchor,rendered,final,inverse);if(t2.mask || t2.hand_basis.valid)return 8;
    touch_p2_draw(0);touch_p2_capture(0,0);
    t2.state_read=t2.state_write=0;t2.last_status=0;SetLastError(812);
    touch_p2_status(1);touch_p2_status(1);touch_p2_status(-6);
    if(GetLastError()!=812 || t2.state_write!=2 || t2.states[0].status!=1 || t2.states[1].status!=-6)return 9;
    t2.state_write=128;t2.state_read=0;touch_p2_status(0);if(t2.state_write!=128 || t2.state_dropped!=1)return 10;
    t2.bound=1;t2.prepared=1;t2.reset=(void *)1;sa.runtime_invoke=t2_fixture_invoke;
    touch_p2_reset();touch_p2_reset();
    if(t2_fixture_resets!=1 || t2.prepared || t2.mask || t2.epoch || t2.generation || t2.last_draw || t2.busy || t2.fault)return 11;
    /* Regressions found in review: per-frame mailbox gaps are not XR retirement;
       a managed fault must still retire private resources at the safe tick. */
    render_installed=1;t2.writer=1;t2.next_key=~0ull;
    stereo_source.enabled=1;stereo_source.generation=&t2_fixture_generation;
    t2_fixture_generation.selection.eligible=1;t2_fixture_generation.selection.generation=8;
    t2.prepared=1;t2.epoch=7;t2.generation=8;touch_p2_lifetime(7);
    xr_eye_mailbox_invalidate(&stereo_eye_runtime.mailbox);touch_p2_tick(1);
    if(!t2.prepared || t2_fixture_resets!=1)return 12;
    touch_p2_lifetime(0);touch_p2_tick(2);
    if(t2.prepared || t2_fixture_resets!=2)return 13;
    t2.prepared=1;t2.epoch=7;t2.generation=8;touch_p2_lifetime(7);t2.fault=1;touch_p2_tick(3);
    if(t2.prepared || t2_fixture_resets!=3)return 14;
    unsigned short ascii[]={123,125},unicode[]={123,256,125};char buffer[8]={0};unsigned size=999;
    if(!touch_p2_ascii(ascii,2,buffer,sizeof(buffer),&size) || size!=3 || buffer[0]!='{' || buffer[1]!='}' || buffer[2]!='\n')return 15;
    size=999;if(touch_p2_ascii(unicode,3,buffer,sizeof(buffer),&size) || size!=999 ||
       touch_p2_ascii(ascii,2,buffer,3,&size) || touch_p2_ascii(ascii,-1,buffer,sizeof(buffer),&size))return 16;
    /* OFF diagnostic is allowed, but any retired route must cancel BEFORE Sample,
       including same-generation eligibility/identity loss. Managed Reset releases refs. */
    t2.bone_bound=1;t2.bone_sample=(void *)2;t2.bone_reset=(void *)3;t2.bone_drain=(void *)4;
    for(unsigned c=0;c<7;++c){
        t2.bone_pending=1;t2.bone_epoch=7;t2.bone_generation=8;t2.bone_until=~0ull;t2.bone_source=(void *)10;
        t2.enabled=0;t2.fault=0;stereo_source.enabled=1;t2.bone_ready=0;
        t2_fixture_generation.selection.eligible=1;t2_fixture_generation.selection.source=(void *)10;
        if(c==0)t2_fixture_generation.selection.eligible=0;
        if(c==1)stereo_source.enabled=0;
        if(c==2)t2.fault=1;
        if(c==3)t2_fixture_generation.selection.source=(void *)11;
        if(c==4)t2.bone_epoch=0;
        if(c==5)t2.bone_generation=0;
        touch_p2_bone_tick();
        if(c<6 && (t2.bone_pending || t2.bone_source || t2.bone_epoch || t2.bone_generation ||
           t2_fixture_bone_samples || t2_fixture_bone_resets!=c+1))return 17;
    }
    if(t2_fixture_bone_samples!=1 || t2_fixture_bone_resets!=6 || !t2.bone_pending || t2.busy)return 18;
    unsigned dropped=t2.diagnostic_dropped;sa.gchandle_new=t2_fixture_pin_fail;t2_fixture_drain_fail=1;
    touch_p2_bone_tick();touch_p2_bone_tick();
    if(t2.bone_pending || t2.bone_source || t2.bone_epoch || t2.bone_generation || t2.bone_until ||
       t2_fixture_bone_samples!=2 || t2.diagnostic_dropped!=dropped+1)return 19;
    t2_fixture_drain_fail=0;t2.bone_pending=1;t2.bone_epoch=7;t2.bone_generation=8;
    t2.bone_source=(void *)10;t2.bone_until=0;t2.bone_ready=1;
    touch_p2_bone_tick();
    if(t2.bone_pending || t2.bone_source || t2_fixture_bone_samples!=2 || t2_fixture_bone_resets!=7)return 20;
    /* P3 cold configuration defaults, exact verified byte transfer and failures. */
    t2.contact_configure=(void *)5;t2.contact_configured=0;t2.contact_config_length=0;t2.fault=0;
    if(!touch_p3_configure_contact() || t3_fixture_configs!=1 || t3_fixture_array_arg || t3_fixture_config_bad || !t2.contact_configured)return 21;
    if(!touch_p3_configure_contact() || t3_fixture_configs!=1)return 22;
    t2.api=(CompositionImageApi){.byte_class=(void *)102,.byte_array_class=(void *)103,.set_value=(void *)105,.get_value=(void *)106,
        .array_new=t3_fixture_array_new,.value_box=t3_fixture_value_box,.object_class=t3_fixture_class,.array_length=t3_fixture_length,
        .pin=t3_fixture_pin,.unpin=t3_fixture_unpin,.invoke=t3_fixture_array_invoke,.unbox=t3_fixture_unbox};
    t2.contact_config_length=3;t2.contact_config[0]='{';t2.contact_config[1]='}';t2.contact_config[2]='\n';t2.contact_configured=0;
    if(!touch_p3_configure_contact() || t3_fixture_configs!=2 || t3_fixture_array_arg!=(void *)101 || t3_fixture_unpins!=1 ||
       t3_fixture_bytes[0]!='{' || t3_fixture_bytes[1]!='}' || t3_fixture_bytes[2]!='\n' || t3_fixture_config_bad)return 23;
    t2.contact_configured=0;t3_fixture_config_reject=1;
    if(touch_p3_configure_contact() || t2.contact_configured || t3_fixture_unpins!=2)return 24;
    t3_fixture_config_reject=0;t3_fixture_array_fail=1;unsigned calls=t3_fixture_configs;
    if(touch_p3_configure_contact() || t3_fixture_configs!=calls || t3_fixture_unpins!=2)return 25;
    t3_fixture_array_fail=0;t3_fixture_pin_fail=1;
    if(touch_p3_configure_contact() || t3_fixture_configs!=calls || t3_fixture_unpins!=2)return 26;
    t3_fixture_pin_fail=0;t3_fixture_copy_fail=1;
    if(touch_p3_configure_contact() || t3_fixture_configs!=calls || t3_fixture_unpins!=3)return 27;
    t3_fixture_copy_fail=0;t2.contact_config_length=4097;
    if(touch_p3_configure_contact() || t3_fixture_configs!=calls)return 28;
    /* Only normal prepared valid-owner tick can service; fault resets exactly once. */
    t2.contact_service=(void *)6;t2.contact_diagnostics=0;
    t2.bound=1;t2.prepared=1;t2.enabled=1;t2.fault=0;t2.epoch=7;t2.generation=8;
    touch_p3_contact_service();
    if(t3_fixture_services!=1)return 29;
    for(unsigned c=0;c<4;++c){t2.bound=c!=0;t2.prepared=c!=1;t2.enabled=c!=2;t2.fault=c==3;touch_p3_contact_service();}
    if(t3_fixture_services!=1)return 30;
    t2.bound=t2.prepared=t2.enabled=1;t2.fault=0;t3_fixture_service_throw=1;unsigned resets=t2_fixture_resets;
    touch_p3_contact_service();
    if(t3_fixture_services!=2 || !t2.fault || t2.prepared || t2_fixture_resets!=resets+1 || t2.busy)return 31;
    /* The early diagnostic lookup may miss while final capture later receives
       this same XR frame. Common scale/pitch must still belong to this frame. */
    t2.enabled=t2.prepared=1;t2.fault=0;final[3]=1;
    input.frame_id=102;input.display_time=901;anchor.units_per_meter=2;anchor.pitch=.2f;
    touch_p3_begin();touch_p2_composed(&input,&anchor,nav,offset,.7);
    if(t2.old_mask)return 32;
    packet.frame=102;packet.time=901;packet.tracked[0]=packet.tracked[1]=1;if(!touch2_publish(&t2.ring,&packet))return 32;
    touch_p3_final(&math,&input,&anchor,rendered,final,inverse);
    if(t2.mask!=3 || t2.units!=1.4f || t2.configured_head_scale!=.7 || t2.authored_pitch!=(double).2f ||
       !touch3_current(&t2.hand_basis,7,8,102,901))return 32;
    return 0;
}
#endif
#endif
