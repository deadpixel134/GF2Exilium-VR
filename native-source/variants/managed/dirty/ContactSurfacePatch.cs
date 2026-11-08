#if GF2VR_TOUCH_PRIVATE && GF2VR_DIRTY_CONTACT
using System;
using System.Collections.Generic;
using System.Globalization;
using UnityEngine;

namespace GF2.Private
{
    public static partial class ControllerDisplay
    {
        private sealed class ContactPatchSkin
        {
            internal Transform[] Bones, ContactAncestors;
            internal Matrix4x4[] BindPoses, Matrices, NormalMatrices;
            internal bool[] Used;
            internal Vector3[] RestVertices, RestNormals;
            internal BoneWeight[] Weights;
            internal int Start;
        }
        private sealed class ContactPatch
        {
            internal ContactPatchSkin[] Skins;
            internal Vector3[] Vertices, Normals;
            internal int[] Triangles;
            internal Vector3 Core, Minimum, Maximum;
            internal float BindError;
            internal Mesh DisplayMesh;
            internal bool DisplayFault;
            internal int Frame=-1;
        }

        private static void ContactDisposeSurface(ContactSurface surface)
        {
            if (surface==null || surface.Patch==null) return;
            // Engine-owned deferred destruction retains any queued DrawMesh
            // references. No private GPU completion is synthesized here.
            if (surface.Patch.DisplayMesh!=null) UnityEngine.Object.Destroy(surface.Patch.DisplayMesh);
            surface.Patch.DisplayMesh=null;
        }

        private static void ContactPatchUseBone(ContactPatchSkin data, int index, float weight)
        {
            if (!Finite(weight) || weight<0) throw new FormatException("patch-weight");
            if (weight==0) return;
            if (index<0 || index>=data.Bones.Length || index>=data.BindPoses.Length || data.Bones[index]==null) throw new FormatException("patch-bone");
            data.Used[index]=true;
        }

        private static void ContactPatchAncestors(ContactPatchSkin data, Transform actor)
        {
            for (int b=0;b<data.Bones.Length;++b) if (data.Used[b]) {
                data.ContactAncestors[b]=null;
                for (int j=0;j<contactCount;++j) if (contactTargets[j].Actor==actor &&
                    (data.Bones[b]==contactTargets[j].Bone || data.Bones[b].IsChildOf(contactTargets[j].Bone))) {
                    data.ContactAncestors[b]=contactTargets[j].Bone; break;
                }
            }
        }

        private static void ContactBakeGeometry(SkinnedMeshRenderer skin, out Vector3[] vertices,
            out Vector3[] normals, out int[] triangles)
        {
            Mesh snapshot=new Mesh();
            snapshot.hideFlags=HideFlags.HideAndDontSave;
            try {
                skin.BakeMesh(snapshot);
                vertices=snapshot.vertices; triangles=snapshot.triangles;
                normals=snapshot.normals;
                if (normals.Length!=vertices.Length && vertices.Length>0 && triangles.Length>0) {
                    snapshot.RecalculateNormals(); normals=snapshot.normals;
                }
            }
            finally { UnityEngine.Object.DestroyImmediate(snapshot); }
        }

        private static ContactPatch ContactBuildPatch(ContactTarget target, ContactSurface surface)
        {
            var batches=new List<ContactPatchSkin>();
            var triangles=new List<int>();
            int vertexTotal=0;
            ContactPatch patch=null;
            try {
                foreach (ContactSurfaceSource source in surface.Sources) {
                    bool[] related=new bool[source.Bones.Length]; bool used=false;
                    for (int b=0;b<related.Length;++b) if (source.Bones[b]!=null &&
                        (source.Bones[b]==target.Bone || source.Bones[b].IsChildOf(target.Bone))) { related[b]=true; used=true; }
                    if (!used) continue;
                    Mesh mesh=source.Mesh;
                    BoneWeight[] originalWeights=mesh.boneWeights;
                    float[] influence=new float[originalWeights.Length];
                    bool influenced=false;
                    for (int v=0;v<influence.Length;++v) {
                        BoneWeight w=originalWeights[v];
                        influence[v]=ContactSurfaceWeight(related,w.boneIndex0,w.weight0)+ContactSurfaceWeight(related,w.boneIndex1,w.weight1)+
                            ContactSurfaceWeight(related,w.boneIndex2,w.weight2)+ContactSurfaceWeight(related,w.boneIndex3,w.weight3);
                        influenced|=influence[v]>=.5f;
                    }
                    if (!influenced) continue;
                    // The measured failing actor's body/clothing meshes have
                    // no morphs. Unverified active morphs retain safe fallback.
                    for (int shape=0;shape<mesh.blendShapeCount;++shape)
                        if (source.Skin.GetBlendShapeWeight(shape)!=0) throw new FormatException("patch-active-morph");
                    Vector3[] sampled, sampledNormals; int[] indices;
                    ContactBakeGeometry(source.Skin,out sampled,out sampledNormals,out indices);
                    Matrix4x4[] bindPoses=mesh.bindposes;
                    if (sampled.Length!=originalWeights.Length || sampledNormals.Length!=sampled.Length ||
                        bindPoses.Length==0 || indices.Length==0 || indices.Length%3!=0 || indices.Length>786432)
                        throw new FormatException(String.Format(CultureInfo.InvariantCulture,
                            "patch-baked-layout mesh={0} vertices={1} weights={2} normals={3} bindposes={4} bones={5} indices={6}",
                            mesh.GetInstanceID(),sampled.Length,originalWeights.Length,sampledNormals.Length,
                            bindPoses.Length,source.Bones.Length,indices.Length));
                    var vertices=new List<Vector3>(); var normals=new List<Vector3>(); var weights=new List<BoneWeight>();
                    int[] map=new int[sampled.Length];
                    for (int t=0;t<indices.Length;t+=3) {
                        int a=indices[t], b=indices[t+1], c=indices[t+2];
                        if (a<0 || b<0 || c<0 || a>=sampled.Length || b>=sampled.Length || c>=sampled.Length)
                            throw new FormatException("patch-index");
                        if (Math.Max(influence[a],Math.Max(influence[b],influence[c]))<.5f ||
                            influence[a]<=.05f || influence[b]<=.05f || influence[c]<=.05f) continue;
                        if (triangles.Count>=24576) throw new FormatException("patch-triangle-limit");
                        for (int corner=0;corner<3;++corner) {
                            int index=indices[t+corner];
                            if (map[index]==0) {
                                if (vertexTotal+vertices.Count>=8192) throw new FormatException("patch-vertex-limit");
                                if (!Finite(sampled[index]) || !Finite(sampledNormals[index]) || sampledNormals[index].sqrMagnitude<1e-12f)
                                    throw new FormatException("patch-nonfinite");
                                vertices.Add(sampled[index]); normals.Add(sampledNormals[index]); weights.Add(originalWeights[index]);
                                map[index]=vertices.Count;
                            }
                            triangles.Add(vertexTotal+map[index]-1);
                        }
                    }
                    if (vertices.Count==0) continue;
                    var data=new ContactPatchSkin {
                        Bones=source.Bones, BindPoses=bindPoses, Start=vertexTotal,
                        RestVertices=vertices.ToArray(), RestNormals=normals.ToArray(), Weights=weights.ToArray(),
                        Used=new bool[source.Bones.Length], ContactAncestors=new Transform[source.Bones.Length],
                        Matrices=new Matrix4x4[source.Bones.Length], NormalMatrices=new Matrix4x4[source.Bones.Length]
                    };
                    for (int v=0;v<data.Weights.Length;++v) {
                        BoneWeight weight=data.Weights[v];
                        float sum=weight.weight0+weight.weight1+weight.weight2+weight.weight3;
                        if (!Finite(sum) || Math.Abs(sum-1)>.001f) throw new FormatException("patch-weight-sum");
                        ContactPatchUseBone(data,weight.boneIndex0,weight.weight0); ContactPatchUseBone(data,weight.boneIndex1,weight.weight1);
                        ContactPatchUseBone(data,weight.boneIndex2,weight.weight2); ContactPatchUseBone(data,weight.boneIndex3,weight.weight3);
                    }
                    for (int b=0;b<data.Bones.Length;++b) if (data.Used[b]) {
                        data.Matrices[b]=data.Bones[b].localToWorldMatrix*data.BindPoses[b];
                        data.NormalMatrices[b]=data.Matrices[b].inverse.transpose;
                    }
                    ContactPatchAncestors(data,target.Actor);
                    Matrix4x4 world=source.Skin.transform.localToWorldMatrix;
                    Matrix4x4 normalWorld=world.inverse.transpose;
                    float unit=contactUnitScale>0?contactUnitScale:1;
                    for (int v=0;v<data.RestVertices.Length;++v) {
                        Vector3 restPoint,restNormal; float error;
                        if (!ContactPatchMath.RecoverRest(data.Matrices,data.NormalMatrices,data.Weights[v],
                            world.MultiplyPoint3x4(data.RestVertices[v]),normalWorld.MultiplyVector(data.RestNormals[v]),
                            out restPoint,out restNormal,out error)) throw new FormatException("patch-recover-singular");
                        surface.PatchBindError=Math.Max(surface.PatchBindError,error);
                        if (error>.001f*unit) throw new FormatException("patch-recover-residual");
                        data.RestVertices[v]=restPoint; data.RestNormals[v]=restNormal;
                        ++surface.PatchRecoveredVertices;
                    }
                    batches.Add(data); vertexTotal+=vertices.Count;
                }
                if (triangles.Count==0) throw new FormatException("patch-empty");
                patch=new ContactPatch {
                    Skins=batches.ToArray(), Vertices=new Vector3[vertexTotal], Normals=new Vector3[vertexTotal],
                    Triangles=triangles.ToArray(), BindError=surface.PatchBindError
                };
                if (contactSettings.ShowContactLocations) try {
                    patch.DisplayMesh=new Mesh(); patch.DisplayMesh.hideFlags=HideFlags.HideAndDontSave;
                    patch.DisplayMesh.MarkDynamic(); patch.DisplayMesh.vertices=patch.Vertices; patch.DisplayMesh.triangles=patch.Triangles;
                }
                catch (Exception) { patch.DisplayFault=true; }
                return patch;
            }
            catch (Exception ex) {
                surface.PatchFailure=ex is FormatException?ex.Message:ex.GetType().Name;
                if (patch!=null && patch.DisplayMesh!=null) UnityEngine.Object.Destroy(patch.DisplayMesh);
                return null;
            }
        }

        private static bool ContactUpdatePatch(ContactPatch patch, Vector3 core)
        {
            Vector3 minimum=new Vector3(Single.PositiveInfinity,Single.PositiveInfinity,Single.PositiveInfinity);
            Vector3 maximum=new Vector3(Single.NegativeInfinity,Single.NegativeInfinity,Single.NegativeInfinity);
            foreach (ContactPatchSkin skin in patch.Skins) {
                for (int b=0;b<skin.Bones.Length;++b) if (skin.Used[b]) {
                    if (skin.Bones[b]==null) return false;
                    Matrix4x4 world=skin.Bones[b].localToWorldMatrix;
                    if (skin.ContactAncestors[b]!=null) for (int j=0;j<contactCount;++j)
                        if (contactTargets[j].Bone==skin.ContactAncestors[b]) {
                            if (contactTargets[j].ContactFrame!=Time.frameCount) return false;
                            world=contactTargets[j].BaseMatrix*contactTargets[j].ActualInverse*world; break;
                        }
                    Matrix4x4 matrix=world*skin.BindPoses[b];
                    if (!ContactFiniteMatrix(matrix) || !Finite(matrix.determinant) || matrix.determinant==0) return false;
                    skin.Matrices[b]=matrix; skin.NormalMatrices[b]=matrix.inverse.transpose;
                }
                for (int v=0;v<skin.RestVertices.Length;++v) {
                    Vector3 position=ContactPatchMath.SkinPoint(skin.Matrices,skin.Weights[v],skin.RestVertices[v]);
                    Vector3 normal=ContactPatchMath.SkinNormal(skin.NormalMatrices,skin.Weights[v],skin.RestNormals[v]);
                    if (!Finite(position) || !Finite(normal)) return false;
                    patch.Vertices[skin.Start+v]=position; patch.Normals[skin.Start+v]=normal;
                    minimum=Vector3.Min(minimum,position); maximum=Vector3.Max(maximum,position);
                }
            }
            // Include the bound tissue core so deeper inward contact remains
            // eligible without extending the open patch behind the whole actor.
            minimum=Vector3.Min(minimum,core); maximum=Vector3.Max(maximum,core);
            patch.Core=core; patch.Minimum=minimum; patch.Maximum=maximum; patch.Frame=Time.frameCount;
            if (contactSettings.ShowContactLocations && patch.DisplayMesh!=null && !patch.DisplayFault) try {
                patch.DisplayMesh.vertices=patch.Vertices;
                patch.DisplayMesh.bounds=new Bounds((minimum+maximum)*.5f,maximum-minimum);
            }
            catch (Exception) { patch.DisplayFault=true; }
            return true;
        }

        private static void ContactPatchPressure(ref ContactTarget target, Vector3 hand, float maximum,
            ref Vector3 desired, ref Vector3 pressure, ref float penetration, ref int inside, int bit)
        {
            ContactPatch patch=target.Surface.Patch;
            Vector3 point, normal; float depth;
            bool touching=ContactPatchMath.Sample(patch.Vertices,patch.Normals,patch.Triangles,patch.Core,
                patch.Minimum,patch.Maximum,hand,contactSettings.HandRadius*contactHandScale,out point,out normal,out depth);
            if (!touching) {
                if (inside==0 && normal.sqrMagnitude>0) target.ContactPoint=point;
                return;
            }
            inside|=bit;
            desired-=normal*Math.Min(depth,maximum); pressure-=normal*depth;
            if (depth>=penetration) target.ContactPoint=point;
            penetration+=depth;
        }
    }
}
#endif
