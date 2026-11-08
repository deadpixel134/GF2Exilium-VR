#if GF2VR_TOUCH_PRIVATE && GF2VR_DIRTY_CONTACT
using System;
using System.Collections.Generic;
using System.Diagnostics;
using UnityEngine;

namespace GF2.Private
{
    public static partial class ControllerDisplay
    {
        private sealed class ContactSurfaceSource
        {
            internal SkinnedMeshRenderer Skin;
            internal Mesh Mesh;
            internal float[] Shapes;
            internal Transform Actor;
            internal Transform[] Bones;
        }
        private sealed class ContactSurface
        {
            internal Vector3 Center;
            internal float Radius, Error;
            internal int Samples, State; // 1 ready, 2 unavailable, 3 explicit legacy, 4 rejected fit
            internal ContactSurfaceSource[] Sources;
            internal string Failure, PatchFailure;
            internal float PatchBindError;
            internal int PatchRecoveredVertices;
            internal ContactPatch Patch;
        }
        private static SkinnedMeshRenderer[] contactSurfaceSkins;
        private static long contactSurfaceBuilds, contactSurfaceFailures, contactSurfaceTicks;

        private static bool ContactSurfaceCurrent(ContactSurface surface)
        {
            if (surface == null || surface.Sources == null) return false;
            for (int i = 0; i < surface.Sources.Length; ++i) {
                ContactSurfaceSource s = surface.Sources[i];
                if (s.Skin == null || s.Mesh == null || !s.Skin.enabled || !s.Skin.gameObject.activeInHierarchy || s.Skin.sharedMesh != s.Mesh ||
                    s.Actor == null || !s.Skin.transform.IsChildOf(s.Actor)) return false;
                for (int j = 0; j < s.Shapes.Length; ++j) if (s.Skin.GetBlendShapeWeight(j) != s.Shapes[j]) return false;
            }
            return true;
        }

        private static bool ContactSurfaceSetMatches(ContactTarget target)
        {
            if (target.Surface == null || target.Surface.Sources == null || contactSurfaceSkins == null) return false;
            if (target.Surface.State == 3) return true;
            int count = 0;
            for (int i = 0; i < contactSurfaceSkins.Length; ++i) {
                SkinnedMeshRenderer skin = contactSurfaceSkins[i];
                if (skin == null || !skin.enabled || !skin.gameObject.activeInHierarchy || skin.sharedMesh == null || skin.GetComponentInParent<Animator>() != target.Animator) continue;
                bool found = false;
                for (int j = 0; j < target.Surface.Sources.Length; ++j) if (target.Surface.Sources[j].Skin == skin && target.Surface.Sources[j].Mesh == skin.sharedMesh) {
                    Transform[] bones=skin.bones;
                    Transform[] before=target.Surface.Sources[j].Bones;
                    if (bones==null || before==null || bones.Length!=before.Length) return false;
                    for (int b=0;b<bones.Length;++b) if (bones[b]!=before[b]) return false;
                    found = true; break;
                }
                if (!found) return false;
                ++count;
            }
            return count == target.Surface.Sources.Length;
        }

        private static ContactSurface ContactBuildSurface(ContactTarget target)
        {
            ContactSurface result = new ContactSurface { State = 2 };
            if (contactSettings.BodyRadius != .06f) { result.State = 3; result.Sources = new ContactSurfaceSource[0]; return result; }
            long start = Stopwatch.GetTimestamp();
            ++contactSurfaceBuilds;
            var sources = new List<ContactSurfaceSource>();
            var points = new List<Vector3>();
            bool incomplete = false;
            try {
                if (contactSurfaceSkins == null) return result;
                for (int i = 0; i < contactSurfaceSkins.Length; ++i) {
                    SkinnedMeshRenderer skin = contactSurfaceSkins[i];
                    if (skin == null || !skin.enabled || !skin.gameObject.activeInHierarchy || skin.sharedMesh == null || skin.GetComponentInParent<Animator>() != target.Animator) continue;
                    Mesh mesh = skin.sharedMesh;
                    if (sources.Count == 128) { incomplete = true; result.Failure="source-limit"; break; }
                    Transform[] bones = skin.bones;
                    var source = new ContactSurfaceSource { Skin = skin, Mesh = mesh, Actor=target.Actor, Bones=bones, Shapes = new float[0] };
                    sources.Add(source);
                    if (bones == null || bones.Length > 2048) { incomplete = true; result.Failure="bone-limit"; continue; }
                    bool[] related = new bool[bones.Length]; bool used = false;
                    for (int b = 0; b < bones.Length; ++b) if (bones[b] != null && (bones[b] == target.Bone || bones[b].IsChildOf(target.Bone))) { related[b] = true; used = true; }
                    if (!used) continue;
                    // Metadata and data read failures are isolated to contact.
                    // An unmeasured clothing renderer cannot prove a body fit.
                    Mesh baked = null;
                    try {
                        if (mesh.vertexCount > 262144) { incomplete = true; result.Failure="vertex-limit"; continue; }
                        BoneWeight[] weights = mesh.boneWeights;
                        if (weights == null || weights.Length != mesh.vertexCount) { incomplete = true; result.Failure="weights-unavailable"; continue; }
                        baked = new Mesh(); baked.hideFlags = HideFlags.HideAndDontSave;
                        skin.BakeMesh(baked);
                        Vector3[] vertices = baked.vertices;
                        if (vertices.Length != weights.Length) { incomplete = true; result.Failure="bake-count"; continue; }
                        Matrix4x4 toBone = target.Bone.worldToLocalMatrix * skin.transform.localToWorldMatrix;
                        int before=points.Count;
                        for (int v = 0; v < vertices.Length; ++v) {
                            BoneWeight w = weights[v];
                            float influence = ContactSurfaceWeight(related,w.boneIndex0,w.weight0) + ContactSurfaceWeight(related,w.boneIndex1,w.weight1) +
                                ContactSurfaceWeight(related,w.boneIndex2,w.weight2) + ContactSurfaceWeight(related,w.boneIndex3,w.weight3);
                            if (influence < .5f) continue;
                            if (points.Count == 32768) { incomplete = true; result.Failure="sample-limit"; break; }
                            Vector3 p = toBone.MultiplyPoint3x4(vertices[v]);
                            if (!Finite(p)) { incomplete = true; result.Failure="nonfinite-sample"; break; }
                            points.Add(p);
                        }
                        if (points.Count>before) {
                            if (mesh.blendShapeCount>256) { incomplete=true; result.Failure="shape-limit"; continue; }
                            source.Shapes=new float[mesh.blendShapeCount];
                            for (int b=0;b<source.Shapes.Length;++b) source.Shapes[b]=skin.GetBlendShapeWeight(b);
                        }
                    }
                    catch (Exception ex) { incomplete = true; result.Failure=ex.GetType().Name; ++contactSurfaceFailures; }
                    finally { if (baked != null) UnityEngine.Object.DestroyImmediate(baked); }
                }
                result.Sources = sources.ToArray(); result.Samples = points.Count;
                Vector3 center; float radius, error, scale;
                if (!incomplete && ContactSurfaceMath.UniformScale(target.Bone.localToWorldMatrix, out scale) &&
                    ContactSurfaceMath.Fit(points.ToArray(),points.Count,out center,out radius,out error)) {
                    result.Center = center; result.Radius = radius; result.Error = error;
                    // Initial design tolerance is provisional. Both measured
                    // residual and state remain visible in the runtime record.
                    result.State = 1;
                }
                if (!incomplete) result.Patch=ContactBuildPatch(target,result);
            }
            catch (Exception ex) { result.Failure=ex.GetType().Name; ++contactSurfaceFailures; }
            finally { contactSurfaceTicks += Stopwatch.GetTimestamp()-start; }
            if (result.Sources == null) result.Sources = sources.ToArray();
            return result;
        }

        private static float ContactSurfaceWeight(bool[] related, int index, float weight)
        { return index >= 0 && index < related.Length && related[index] ? weight : 0; }

        // Capture every animation basis and world proxy before any target
        // writes. An earlier target cannot contaminate a later target's pose.
        private static void ContactSnapshot(int index)
        {
            ContactTarget t = contactTargets[index];
            if (!ContactSurfaceCurrent(t.Surface)) {
                t.SurfaceInvalid=true;
                t.InsideMask=0; t.HandTicket=-1;
                contactNeedsRebind=true;
                contactHapticValid=0;
            }
            Vector3 current=t.Bone.localPosition, scale=t.Bone.localScale; Quaternion rotation=t.Bone.localRotation;
            if (!t.Owned || !ContactExact(current,t.LastWritten)) t.Base=current;
            if (!t.RotationOwned || !ContactExact(rotation,t.LastRotation)) t.BaseRotation=rotation;
            if (!t.ScaleOwned || !ContactExact(scale,t.LastScale)) t.BaseScale=scale;
            Matrix4x4 matrix=t.Parent.localToWorldMatrix*Matrix4x4.TRS(t.Base,t.BaseRotation,t.BaseScale);
            t.BaseMatrix=matrix; t.ActualInverse=t.Bone.worldToLocalMatrix;
            t.ContactPosition=t.Parent.TransformPoint(t.Base);
            t.ContactRadius=contactSettings.BodyRadius*contactUnitScale;
            t.SurfaceState=t.Surface==null || t.SurfaceInvalid?2:t.Surface.State;
            float uniform;
            if (t.Surface!=null && !t.SurfaceInvalid && t.Surface.State==1 && ContactSurfaceMath.UniformScale(matrix,out uniform)) {
                float radius=t.Surface.Radius*uniform;
                float offset=(matrix.MultiplyPoint3x4(t.Surface.Center)-t.ContactPosition).magnitude;
                if (t.Surface.Error*uniform <= contactSettings.HandRadius*contactHandScale*.15f &&
                    radius>=.005f*contactUnitScale && radius<=.25f*contactUnitScale && offset<=.25f*contactUnitScale) {
                    t.ContactPosition=matrix.MultiplyPoint3x4(t.Surface.Center);
                    t.ContactRadius=t.Surface.Radius*uniform;
                } else t.SurfaceState=4;
            } else if (t.SurfaceState==1) t.SurfaceState=2;
            if (t.Surface!=null && !t.SurfaceInvalid && t.Surface.Patch!=null && t.SurfaceState!=1) t.SurfaceState=5;
            t.ContactPoint=t.ContactPosition;
            t.ContactFrame=Time.frameCount;
            contactTargets[index]=t;
        }
    }
}
#endif
