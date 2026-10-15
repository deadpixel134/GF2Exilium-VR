#ifndef GF2_CAMERA_DATA_SETTINGS_H
#define GF2_CAMERA_DATA_SETTINGS_H
#include "managed_eye_camera.h"
/* Unity owner thread, caller-owned AdditionalCameraData components only.
   This copies the SCALAR subset, not the component/stack/render eligibility.
   Metadata: CAMERA-DATA-001 PID21368. Never invoke a native method-body pointer.
   Lists, volumeTrigger and derived cameraOutput require separate preparation.
   A successful return MUST NOT mark ManagedEyes ready or authorize publication. */
typedef struct {
    const char *get,*set,*type,*image,*ns,*name;
    unsigned size;
} CameraDataSettingSpec;
#define CDS_BOOL(n) {"get_" n,"set_" n,"System.Boolean","mscorlib.dll","System","Boolean",1}
#define CDS_ENUM(n,t) {"get_" n,"set_" n,"UnityEngine.Rendering.Universal." t,"Unity.RenderPipelines.Universal.Runtime.dll","UnityEngine.Rendering.Universal",t,4}
static const CameraDataSettingSpec camera_data_setting_specs[]={
    CDS_BOOL("CloseAdditionalLightShadowForCharacter"),
    CDS_BOOL("renderShadows"),
    CDS_ENUM("requiresDepthOption","CameraOverrideOption"),
    CDS_ENUM("requiresColorOption","CameraOverrideOption"),
    CDS_ENUM("renderType","CameraRenderType"),
    CDS_ENUM("resolutionQuality","ResolutionQuality"),
    CDS_BOOL("hasHSR"), CDS_BOOL("needAlpha"), CDS_BOOL("enableSGSR"),
    CDS_ENUM("depthPipelineSetting","DepthPassPipelineSetting"),
    CDS_BOOL("clearDepth"),
    {"GetRenderer","SetRenderer","System.Int32","mscorlib.dll","System","Int32",4},
    {"get_volumeLayerMask","set_volumeLayerMask","UnityEngine.LayerMask","UnityEngine.CoreModule.dll","UnityEngine","LayerMask",4},
    CDS_BOOL("renderPostProcessing"), CDS_BOOL("opaqueDepthOnlyEnable"),
    CDS_ENUM("antialiasing","AntialiasingMode"),
    CDS_ENUM("smaaQuality","SMAAQuality"),
    CDS_ENUM("smaaEdgeDection","SMAAEdgeDection"),
    CDS_ENUM("fxaaQuality","FXAAQuality"),
    CDS_BOOL("stopNaN"), CDS_BOOL("dithering")
};
#undef CDS_BOOL
#undef CDS_ENUM
#define CAMERA_DATA_SETTING_COUNT (sizeof(camera_data_setting_specs)/sizeof(camera_data_setting_specs[0]))
typedef struct {
    ManagedEyeApi calls;
    void *component_class;
    void *get[CAMERA_DATA_SETTING_COUNT],*set[CAMERA_DATA_SETTING_COUNT];
    void *value_class[CAMERA_DATA_SETTING_COUNT];
    unsigned bound;
} CameraDataSettingsApi;
typedef union {unsigned bits;unsigned char bytes[4];} CameraDataScalar;
/* All values are transient in one copy call; no snapshot/root escapes. The
   caller must keep generation validity through the call and recheck afterward.
   On any error destinations are partially configured and remain ineligible;
   caller discards/retries its private preparation, never renders a partial pair.
   Caller must pass distinct privately owned, unused destination components. */
static int camera_data_settings_copy(const CameraDataSettingsApi *a,void *source,
                                     void *left,void *right) {
    GF2VR_PREPARE_STEP(700);
    if(!a || !a->bound || !source || !left || !right || source==left || source==right || left==right ||
       !a->component_class || !a->calls.object_class || !a->calls.pin || !a->calls.unpin ||
       !a->calls.invoke || !a->calls.unbox) return 0;
    void *objects[]={source,left,right};unsigned roots[3]={0};int ok=0;
    CameraDataScalar values[CAMERA_DATA_SETTING_COUNT]={0};
    GF2VR_PREPARE_STEP(701);
    for(unsigned i=0;i<CAMERA_DATA_SETTING_COUNT;++i)
        if(!a->get[i] || !a->set[i] || !a->value_class[i]) return 0;
    for(unsigned i=0;i<3;++i) {
        GF2VR_PREPARE_STEP(702);
        if(a->calls.object_class(objects[i])!=a->component_class) goto done;
        GF2VR_PREPARE_STEP(703);
        roots[i]=a->calls.pin(objects[i],1);if(!roots[i]) goto done;
    }
    /* Finish every source read before the first destination write. Preserve
       unknown enum/renderer values verbatim; no guessed enum numeric constants.
       Copy options, not computed requiresDepthTexture/requiresColorTexture
       getters whose setters can replace the authored tri-state options. */
    for(unsigned i=0;i<CAMERA_DATA_SETTING_COUNT;++i) {
        void *boxed=0;
        GF2VR_PREPARE_STEP(1000+3*i);
        if(!managed_eye_call(&a->calls,a->get[i],source,0,&boxed) || !boxed) goto done;
        GF2VR_PREPARE_STEP(1001+3*i);
        unsigned root=a->calls.pin(boxed,1);if(!root) goto done;
        GF2VR_PREPARE_STEP(1002+3*i);
        void *value=a->calls.object_class(boxed)==a->value_class[i]?a->calls.unbox(boxed):0;
        if(value) for(unsigned j=0;j<camera_data_setting_specs[i].size;++j)
            values[i].bytes[j]=((unsigned char *)value)[j];
        a->calls.unpin(root);if(!value) goto done;
    }
    for(unsigned eye=1;eye<3;++eye)
        for(unsigned i=0;i<CAMERA_DATA_SETTING_COUNT;++i) {
            void *args[]={&values[i]};
            GF2VR_PREPARE_STEP(1100+32*(eye-1)+i);
            if(!managed_eye_call(&a->calls,a->set[i],objects[eye],args,0)) goto done;
        }
    ok=1;
done:
    for(unsigned i=3;i>0;--i) if(roots[i-1]) a->calls.unpin(roots[i-1]);
    return ok;
}
#endif
