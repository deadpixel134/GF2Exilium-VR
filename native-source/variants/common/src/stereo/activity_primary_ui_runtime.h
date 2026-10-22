#ifdef GF2VR_STEAM_COMPAT
#line 1
#include "il2cpp_export.h"
#ifndef GF2_ACTIVITY_PRIMARY_UI_RUNTIME_H
#define GF2_ACTIVITY_PRIMARY_UI_RUNTIME_H
#include "activity_ui_config.h"
#ifdef GF2VR_ACTIVITY_UI_SCALE_RESTORE
#include "activity_canvas_scale.h"
#endif
#ifdef GF2VR_ACTIVITY_UI_CANVAS_MEMBERSHIP_OBSERVE
static void acm_set_before(void *,unsigned);
static void acm_set_after(void *,unsigned,int);
#endif
/* Main owns config/managed reads; XR publishes only its panel toggle bit. */
static unsigned primary_ui_aux_shown,primary_ui_loaded,primary_ui_enabled=1;
#ifdef GF2VR_ACTIVITY_CANVAS_SCOPE
#include "activity_canvas_scope.h"
static unsigned primary_ui_scope_disabled;
static int primary_ui_scoped_root(void *,void *);
#ifdef GF2VR_ACTIVITY_UI_GENERATION_LEASE
static void *primary_ui_generation_context;
static U64 primary_ui_generation_id;
#endif
#endif
static void primary_ui_aux(unsigned shown) {__atomic_store_n(&primary_ui_aux_shown,shown!=0,__ATOMIC_RELEASE);}
static unsigned primary_ui_enabled_load(void) {
    if(primary_ui_loaded)return primary_ui_enabled;primary_ui_loaded=1;
    DWORD error=GetLastError();unsigned value=1,status=1;
    HANDLE f=CreateFileW(GF2VR_ACTIVITY_UI_CONFIG,0x80000000u,FILE_SHARE_READ,0,3,FILE_ATTRIBUTE_NORMAL,0);
    if(f!=INVALID_HANDLE_VALUE) {
        static char text[4097];DWORD length=0;
        int read=ReadFile(f,text,sizeof(text),&length,0);CloseHandle(f);
        if(!read || length>=sizeof(text) || !activity_ui_config_parse(text,length,&value))status=0;
        else status=2;
    }else if(GetLastError()!=2 && GetLastError()!=3)status=0;
    primary_ui_enabled=status?value:0;event("activity_primary_ui_config",((U64)status<<32)|primary_ui_enabled,GetCurrentThreadId());
    SetLastError(error);return primary_ui_enabled;
}
static int primary_ui_visible(void) {
#ifdef GF2VR_ACTIVITY_CANVAS_SCOPE
    if(primary_ui_scope_disabled)return 0;
#endif
    return primary_ui_enabled_load() && !__atomic_load_n(&primary_ui_aux_shown,__ATOMIC_ACQUIRE);
}
enum {AP_FIND,AP_ITEM,AP_ROOT,AP_MODE,AP_CAMERA,AP_PLANE,AP_RECT,AP_ACTIVE,AP_METHODS};
static struct {
    ManagedEyeApi calls;void *canvas_class,*array_class,*object_array_class,*float_class,*int_class,*mode_class,*type;
    void *methods[AP_METHODS];unsigned attempted,ready,stage;
} primary_ui_api;
static int primary_ui_bind(const ManagedEyeApi *calls) {
    if(primary_ui_api.attempted)return primary_ui_api.ready;primary_ui_api.attempted=1;
    HINSTANCE game=GetModuleHandleW(L"GameAssembly.dll");if(!game)return 0;
    unsigned char (*generic)(void *)=(void *)il2cpp_export(game,"il2cpp_method_is_generic");
    void *(*array_class)(void *,unsigned)=(void *)il2cpp_export(game,"il2cpp_array_class_get");
    void *(*class_type)(void *)=(void *)il2cpp_export(game,"il2cpp_class_get_type");
    void *(*type_object)(void *)=(void *)il2cpp_export(game,"il2cpp_type_get_object");
    int (*size)(void *,unsigned *)=(void *)il2cpp_export(game,"il2cpp_class_value_size");
    if(!generic || !array_class || !class_type || !type_object || !size || !calls->rect_class || !calls->bool_class)return 0;
    void *canvas=render_class("UnityEngine.UIModule.dll","UnityEngine","Canvas");
    void *object=render_class("UnityEngine.CoreModule.dll","UnityEngine","Object");
    void *array=render_class("mscorlib.dll","System","Array");
    void *behaviour=render_class("UnityEngine.CoreModule.dll","UnityEngine","Behaviour");
    primary_ui_api.float_class=render_class("mscorlib.dll","System","Single");
    primary_ui_api.int_class=render_class("mscorlib.dll","System","Int32");
    primary_ui_api.mode_class=render_class("UnityEngine.UIModule.dll","UnityEngine","RenderMode");
    unsigned align=0;primary_ui_api.stage=2;
    if(!canvas || !object || !array || !behaviour || !primary_ui_api.float_class ||
       size(primary_ui_api.float_class,&align)!=4 || !align || align>4 ||
       !primary_ui_api.mode_class || size(primary_ui_api.mode_class,&align)!=4 || !align || align>4 ||
       !primary_ui_api.int_class || size(primary_ui_api.int_class,&align)!=4 || !align || align>4)return 0;
    primary_ui_api.canvas_class=canvas;primary_ui_api.array_class=array_class(canvas,1);
    primary_ui_api.object_array_class=array_class(object,1);primary_ui_api.calls=*calls;
    const char *names[]={"FindObjectsOfType","GetValue","get_rootCanvas","get_renderMode","get_worldCamera","get_planeDistance","get_pixelRect","get_isActiveAndEnabled"};
    const char *result[]={"UnityEngine.Object[]","System.Object","UnityEngine.Canvas","UnityEngine.RenderMode","UnityEngine.Camera","System.Single","UnityEngine.Rect","System.Boolean"};
    for(unsigned i=0;i<AP_METHODS;++i) {
        void *owner=i==AP_FIND?object:i==AP_ITEM?array:i==AP_ACTIVE?behaviour:canvas;
        primary_ui_api.methods[i]=eye_binding_method(owner,names[i],i==AP_FIND?"System.Type":i==AP_ITEM?"System.Int32":0,result[i],i==AP_FIND,generic);
        if(!primary_ui_api.methods[i]){primary_ui_api.stage=20+i;return 0;}
    }
    /* Type objects remain local/rooted during Find; do not retain an unrooted value. */
    primary_ui_api.type=class_type(canvas); /* metadata Type pointer, process owned */
    primary_ui_api.ready=primary_ui_api.type && primary_ui_api.array_class && primary_ui_api.object_array_class;
    return primary_ui_api.ready;
}
static int primary_ui_value(unsigned method,void *object,void *klass,void *out,unsigned bytes) {
    SourcePoseApi values={0};values.calls=primary_ui_api.calls;
    return source_pose_value(&values,primary_ui_api.methods[method],object,klass,out,bytes);
}
static int primary_ui_root_read(void *canvas,void *camera,float *plane,int width,int height) {
    const ManagedEyeApi *c=&primary_ui_api.calls;void *bound=0,*root=0;
    unsigned mode=99;unsigned char active=0;ActivityViewport rect;
    if(!canvas || c->object_class(canvas)!=primary_ui_api.canvas_class ||
       !primary_ui_value(AP_MODE,canvas,primary_ui_api.mode_class,&mode,4) || mode!=1 ||
       !primary_ui_value(AP_ACTIVE,canvas,c->bool_class,&active,1) || active!=1 ||
       !managed_eye_call(c,primary_ui_api.methods[AP_CAMERA],canvas,0,&bound) || bound!=camera ||
       !managed_eye_call(c,primary_ui_api.methods[AP_ROOT],canvas,0,&root) || root!=canvas ||
       !primary_ui_value(AP_PLANE,canvas,primary_ui_api.float_class,plane,4) || !(*plane>0) ||
       !primary_ui_value(AP_RECT,canvas,c->rect_class,&rect,16))return 0;
    float checked;if(!eye_optics_float(*plane,&checked))return 0;
    return rect.v[0]==0 && rect.v[1]==0 && rect.v[2]==width && rect.v[3]==height;
}
static int primary_ui_source_current(ActivitySourceGroup *a,ActivityPrimarySource *u) {
    float plane=0;
#ifdef GF2VR_ACTIVITY_UI_LIFETIME
    if(!u->canvas)return !u->canvas_root && !u->camera_root;
    /* UI eligibility remains strict, but no longer invalidates the complete
       world owner. Revalidate the exact original typed UI camera/role here. */
    SourceTopologyEntry entry={0};PhaseRoleSnapshot role={0};
    if(!source_topology_entry(a->topology->api,u->entry.identity.source,&entry))return 0;
    phase_role_read(a->binding,entry.identity.source,phase_list_bind,&role);
    if(!activity_entry_same(&entry,&u->entry) || !activity_role_same(&role,&u->role))return 0;
#endif
#ifdef GF2VR_ACTIVITY_UI_GENERATION_LEASE
    if(primary_ui_generation_context==a)return primary_ui_generation_id==a->source->selection.generation &&
        u->camera_root && u->canvas_root && primary_ui_scoped_root(u->canvas,u->entry.identity.source);
#endif
#ifdef GF2VR_ACTIVITY_CANVAS_SCOPE
    if(u->camera_root && u->canvas_root && primary_ui_scoped_root(u->canvas,u->entry.identity.source)) {
#ifdef GF2VR_ACTIVITY_UI_GENERATION_LEASE
        return 0; /* an owned lease never grants another source context */
#else
        return 1;
#endif
    }
#endif
    return u->camera_root && u->canvas_root && primary_ui_api.ready &&
        primary_ui_root_read(u->canvas,u->entry.identity.source,&plane,u->entry.width,u->entry.height) && plane==u->plane;
}
static int primary_ui_current(void *context) {
    ActivitySourceGroup *a=context;if(!a || !primary_ui_source_current(a,&a->active.primary))return 0;
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
    if(a->active.dialogue.canvas && !primary_ui_source_current(a,&a->active.dialogue))return 0;
#endif
    return 1;
}

#ifdef GF2VR_ACTIVITY_CANVAS_SCOPE
/* Bind public signatures in the actual image. No assumed native Canvas layout. */
static struct {
    void *set_mode,*get_transform,*get_matrix,*get_rect,*matrix_class,*rect_transform_class;
#ifdef GF2VR_ACTIVITY_UI_SCALE_RESTORE
    void *get_scale,*set_scale;
#endif
    unsigned attempted,ready;
} primary_ui_canvas_api;
typedef struct {
    ActivityCanvasScope scope;void *canvas,*transform,*camera;float plane;
    unsigned canvas_root,transform_root,camera_root;
#ifdef GF2VR_ACTIVITY_UI_SCALE_RESTORE
    CanvasScaleSnapshot scale;
#endif
} PrimaryUiCanvasOwner;
static PrimaryUiCanvasOwner primary_ui_canvas;
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
static PrimaryUiCanvasOwner dialogue_ui_canvas;
#endif
#ifdef GF2VR_ACTIVITY_UI_LAYOUT_OBSERVE
#include "activity_ui_layout_observe.h"
#else
#define GF2VR_UI_LAYOUT_RESULT(o,g,v) (v)
#endif
static int primary_ui_canvas_bind(void) {
    if(primary_ui_canvas_api.attempted)return primary_ui_canvas_api.ready;
    primary_ui_canvas_api.attempted=1;
    HINSTANCE game=GetModuleHandleW(L"GameAssembly.dll");
    unsigned char (*generic)(void *)=(void *)il2cpp_export(game,"il2cpp_method_is_generic");
    int (*size)(void *,unsigned *)=(void *)il2cpp_export(game,"il2cpp_class_value_size");
    if(!primary_ui_api.ready || !generic || !size)return 0;
    void *component=render_class("UnityEngine.CoreModule.dll","UnityEngine","Component");
    void *transform=render_class("UnityEngine.CoreModule.dll","UnityEngine","Transform");
    primary_ui_canvas_api.rect_transform_class=render_class("UnityEngine.CoreModule.dll","UnityEngine","RectTransform");
    primary_ui_canvas_api.matrix_class=render_class("UnityEngine.CoreModule.dll","UnityEngine","Matrix4x4");
    unsigned align=0;
    if(!component || !transform || !primary_ui_canvas_api.rect_transform_class || !primary_ui_canvas_api.matrix_class ||
       size(primary_ui_canvas_api.matrix_class,&align)!=64 || !align || align>4)return 0;
    primary_ui_canvas_api.set_mode=eye_binding_method(primary_ui_api.canvas_class,"set_renderMode","UnityEngine.RenderMode","System.Void",0,generic);
    primary_ui_canvas_api.get_transform=eye_binding_method(component,"get_transform",0,"UnityEngine.Transform",0,generic);
    primary_ui_canvas_api.get_matrix=eye_binding_method(transform,"get_localToWorldMatrix",0,"UnityEngine.Matrix4x4",0,generic);
    primary_ui_canvas_api.get_rect=eye_binding_method(primary_ui_canvas_api.rect_transform_class,"get_rect",0,"UnityEngine.Rect",0,generic);
#ifdef GF2VR_ACTIVITY_UI_SCALE_RESTORE
    primary_ui_canvas_api.get_scale=eye_binding_method(primary_ui_api.canvas_class,"get_scaleFactor",0,"System.Single",0,generic);
    primary_ui_canvas_api.set_scale=eye_binding_method(primary_ui_api.canvas_class,"set_scaleFactor","System.Single","System.Void",0,generic);
    if(!primary_ui_canvas_api.get_scale || !primary_ui_canvas_api.set_scale)return 0;
#endif
    return primary_ui_canvas_api.ready=primary_ui_canvas_api.set_mode && primary_ui_canvas_api.get_transform &&
        primary_ui_canvas_api.get_matrix && primary_ui_canvas_api.get_rect;
}
static int primary_ui_canvas_mode(void *context,unsigned *mode) {
    PrimaryUiCanvasOwner *o=context?context:&primary_ui_canvas;
    return primary_ui_value(AP_MODE,o->canvas,primary_ui_api.mode_class,mode,4);
}
#ifdef GF2VR_ACTIVITY_UI_SCALE_RESTORE
static int primary_ui_scale_current(void *context) {
    PrimaryUiCanvasOwner *o=context;const ManagedEyeApi *c=&primary_ui_api.calls;
    void *root=0,*camera=0,*transform=0;unsigned mode=99;
    return o && o->scope.active && o->scope.owner==GetCurrentThreadId() && o->scope.serial &&
        o->canvas_root && o->camera_root && o->transform_root &&
        managed_eye_call(c,primary_ui_api.methods[AP_ROOT],o->canvas,0,&root) && root==o->canvas &&
        managed_eye_call(c,primary_ui_api.methods[AP_CAMERA],o->canvas,0,&camera) && camera==o->camera &&
        managed_eye_call(c,primary_ui_canvas_api.get_transform,o->canvas,0,&transform) && transform==o->transform &&
        c->object_class(transform)==primary_ui_canvas_api.rect_transform_class &&
        primary_ui_canvas_mode(o,&mode) && mode==2;
}
static int primary_ui_scale_read(void *context,float *out) {
    PrimaryUiCanvasOwner *o=context;SourcePoseApi read={0};read.calls=primary_ui_api.calls;
    return source_pose_value(&read,primary_ui_canvas_api.get_scale,o->canvas,primary_ui_api.float_class,out,4);
}
static int primary_ui_scale_write(void *context,float value) {
    PrimaryUiCanvasOwner *o=context;void *args[]={&value};
    if(!primary_ui_scale_current(o))return 0;
    return managed_eye_call(&primary_ui_api.calls,primary_ui_canvas_api.set_scale,o->canvas,args,0);
}
static int primary_ui_scale_viewport(void *context,int *width,int *height) {
    PrimaryUiCanvasOwner *o=context;ActivityViewport rect;
    if(!activity_viewport_read(&primary_ui_api.calls,o->camera,&rect))return 0;
    if(rect.v[0]!=0 || rect.v[1]!=0 || rect.v[2]!=(float)o->scale.width || rect.v[3]!=(float)o->scale.height) {
        /* A changed valid viewport is distinct from an unknown failed read. */
        *width=0;*height=0;return 1;
    }
    *width=o->scale.width;*height=o->scale.height;return 1;
}
#endif
static int primary_ui_canvas_set(void *context,unsigned mode) {
    PrimaryUiCanvasOwner *o=context?context:&primary_ui_canvas;const ManagedEyeApi *c=&primary_ui_api.calls;void *root=0,*camera=0;
    /* Unity's native setter walks to the current root. A retained object may
       have been reparented/rebound by the game during a generation lease.
       Never restore through a new owner's root or camera association. Geometry
       changes alone still allow restoration of our own Canvas's render mode. */
    if(!o->canvas_root || !o->camera_root ||
       !managed_eye_call(c,primary_ui_api.methods[AP_ROOT],o->canvas,0,&root) || root!=o->canvas ||
       !managed_eye_call(c,primary_ui_api.methods[AP_CAMERA],o->canvas,0,&camera) || camera!=o->camera)return 0;
#ifdef GF2VR_ACTIVITY_UI_SCALE_RESTORE
    if(mode==1) {
        CanvasScaleApi api={o,primary_ui_scale_current,primary_ui_scale_read,primary_ui_scale_write,primary_ui_scale_viewport};
        if(!canvas_scale_restore(&o->scale,&api))return 0;
    }
#endif
    void *args[]={&mode};
#ifdef GF2VR_ACTIVITY_UI_LAYOUT_OBSERVE
    activity_ui_layout_note(o,1,mode,0,0,0);
#endif
#ifdef GF2VR_ACTIVITY_UI_CANVAS_MEMBERSHIP_OBSERVE
    acm_set_before(o->canvas,mode);
    int ok=managed_eye_call(&primary_ui_api.calls,primary_ui_canvas_api.set_mode,o->canvas,args,0);
    acm_set_after(o->canvas,mode,ok);
#ifdef GF2VR_ACTIVITY_UI_LAYOUT_OBSERVE
    activity_ui_layout_note(o,2,mode,ok!=0,0,0);
#endif
    return ok;
#else
    int ok=managed_eye_call(&primary_ui_api.calls,primary_ui_canvas_api.set_mode,o->canvas,args,0);
#ifdef GF2VR_ACTIVITY_UI_LAYOUT_OBSERVE
    activity_ui_layout_note(o,2,mode,ok!=0,0,0);
#endif
    return ok;
#endif
}
static int primary_ui_canvas_geometry(void *context,float out[20]) {
    PrimaryUiCanvasOwner *o=context?context:&primary_ui_canvas;const ManagedEyeApi *c=&primary_ui_api.calls;void *bound=0,*root=0,*transform=0;
    unsigned char active=0;float plane=0;
    if(!o->canvas_root || !o->transform_root || !o->camera_root ||
       !managed_eye_call(c,primary_ui_api.methods[AP_CAMERA],o->canvas,0,&bound) || bound!=o->camera ||
       !managed_eye_call(c,primary_ui_api.methods[AP_ROOT],o->canvas,0,&root) || root!=o->canvas ||
       !primary_ui_value(AP_ACTIVE,root,c->bool_class,&active,1) || active!=1 ||
       !primary_ui_value(AP_PLANE,root,primary_ui_api.float_class,&plane,4) || plane!=o->plane ||
       !managed_eye_call(c,primary_ui_canvas_api.get_transform,root,0,&transform) || transform!=o->transform ||
       c->object_class(transform)!=primary_ui_canvas_api.rect_transform_class)return 0;
    SourcePoseApi read={0};read.calls=*c;
    return GF2VR_UI_LAYOUT_RESULT(o,out,
        source_pose_value(&read,primary_ui_canvas_api.get_matrix,transform,primary_ui_canvas_api.matrix_class,out,64) &&
        source_pose_value(&read,primary_ui_canvas_api.get_rect,transform,c->rect_class,out+16,16));
}
static ActivityCanvasScopeApi primary_ui_canvas_owner_ops(PrimaryUiCanvasOwner *o) {
    return (ActivityCanvasScopeApi){o,primary_ui_canvas_mode,primary_ui_canvas_set,primary_ui_canvas_geometry};
}
static ActivityCanvasScopeApi primary_ui_canvas_ops(void){return primary_ui_canvas_owner_ops(&primary_ui_canvas);}
static void primary_ui_canvas_release_owner(PrimaryUiCanvasOwner *o) {
    if(o->scope.active)return; /* unknown restoration retains independent roots */
    const ManagedEyeApi *c=&primary_ui_api.calls;
    if(o->transform_root)c->unpin(o->transform_root);
    if(o->canvas_root)c->unpin(o->canvas_root);
    if(o->camera_root)c->unpin(o->camera_root);
    o->canvas_root=o->transform_root=o->camera_root=0;
    o->canvas=o->transform=o->camera=0;
#ifdef GF2VR_ACTIVITY_UI_SCALE_RESTORE
    o->scale=(CanvasScaleSnapshot){0};
#endif
}
static void primary_ui_canvas_release(void){primary_ui_canvas_release_owner(&primary_ui_canvas);}
/* A failed old presentation transaction need not be unresolved ownership.
   Certify the original mode on the SAME retained root/camera, without using
   active/plane/geometry as restoration proof or accepting the old output. */
static int primary_ui_canvas_restoration_proved_owner(PrimaryUiCanvasOwner *o) {
    const ManagedEyeApi *c=&primary_ui_api.calls;
    if(o->scope.active || o->scope.owner!=GetCurrentThreadId() ||
       !primary_ui_api.ready || !o->canvas_root || !o->camera_root ||
       !o->transform_root || !o->canvas || !o->camera ||
       !c->object_class || c->object_class(o->canvas)!=primary_ui_api.canvas_class)return 0;
    void *canvas=o->canvas,*camera=o->camera,*root=0,*bound=0;
    unsigned cr=o->canvas_root,tr=o->transform_root,rr=o->camera_root;
    unsigned mode=99;
    if(!managed_eye_call(c,primary_ui_api.methods[AP_ROOT],canvas,0,&root) || root!=canvas ||
       !managed_eye_call(c,primary_ui_api.methods[AP_CAMERA],canvas,0,&bound) || bound!=camera ||
       !primary_ui_canvas_mode(o,&mode) || mode!=1)return 0;
    root=bound=0;mode=99;
    if(!managed_eye_call(c,primary_ui_api.methods[AP_ROOT],canvas,0,&root) || root!=canvas ||
       !managed_eye_call(c,primary_ui_api.methods[AP_CAMERA],canvas,0,&bound) || bound!=camera ||
       !primary_ui_canvas_mode(o,&mode) || mode!=1)return 0;
    return !o->scope.active && o->scope.owner==GetCurrentThreadId() &&
        o->canvas==canvas && o->camera==camera &&
        o->canvas_root==cr && o->transform_root==tr && o->camera_root==rr;
}
static int primary_ui_canvas_restoration_proved(void){return primary_ui_canvas_restoration_proved_owner(&primary_ui_canvas);}
static int primary_ui_canvas_restore_owner(PrimaryUiCanvasOwner *o,unsigned thread) {
    if(!o->scope.active)return 1;
    ActivityCanvasScopeApi ops=primary_ui_canvas_owner_ops(o);
    int ok=acs_restore(&o->scope,&ops,thread);
    if(thread==o->scope.owner) {
        if(!ok && !primary_ui_canvas_restoration_proved_owner(o))primary_ui_scope_disabled=1;
        primary_ui_canvas_release_owner(o);
    }
    return ok;
}
static int primary_ui_canvas_restore(unsigned thread){return primary_ui_canvas_restore_owner(&primary_ui_canvas,thread);}
static int primary_ui_scoped_root_owner(PrimaryUiCanvasOwner *o,void *canvas,void *camera) {
    if(!o->scope.active || o->scope.failed ||
       o->scope.owner!=GetCurrentThreadId() || canvas!=o->canvas || camera!=o->camera)return 0;
    unsigned mode=99;float check[20];
    return primary_ui_canvas_mode(o,&mode) && mode==2 && primary_ui_canvas_geometry(o,check) &&
        acs_geometry_equal(o->scope.geometry,check);
}
static int primary_ui_scoped_root(void *canvas,void *camera){
    if(primary_ui_scoped_root_owner(&primary_ui_canvas,canvas,camera))return 1;
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
    return primary_ui_scoped_root_owner(&dialogue_ui_canvas,canvas,camera);
#else
    return 0;
#endif
}
static int primary_ui_canvas_begin_owner(PrimaryUiCanvasOwner *o,ActivityPrimarySource *u,U64 serial,int camera) {
    const ManagedEyeApi *c=&primary_ui_api.calls;
    if(primary_ui_scope_disabled || o->scope.active || !primary_ui_canvas_bind())return 0;
    o->canvas=u->canvas;o->camera=u->entry.identity.source;o->plane=u->plane;
    o->canvas_root=c->pin(u->canvas,1);
    if(!o->canvas_root)goto fail;
    o->camera_root=c->pin(u->entry.identity.source,1);
    if(!o->camera_root)goto fail;
    if(!managed_eye_call(c,primary_ui_canvas_api.get_transform,u->canvas,0,&o->transform) ||
       !o->transform || c->object_class(o->transform)!=primary_ui_canvas_api.rect_transform_class)goto fail;
    o->transform_root=c->pin(o->transform,1);
    if(!o->transform_root)goto fail;
#ifdef GF2VR_ACTIVITY_UI_SCALE_RESTORE
    float scale=0;unsigned mode=99;
    if(!primary_ui_canvas_mode(o,&mode) || mode!=1 || !primary_ui_scale_read(o,&scale) ||
       !canvas_scale_capture(&o->scale,scale,u->entry.width,u->entry.height))goto fail;
    static unsigned reports;
    if(reports++<32) {
        union {float f;unsigned bits;} value={scale};
        unsigned lane=0;
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
        lane=o==&dialogue_ui_canvas;
#endif
        event("activity_ui_scale_capture",((U64)lane<<32)|value.bits,GetCurrentThreadId());
    }
#endif
    ActivityCanvasScopeApi ops=primary_ui_canvas_owner_ops(o);
    if(acs_begin(&o->scope,&ops,GetCurrentThreadId(),serial,camera))return 1;
fail:
    primary_ui_scope_disabled=1;primary_ui_canvas_release_owner(o);return 0;
}
static int primary_ui_canvas_begin(ActivityPrimarySource *u,U64 serial,int camera){return primary_ui_canvas_begin_owner(&primary_ui_canvas,u,serial,camera);}
#ifdef GF2VR_ACTIVITY_UI_GENERATION_LEASE
static void primary_ui_generation_report(unsigned operation,int ok) {
    static unsigned reports;
    if(reports>=64 || (ok && reports>=32))return;++reports;
    char line[700],*p=source_prefix(line,"activity_primary_ui_generation_lease");
    p=literal(p,",\"generation\":");p=hexadecimal(p,primary_ui_generation_id);
    p=literal(p,",\"operation\":");p=decimal(p,operation);
    p=literal(p,",\"ok\":");p=decimal(p,ok!=0);
    p=literal(p,",\"active\":");p=decimal(p,primary_ui_canvas.scope.active);
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
    p=literal(p,",\"dialogue_active\":");p=decimal(p,dialogue_ui_canvas.scope.active);
#endif
    p=literal(p,",\"optional_disabled\":");p=decimal(p,primary_ui_scope_disabled);
    p=literal(p,"}\n");source_log(line,p);
}
static int primary_ui_generation_prepare(void *context,U64 generation) {
    ActivitySourceGroup *a=context;
    if(!a || !a->source || a->source->owner_thread!=GetCurrentThreadId() || !generation || primary_ui_scope_disabled ||
       !activity_source_current(a,generation) || !primary_ui_current(a))return 0;
    if(primary_ui_canvas.scope.active)return primary_ui_generation_context==a &&
        primary_ui_generation_id==generation && primary_ui_current(a);
    primary_ui_generation_context=a;primary_ui_generation_id=generation;
    /* camera=1 is an internal lease tag, never a Unity/producer camera grant.
       Exact Canvas/camera identities are independently pinned and checked. */
    int ok=primary_ui_canvas_begin(&a->active.primary,generation,1);
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
    if(ok && a->active.dialogue.canvas){
        ok=primary_ui_canvas_begin_owner(&dialogue_ui_canvas,&a->active.dialogue,generation,2);
        if(!ok)(void)primary_ui_canvas_restore(GetCurrentThreadId());
    }
#endif
    primary_ui_generation_report(0,ok);
    if(!primary_ui_canvas.scope.active){primary_ui_generation_context=0;primary_ui_generation_id=0;}
    return ok;
}
static int primary_ui_generation_release(void *context) {
    if(primary_ui_generation_context && context!=primary_ui_generation_context)return 1;
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
    if(dialogue_ui_canvas.scope.active){
        if(dialogue_ui_canvas.scope.owner!=GetCurrentThreadId())return 0;
        (void)primary_ui_canvas_restore_owner(&dialogue_ui_canvas,GetCurrentThreadId());
    }
#endif
    if(!primary_ui_canvas.scope.active){primary_ui_generation_context=0;primary_ui_generation_id=0;return 1;}
    if(primary_ui_canvas.scope.owner!=GetCurrentThreadId())return 0;
    int ok=primary_ui_canvas_restore(GetCurrentThreadId());primary_ui_generation_report(1,ok);
    /* An uncertain restoration keeps the independent Canvas/Transform/camera
       pins and disables this optional route. Do not retain a stale ASG pointer
       or prevent the already drained world group from being collected. */
    primary_ui_generation_context=0;primary_ui_generation_id=0;return 1;
}
#ifdef GF2VR_ACTIVITY_UI_LIFETIME
static int primary_ui_transition_release(void *context) {
    if(!primary_ui_generation_release(context))return 0;
    /* Generation collection can quarantine an uncertain original lease and
       discard the world. UI-only replacement must not overwrite that owner. */
    return !primary_ui_canvas.scope.active && !dialogue_ui_canvas.scope.active &&
        !primary_ui_canvas.canvas_root && !primary_ui_canvas.transform_root && !primary_ui_canvas.camera_root &&
        !dialogue_ui_canvas.canvas_root && !dialogue_ui_canvas.transform_root && !dialogue_ui_canvas.camera_root;
}
#endif
#endif
#endif
static int primary_ui_capture_source(ActivitySourceGroup *a,ActivityPrimarySource *output,ActivityPrimarySource *old,const void *entry,const void *role) {
    const SourceTopologyEntry *e=entry;const PhaseRoleSnapshot *r=role;
    const ManagedEyeApi *c=&a->binding->refs.settings.calls;
    if(!primary_ui_enabled_load() || !primary_ui_bind(c))return 0;
    primary_ui_api.stage=3;
    ActivityPrimarySource value={0};value.entry=*e;value.role=*r;
    unsigned ar=0,tr=0;int ok=0;void *array=0,*type=0;
    value.camera_root=c->pin(e->identity.source,1);if(!value.camera_root)goto end;
    if(old->canvas_root && old->entry.identity.source==e->identity.source) {
        value.canvas_root=c->pin(old->canvas,1);value.canvas=old->canvas;
        if(value.canvas_root) {
#ifdef GF2VR_ACTIVITY_UI_GENERATION_LEASE
            if(primary_ui_scoped_root(value.canvas,e->identity.source)) {value.plane=old->plane;ok=1;goto verify;}
#endif
            if(primary_ui_root_read(value.canvas,e->identity.source,&value.plane,e->width,e->height)){ok=1;goto verify;}
        }
        if(value.canvas_root)c->unpin(value.canvas_root);value.canvas_root=0;value.canvas=0;
    }
    HINSTANCE game=GetModuleHandleW(L"GameAssembly.dll");
    void *(*type_object)(void *)=(void *)il2cpp_export(game,"il2cpp_type_get_object");
    if(!type_object || !(type=type_object(primary_ui_api.type)) || !(tr=c->pin(type,1)))goto end;
    primary_ui_api.stage=4;
    void *args[]={type};if(!managed_eye_call(c,primary_ui_api.methods[AP_FIND],0,args,&array) || !array || !(ar=c->pin(array,1)))goto end;
    void *klass=c->object_class(array);if(klass!=primary_ui_api.array_class && klass!=primary_ui_api.object_array_class)goto end;
    primary_ui_api.stage=5;unsigned count=sa.array_length(array);if(count>128)goto end;
    for(unsigned n=0;n<count;++n) {
        int index=(int)n;void *ia[]={&index},*canvas=0,*root=0,*bound=0;
        if(!managed_eye_call(c,primary_ui_api.methods[AP_ITEM],array,ia,&canvas) || !canvas || c->object_class(canvas)!=primary_ui_api.canvas_class)goto end;
        unsigned cr=c->pin(canvas,1);if(!cr)goto end;unsigned rr=0;int row=1;
        if(!managed_eye_call(c,primary_ui_api.methods[AP_ROOT],canvas,0,&root) || !root)row=0;
        if(row && !(rr=c->pin(root,1)))row=0;
        if(row && (c->object_class(root)!=primary_ui_api.canvas_class || !managed_eye_call(c,primary_ui_api.methods[AP_CAMERA],root,0,&bound)))row=0;
        if(row && bound==e->identity.source) {
            float plane=0;
            if(!primary_ui_root_read(root,e->identity.source,&plane,e->width,e->height))row=0;
            else if(value.canvas && value.canvas!=root)row=0; /* No arbitrary subset of roots. */
            else if(!value.canvas){value.canvas=root;value.plane=plane;value.canvas_root=rr;rr=0;}
        }
        if(rr)c->unpin(rr);c->unpin(cr);if(!row){primary_ui_api.stage=6;goto end;}
    }
    if(sa.array_length(array)!=count || !value.canvas_root)goto end;ok=1;
verify: {
    primary_ui_api.stage=7;
    float plane=0;ActivityViewport rect;
    int root_ok=primary_ui_root_read(value.canvas,e->identity.source,&plane,e->width,e->height) && plane==value.plane;
#ifdef GF2VR_ACTIVITY_UI_GENERATION_LEASE
    root_ok=root_ok || primary_ui_scoped_root(value.canvas,e->identity.source);
#endif
    if(!root_ok ||
       !activity_viewport_read(c,e->identity.source,&rect) || rect.v[0]!=0 || rect.v[1]!=0 || rect.v[2]!=e->width || rect.v[3]!=e->height)ok=0;
    if(ok){*output=value;value=(ActivityPrimarySource){0};}
}
end:
    if(ar)c->unpin(ar);if(tr)c->unpin(tr);
    if(value.canvas_root)c->unpin(value.canvas_root);if(value.camera_root)c->unpin(value.camera_root);
    static unsigned reports,last_ok[2]={2,2};static void *last_source[2],*last_canvas[2];
    unsigned lane=e->orthographic!=0;
    void *reported_canvas=ok?output->canvas:0;
    if(reports<32 && (last_ok[lane]!=(unsigned)ok || last_source[lane]!=e->identity.source || last_canvas[lane]!=reported_canvas)) {
        DWORD error=GetLastError();++reports;last_ok[lane]=ok;last_source[lane]=e->identity.source;last_canvas[lane]=reported_canvas;
        char line[900],*p=source_prefix(line,e->orthographic?"activity_dialogue_ui_source":"activity_primary_ui_source");
        p=literal(p,",\"ok\":");p=decimal(p,ok);p=literal(p,",\"stage\":");p=decimal(p,ok?0:primary_ui_api.stage);
        p=literal(p,",\"source\":");p=hexadecimal(p,(U64)e->identity.source);p=literal(p,",\"canvas\":");p=hexadecimal(p,(U64)reported_canvas);
        p=literal(p,"}\n");source_log(line,p);SetLastError(error);
    }
    return ok;
}
static int primary_ui_capture(void *context,void *snapshot,const void *entry,const void *role) {
    ActivitySourceGroup *a=context;ActivitySourceSnapshot *s=snapshot;
    return primary_ui_capture_source(a,&s->primary,&a->active.primary,entry,role);
}
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
static int dialogue_ui_capture(void *context,void *snapshot,const void *entry,const void *role) {
    ActivitySourceGroup *a=context;ActivitySourceSnapshot *s=snapshot;
    return primary_ui_capture_source(a,&s->dialogue,&a->active.dialogue,entry,role);
}
static int dialogue_ui_geometry(void *context,U64 generation,float out[20]) {
    ActivitySourceGroup *a=context;
    if(primary_ui_generation_context!=a || primary_ui_generation_id!=generation ||
       !primary_ui_current(a) || !dialogue_ui_canvas.scope.active || dialogue_ui_canvas.scope.failed)return 0;
    if(!primary_ui_canvas_geometry(&dialogue_ui_canvas,out) || !acs_geometry_equal(dialogue_ui_canvas.scope.geometry,out))return 0;
    return primary_ui_current(a);
}
static int primary_ui_geometry(void *context,U64 generation,float out[20]) {
    ActivitySourceGroup *a=context;
    if(primary_ui_generation_context!=a || primary_ui_generation_id!=generation ||
       !primary_ui_current(a) || !primary_ui_canvas.scope.active || primary_ui_canvas.scope.failed)return 0;
    if(!primary_ui_canvas_geometry(&primary_ui_canvas,out) || !acs_geometry_equal(primary_ui_canvas.scope.geometry,out))return 0;
    return primary_ui_current(a);
}
#endif

#endif

#else
#line 1
#ifndef GF2_ACTIVITY_PRIMARY_UI_RUNTIME_H
#define GF2_ACTIVITY_PRIMARY_UI_RUNTIME_H
#include "activity_ui_config.h"
#ifdef GF2VR_ACTIVITY_UI_SCALE_RESTORE
#include "activity_canvas_scale.h"
#endif
#ifdef GF2VR_ACTIVITY_UI_CANVAS_MEMBERSHIP_OBSERVE
static void acm_set_before(void *,unsigned);
static void acm_set_after(void *,unsigned,int);
#endif
/* Main owns config/managed reads; XR publishes only its panel toggle bit. */
static unsigned primary_ui_aux_shown,primary_ui_loaded,primary_ui_enabled=1;
#ifdef GF2VR_ACTIVITY_CANVAS_SCOPE
#include "activity_canvas_scope.h"
static unsigned primary_ui_scope_disabled;
static int primary_ui_scoped_root(void *,void *);
#ifdef GF2VR_ACTIVITY_UI_GENERATION_LEASE
static void *primary_ui_generation_context;
static U64 primary_ui_generation_id;
#endif
#endif
static void primary_ui_aux(unsigned shown) {__atomic_store_n(&primary_ui_aux_shown,shown!=0,__ATOMIC_RELEASE);}
static unsigned primary_ui_enabled_load(void) {
    if(primary_ui_loaded)return primary_ui_enabled;primary_ui_loaded=1;
    DWORD error=GetLastError();unsigned value=1,status=1;
    HANDLE f=CreateFileW(GF2VR_ACTIVITY_UI_CONFIG,0x80000000u,FILE_SHARE_READ,0,3,FILE_ATTRIBUTE_NORMAL,0);
    if(f!=INVALID_HANDLE_VALUE) {
        static char text[4097];DWORD length=0;
        int read=ReadFile(f,text,sizeof(text),&length,0);CloseHandle(f);
        if(!read || length>=sizeof(text) || !activity_ui_config_parse(text,length,&value))status=0;
        else status=2;
    }else if(GetLastError()!=2 && GetLastError()!=3)status=0;
    primary_ui_enabled=status?value:0;event("activity_primary_ui_config",((U64)status<<32)|primary_ui_enabled,GetCurrentThreadId());
    SetLastError(error);return primary_ui_enabled;
}
static int primary_ui_visible(void) {
#ifdef GF2VR_ACTIVITY_CANVAS_SCOPE
    if(primary_ui_scope_disabled)return 0;
#endif
    return primary_ui_enabled_load() && !__atomic_load_n(&primary_ui_aux_shown,__ATOMIC_ACQUIRE);
}
enum {AP_FIND,AP_ITEM,AP_ROOT,AP_MODE,AP_CAMERA,AP_PLANE,AP_RECT,AP_ACTIVE,AP_METHODS};
static struct {
    ManagedEyeApi calls;void *canvas_class,*array_class,*object_array_class,*float_class,*int_class,*mode_class,*type;
    void *methods[AP_METHODS];unsigned attempted,ready,stage;
} primary_ui_api;
static int primary_ui_bind(const ManagedEyeApi *calls) {
    if(primary_ui_api.attempted)return primary_ui_api.ready;primary_ui_api.attempted=1;
    HINSTANCE game=GetModuleHandleW(L"GameAssembly.dll");if(!game)return 0;
    unsigned char (*generic)(void *)=(void *)GetProcAddress(game,"il2cpp_method_is_generic");
    void *(*array_class)(void *,unsigned)=(void *)GetProcAddress(game,"il2cpp_array_class_get");
    void *(*class_type)(void *)=(void *)GetProcAddress(game,"il2cpp_class_get_type");
    void *(*type_object)(void *)=(void *)GetProcAddress(game,"il2cpp_type_get_object");
    int (*size)(void *,unsigned *)=(void *)GetProcAddress(game,"il2cpp_class_value_size");
    if(!generic || !array_class || !class_type || !type_object || !size || !calls->rect_class || !calls->bool_class)return 0;
    void *canvas=render_class("UnityEngine.UIModule.dll","UnityEngine","Canvas");
    void *object=render_class("UnityEngine.CoreModule.dll","UnityEngine","Object");
    void *array=render_class("mscorlib.dll","System","Array");
    void *behaviour=render_class("UnityEngine.CoreModule.dll","UnityEngine","Behaviour");
    primary_ui_api.float_class=render_class("mscorlib.dll","System","Single");
    primary_ui_api.int_class=render_class("mscorlib.dll","System","Int32");
    primary_ui_api.mode_class=render_class("UnityEngine.UIModule.dll","UnityEngine","RenderMode");
    unsigned align=0;primary_ui_api.stage=2;
    if(!canvas || !object || !array || !behaviour || !primary_ui_api.float_class ||
       size(primary_ui_api.float_class,&align)!=4 || !align || align>4 ||
       !primary_ui_api.mode_class || size(primary_ui_api.mode_class,&align)!=4 || !align || align>4 ||
       !primary_ui_api.int_class || size(primary_ui_api.int_class,&align)!=4 || !align || align>4)return 0;
    primary_ui_api.canvas_class=canvas;primary_ui_api.array_class=array_class(canvas,1);
    primary_ui_api.object_array_class=array_class(object,1);primary_ui_api.calls=*calls;
    const char *names[]={"FindObjectsOfType","GetValue","get_rootCanvas","get_renderMode","get_worldCamera","get_planeDistance","get_pixelRect","get_isActiveAndEnabled"};
    const char *result[]={"UnityEngine.Object[]","System.Object","UnityEngine.Canvas","UnityEngine.RenderMode","UnityEngine.Camera","System.Single","UnityEngine.Rect","System.Boolean"};
    for(unsigned i=0;i<AP_METHODS;++i) {
        void *owner=i==AP_FIND?object:i==AP_ITEM?array:i==AP_ACTIVE?behaviour:canvas;
        primary_ui_api.methods[i]=eye_binding_method(owner,names[i],i==AP_FIND?"System.Type":i==AP_ITEM?"System.Int32":0,result[i],i==AP_FIND,generic);
        if(!primary_ui_api.methods[i]){primary_ui_api.stage=20+i;return 0;}
    }
    /* Type objects remain local/rooted during Find; do not retain an unrooted value. */
    primary_ui_api.type=class_type(canvas); /* metadata Type pointer, process owned */
    primary_ui_api.ready=primary_ui_api.type && primary_ui_api.array_class && primary_ui_api.object_array_class;
    return primary_ui_api.ready;
}
static int primary_ui_value(unsigned method,void *object,void *klass,void *out,unsigned bytes) {
    SourcePoseApi values={0};values.calls=primary_ui_api.calls;
    return source_pose_value(&values,primary_ui_api.methods[method],object,klass,out,bytes);
}
static int primary_ui_root_read(void *canvas,void *camera,float *plane,int width,int height) {
    const ManagedEyeApi *c=&primary_ui_api.calls;void *bound=0,*root=0;
    unsigned mode=99;unsigned char active=0;ActivityViewport rect;
    if(!canvas || c->object_class(canvas)!=primary_ui_api.canvas_class ||
       !primary_ui_value(AP_MODE,canvas,primary_ui_api.mode_class,&mode,4) || mode!=1 ||
       !primary_ui_value(AP_ACTIVE,canvas,c->bool_class,&active,1) || active!=1 ||
       !managed_eye_call(c,primary_ui_api.methods[AP_CAMERA],canvas,0,&bound) || bound!=camera ||
       !managed_eye_call(c,primary_ui_api.methods[AP_ROOT],canvas,0,&root) || root!=canvas ||
       !primary_ui_value(AP_PLANE,canvas,primary_ui_api.float_class,plane,4) || !(*plane>0) ||
       !primary_ui_value(AP_RECT,canvas,c->rect_class,&rect,16))return 0;
    float checked;if(!eye_optics_float(*plane,&checked))return 0;
    return rect.v[0]==0 && rect.v[1]==0 && rect.v[2]==width && rect.v[3]==height;
}
static int primary_ui_source_current(ActivitySourceGroup *a,ActivityPrimarySource *u) {
    float plane=0;
#ifdef GF2VR_ACTIVITY_UI_LIFETIME
    if(!u->canvas)return !u->canvas_root && !u->camera_root;
    /* UI eligibility remains strict, but no longer invalidates the complete
       world owner. Revalidate the exact original typed UI camera/role here. */
    SourceTopologyEntry entry={0};PhaseRoleSnapshot role={0};
    if(!source_topology_entry(a->topology->api,u->entry.identity.source,&entry))return 0;
    phase_role_read(a->binding,entry.identity.source,phase_list_bind,&role);
    if(!activity_entry_same(&entry,&u->entry) || !activity_role_same(&role,&u->role))return 0;
#endif
#ifdef GF2VR_ACTIVITY_UI_GENERATION_LEASE
    if(primary_ui_generation_context==a)return primary_ui_generation_id==a->source->selection.generation &&
        u->camera_root && u->canvas_root && primary_ui_scoped_root(u->canvas,u->entry.identity.source);
#endif
#ifdef GF2VR_ACTIVITY_CANVAS_SCOPE
    if(u->camera_root && u->canvas_root && primary_ui_scoped_root(u->canvas,u->entry.identity.source)) {
#ifdef GF2VR_ACTIVITY_UI_GENERATION_LEASE
        return 0; /* an owned lease never grants another source context */
#else
        return 1;
#endif
    }
#endif
    return u->camera_root && u->canvas_root && primary_ui_api.ready &&
        primary_ui_root_read(u->canvas,u->entry.identity.source,&plane,u->entry.width,u->entry.height) && plane==u->plane;
}
static int primary_ui_current(void *context) {
    ActivitySourceGroup *a=context;if(!a || !primary_ui_source_current(a,&a->active.primary))return 0;
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
    if(a->active.dialogue.canvas && !primary_ui_source_current(a,&a->active.dialogue))return 0;
#endif
    return 1;
}

#ifdef GF2VR_ACTIVITY_CANVAS_SCOPE
/* Bind public signatures in the actual image. No assumed native Canvas layout. */
static struct {
    void *set_mode,*get_transform,*get_matrix,*get_rect,*matrix_class,*rect_transform_class;
#ifdef GF2VR_ACTIVITY_UI_SCALE_RESTORE
    void *get_scale,*set_scale;
#endif
    unsigned attempted,ready;
} primary_ui_canvas_api;
typedef struct {
    ActivityCanvasScope scope;void *canvas,*transform,*camera;float plane;
    unsigned canvas_root,transform_root,camera_root;
#ifdef GF2VR_ACTIVITY_UI_SCALE_RESTORE
    CanvasScaleSnapshot scale;
#endif
} PrimaryUiCanvasOwner;
static PrimaryUiCanvasOwner primary_ui_canvas;
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
static PrimaryUiCanvasOwner dialogue_ui_canvas;
#endif
#ifdef GF2VR_ACTIVITY_UI_LAYOUT_OBSERVE
#include "activity_ui_layout_observe.h"
#else
#define GF2VR_UI_LAYOUT_RESULT(o,g,v) (v)
#endif
static int primary_ui_canvas_bind(void) {
    if(primary_ui_canvas_api.attempted)return primary_ui_canvas_api.ready;
    primary_ui_canvas_api.attempted=1;
    HINSTANCE game=GetModuleHandleW(L"GameAssembly.dll");
    unsigned char (*generic)(void *)=(void *)GetProcAddress(game,"il2cpp_method_is_generic");
    int (*size)(void *,unsigned *)=(void *)GetProcAddress(game,"il2cpp_class_value_size");
    if(!primary_ui_api.ready || !generic || !size)return 0;
    void *component=render_class("UnityEngine.CoreModule.dll","UnityEngine","Component");
    void *transform=render_class("UnityEngine.CoreModule.dll","UnityEngine","Transform");
    primary_ui_canvas_api.rect_transform_class=render_class("UnityEngine.CoreModule.dll","UnityEngine","RectTransform");
    primary_ui_canvas_api.matrix_class=render_class("UnityEngine.CoreModule.dll","UnityEngine","Matrix4x4");
    unsigned align=0;
    if(!component || !transform || !primary_ui_canvas_api.rect_transform_class || !primary_ui_canvas_api.matrix_class ||
       size(primary_ui_canvas_api.matrix_class,&align)!=64 || !align || align>4)return 0;
    primary_ui_canvas_api.set_mode=eye_binding_method(primary_ui_api.canvas_class,"set_renderMode","UnityEngine.RenderMode","System.Void",0,generic);
    primary_ui_canvas_api.get_transform=eye_binding_method(component,"get_transform",0,"UnityEngine.Transform",0,generic);
    primary_ui_canvas_api.get_matrix=eye_binding_method(transform,"get_localToWorldMatrix",0,"UnityEngine.Matrix4x4",0,generic);
    primary_ui_canvas_api.get_rect=eye_binding_method(primary_ui_canvas_api.rect_transform_class,"get_rect",0,"UnityEngine.Rect",0,generic);
#ifdef GF2VR_ACTIVITY_UI_SCALE_RESTORE
    primary_ui_canvas_api.get_scale=eye_binding_method(primary_ui_api.canvas_class,"get_scaleFactor",0,"System.Single",0,generic);
    primary_ui_canvas_api.set_scale=eye_binding_method(primary_ui_api.canvas_class,"set_scaleFactor","System.Single","System.Void",0,generic);
    if(!primary_ui_canvas_api.get_scale || !primary_ui_canvas_api.set_scale)return 0;
#endif
    return primary_ui_canvas_api.ready=primary_ui_canvas_api.set_mode && primary_ui_canvas_api.get_transform &&
        primary_ui_canvas_api.get_matrix && primary_ui_canvas_api.get_rect;
}
static int primary_ui_canvas_mode(void *context,unsigned *mode) {
    PrimaryUiCanvasOwner *o=context?context:&primary_ui_canvas;
    return primary_ui_value(AP_MODE,o->canvas,primary_ui_api.mode_class,mode,4);
}
#ifdef GF2VR_ACTIVITY_UI_SCALE_RESTORE
static int primary_ui_scale_current(void *context) {
    PrimaryUiCanvasOwner *o=context;const ManagedEyeApi *c=&primary_ui_api.calls;
    void *root=0,*camera=0,*transform=0;unsigned mode=99;
    return o && o->scope.active && o->scope.owner==GetCurrentThreadId() && o->scope.serial &&
        o->canvas_root && o->camera_root && o->transform_root &&
        managed_eye_call(c,primary_ui_api.methods[AP_ROOT],o->canvas,0,&root) && root==o->canvas &&
        managed_eye_call(c,primary_ui_api.methods[AP_CAMERA],o->canvas,0,&camera) && camera==o->camera &&
        managed_eye_call(c,primary_ui_canvas_api.get_transform,o->canvas,0,&transform) && transform==o->transform &&
        c->object_class(transform)==primary_ui_canvas_api.rect_transform_class &&
        primary_ui_canvas_mode(o,&mode) && mode==2;
}
static int primary_ui_scale_read(void *context,float *out) {
    PrimaryUiCanvasOwner *o=context;SourcePoseApi read={0};read.calls=primary_ui_api.calls;
    return source_pose_value(&read,primary_ui_canvas_api.get_scale,o->canvas,primary_ui_api.float_class,out,4);
}
static int primary_ui_scale_write(void *context,float value) {
    PrimaryUiCanvasOwner *o=context;void *args[]={&value};
    if(!primary_ui_scale_current(o))return 0;
    return managed_eye_call(&primary_ui_api.calls,primary_ui_canvas_api.set_scale,o->canvas,args,0);
}
static int primary_ui_scale_viewport(void *context,int *width,int *height) {
    PrimaryUiCanvasOwner *o=context;ActivityViewport rect;
    if(!activity_viewport_read(&primary_ui_api.calls,o->camera,&rect))return 0;
    if(rect.v[0]!=0 || rect.v[1]!=0 || rect.v[2]!=(float)o->scale.width || rect.v[3]!=(float)o->scale.height) {
        /* A changed valid viewport is distinct from an unknown failed read. */
        *width=0;*height=0;return 1;
    }
    *width=o->scale.width;*height=o->scale.height;return 1;
}
#endif
static int primary_ui_canvas_set(void *context,unsigned mode) {
    PrimaryUiCanvasOwner *o=context?context:&primary_ui_canvas;const ManagedEyeApi *c=&primary_ui_api.calls;void *root=0,*camera=0;
    /* Unity's native setter walks to the current root. A retained object may
       have been reparented/rebound by the game during a generation lease.
       Never restore through a new owner's root or camera association. Geometry
       changes alone still allow restoration of our own Canvas's render mode. */
    if(!o->canvas_root || !o->camera_root ||
       !managed_eye_call(c,primary_ui_api.methods[AP_ROOT],o->canvas,0,&root) || root!=o->canvas ||
       !managed_eye_call(c,primary_ui_api.methods[AP_CAMERA],o->canvas,0,&camera) || camera!=o->camera)return 0;
#ifdef GF2VR_ACTIVITY_UI_SCALE_RESTORE
    if(mode==1) {
        CanvasScaleApi api={o,primary_ui_scale_current,primary_ui_scale_read,primary_ui_scale_write,primary_ui_scale_viewport};
        if(!canvas_scale_restore(&o->scale,&api))return 0;
    }
#endif
    void *args[]={&mode};
#ifdef GF2VR_ACTIVITY_UI_LAYOUT_OBSERVE
    activity_ui_layout_note(o,1,mode,0,0,0);
#endif
#ifdef GF2VR_ACTIVITY_UI_CANVAS_MEMBERSHIP_OBSERVE
    acm_set_before(o->canvas,mode);
    int ok=managed_eye_call(&primary_ui_api.calls,primary_ui_canvas_api.set_mode,o->canvas,args,0);
    acm_set_after(o->canvas,mode,ok);
#ifdef GF2VR_ACTIVITY_UI_LAYOUT_OBSERVE
    activity_ui_layout_note(o,2,mode,ok!=0,0,0);
#endif
    return ok;
#else
    int ok=managed_eye_call(&primary_ui_api.calls,primary_ui_canvas_api.set_mode,o->canvas,args,0);
#ifdef GF2VR_ACTIVITY_UI_LAYOUT_OBSERVE
    activity_ui_layout_note(o,2,mode,ok!=0,0,0);
#endif
    return ok;
#endif
}
static int primary_ui_canvas_geometry(void *context,float out[20]) {
    PrimaryUiCanvasOwner *o=context?context:&primary_ui_canvas;const ManagedEyeApi *c=&primary_ui_api.calls;void *bound=0,*root=0,*transform=0;
    unsigned char active=0;float plane=0;
    if(!o->canvas_root || !o->transform_root || !o->camera_root ||
       !managed_eye_call(c,primary_ui_api.methods[AP_CAMERA],o->canvas,0,&bound) || bound!=o->camera ||
       !managed_eye_call(c,primary_ui_api.methods[AP_ROOT],o->canvas,0,&root) || root!=o->canvas ||
       !primary_ui_value(AP_ACTIVE,root,c->bool_class,&active,1) || active!=1 ||
       !primary_ui_value(AP_PLANE,root,primary_ui_api.float_class,&plane,4) || plane!=o->plane ||
       !managed_eye_call(c,primary_ui_canvas_api.get_transform,root,0,&transform) || transform!=o->transform ||
       c->object_class(transform)!=primary_ui_canvas_api.rect_transform_class)return 0;
    SourcePoseApi read={0};read.calls=*c;
    return GF2VR_UI_LAYOUT_RESULT(o,out,
        source_pose_value(&read,primary_ui_canvas_api.get_matrix,transform,primary_ui_canvas_api.matrix_class,out,64) &&
        source_pose_value(&read,primary_ui_canvas_api.get_rect,transform,c->rect_class,out+16,16));
}
static ActivityCanvasScopeApi primary_ui_canvas_owner_ops(PrimaryUiCanvasOwner *o) {
    return (ActivityCanvasScopeApi){o,primary_ui_canvas_mode,primary_ui_canvas_set,primary_ui_canvas_geometry};
}
static ActivityCanvasScopeApi primary_ui_canvas_ops(void){return primary_ui_canvas_owner_ops(&primary_ui_canvas);}
static void primary_ui_canvas_release_owner(PrimaryUiCanvasOwner *o) {
    if(o->scope.active)return; /* unknown restoration retains independent roots */
    const ManagedEyeApi *c=&primary_ui_api.calls;
    if(o->transform_root)c->unpin(o->transform_root);
    if(o->canvas_root)c->unpin(o->canvas_root);
    if(o->camera_root)c->unpin(o->camera_root);
    o->canvas_root=o->transform_root=o->camera_root=0;
    o->canvas=o->transform=o->camera=0;
#ifdef GF2VR_ACTIVITY_UI_SCALE_RESTORE
    o->scale=(CanvasScaleSnapshot){0};
#endif
}
static void primary_ui_canvas_release(void){primary_ui_canvas_release_owner(&primary_ui_canvas);}
/* A failed old presentation transaction need not be unresolved ownership.
   Certify the original mode on the SAME retained root/camera, without using
   active/plane/geometry as restoration proof or accepting the old output. */
static int primary_ui_canvas_restoration_proved_owner(PrimaryUiCanvasOwner *o) {
    const ManagedEyeApi *c=&primary_ui_api.calls;
    if(o->scope.active || o->scope.owner!=GetCurrentThreadId() ||
       !primary_ui_api.ready || !o->canvas_root || !o->camera_root ||
       !o->transform_root || !o->canvas || !o->camera ||
       !c->object_class || c->object_class(o->canvas)!=primary_ui_api.canvas_class)return 0;
    void *canvas=o->canvas,*camera=o->camera,*root=0,*bound=0;
    unsigned cr=o->canvas_root,tr=o->transform_root,rr=o->camera_root;
    unsigned mode=99;
    if(!managed_eye_call(c,primary_ui_api.methods[AP_ROOT],canvas,0,&root) || root!=canvas ||
       !managed_eye_call(c,primary_ui_api.methods[AP_CAMERA],canvas,0,&bound) || bound!=camera ||
       !primary_ui_canvas_mode(o,&mode) || mode!=1)return 0;
    root=bound=0;mode=99;
    if(!managed_eye_call(c,primary_ui_api.methods[AP_ROOT],canvas,0,&root) || root!=canvas ||
       !managed_eye_call(c,primary_ui_api.methods[AP_CAMERA],canvas,0,&bound) || bound!=camera ||
       !primary_ui_canvas_mode(o,&mode) || mode!=1)return 0;
    return !o->scope.active && o->scope.owner==GetCurrentThreadId() &&
        o->canvas==canvas && o->camera==camera &&
        o->canvas_root==cr && o->transform_root==tr && o->camera_root==rr;
}
static int primary_ui_canvas_restoration_proved(void){return primary_ui_canvas_restoration_proved_owner(&primary_ui_canvas);}
static int primary_ui_canvas_restore_owner(PrimaryUiCanvasOwner *o,unsigned thread) {
    if(!o->scope.active)return 1;
    ActivityCanvasScopeApi ops=primary_ui_canvas_owner_ops(o);
    int ok=acs_restore(&o->scope,&ops,thread);
    if(thread==o->scope.owner) {
        if(!ok && !primary_ui_canvas_restoration_proved_owner(o))primary_ui_scope_disabled=1;
        primary_ui_canvas_release_owner(o);
    }
    return ok;
}
static int primary_ui_canvas_restore(unsigned thread){return primary_ui_canvas_restore_owner(&primary_ui_canvas,thread);}
static int primary_ui_scoped_root_owner(PrimaryUiCanvasOwner *o,void *canvas,void *camera) {
    if(!o->scope.active || o->scope.failed ||
       o->scope.owner!=GetCurrentThreadId() || canvas!=o->canvas || camera!=o->camera)return 0;
    unsigned mode=99;float check[20];
    return primary_ui_canvas_mode(o,&mode) && mode==2 && primary_ui_canvas_geometry(o,check) &&
        acs_geometry_equal(o->scope.geometry,check);
}
static int primary_ui_scoped_root(void *canvas,void *camera){
    if(primary_ui_scoped_root_owner(&primary_ui_canvas,canvas,camera))return 1;
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
    return primary_ui_scoped_root_owner(&dialogue_ui_canvas,canvas,camera);
#else
    return 0;
#endif
}
static int primary_ui_canvas_begin_owner(PrimaryUiCanvasOwner *o,ActivityPrimarySource *u,U64 serial,int camera) {
    const ManagedEyeApi *c=&primary_ui_api.calls;
    if(primary_ui_scope_disabled || o->scope.active || !primary_ui_canvas_bind())return 0;
    o->canvas=u->canvas;o->camera=u->entry.identity.source;o->plane=u->plane;
    o->canvas_root=c->pin(u->canvas,1);
    if(!o->canvas_root)goto fail;
    o->camera_root=c->pin(u->entry.identity.source,1);
    if(!o->camera_root)goto fail;
    if(!managed_eye_call(c,primary_ui_canvas_api.get_transform,u->canvas,0,&o->transform) ||
       !o->transform || c->object_class(o->transform)!=primary_ui_canvas_api.rect_transform_class)goto fail;
    o->transform_root=c->pin(o->transform,1);
    if(!o->transform_root)goto fail;
#ifdef GF2VR_ACTIVITY_UI_SCALE_RESTORE
    float scale=0;unsigned mode=99;
    if(!primary_ui_canvas_mode(o,&mode) || mode!=1 || !primary_ui_scale_read(o,&scale) ||
       !canvas_scale_capture(&o->scale,scale,u->entry.width,u->entry.height))goto fail;
    static unsigned reports;
    if(reports++<32) {
        union {float f;unsigned bits;} value={scale};
        unsigned lane=0;
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
        lane=o==&dialogue_ui_canvas;
#endif
        event("activity_ui_scale_capture",((U64)lane<<32)|value.bits,GetCurrentThreadId());
    }
#endif
    ActivityCanvasScopeApi ops=primary_ui_canvas_owner_ops(o);
    if(acs_begin(&o->scope,&ops,GetCurrentThreadId(),serial,camera))return 1;
fail:
    primary_ui_scope_disabled=1;primary_ui_canvas_release_owner(o);return 0;
}
static int primary_ui_canvas_begin(ActivityPrimarySource *u,U64 serial,int camera){return primary_ui_canvas_begin_owner(&primary_ui_canvas,u,serial,camera);}
#ifdef GF2VR_ACTIVITY_UI_GENERATION_LEASE
static void primary_ui_generation_report(unsigned operation,int ok) {
    static unsigned reports;
    if(reports>=64 || (ok && reports>=32))return;++reports;
    char line[700],*p=source_prefix(line,"activity_primary_ui_generation_lease");
    p=literal(p,",\"generation\":");p=hexadecimal(p,primary_ui_generation_id);
    p=literal(p,",\"operation\":");p=decimal(p,operation);
    p=literal(p,",\"ok\":");p=decimal(p,ok!=0);
    p=literal(p,",\"active\":");p=decimal(p,primary_ui_canvas.scope.active);
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
    p=literal(p,",\"dialogue_active\":");p=decimal(p,dialogue_ui_canvas.scope.active);
#endif
    p=literal(p,",\"optional_disabled\":");p=decimal(p,primary_ui_scope_disabled);
    p=literal(p,"}\n");source_log(line,p);
}
static int primary_ui_generation_prepare(void *context,U64 generation) {
    ActivitySourceGroup *a=context;
    if(!a || !a->source || a->source->owner_thread!=GetCurrentThreadId() || !generation || primary_ui_scope_disabled ||
       !activity_source_current(a,generation) || !primary_ui_current(a))return 0;
    if(primary_ui_canvas.scope.active)return primary_ui_generation_context==a &&
        primary_ui_generation_id==generation && primary_ui_current(a);
    primary_ui_generation_context=a;primary_ui_generation_id=generation;
    /* camera=1 is an internal lease tag, never a Unity/producer camera grant.
       Exact Canvas/camera identities are independently pinned and checked. */
    int ok=primary_ui_canvas_begin(&a->active.primary,generation,1);
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
    if(ok && a->active.dialogue.canvas){
        ok=primary_ui_canvas_begin_owner(&dialogue_ui_canvas,&a->active.dialogue,generation,2);
        if(!ok)(void)primary_ui_canvas_restore(GetCurrentThreadId());
    }
#endif
    primary_ui_generation_report(0,ok);
    if(!primary_ui_canvas.scope.active){primary_ui_generation_context=0;primary_ui_generation_id=0;}
    return ok;
}
static int primary_ui_generation_release(void *context) {
    if(primary_ui_generation_context && context!=primary_ui_generation_context)return 1;
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
    if(dialogue_ui_canvas.scope.active){
        if(dialogue_ui_canvas.scope.owner!=GetCurrentThreadId())return 0;
        (void)primary_ui_canvas_restore_owner(&dialogue_ui_canvas,GetCurrentThreadId());
    }
#endif
    if(!primary_ui_canvas.scope.active){primary_ui_generation_context=0;primary_ui_generation_id=0;return 1;}
    if(primary_ui_canvas.scope.owner!=GetCurrentThreadId())return 0;
    int ok=primary_ui_canvas_restore(GetCurrentThreadId());primary_ui_generation_report(1,ok);
    /* An uncertain restoration keeps the independent Canvas/Transform/camera
       pins and disables this optional route. Do not retain a stale ASG pointer
       or prevent the already drained world group from being collected. */
    primary_ui_generation_context=0;primary_ui_generation_id=0;return 1;
}
#ifdef GF2VR_ACTIVITY_UI_LIFETIME
static int primary_ui_transition_release(void *context) {
    if(!primary_ui_generation_release(context))return 0;
    /* Generation collection can quarantine an uncertain original lease and
       discard the world. UI-only replacement must not overwrite that owner. */
    return !primary_ui_canvas.scope.active && !dialogue_ui_canvas.scope.active &&
        !primary_ui_canvas.canvas_root && !primary_ui_canvas.transform_root && !primary_ui_canvas.camera_root &&
        !dialogue_ui_canvas.canvas_root && !dialogue_ui_canvas.transform_root && !dialogue_ui_canvas.camera_root;
}
#endif
#endif
#endif
static int primary_ui_capture_source(ActivitySourceGroup *a,ActivityPrimarySource *output,ActivityPrimarySource *old,const void *entry,const void *role) {
    const SourceTopologyEntry *e=entry;const PhaseRoleSnapshot *r=role;
    const ManagedEyeApi *c=&a->binding->refs.settings.calls;
    if(!primary_ui_enabled_load() || !primary_ui_bind(c))return 0;
    primary_ui_api.stage=3;
    ActivityPrimarySource value={0};value.entry=*e;value.role=*r;
    unsigned ar=0,tr=0;int ok=0;void *array=0,*type=0;
    value.camera_root=c->pin(e->identity.source,1);if(!value.camera_root)goto end;
    if(old->canvas_root && old->entry.identity.source==e->identity.source) {
        value.canvas_root=c->pin(old->canvas,1);value.canvas=old->canvas;
        if(value.canvas_root) {
#ifdef GF2VR_ACTIVITY_UI_GENERATION_LEASE
            if(primary_ui_scoped_root(value.canvas,e->identity.source)) {value.plane=old->plane;ok=1;goto verify;}
#endif
            if(primary_ui_root_read(value.canvas,e->identity.source,&value.plane,e->width,e->height)){ok=1;goto verify;}
        }
        if(value.canvas_root)c->unpin(value.canvas_root);value.canvas_root=0;value.canvas=0;
    }
    HINSTANCE game=GetModuleHandleW(L"GameAssembly.dll");
    void *(*type_object)(void *)=(void *)GetProcAddress(game,"il2cpp_type_get_object");
    if(!type_object || !(type=type_object(primary_ui_api.type)) || !(tr=c->pin(type,1)))goto end;
    primary_ui_api.stage=4;
    void *args[]={type};if(!managed_eye_call(c,primary_ui_api.methods[AP_FIND],0,args,&array) || !array || !(ar=c->pin(array,1)))goto end;
    void *klass=c->object_class(array);if(klass!=primary_ui_api.array_class && klass!=primary_ui_api.object_array_class)goto end;
    primary_ui_api.stage=5;unsigned count=sa.array_length(array);if(count>128)goto end;
    for(unsigned n=0;n<count;++n) {
        int index=(int)n;void *ia[]={&index},*canvas=0,*root=0,*bound=0;
        if(!managed_eye_call(c,primary_ui_api.methods[AP_ITEM],array,ia,&canvas) || !canvas || c->object_class(canvas)!=primary_ui_api.canvas_class)goto end;
        unsigned cr=c->pin(canvas,1);if(!cr)goto end;unsigned rr=0;int row=1;
        if(!managed_eye_call(c,primary_ui_api.methods[AP_ROOT],canvas,0,&root) || !root)row=0;
        if(row && !(rr=c->pin(root,1)))row=0;
        if(row && (c->object_class(root)!=primary_ui_api.canvas_class || !managed_eye_call(c,primary_ui_api.methods[AP_CAMERA],root,0,&bound)))row=0;
        if(row && bound==e->identity.source) {
            float plane=0;
            if(!primary_ui_root_read(root,e->identity.source,&plane,e->width,e->height))row=0;
            else if(value.canvas && value.canvas!=root)row=0; /* No arbitrary subset of roots. */
            else if(!value.canvas){value.canvas=root;value.plane=plane;value.canvas_root=rr;rr=0;}
        }
        if(rr)c->unpin(rr);c->unpin(cr);if(!row){primary_ui_api.stage=6;goto end;}
    }
    if(sa.array_length(array)!=count || !value.canvas_root)goto end;ok=1;
verify: {
    primary_ui_api.stage=7;
    float plane=0;ActivityViewport rect;
    int root_ok=primary_ui_root_read(value.canvas,e->identity.source,&plane,e->width,e->height) && plane==value.plane;
#ifdef GF2VR_ACTIVITY_UI_GENERATION_LEASE
    root_ok=root_ok || primary_ui_scoped_root(value.canvas,e->identity.source);
#endif
    if(!root_ok ||
       !activity_viewport_read(c,e->identity.source,&rect) || rect.v[0]!=0 || rect.v[1]!=0 || rect.v[2]!=e->width || rect.v[3]!=e->height)ok=0;
    if(ok){*output=value;value=(ActivityPrimarySource){0};}
}
end:
    if(ar)c->unpin(ar);if(tr)c->unpin(tr);
    if(value.canvas_root)c->unpin(value.canvas_root);if(value.camera_root)c->unpin(value.camera_root);
    static unsigned reports,last_ok[2]={2,2};static void *last_source[2],*last_canvas[2];
    unsigned lane=e->orthographic!=0;
    void *reported_canvas=ok?output->canvas:0;
    if(reports<32 && (last_ok[lane]!=(unsigned)ok || last_source[lane]!=e->identity.source || last_canvas[lane]!=reported_canvas)) {
        DWORD error=GetLastError();++reports;last_ok[lane]=ok;last_source[lane]=e->identity.source;last_canvas[lane]=reported_canvas;
        char line[900],*p=source_prefix(line,e->orthographic?"activity_dialogue_ui_source":"activity_primary_ui_source");
        p=literal(p,",\"ok\":");p=decimal(p,ok);p=literal(p,",\"stage\":");p=decimal(p,ok?0:primary_ui_api.stage);
        p=literal(p,",\"source\":");p=hexadecimal(p,(U64)e->identity.source);p=literal(p,",\"canvas\":");p=hexadecimal(p,(U64)reported_canvas);
        p=literal(p,"}\n");source_log(line,p);SetLastError(error);
    }
    return ok;
}
static int primary_ui_capture(void *context,void *snapshot,const void *entry,const void *role) {
    ActivitySourceGroup *a=context;ActivitySourceSnapshot *s=snapshot;
    return primary_ui_capture_source(a,&s->primary,&a->active.primary,entry,role);
}
#ifdef GF2VR_ACTIVITY_DIALOGUE_UI
static int dialogue_ui_capture(void *context,void *snapshot,const void *entry,const void *role) {
    ActivitySourceGroup *a=context;ActivitySourceSnapshot *s=snapshot;
    return primary_ui_capture_source(a,&s->dialogue,&a->active.dialogue,entry,role);
}
static int dialogue_ui_geometry(void *context,U64 generation,float out[20]) {
    ActivitySourceGroup *a=context;
    if(primary_ui_generation_context!=a || primary_ui_generation_id!=generation ||
       !primary_ui_current(a) || !dialogue_ui_canvas.scope.active || dialogue_ui_canvas.scope.failed)return 0;
    if(!primary_ui_canvas_geometry(&dialogue_ui_canvas,out) || !acs_geometry_equal(dialogue_ui_canvas.scope.geometry,out))return 0;
    return primary_ui_current(a);
}
static int primary_ui_geometry(void *context,U64 generation,float out[20]) {
    ActivitySourceGroup *a=context;
    if(primary_ui_generation_context!=a || primary_ui_generation_id!=generation ||
       !primary_ui_current(a) || !primary_ui_canvas.scope.active || primary_ui_canvas.scope.failed)return 0;
    if(!primary_ui_canvas_geometry(&primary_ui_canvas,out) || !acs_geometry_equal(primary_ui_canvas.scope.geometry,out))return 0;
    return primary_ui_current(a);
}
#endif

#endif

#endif
