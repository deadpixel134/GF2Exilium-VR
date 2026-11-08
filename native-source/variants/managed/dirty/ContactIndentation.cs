#if GF2VR_TOUCH_PRIVATE && GF2VR_DIRTY_CONTACT
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.Text;
using UnityEngine;
using UnityEngine.Rendering;

namespace GF2.Private
{
    public static partial class ControllerDisplay
    {
        private sealed class ContactIndentSkin
        {
            internal SkinnedMeshRenderer Skin;
            internal Mesh Original, Owned, Snapshot;
            internal Transform[] Bones;
            internal BoneWeight[] Weights;
            internal Matrix4x4[] BindPoses, Matrices, NormalMatrices;
            internal Vector3[] BaseVertices, BaseNormals, Vertices, Normals, Offsets, Goals, NormalChanges;
            internal Vector4[] Tangents, OutputTangents;
            internal int[] Triangles;
            internal bool[] Touched;
            internal bool[] Used;
            internal List<Vector3> SamplePoints, SampleNormals;
            internal List<Vector4> SampleTangents;
            internal float[] MorphWeights, BaseMorphWeights;
            internal bool BaseReady;
            internal string Failure;
            internal int ChangedVertices;
            internal int RequiredInfluences;
        }
        private static readonly List<ContactIndentSkin> contactIndentSkins=new List<ContactIndentSkin>();
        private static long contactIndentFrames, contactIndentTicks;

        private static ContactIndentSkin ContactIndentOwner(SkinnedMeshRenderer skin)
        {
            for (int i=0;i<contactIndentSkins.Count;++i) if (contactIndentSkins[i].Skin==skin) return contactIndentSkins[i];
            return null;
        }

        private static Mesh ContactSourceMesh(SkinnedMeshRenderer skin)
        {
            if (skin==null) return null;
            ContactIndentSkin owner=ContactIndentOwner(skin);
            return owner!=null && owner.Owned!=null && skin.sharedMesh==owner.Owned?owner.Original:skin.sharedMesh;
        }

        private static void ContactReadMorphs(ContactIndentSkin owner)
        {
            for (int i=0;i<owner.MorphWeights.Length;++i) {
                float weight=owner.Skin.GetBlendShapeWeight(i);
                if (!Finite(weight)) throw new FormatException("press-morph-weight");
                owner.MorphWeights[i]=weight;
            }
        }

        private static void ContactWriteMorphs(ContactIndentSkin owner)
        {
            for (int i=0;i<owner.MorphWeights.Length;++i) owner.Skin.SetBlendShapeWeight(i,owner.MorphWeights[i]);
        }

        private static void ContactBakeOriginal(SkinnedMeshRenderer skin, Mesh destination)
        {
            ContactIndentSkin owner=ContactIndentOwner(skin);
            if (owner==null || owner.Owned==null || skin.sharedMesh!=owner.Owned) { skin.BakeMesh(destination); return; }
            ContactReadMorphs(owner);
            try {
                skin.sharedMesh=owner.Original;
                ContactWriteMorphs(owner);
                skin.BakeMesh(destination);
            }
            finally {
                if (skin!=null && skin.sharedMesh==owner.Original) {
                    skin.sharedMesh=owner.Owned;
                    ContactWriteMorphs(owner);
                }
            }
        }

        private static void ContactReleaseIndent(ContactIndentSkin owner)
        {
            if (owner.Skin!=null && owner.Owned!=null && owner.Skin.sharedMesh==owner.Owned) {
                ContactReadMorphs(owner);
                owner.Skin.sharedMesh=owner.Original;
                ContactWriteMorphs(owner);
                ContactMarkBindingReport(false);
            }
            owner.ChangedVertices=0;
            if (owner.Offsets!=null) Array.Clear(owner.Offsets,0,owner.Offsets.Length);
        }

        private static void ContactDisposeIndent(ContactIndentSkin owner)
        {
            // A failed restoration keeps the live mesh owner for the next
            // cleanup attempt. Never destroy a mesh still used by our renderer.
            ContactReleaseIndent(owner);
            if (owner.Owned!=null) UnityEngine.Object.Destroy(owner.Owned);
            if (owner.Snapshot!=null) UnityEngine.Object.Destroy(owner.Snapshot);
            owner.Owned=owner.Snapshot=null;
        }

        private static void ContactSuspendIndentations()
        {
            foreach (ContactIndentSkin owner in contactIndentSkins) try { ContactReleaseIndent(owner); }
                catch (Exception ex) { owner.Failure=ex.GetType().Name; }
        }

        private static void ContactClearIndentations()
        {
            for (int i=contactIndentSkins.Count-1;i>=0;--i) try {
                ContactDisposeIndent(contactIndentSkins[i]); contactIndentSkins.RemoveAt(i);
            }
            catch (Exception ex) { contactIndentSkins[i].Failure=ex.GetType().Name; }
        }

        private static bool ContactIndentNeeded(ContactIndentSkin owner)
        {
            if (owner.Skin==null || owner.Original==null || ContactSourceMesh(owner.Skin)!=owner.Original) return false;
            for (int i=0;i<contactCount;++i) if (ContactSettings.DetectionOnly(contactTargets[i].Region) &&
                (contactTargets[i].Region==3?contactSettings.FacePressMeters:contactSettings.ThighPressMeters)>0 &&
                contactTargets[i].Surface!=null && contactTargets[i].Surface.Patch!=null)
                foreach (ContactPatchSkin data in contactTargets[i].Surface.Patch.Skins) if (data.SourceSkin==owner.Skin) return true;
            return false;
        }

        private static void ContactPrepareIndentations()
        {
            for (int i=contactIndentSkins.Count-1;i>=0;--i) if (!ContactIndentNeeded(contactIndentSkins[i])) {
                try { ContactDisposeIndent(contactIndentSkins[i]); contactIndentSkins.RemoveAt(i); }
                catch (Exception ex) { contactIndentSkins[i].Failure=ex.GetType().Name; }
            }
            for (int i=0;i<contactCount;++i) {
                ContactTarget target=contactTargets[i];
                if (!ContactSettings.DetectionOnly(target.Region) || target.Surface==null || target.Surface.Patch==null) continue;
                if ((target.Region==3?contactSettings.FacePressMeters:contactSettings.ThighPressMeters)<=0) continue;
                foreach (ContactPatchSkin data in target.Surface.Patch.Skins) {
                    if (data.SourceSkin==null) continue;
                    ContactIndentSkin previous=ContactIndentOwner(data.SourceSkin);
                    if (previous!=null) {
                        if (previous.Owned!=null || previous.Failure!="press-skin-quality" || ContactSkinLimit(previous.Skin)<previous.RequiredInfluences) continue;
                        ContactDisposeIndent(previous); contactIndentSkins.Remove(previous);
                    }
                    if (contactIndentSkins.Count>=64) continue;
                    var owner=new ContactIndentSkin {Skin=data.SourceSkin,Original=ContactSourceMesh(data.SourceSkin),Bones=data.Bones};
                    contactIndentSkins.Add(owner);
                    try { ContactBuildIndent(owner); }
                    catch (Exception ex) {
                        owner.Failure=ex is FormatException?ex.Message:ex.GetType().Name;
                        try { ContactDisposeIndent(owner); } catch (Exception) { }
                    }
                }
            }
        }

        private static void ContactBuildIndent(ContactIndentSkin owner)
        {
            Mesh source=owner.Original;
            int count=source.vertexCount;
            if (count==0 || count>262144 || source.blendShapeCount>256) throw new FormatException("press-source-limit");
            owner.Weights=source.boneWeights; owner.BindPoses=source.bindposes;
            if (owner.Weights.Length!=count || owner.BindPoses.Length!=owner.Bones.Length) throw new FormatException("press-skin-layout");
            // Borrowed read-only mesh storage. Do not retain or dispose this view.
            var influences=source.GetBonesPerVertex();
            if (influences.Length!=count) throw new FormatException("press-influence-count");
            for (int v=0;v<influences.Length;++v) {
                if (influences[v]==0 || influences[v]>4) throw new FormatException("press-variable-influences");
                owner.RequiredInfluences=Math.Max(owner.RequiredInfluences,(int)influences[v]);
            }
            ContactIndentQuality(owner);
            owner.Used=new bool[owner.Bones.Length];
            foreach (BoneWeight weight in owner.Weights) {
                ContactIndentUseBone(owner,weight.boneIndex0,weight.weight0); ContactIndentUseBone(owner,weight.boneIndex1,weight.weight1);
                ContactIndentUseBone(owner,weight.boneIndex2,weight.weight2); ContactIndentUseBone(owner,weight.boneIndex3,weight.weight3);
            }
            owner.MorphWeights=new float[source.blendShapeCount]; owner.BaseMorphWeights=new float[source.blendShapeCount];
            owner.Snapshot=new Mesh(); owner.Snapshot.hideFlags=HideFlags.HideAndDontSave;
            owner.Skin.BakeMesh(owner.Snapshot);
            if (owner.Snapshot.vertexCount!=count || owner.Snapshot.subMeshCount!=source.subMeshCount) throw new FormatException("press-baked-layout");
            // BakeMesh stream retention is checked before installing a copy.
            // Missing UV/color/tangent data must leave the original skin active.
            for (int attribute=(int)VertexAttribute.Normal;attribute<=(int)VertexAttribute.TexCoord7;++attribute) {
                var kind=(VertexAttribute)attribute;
                if (source.HasVertexAttribute(kind) && (!owner.Snapshot.HasVertexAttribute(kind) ||
                    source.GetVertexAttributeDimension(kind)!=owner.Snapshot.GetVertexAttributeDimension(kind)))
                    throw new FormatException("press-missing-attribute-"+attribute);
            }
            owner.Owned=UnityEngine.Object.Instantiate((UnityEngine.Object)owner.Snapshot) as Mesh;
            if (owner.Owned==null) throw new FormatException("press-mesh-copy");
            owner.Owned.name=source.name;
            owner.Owned.hideFlags=HideFlags.HideAndDontSave;
            owner.Owned.MarkDynamic(); owner.Owned.boneWeights=owner.Weights; owner.Owned.bindposes=owner.BindPoses;
            owner.Owned.ClearBlendShapes();
            var zero=new Vector3[count];
            int frames=0;
            for (int shape=0;shape<source.blendShapeCount;++shape) {
                int frameCount=source.GetBlendShapeFrameCount(shape);
                if (frameCount<=0 || frames+frameCount>1024) throw new FormatException("press-shape-frame-limit");
                frames+=frameCount;
                for (int frame=0;frame<frameCount;++frame)
                    owner.Owned.AddBlendShapeFrame(source.GetBlendShapeName(shape),source.GetBlendShapeFrameWeight(shape,frame),zero,null,null);
            }
            owner.Triangles=owner.Snapshot.triangles;
            if (owner.Triangles.Length==0 || owner.Triangles.Length%3!=0 || owner.Triangles.Length>786432) throw new FormatException("press-triangle-layout");
            foreach (int index in owner.Triangles) if (index<0 || index>=count) throw new FormatException("press-index");
            owner.Matrices=new Matrix4x4[owner.Bones.Length]; owner.NormalMatrices=new Matrix4x4[owner.Bones.Length];
            owner.BaseVertices=new Vector3[count]; owner.BaseNormals=new Vector3[count];
            owner.Vertices=new Vector3[count]; owner.Normals=new Vector3[count];
            owner.Offsets=new Vector3[count]; owner.Goals=new Vector3[count]; owner.NormalChanges=new Vector3[count]; owner.Touched=new bool[count];
            owner.SamplePoints=new List<Vector3>(count); owner.SampleNormals=new List<Vector3>(count);
            owner.SampleTangents=new List<Vector4>(count);
            owner.Tangents=source.HasVertexAttribute(VertexAttribute.Tangent)?new Vector4[count]:new Vector4[0];
            owner.OutputTangents=new Vector4[owner.Tangents.Length];
            ContactRefreshIndentBasis(owner);
            owner.Owned.vertices=owner.BaseVertices; owner.Owned.normals=owner.BaseNormals;
            if (owner.Tangents.Length==count) owner.Owned.tangents=owner.Tangents;
            owner.Owned.bounds=source.bounds;
        }

        private static void ContactIndentMatrices(ContactIndentSkin owner)
        {
            for (int b=0;b<owner.Bones.Length;++b) if (owner.Used[b]) {
                if (owner.Bones[b]==null) throw new FormatException("press-missing-bone");
                Matrix4x4 matrix=owner.Bones[b].localToWorldMatrix*owner.BindPoses[b];
                if (!ContactFiniteMatrix(matrix) || matrix.determinant==0) throw new FormatException("press-bone-matrix");
                owner.Matrices[b]=matrix; owner.NormalMatrices[b]=matrix.inverse.transpose;
            }
        }

        private static void ContactIndentQuality(ContactIndentSkin owner)
        {
            // The inverse must match the weights actually drawn by the renderer.
            // Unsupported truncation keeps the authored renderer in control.
            if (ContactSkinLimit(owner.Skin)<owner.RequiredInfluences) throw new FormatException("press-skin-quality");
        }

        private static int ContactSkinLimit(SkinnedMeshRenderer skin)
        { return skin.quality==SkinQuality.Auto?(int)QualitySettings.skinWeights:(int)skin.quality; }

        private static void ContactIndentUseBone(ContactIndentSkin owner, int bone, float weight)
        {
            if (!Finite(weight) || weight<0) throw new FormatException("press-weight");
            if (weight==0) return;
            if (bone<0 || bone>=owner.Bones.Length || owner.Bones[bone]==null) throw new FormatException("press-weight-bone");
            owner.Used[bone]=true;
        }

        private static void ContactRefreshIndentBasis(ContactIndentSkin owner)
        {
            ContactIndentQuality(owner);
            ContactIndentMatrices(owner);
            ContactReadMorphs(owner);
            bool changed=!owner.BaseReady;
            for (int i=0;i<owner.MorphWeights.Length;++i) changed|=owner.MorphWeights[i]!=owner.BaseMorphWeights[i];
            if (!changed) return;
            ContactBakeOriginal(owner.Skin,owner.Snapshot);
            owner.Snapshot.GetVertices(owner.SamplePoints); owner.Snapshot.GetNormals(owner.SampleNormals);
            owner.Snapshot.GetTangents(owner.SampleTangents);
            if (owner.SamplePoints.Count!=owner.Weights.Length || owner.SampleNormals.Count!=owner.SamplePoints.Count ||
                owner.SampleTangents.Count!=owner.Tangents.Length) throw new FormatException("press-bake-count");
            Matrix4x4 world=owner.Skin.transform.localToWorldMatrix, normalWorld=world.inverse.transpose;
            for (int v=0;v<owner.SamplePoints.Count;++v) {
                BoneWeight weight=owner.Weights[v];
                float sum=weight.weight0+weight.weight1+weight.weight2+weight.weight3;
                if (!Finite(sum) || Math.Abs(sum-1)>.001f) throw new FormatException("press-weight-sum");
                Vector3 point, normal; float error;
                if (!ContactPatchMath.RecoverRest(owner.Matrices,owner.NormalMatrices,weight,
                    world.MultiplyPoint3x4(owner.SamplePoints[v]),normalWorld.MultiplyVector(owner.SampleNormals[v]),out point,out normal,out error) ||
                    error>.001f*(contactUnitScale>0?contactUnitScale:1)) throw new FormatException("press-rest-recovery");
                owner.BaseVertices[v]=point; owner.BaseNormals[v]=normal.normalized;
                if (owner.Tangents.Length==owner.SamplePoints.Count) {
                    Matrix4x4 inverse;
                    if (!ContactPatchMath.InverseSkin(owner.Matrices,weight,out inverse)) throw new FormatException("press-tangent-matrix");
                    Vector4 tangent=owner.SampleTangents[v];
                    Vector3 direction=inverse.MultiplyVector(world.MultiplyVector(new Vector3(tangent.x,tangent.y,tangent.z))).normalized;
                    if (!Finite(direction) || !Finite(tangent.w)) throw new FormatException("press-tangent");
                    owner.Tangents[v]=new Vector4(direction.x,direction.y,direction.z,tangent.w);
                }
            }
            Array.Copy(owner.MorphWeights,owner.BaseMorphWeights,owner.MorphWeights.Length);
            owner.BaseReady=true;
        }

        private static void ContactIndentHand(ContactIndentSkin owner, ContactTarget target, ContactPatchSkin data, Vector3 hand)
        {
            ContactPatch patch=target.Surface.Patch;
            Vector3 point, normal; float depth;
            float radius=contactSettings.HandRadius*contactHandScale;
            bool hit=patch.RegionVertices!=null?
                ContactPatchMath.SampleSegment(patch.RegionVertices,patch.RegionNormals,patch.RegionTriangles,patch.RegionIndexCount,
                    patch.SegmentStart,patch.SegmentFinish,patch.Minimum,patch.Maximum,hand,radius,out point,out normal,out depth):
                ContactPatchMath.Sample(patch.Vertices,patch.Normals,patch.Triangles,patch.Core,patch.Minimum,patch.Maximum,hand,radius,out point,out normal,out depth);
            if (!hit) return;
            float maximum=(target.Region==3?contactSettings.FacePressMeters:contactSettings.ThighPressMeters)*contactUnitScale;
            if (!Finite(maximum) || maximum<=0) return;
            Vector3 axis=patch.SegmentFinish-patch.SegmentStart;
            for (int j=0;j<data.SampleIndices.Length;++j) {
                int v=data.SampleIndices[j];
                Vector3 world=ContactPatchMath.SkinPoint(owner.Matrices,owner.Weights[v],owner.BaseVertices[v]);
                if (patch.RegionVertices!=null) {
                    float along=Vector3.Dot(world-patch.SegmentStart,axis);
                    if (along<0 || along>axis.sqrMagnitude) continue;
                }
                Vector3 outward=ContactPatchMath.SkinNormal(owner.NormalMatrices,owner.Weights[v],owner.BaseNormals[v]);
                Vector3 goal=ContactPressMath.Goal(world,outward,point,normal,depth,radius*2,maximum);
                if (goal.sqrMagnitude==0) continue;
                Matrix4x4 inverse;
                if (!ContactPatchMath.InverseSkin(owner.Matrices,owner.Weights[v],out inverse)) throw new FormatException("press-inverse");
                Vector3 local=inverse.MultiplyVector(goal);
                if (!Finite(local)) throw new FormatException("press-nonfinite-goal");
                // Overlapping hands/patches share the strongest local displacement.
                if (local.sqrMagnitude>owner.Goals[v].sqrMagnitude) owner.Goals[v]=local;
            }
        }

        private static void ContactApplyIndentMesh(ContactIndentSkin owner, float dt)
        {
            int changed=0;
            for (int v=0;v<owner.Vertices.Length;++v) {
                if (owner.Offsets[v].sqrMagnitude==0 && owner.Goals[v].sqrMagnitude==0) {
                    owner.Vertices[v]=owner.BaseVertices[v]; owner.Touched[v]=false; continue;
                }
                owner.Offsets[v]=ContactPressMath.Relax(owner.Offsets[v],owner.Goals[v],dt,contactUnitScale);
                if (!Finite(owner.Offsets[v])) throw new FormatException("press-offset");
                owner.Vertices[v]=owner.BaseVertices[v]+owner.Offsets[v];
                owner.Touched[v]=owner.Offsets[v].sqrMagnitude>0;
                if (owner.Touched[v]) ++changed;
            }
            owner.ChangedVertices=changed;
            if (changed==0) { ContactReleaseIndent(owner); return; }
            Array.Clear(owner.NormalChanges,0,owner.NormalChanges.Length);
            for (int t=0;t<owner.Triangles.Length;t+=3) {
                int a=owner.Triangles[t], b=owner.Triangles[t+1], c=owner.Triangles[t+2];
                if (!owner.Touched[a] && !owner.Touched[b] && !owner.Touched[c]) continue;
                Vector3 before=Vector3.Cross(owner.BaseVertices[b]-owner.BaseVertices[a],owner.BaseVertices[c]-owner.BaseVertices[a]).normalized;
                Vector3 after=Vector3.Cross(owner.Vertices[b]-owner.Vertices[a],owner.Vertices[c]-owner.Vertices[a]).normalized;
                Vector3 delta=after-before;
                owner.NormalChanges[a]+=delta; owner.NormalChanges[b]+=delta; owner.NormalChanges[c]+=delta;
            }
            for (int v=0;v<owner.Normals.Length;++v) {
                if (owner.NormalChanges[v].sqrMagnitude==0) {
                    owner.Normals[v]=owner.BaseNormals[v];
                    if (owner.Tangents.Length==owner.Normals.Length) owner.OutputTangents[v]=owner.Tangents[v];
                    continue;
                }
                owner.Normals[v]=(owner.BaseNormals[v]+owner.NormalChanges[v]).normalized;
                if (!Finite(owner.Normals[v]) || owner.Normals[v].sqrMagnitude==0) throw new FormatException("press-normal");
                if (owner.Tangents.Length==owner.Normals.Length) {
                    Vector4 tangent=owner.Tangents[v];
                    if (owner.NormalChanges[v].sqrMagnitude>0) {
                        Vector3 direction=new Vector3(tangent.x,tangent.y,tangent.z);
                        direction=(direction-owner.Normals[v]*Vector3.Dot(owner.Normals[v],direction)).normalized;
                        tangent=new Vector4(direction.x,direction.y,direction.z,tangent.w);
                    }
                    owner.OutputTangents[v]=tangent;
                }
            }
            owner.Owned.vertices=owner.Vertices; owner.Owned.normals=owner.Normals;
            if (owner.OutputTangents.Length==owner.Vertices.Length) owner.Owned.tangents=owner.OutputTangents;
            owner.Owned.bounds=owner.Original.bounds;
            if (owner.Skin.sharedMesh!=owner.Owned) {
                if (owner.Skin.sharedMesh!=owner.Original) throw new FormatException("press-external-mesh");
                ContactReadMorphs(owner); owner.Skin.sharedMesh=owner.Owned; ContactWriteMorphs(owner);
                ContactMarkBindingReport(false);
            }
        }

        private static void ContactUpdateIndentations(float dt, bool hands)
        {
            long started=Stopwatch.GetTimestamp();
            try {
                foreach (ContactIndentSkin owner in contactIndentSkins) {
                    if (owner.Owned==null) continue;
                    try {
                        if (!ContactIndentNeeded(owner)) { ContactReleaseIndent(owner); continue; }
                        bool input=false;
                        if (hands) for (int i=0;i<contactCount && !input;++i) {
                            ContactTarget candidate=contactTargets[i];
                            if (!ContactSettings.DetectionOnly(candidate.Region) || candidate.SurfaceState!=5 || candidate.InsideMask==0) continue;
                            foreach (ContactPatchSkin data in candidate.Surface.Patch.Skins) if (data.SourceSkin==owner.Skin) { input=true; break; }
                        }
                        if (!input && owner.ChangedVertices==0) continue;
                        ContactRefreshIndentBasis(owner);
                        Array.Clear(owner.Goals,0,owner.Goals.Length);
                        if (hands) for (int i=0;i<contactCount;++i) {
                            ContactTarget target=contactTargets[i];
                            if (!ContactSettings.DetectionOnly(target.Region) || target.SurfaceState!=5 ||
                                target.ContactFrame!=Time.frameCount || target.InsideMask==0) continue;
                            foreach (ContactPatchSkin data in target.Surface.Patch.Skins) if (data.SourceSkin==owner.Skin) {
                                if ((target.InsideMask&contactMask&1)!=0) ContactIndentHand(owner,target,data,contactLeft);
                                if ((target.InsideMask&contactMask&2)!=0) ContactIndentHand(owner,target,data,contactRight);
                            }
                        }
                        ContactApplyIndentMesh(owner,dt);
                        if (owner.Failure!=null) { owner.Failure=null; ContactMarkBindingReport(false); }
                    }
                    catch (Exception ex) {
                        owner.Failure=ex is FormatException?ex.Message:ex.GetType().Name;
                        ContactMarkBindingReport(false);
                        try { ContactReleaseIndent(owner); } catch (Exception) { }
                    }
                }
            }
            finally { ++contactIndentFrames; contactIndentTicks+=Stopwatch.GetTimestamp()-started; }
        }

        private static void ContactAppendIndentationDiagnostics(StringBuilder text)
        {
            text.Append(",\"indent_frames\":").Append(contactIndentFrames);
            text.Append(",\"indent_ms_total\":").Append((contactIndentTicks*1000.0/Stopwatch.Frequency).ToString("R",CultureInfo.InvariantCulture));
            text.Append(",\"indent_skins\":[");
            for (int i=0;i<Math.Min(contactIndentSkins.Count,8);++i) {
                if (i>0) text.Append(',');
                ContactIndentSkin owner=contactIndentSkins[i];
                text.Append("{\"skin\":"); Quoted(text,owner.Skin==null?null:owner.Skin.name);
                text.Append(",\"ready\":").Append(owner.Owned!=null && owner.Failure==null?"true":"false");
                text.Append(",\"active\":").Append(owner.Skin!=null && owner.Owned!=null && owner.Skin.sharedMesh==owner.Owned?"true":"false");
                text.Append(",\"vertices\":").Append(owner.ChangedVertices);
                text.Append(",\"required_influences\":").Append(owner.RequiredInfluences);
                text.Append(",\"failure\":"); Quoted(text,owner.Failure); text.Append('}');
            }
            text.Append(']');
        }
    }
}
#endif
