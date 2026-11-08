#if GF2VR_TOUCH_PRIVATE && GF2VR_DIRTY_CONTACT
using System;
using System.Diagnostics;
using UnityEngine;
using UnityEngine.Rendering;

namespace GF2.Private
{
    public static partial class ControllerDisplay
    {
        private static Material contactIdleMaterial, contactActiveMaterial;
        private static bool contactIndicatorAttempted;
        private static bool contactIndicatorFault;
        private static long contactIndicatorDraws;

        private static Material ContactIndicatorMaterial(Shader shader, Color color)
        {
            Material material=new Material(shader);
            material.hideFlags=HideFlags.HideAndDontSave;
            ColorProperty(material,"_Color",color);
            IntProperty(material,"_SrcBlend",(int)BlendMode.SrcAlpha);
            IntProperty(material,"_DstBlend",(int)BlendMode.OneMinusSrcAlpha);
            IntProperty(material,"_ZWrite",0);
            IntProperty(material,"_ZTest",(int)CompareFunction.Always);
            IntProperty(material,"_Cull",(int)CullMode.Off);
            material.renderQueue=3100;
            return material;
        }

        private static void PrepareContactIndicators()
        {
            if (!contactSettings.ShowContactLocations || contactIndicatorAttempted) return;
            contactIndicatorAttempted=true;
            try {
                Shader shader=Supported("Hidden/Internal-Colored");
                if (shader==null) return;
                contactIdleMaterial=ContactIndicatorMaterial(shader,new Color(0,.85f,1,.18f));
                contactActiveMaterial=ContactIndicatorMaterial(shader,new Color(1,.35f,.02f,.35f));
            }
            catch (Exception) { ClearContactIndicators(); contactIndicatorAttempted=true; contactIndicatorFault=true; }
        }

        private static void ClearContactIndicators()
        {
            if (contactIdleMaterial!=null) UnityEngine.Object.Destroy(contactIdleMaterial);
            if (contactActiveMaterial!=null) UnityEngine.Object.Destroy(contactActiveMaterial);
            contactIdleMaterial=contactActiveMaterial=null;
            contactIndicatorAttempted=false;
            contactIndicatorFault=false;
        }

        private static void QueueContactIndicator(Vector3 center,float radius,Material material,Camera left,Camera right)
        {
            Matrix4x4 matrix=Matrix4x4.TRS(center,Quaternion.identity,Vector3.one*(radius/primitiveRadius));
            Graphics.DrawMesh(ownedMesh,matrix,material,drawLayer,left,0,null,ShadowCastingMode.Off,false,null,LightProbeUsage.Off,null);
            Graphics.DrawMesh(ownedMesh,matrix,material,drawLayer,right,0,null,ShadowCastingMode.Off,false,null,LightProbeUsage.Off,null);
        }

        private static void DrawContactIndicators(Camera left,Camera right)
        {
            if (!contactRunning || contactSettings==null || !contactSettings.ShowContactLocations ||
                contactIndicatorFault || contactIdleMaterial==null || contactActiveMaterial==null || !ContactOwnerValid()) return;
            long now=Stopwatch.GetTimestamp();
            if (contactHandStamp==0 || now<contactHandStamp || now-contactHandStamp>Stopwatch.Frequency/10) return;
            try { for (int i=0;i<contactCount;++i) {
                ContactTarget t=contactTargets[i];
                if (ContactSettings.DetectionOnly(t.Region) && t.SurfaceState!=5) continue;
                // The previous completed contact sample can be drawn before
                // this frame's callback, but never across a gap or binding loss.
                if (t.ContactFrame<Time.frameCount-1 || !ContactValidTarget(t) || !Finite(t.ContactPosition) || !Finite(t.ContactRadius) || t.ContactRadius<=0) continue;
                Material material=t.InsideMask!=0 && (t.InsideMask&contactMask)!=0 ? contactActiveMaterial : contactIdleMaterial;
                if (t.SurfaceState==5) {
                    ContactPatch patch=t.Surface.Patch;
                    if (patch.Frame<Time.frameCount-1 || patch.DisplayMesh==null || patch.DisplayFault) continue;
                    Graphics.DrawMesh(patch.DisplayMesh,Matrix4x4.identity,material,drawLayer,left,0,null,ShadowCastingMode.Off,false,null,LightProbeUsage.Off,null);
                    Graphics.DrawMesh(patch.DisplayMesh,Matrix4x4.identity,material,drawLayer,right,0,null,ShadowCastingMode.Off,false,null,LightProbeUsage.Off,null);
                } else QueueContactIndicator(t.ContactPosition,t.ContactRadius,material,left,right);
                QueueContactIndicator(t.ContactPoint,Math.Min(t.ContactRadius*.12f,.005f*contactUnitScale),material,left,right);
                ++contactIndicatorDraws;
            } }
            catch (Exception) { contactIndicatorFault=true; }
        }
    }
}
#endif
