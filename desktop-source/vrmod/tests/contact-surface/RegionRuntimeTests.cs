// Minimal engine host for the production ContactRegions helpers. Real engine
// timing, BakeMesh morph behavior and performance remain hardware boundaries.
#pragma warning disable CS0649
using UnityEngine;

namespace GF2.Private
{
    public static partial class ControllerDisplay
    {
        private struct ContactTarget
        {
            internal Transform Bone, SegmentEnd, Actor;
            internal Animator Animator;
            internal int Region, SurfaceState, ContactFrame, InsideMask, UnobservedMask;
            internal long HandTicket;
            internal ContactSurface Surface;
        }
        private sealed class ContactSurfaceSource
        {
            internal SkinnedMeshRenderer Skin;
            internal Mesh Mesh;
            internal Transform Actor;
            internal Transform[] Bones;
            internal float[] Shapes;
        }
        private sealed class ContactSurface
        {
            internal int State, Samples;
            internal string Failure;
            internal ContactSurfaceSource[] Sources;
            internal ContactPatch Patch;
        }
        private sealed class ContactPatch
        {
            internal ContactPatchSkin[] Skins;
            internal Vector3[] Vertices, Normals, RegionVertices, RegionNormals, ClipA, ClipB, ClipNormalA, ClipNormalB;
            internal int[] Triangles;
            internal int[] RegionTriangles;
            internal Vector3 Minimum, Maximum;
            internal Transform RegionStart, RegionEnd;
            internal Vector3 SegmentStart, SegmentFinish, Core;
            internal int Frame, RegionIndexCount;
        }
        private sealed class ContactPatchSkin
        {
            internal SkinnedMeshRenderer SourceSkin;
            internal Transform[] Bones;
            internal SkinnedMeshRenderer DynamicSkin;
            internal Mesh DynamicSource, Snapshot;
            internal List<Vector3> SampledVertices, SampledNormals;
            internal int[] SampleIndices;
            internal int Start;
        }
        private static SkinnedMeshRenderer[] contactSurfaceSkins;
        private static int contactCount;
        private static ContactSettings contactSettings=new();
        private static float contactUnitScale=1,contactHandScale=1;
        private static ContactTarget[] contactTargets;
        private static long contactSurfaceBuilds, contactSurfaceFailures, contactSurfaceTicks, contactTicket;
        private static int contactHapticValid, contactHapticInside, contactMask;
        private static Vector3 contactLeft, contactRight;
        private static bool contactBindingReportPending;
        private static int contactBindingReportFrame,contactLastFrame,contactStatus;
        private sealed class Prepared { internal int Kind; internal GameObject Root; internal Animator ContextActor; }
        private static Prepared prepared;
        private static bool ContactName(string name) => name=="Chest_L" || name=="Butt_L_Back01";
        private static bool ContactVisiblePoint(Vector3 position,int layer) => position.x>=0;
        private static void Quoted(System.Text.StringBuilder text,string value) => text.Append(System.Text.Json.JsonSerializer.Serialize(value));
        private static ContactPatch ContactBuildPatch(ContactTarget target,ContactSurface surface) => null;
        private static bool Finite(Vector3 value) => ContactMotion.Finite(value);
        private static bool Finite(float value) => ContactMotion.Finite(value);
        private static bool ContactFiniteMatrix(Matrix4x4 value) => Finite(value.MultiplyPoint3x4(Vector3.zero)) && Finite(value.determinant);
        private static void ContactPatchPressure(ref ContactTarget target,Vector3 hand,float maximum,
            ref Vector3 desired,ref Vector3 pressure,ref float penetration,ref int inside,int bit)
        {
            var patch=target.Surface.Patch;
            if (ContactPatchMath.Sample(patch.Vertices,patch.Normals,patch.Triangles,patch.Core,
                new(-1,-1,-1),new(1,1,1),hand,.035f,out _,out _,out _)) inside|=bit;
        }

        internal static int RunRegionRuntimeTests()
        {
            int checks=0;
            void Check(bool result,string label) { ++checks; if (!result) throw new Exception(label); }
            var neck=new Transform {name="Neck_M"};
            var head=new Transform {name="Head_M",parent=neck};
            var leftEye=new Transform {name="Face_Eye_L",parent=head};
            var rightEye=new Transform {name="Face_Eye_R",parent=head};
            Check(ContactFaceSource(head,new[]{head,leftEye,rightEye}),"actual head/eye face skeleton accepted");
            Check(!ContactFaceSource(head,new[]{head}),"head-only hair excluded");
            Check(!ContactFaceSource(head,new[]{head,leftEye,rightEye,neck}),"body skin excluded from face");
            var hip=new Transform {name="Hip_L"};
            var part=new Transform {name="HipPart1_L",parent=hip};
            var knee=new Transform {name="Knee_L",parent=part};
            var ankle=new Transform {name="Ankle_L",parent=knee};
            var bones=new[]{hip,part,knee,ankle};
            Check(ContactThighEnd(hip,bones,4)==knee && ContactThighEnd(hip,bones,5)==null,"side-specific knee binding");
            var target=new ContactTarget {Bone=hip,Region=4};
            Check(ContactRelatedBone(target,hip) && ContactRelatedBone(target,part),"upper-leg influences selected");
            Check(!ContactRelatedBone(target,knee) && !ContactRelatedBone(target,ankle),"distal descendants excluded");

            var source=new Mesh {vertexCount=3};
            var renderer=new SkinnedMeshRenderer {sharedMesh=source,transform=new Transform {localToWorldMatrix=Matrix4x4.Translate(2,3,4)}};
            renderer.Points=new[]{new Vector3(0,0,0),Vector3.right,Vector3.up};
            var skin=new ContactPatchSkin {
                DynamicSkin=renderer,DynamicSource=source,Snapshot=new Mesh(),SampleIndices=new[]{0,1,2},
                SampledVertices=new List<Vector3>(3),SampledNormals=new List<Vector3>(3)
            };
            var patch=new ContactPatch {Vertices=new Vector3[3],Normals=new Vector3[3]};
            Vector3 low=new(float.PositiveInfinity,float.PositiveInfinity,float.PositiveInfinity), high=new(float.NegativeInfinity,float.NegativeInfinity,float.NegativeInfinity);
            Check(ContactUpdateFaceSkin(skin,patch,ref low,ref high) && patch.Vertices[0].z==4,"face snapshot transformed to current world");
            renderer.Morph=.07f;
            Check(ContactUpdateFaceSkin(skin,patch,ref low,ref high) && Math.Abs(patch.Vertices[0].z-4.07f)<1e-5f && renderer.BakeCount==2,"changed facial expression resampled without rest rebinding");
            renderer.sharedMesh=new Mesh();
            Check(!ContactUpdateFaceSkin(skin,patch,ref low,ref high),"replaced face source rejected");
            renderer.sharedMesh=source; renderer.InvalidNormals=true;
            Check(!ContactUpdateFaceSkin(skin,patch,ref low,ref high),"invalid dynamic face geometry rejected");

            Time.frameCount=10; contactTicket=1; contactMask=1; contactLeft=new(.2f,.2f,.01f);
            patch=new ContactPatch {Frame=10,Vertices=new[]{Vector3.zero,Vector3.right,Vector3.up},
                Normals=new[]{Vector3.forward,Vector3.forward,Vector3.forward},Triangles=new[]{0,1,2},Core=new(.25f,.25f,-.12f)};
            contactTargets=new[]{new ContactTarget {Region=3,SurfaceState=5,ContactFrame=10,Surface=new ContactSurface {Patch=patch}}};
            var haptics=new ContactHapticEdges();
            void Frame() { contactHapticValid=1;contactHapticInside=0;ContactApplyRegion(0,true);haptics.Observe(contactHapticValid,contactHapticInside,contactTicket++); }
            Frame(); Check(contactTargets[0].InsideMask==1 && haptics.Drain(contactTicket,100)==1,"face entry reaches actual haptic aggregator");
            Frame(); Check(haptics.Drain(contactTicket,100)==0,"continued face contact never repeats");
            contactTargets[0].SurfaceState=2; Frame();
            Check(contactHapticValid==0 && contactTargets[0].InsideMask==0,"missing region preserves held haptic uncertainty");
            contactTargets[0].SurfaceState=5; Frame();
            Check(haptics.Drain(contactTicket,100)==0,"geometry recovery while still touching does not reenter");
            contactLeft=new(.2f,.2f,.2f);Frame();
            Check(contactTargets[0].InsideMask==0,"valid full separation observed");
            contactLeft=new(.2f,.2f,.01f);Frame();
            Check(haptics.Drain(contactTicket,100)==1,"separation then recontact rearms haptics");
            Time.frameCount=20;contactLastFrame=-1;contactStatus=2;
            ContactMarkBindingReport();
            Check(ContactServiceStatus()==2,"initial zero-target state before callbacks cannot claim stable binding report");
            contactLastFrame=20;
            Check(ContactServiceStatus()==2,"same-frame stale sample after binding still waits");
            contactLastFrame=21;
            Check(ContactServiceStatus()==3,"first initialized zero-target callback requests evidence");
            contactBindingReportPending=false;
            Check(ContactServiceStatus()==2,"completed binding observation does not repeatedly request report");
            Time.frameCount=22;ContactMarkBindingReport();contactLastFrame=23;contactStatus=1;
            Check(ContactServiceStatus()==3,"changed background requests next completed binding observation");
            Time.frameCount=24;ContactMarkBindingReport(false);
            Check(ContactServiceStatus()==3,"repeated pressure events cannot starve pending binding evidence");
            Check(ContactBindingHint("Butt_L_Back01") && ContactBindingHint("breast_L") && !ContactBindingHint("Weapon"),"bounded candidate hints include alternate breast naming");
            var actor=new Transform {name="Actor with \"quoted\" name"};
            var animator=new Animator {transform=actor};
            var butt=new Transform {name="Butt_L_Back01",parent=hip,position=new Vector3(-1,0,0)};
            contactSurfaceSkins=new[]{new SkinnedMeshRenderer {Owner=animator,sharedMesh=source,bones=new[]{butt,hip}}};
            prepared=new Prepared {Kind=5};
            var diagnostic=new System.Text.StringBuilder("{");
            ContactAppendBindingEvidence(diagnostic);
            diagnostic.Remove(1,1);diagnostic.Append('}');
            using(var json=System.Text.Json.JsonDocument.Parse(diagnostic.ToString())) {
                var record=json.RootElement.GetProperty("binding_actors")[0];
                Check(record.GetProperty("actor_name").GetString()==actor.name,"binding evidence actor identity survives JSON escaping");
                var candidate=record.GetProperty("candidate_bones")[0];
                Check(candidate.GetProperty("configured").GetBoolean() && !candidate.GetProperty("visible").GetBoolean(),"binding evidence distinguishes configured target from visibility rejection");
            }
            contactSurfaceSkins[0].bones=Enumerable.Range(0,32).Select(i=>new Transform {name="Chest_"+i+new string('x',45),parent=hip}).ToArray();
            diagnostic=new System.Text.StringBuilder("{");ContactAppendBindingEvidence(diagnostic);diagnostic.Remove(1,1);diagnostic.Append('}');
            using(var json=System.Text.Json.JsonDocument.Parse(diagnostic.ToString())) {
                var record=json.RootElement.GetProperty("binding_actors")[0];
                Check(record.GetProperty("actor_id").GetInt32()==actor.GetInstanceID() && record.GetProperty("bone_limit").GetBoolean(),"large bone candidate sets retain actor identity with explicit truncation");
                Check(record.GetProperty("candidate_bones").GetArrayLength()>0 && diagnostic.Length<2200,"bounded binding evidence remains valid and nonempty");
            }
            contactSurfaceSkins=null;prepared=null;
            return checks;
        }
    }
}

namespace UnityEngine
{
    internal sealed class GameObject { internal bool activeInHierarchy=true; internal Transform transform; internal int layer; }
    internal sealed class Transform
    {
        internal string name;
        internal Transform parent;
        internal GameObject gameObject=new();
        internal Matrix4x4 localToWorldMatrix=Matrix4x4.Scale(1,1,1);
        internal Vector3 position;
        internal int GetInstanceID() => GetHashCode();
        internal bool IsChildOf(Transform ancestor) { for(var value=this;value!=null;value=value.parent) if(value==ancestor)return true;return false; }
    }
    internal sealed class Animator { internal bool isActiveAndEnabled=true; internal GameObject gameObject=new(); internal Transform transform; }
    internal sealed class SkinnedMeshRenderer
    {
        internal string name="test-skin";
        internal SkinQuality quality=SkinQuality.Auto;
        internal bool enabled=true,InvalidNormals;
        internal GameObject gameObject=new();
        internal Transform transform=new();
        private Mesh mesh;
        internal Mesh sharedMesh { get=>mesh;set {mesh=value;Morph=0;Morphs.Clear();} }
        internal Transform[] bones;
        internal Vector3[] Points;
        internal float Morph;
        internal Dictionary<int,float> Morphs=new();
        internal int BakeCount;
        internal Animator Owner;
        internal T GetComponentInParent<T>() where T:class => Owner as T;
        internal float GetBlendShapeWeight(int index) => Morphs.TryGetValue(index,out float weight)?weight:Morph;
        internal void SetBlendShapeWeight(int index,float weight) {Morphs[index]=weight;if(index==0)Morph=weight;}
        internal bool LoseUvOnBake,ThrowOnBake;
        internal void BakeMesh(Mesh output) { ++BakeCount; if(ThrowOnBake)throw new InvalidOperationException("test-bake-failure");BakeTestMesh(this,output); }
        private static void BakeTestMesh(SkinnedMeshRenderer skin,Mesh output)
        {
            Mesh source=skin.sharedMesh;
            Vector3[] points=source.IsCopy?source.RawVertices:skin.Points??source.RawVertices;
            if(points.Length==0 && source.vertexCount>0)points=new Vector3[source.vertexCount];
            var vertices=new Vector3[points.Length];
            var normals=new Vector3[points.Length];
            for(int i=0;i<points.Length;++i){
                Vector3 point=points[i];
                if(source.blendShapeCount==0)point+=Vector3.forward*skin.Morph;
                else for(int shape=0;shape<source.blendShapeCount;++shape)point+=source.ShapeDeltas[shape][i]*skin.GetBlendShapeWeight(shape)/100;
                Vector3 normal=Vector3.forward;
                if(skin.bones!=null && source.boneWeights.Length==points.Length && source.bindposes.Length==skin.bones.Length){
                    var matrices=skin.bones.Select((b,index)=>b.localToWorldMatrix*source.bindposes[index]).ToArray();
                    point=skin.transform.localToWorldMatrix.inverse.MultiplyPoint3x4(GF2.Private.ContactPatchMath.SkinPoint(matrices,source.boneWeights[i],point));
                }
                vertices[i]=point;normals[i]=skin.InvalidNormals?Vector3.zero:normal;
            }
            output.vertices=vertices;output.normals=normals;
            output.triangles=(int[])source.triangles.Clone();output.subMeshCount=source.subMeshCount;
            output.tangents=(Vector4[])source.tangents.Clone();
            output.Attributes=new HashSet<Rendering.VertexAttribute>(source.Attributes);
            if(skin.LoseUvOnBake)output.Attributes.Remove(Rendering.VertexAttribute.TexCoord0);
        }
    }
    internal static class Time { internal static int frameCount; }
}
