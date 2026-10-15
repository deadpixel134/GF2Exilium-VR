#ifndef GF2_MANAGED_EYE_CAMERA_H
#define GF2_MANAGED_EYE_CAMERA_H
#ifndef GF2VR_PREPARE_STEP
#define GF2VR_PREPARE_STEP(n) ((void)0)
#endif
#ifndef GF2VR_RETIRE_FAILURE
#define GF2VR_RETIRE_FAILURE(reason,eye) ((void)0)
#endif
/* Unity main-thread only. Method tokens must be resolved with the exact
   signatures below. No Instantiate: a source GameObject may carry game scripts.
   These cameras are preparation objects, not proof of pipeline eligibility.
   Targets and in-flight GPU resources have a separate completion lifetime. */
typedef struct {
    void *game_object_class,*camera_class,*camera_type;
    void *ctor;          /* GameObject::.ctor() instance void */
    void *set_active;    /* GameObject::SetActive(bool) instance void */
    void *add_component; /* GameObject::AddComponent(System.Type) -> Component */
    void *copy_from;     /* Camera::CopyFrom(Camera) instance void */
    void *set_enabled;   /* Behaviour::set_enabled(bool) instance void */
    void *set_target;    /* Camera::set_targetTexture(RenderTexture) instance void */
    void *destroy;       /* Object::Destroy(Object) static void, deferred */
    void *target_class,*target_ctor; /* RenderTexture::.ctor(Int32,Int32,Int32) */
    void *target_create,*target_is_created,*target_release;
    void *(*unbox)(void *);
    void *set_view,*set_projection,*reset_culling;
    void *get_transform,*set_position_rotation;
    void *(*object_new)(void *);
    void *(*object_class)(void *);
    unsigned (*pin)(void *,int);
    void (*unpin)(unsigned);
    void *(*invoke)(void *,void *,void **,void **);
    void *object_alive,*bool_class; /* optional Object::op_Implicit(Object) -> Boolean */
#ifdef GF2VR_ACTIVITY_VIEWPORT
    void *rect_class,*get_pixel_rect,*set_pixel_rect; /* Optional outside stacked eyes. */
#endif
} ManagedEyeApi;
typedef struct {
    void *object,*camera;
    unsigned object_root,camera_root;
    void *target;
    unsigned target_root,target_constructed;
} ManagedEyeSlot;
typedef struct {
    ManagedEyeSlot eyes[2];
    unsigned ready,borrowed,targets_ready;
    unsigned matrices_ready;
    unsigned long long matrix_frame;
} ManagedEyes;
/* Unity 2019.4 Matrix4x4 sequential column-major value layout. These are Unity
   camera matrices supplied by the producer, not D3D GPU-adjusted matrices. */
typedef struct {float elements[16];} ManagedEyeMatrix;
typedef struct {float position[3],rotation[4];} ManagedEyePose;

static int managed_eye_call(const ManagedEyeApi *a,void *method,void *object,void **args,void **result) {
    void *exception=0;
    void *value=a->invoke(method,object,args,&exception);
    if(result) *result=exception?0:value;
    return exception==0;
}
/* A rooted managed wrapper may outlive its Unity native object. Only a typed,
   successful public liveness query proves absence; missing APIs/errors remain
   unknown. Never inspect private native pointers or treat an exception as death. */
static int managed_eye_verified_dead(const ManagedEyeApi *a,void *object) {
    if(!object || !a->object_alive || !a->bool_class || !a->object_class ||
       !a->pin || !a->unpin || !a->unbox) {GF2VR_RETIRE_FAILURE(10,2);return 0;}
    void *args[]={object},*boxed=0;
    if(!managed_eye_call(a,a->object_alive,0,args,&boxed) || !boxed) {GF2VR_RETIRE_FAILURE(10,2);return 0;}
    unsigned root=a->pin(boxed,1);if(!root) {GF2VR_RETIRE_FAILURE(10,2);return 0;}
    int dead=0;unsigned reason=10;
    if(a->object_class(boxed)==a->bool_class) {
        unsigned char *value=a->unbox(boxed);
        dead=value && *value==0;
        if(value && *value<=1) reason=dead?8:9;
    }
    a->unpin(root);(void)reason;GF2VR_RETIRE_FAILURE(reason,2);return dead;
}
/* Refuse retirement while the engine/consumer still owns a borrowed pair.
   Successful Destroy schedules Unity destruction; it is NOT a GPU fence.
   On a destruction exception keep the root and refuse reuse until cleanup works. */
static int managed_eyes_retire(const ManagedEyeApi *a,ManagedEyes *p) {
    p->ready=0;
    p->targets_ready=0;
    p->matrices_ready=0;
    if(p->borrowed) {GF2VR_RETIRE_FAILURE(3,2);return 0;}
    int clean=1;
    for(unsigned i=2;i>0;--i) {
        ManagedEyeSlot *e=&p->eyes[i-1];
        if(e->target_root && e->camera_root) {
            void *args[]={0};
            if(!managed_eye_call(a,a->set_target,e->camera,args,0)) {
                GF2VR_RETIRE_FAILURE(4,i-1);
                if(!managed_eye_verified_dead(a,e->camera)) {clean=0;continue;}
            }
        }
        if(e->object_root) {
            void *args[]={e->object};
            if(!managed_eye_call(a,a->destroy,0,args,0)) {
                GF2VR_RETIRE_FAILURE(5,i-1);
                if(!managed_eye_verified_dead(a,e->object) ||
                   (e->camera_root && !managed_eye_verified_dead(a,e->camera))) {clean=0;continue;}
            }
            if(e->camera_root) a->unpin(e->camera_root);
            a->unpin(e->object_root);
            e->object=0;e->camera=0;e->object_root=0;e->camera_root=0;
        }
        if(e->target_root) {
            unsigned absent=0;
            if(e->target_constructed && !managed_eye_call(a,a->target_release,e->target,0,0)) {
                GF2VR_RETIRE_FAILURE(6,i-1);
#ifdef GF2VR_ACTIVITY_TARGET_RETIRE_FIX
                /* A scene may remove native Unity objects while rooted managed
                   wrappers survive. Release failure is NOT absence. The exact
                   public typed query must prove this particular target dead. */
                absent=managed_eye_verified_dead(a,e->target);
#endif
                if(!absent){clean=0;continue;}
            }
            void *args[]={e->target};
            if(!absent && !managed_eye_call(a,a->destroy,0,args,0)) {
                GF2VR_RETIRE_FAILURE(7,i-1);
#ifdef GF2VR_ACTIVITY_TARGET_RETIRE_FIX
                if(!managed_eye_verified_dead(a,e->target))
#endif
                {clean=0;continue;}
            }
            a->unpin(e->target_root);
            e->target=0;e->target_root=0;e->target_constructed=0;
        }
    }
    return clean;
}
static int managed_eye_matrix_finite(const ManagedEyeMatrix *m) {
    if(!m) return 0;
    for(unsigned i=0;i<16;++i) {
        union {float value;unsigned bits;} v;v.value=m->elements[i];
        if((v.bits&0x7f800000u)==0x7f800000u) return 0;
    }
    return 1;
}
/* Invalid input/partial setter failure cannot revive the previous frame.
   Source-to-XR pose/reference composition belongs to the producer; no neutral
   capture, source transform write or native graphics convention conversion here. */
#ifdef GF2VR_ACTIVITY_STACK
static int managed_eyes_matrices_mode(const ManagedEyeApi *a,ManagedEyes *p,
#else
static int managed_eyes_matrices(const ManagedEyeApi *a,ManagedEyes *p,
#endif
                                unsigned long long frame,
                                const ManagedEyeMatrix views[2],
                                const ManagedEyeMatrix projections[2],
                                const ManagedEyePose poses[2]
#ifdef GF2VR_ACTIVITY_STACK
                                ,unsigned overlay
#endif
                                ) {
    if(p->borrowed) return 0;
    p->matrices_ready=0;
    if(!p->ready ||
#ifdef GF2VR_ACTIVITY_STACK
       (!p->targets_ready && !overlay) ||
#else
       !p->targets_ready ||
#endif
       !frame || frame<=p->matrix_frame ||
       !a->set_view || !a->set_projection || !a->reset_culling ||
       !a->get_transform || !a->set_position_rotation || !views || !projections || !poses) return 0;
    p->matrix_frame=frame;
    for(unsigned i=0;i<2;++i)
        if(!managed_eye_matrix_finite(&views[i]) || !managed_eye_matrix_finite(&projections[i])) return 0;
    for(unsigned i=0;i<2;++i) {
        for(unsigned j=0;j<7;++j) {
            union {float value;unsigned bits;} v;
            v.value=j<3?poses[i].position[j]:poses[i].rotation[j-3];
            if((v.bits&0x7f800000u)==0x7f800000u) return 0;
        }
        float norm=0;
        for(unsigned j=0;j<4;++j) norm+=poses[i].rotation[j]*poses[i].rotation[j];
        if(!(norm>=0.999f && norm<=1.001f)) return 0;
    }
    for(unsigned i=0;i<2;++i) {
        void *transform=0;
        if(!managed_eye_call(a,a->get_transform,p->eyes[i].camera,0,&transform) || !transform) return 0;
        unsigned root=a->pin(transform,1);if(!root) return 0;
        void *pose_args[]={(void *)poses[i].position,(void *)poses[i].rotation};
        int pose_ok=managed_eye_call(a,a->set_position_rotation,transform,pose_args,0);
        a->unpin(root);if(!pose_ok) return 0;
        void *view_args[]={(void *)&views[i]};
        void *projection_args[]={(void *)&projections[i]};
        if(!managed_eye_call(a,a->set_view,p->eyes[i].camera,view_args,0) ||
           !managed_eye_call(a,a->set_projection,p->eyes[i].camera,projection_args,0) ||
           !managed_eye_call(a,a->reset_culling,p->eyes[i].camera,0,0)) return 0;
    }
    p->matrices_ready=1;return 1;
}
#ifdef GF2VR_ACTIVITY_STACK
static int managed_eyes_matrices(const ManagedEyeApi *a,ManagedEyes *p,
    unsigned long long frame,const ManagedEyeMatrix views[2],const ManagedEyeMatrix projections[2],
    const ManagedEyePose poses[2]) {
    return managed_eyes_matrices_mode(a,p,frame,views,projections,poses,0);
}
#endif
static int managed_eyes_create(const ManagedEyeApi *a,ManagedEyes *p,void *source) {
    GF2VR_PREPARE_STEP(100);
    if(p->ready || p->borrowed || p->eyes[0].object_root || p->eyes[1].object_root ||
       p->eyes[0].target_root || p->eyes[1].target_root) return 0;
    GF2VR_PREPARE_STEP(101);
    if(!a->game_object_class || !a->camera_class || !a->camera_type || !a->ctor ||
       !a->set_active || !a->add_component || !a->copy_from || !a->set_enabled ||
       !a->set_target || !a->destroy || !a->object_new || !a->object_class ||
       !a->pin || !a->unpin || !a->invoke || !source ||
       a->object_class(source)!=a->camera_class) return 0;
    GF2VR_PREPARE_STEP(102);
    unsigned source_root=a->pin(source,1);if(!source_root) return 0;
    GF2VR_PREPARE_STEP(103);
    unsigned type_root=a->pin(a->camera_type,1);
    if(!type_root) {a->unpin(source_root);return 0;}
    for(unsigned i=0;i<2;++i) {
        ManagedEyeSlot *e=&p->eyes[i];
    GF2VR_PREPARE_STEP(104);
        void *object=a->object_new(a->game_object_class);
        if(!object) goto failed;
    GF2VR_PREPARE_STEP(105);
        unsigned root=a->pin(object,1);
        /* Before .ctor there is no native GameObject to destroy. */
        if(!root) goto failed;
        e->object=object;e->object_root=root;
    GF2VR_PREPARE_STEP(106);
        if(!managed_eye_call(a,a->ctor,object,0,0)) goto failed;
        unsigned char off=0;void *off_args[]={&off};
    GF2VR_PREPARE_STEP(107);
        if(!managed_eye_call(a,a->set_active,object,off_args,0)) goto failed;
        void *type_args[]={a->camera_type};void *camera=0;
    GF2VR_PREPARE_STEP(108);
        if(!managed_eye_call(a,a->add_component,object,type_args,&camera) || !camera ||
           camera==source || (i && camera==p->eyes[0].camera) ||
           a->object_class(camera)!=a->camera_class) goto failed;
    GF2VR_PREPARE_STEP(109);
        e->camera=camera;e->camera_root=a->pin(camera,1);
        if(!e->camera_root) goto failed;
        void *source_args[]={source};
    GF2VR_PREPARE_STEP(110);
        if(!managed_eye_call(a,a->copy_from,camera,source_args,0)) goto failed;
        /* CopyFrom may copy source settings; explicitly isolate the resulting
           camera before exposing it. Never mutate the source or its target. */
    GF2VR_PREPARE_STEP(111);
        if(!managed_eye_call(a,a->set_enabled,camera,off_args,0)) goto failed;
        void *null_args[]={0};
    GF2VR_PREPARE_STEP(112);
        if(!managed_eye_call(a,a->set_target,camera,null_args,0)) goto failed;
    }
    a->unpin(type_root);a->unpin(source_root);p->ready=1;return 1;
failed:
    a->unpin(type_root);a->unpin(source_root);
    managed_eyes_retire(a,p);return 0;
}
/* Prepare private constructor-default-format targets, with a 24-bit depth request.
   Extents come from the render owner; this function does not choose XR quality.
   Create + IsCreated verifies allocation only, never freshness/GPU completion.
   Any partial failure invalidates and retires the whole unused camera pair. */
static int managed_eyes_targets(const ManagedEyeApi *a,ManagedEyes *p,int width,int height) {
    GF2VR_PREPARE_STEP(113);
    if(!p->ready || p->borrowed || p->targets_ready ||
       p->eyes[0].target_root || p->eyes[1].target_root) return 0;
    GF2VR_PREPARE_STEP(114);
    if(width<=0 || height<=0 || !a->target_class || !a->target_ctor ||
       !a->target_create || !a->target_is_created || !a->target_release || !a->unbox) return 0;
    for(unsigned i=0;i<2;++i) {
        ManagedEyeSlot *e=&p->eyes[i];
    GF2VR_PREPARE_STEP(115);
        void *target=a->object_new(a->target_class);if(!target) goto failed;
    GF2VR_PREPARE_STEP(116);
        unsigned root=a->pin(target,1);if(!root) goto failed;
        e->target=target;e->target_root=root;
        int depth=24;void *args[]={&width,&height,&depth};
    GF2VR_PREPARE_STEP(117);
        if(!managed_eye_call(a,a->target_ctor,target,args,0)) goto failed;
        e->target_constructed=1;
        void *boxed=0;
    GF2VR_PREPARE_STEP(118);
        if(!managed_eye_call(a,a->target_create,target,0,&boxed) || !boxed) goto failed;
    GF2VR_PREPARE_STEP(119);
        unsigned char *created=a->unbox(boxed);if(!created || !*created) goto failed;
    GF2VR_PREPARE_STEP(120);
        if(!managed_eye_call(a,a->target_is_created,target,0,&boxed) || !boxed) goto failed;
    GF2VR_PREPARE_STEP(121);
        created=a->unbox(boxed);if(!created || !*created) goto failed;
    }
    /* Both resources must exist before either camera is attached. */
    for(unsigned i=0;i<2;++i) {
        void *args[]={p->eyes[i].target};
    GF2VR_PREPARE_STEP(122);
        if(!managed_eye_call(a,a->set_target,p->eyes[i].camera,args,0)) goto failed;
    }
    p->targets_ready=1;return 1;
failed:
    managed_eyes_retire(a,p);return 0;
}
/* Only one borrow at a time. Caller keeps it until all use of the cameras has
   drained; a CPU render return alone cannot authorize GPU resource retirement. */
static int managed_eyes_borrow(ManagedEyes *p,unsigned long long frame,void **left,void **right) {
    if(!p->ready || !p->targets_ready || !p->matrices_ready ||
       frame!=p->matrix_frame || p->borrowed || !left || !right) return 0;
    p->borrowed=1;p->matrices_ready=0;
    *left=p->eyes[0].camera;*right=p->eyes[1].camera;return 1;
}
static void managed_eyes_return(ManagedEyes *p) {p->borrowed=0;}
#endif
