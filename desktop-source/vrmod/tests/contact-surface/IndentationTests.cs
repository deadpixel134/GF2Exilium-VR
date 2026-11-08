using UnityEngine;
using UnityEngine.Rendering;

namespace GF2.Private
{
    public static partial class ControllerDisplay
    {
        internal static int RunIndentationTests()
        {
            int checks=0;
            void Check(bool value,string label) {++checks;if(!value)throw new Exception(label);}
            Check(ContactSettings.Read(null,out var defaults) && defaults.FacePressMeters==.008f && defaults.ThighPressMeters==.006f,"user-selected face8mm and thigh6mm defaults");
            Check(ContactSurfaceMath.IncludeTriangle(2,.2f,.1f,0),"mixed Root/butt weights retain actual nonzero butt surface support");
            Check(!ContactSurfaceMath.IncludeTriangle(2,0,0,0),"zero butt influence never invents pelvis surface");
            Check(!ContactSurfaceMath.IncludeTriangle(1,.2f,.1f,0),"breast triangle selection unchanged");
            Check(!ContactSurfaceMath.IncludeTriangle(2,float.NaN,.2f,.1f),"invalid influence rejected");
            foreach(float penetration in new[]{.001f,.01f,1f}) {
                Vector3 goal=ContactPressMath.Goal(Vector3.zero,Vector3.forward,Vector3.zero,Vector3.forward,penetration,.07f,.006f);
                Check(goal.z<0 && goal.magnitude<=.006f,"face indentation goes inward within configured depth");
            }
            Check(ContactPressMath.Goal(new(.08f,0,0),Vector3.forward,Vector3.zero,Vector3.forward,.1f,.07f,.015f).sqrMagnitude==0,"pressure remains local");
            Check(ContactPressMath.Goal(Vector3.zero,Vector3.forward*-1,Vector3.zero,Vector3.forward,.1f,.07f,.015f).sqrMagnitude==0,"opposite interior surface excluded");
            Check(ContactPressMath.Goal(Vector3.zero,Vector3.forward,Vector3.zero,Vector3.forward,.1f,.07f,0).sqrMagnitude==0,"zero depth disables pressing");
            Vector3 offset=new(0,0,-.01f);
            for(int i=0;i<200;++i)offset=ContactPressMath.Relax(offset,Vector3.zero,1f/90,1);
            Check(offset.sqrMagnitude==0,"release returns exactly to undeformed basis");
            ContactClearIndentations();
            var bone=new Transform {name="Head_M",localToWorldMatrix=Matrix4x4.Translate(0,0,2)};
            var source=new Mesh {vertices=new[]{Vector3.zero,Vector3.right,Vector3.up},normals=new[]{Vector3.forward,Vector3.forward,Vector3.forward},
                triangles=new[]{0,1,2},boneWeights=Enumerable.Repeat(new BoneWeight {boneIndex0=0,weight0=1},3).ToArray(),
                bindposes=new[]{Matrix4x4.Scale(1,1,1)},bounds=new Bounds(Vector3.zero,new(2,2,2))};
            source.Attributes.Add(VertexAttribute.TexCoord0);
            source.Attributes.Add(VertexAttribute.Tangent);
            source.tangents=Enumerable.Repeat(new Vector4(1,0,0,1),3).ToArray();
            source.AddBlendShapeFrame("Smile",100,Enumerable.Repeat(new Vector3(0,0,.1f),3).ToArray(),null,null);
            source.isReadable=false;
            var skin=new SkinnedMeshRenderer {sharedMesh=source,bones=new[]{bone}};
            skin.SetBlendShapeWeight(0,50);
            var owner=new ContactIndentSkin {Skin=skin,Original=source,Bones=skin.bones};
            contactIndentSkins.Add(owner);
            ContactBuildIndent(owner);
            Check(skin.sharedMesh==source && owner.Owned!=null,"cold preparation keeps borrowed renderer unchanged");
            Check(owner.Owned.name==source.name,"owned copy retains game-visible mesh name");
            Check(owner.Owned.HasVertexAttribute(VertexAttribute.TexCoord0) && owner.Owned.subMeshCount==source.subMeshCount,"authored UV and submeshes retained");
            Check(owner.Owned.GetBlendShapeName(0)=="Smile" && owner.Owned.GetBlendShapeFrameWeight(0,0)==100,"blendshape channel contract retained");
            Check(Math.Abs(owner.BaseVertices[0].z-.05f)<1e-5f,"current expression recovered to rest coordinates");
            var data=new ContactPatchSkin {SourceSkin=skin,Bones=skin.bones,SampleIndices=new[]{0,1,2}};
            var patch=new ContactPatch {Skins=new[]{data},Vertices=new[]{new Vector3(0,0,2.05f),new Vector3(1,0,2.05f),new Vector3(0,1,2.05f)},
                Normals=new[]{Vector3.forward,Vector3.forward,Vector3.forward},Triangles=new[]{0,1,2},Core=new(.25f,.25f,1.9f),
                Minimum=new(0,0,1.9f),Maximum=new(1,1,2.05f),Frame=Time.frameCount};
            contactTargets=new[]{new ContactTarget {Region=3,SurfaceState=5,ContactFrame=Time.frameCount,InsideMask=1,Surface=new ContactSurface {Patch=patch}}};
            contactCount=1;contactMask=1;contactLeft=new(0,0,2.07f);
            ContactUpdateIndentations(1f/90,true);
            Check(skin.sharedMesh==owner.Owned && owner.ChangedVertices==1,"local pressure activates owned mesh copy");
            Check(owner.Owned.RawVertices[0].z<owner.BaseVertices[0].z && owner.Owned.RawVertices[1].z==owner.BaseVertices[1].z,"pressure changes local vertex only");
            Vector4 tangent=owner.Owned.tangents[0];
            Check(Math.Abs(Vector3.Dot(owner.Owned.normals[0],new Vector3(tangent.x,tangent.y,tangent.z)))<1e-6f && tangent.w==1,"pressed normal/tangent frame preserves authored handedness");
            Vector3 singleGoal=owner.Goals[0];
            contactMask=3;contactRight=contactLeft;contactTargets[0].InsideMask=3;
            ContactUpdateIndentations(1f/90,true);
            Check((owner.Goals[0]-singleGoal).sqrMagnitude<1e-12f,"overlapping hands do not double local indentation");
            Check(ContactSourceMesh(skin)==source && skin.GetBlendShapeWeight(0)==50,"canonical source and expression survive renderer swap");
            skin.SetBlendShapeWeight(0,75);
            var snapshot=new Mesh();ContactBakeOriginal(skin,snapshot);
            Check(Math.Abs(snapshot.RawVertices[0].z-2.075f)<1e-5f,"contact sampling reads original expression without indentation feedback");
            Check(skin.sharedMesh==owner.Owned && skin.GetBlendShapeWeight(0)==75,"temporary original bake restores owned mesh and weights");
            ContactRefreshIndentBasis(owner);
            Check(Math.Abs(owner.BaseVertices[0].z-.075f)<1e-5f,"expression change refreshes basis independently of pressure");
            skin.ThrowOnBake=true;
            try {ContactBakeOriginal(skin,snapshot);throw new Exception("expected bake failure");}catch(InvalidOperationException){ }
            Check(skin.sharedMesh==owner.Owned && skin.GetBlendShapeWeight(0)==75,"failed original bake restores renderer in finally");
            skin.SetBlendShapeWeight(0,80);
            ContactUpdateIndentations(1f/90,true);
            Check(owner.Failure!=null && skin.sharedMesh==source,"temporary bake failure restores original renderer and isolates press effect");
            skin.ThrowOnBake=false;
            ContactUpdateIndentations(1f/90,true);
            Check(owner.Failure==null && skin.sharedMesh==owner.Owned,"valid current surface recovers pressure after temporary failure");
            skin.SetBlendShapeWeight(0,75);ContactRefreshIndentBasis(owner);
            ContactSuspendIndentations();
            Check(skin.sharedMesh==source && skin.GetBlendShapeWeight(0)==75 && owner.Offsets[0].sqrMagnitude==0,"tracking/lifetime suspension restores borrowed mesh and live expression");
            owner.Goals[0]=new(0,0,-.006f);ContactApplyIndentMesh(owner,1f/90);
            var external=new Mesh();skin.sharedMesh=external;ContactDisposeIndent(owner);contactIndentSkins.Clear();
            Check(skin.sharedMesh==external && owner.Owned==null,"external mesh replacement wins over restoration");
            var secondSkin=new SkinnedMeshRenderer {sharedMesh=source,bones=new[]{bone}};
            var thirdSkin=new SkinnedMeshRenderer {sharedMesh=source,bones=new[]{bone}};
            var second=new ContactIndentSkin {Skin=secondSkin,Original=source,Bones=secondSkin.bones};
            var third=new ContactIndentSkin {Skin=thirdSkin,Original=source,Bones=thirdSkin.bones};
            contactIndentSkins.Add(second);contactIndentSkins.Add(third);
            ContactBuildIndent(second);ContactBuildIndent(third);
            second.Goals[0]=new(0,0,-.006f);ContactApplyIndentMesh(second,1f/90);
            Check(secondSkin.sharedMesh==second.Owned && thirdSkin.sharedMesh==source && second.Owned!=third.Owned,"shared actor asset has independent runtime mesh ownership");
            third.Goals[0]=new(0,0,-.015f);ContactApplyIndentMesh(third,1f/90);
            ContactReleaseIndent(second);
            Check(secondSkin.sharedMesh==source && thirdSkin.sharedMesh==third.Owned,"restoring one actor never restores another actor mesh");
            ContactClearIndentations();
            var bad=new ContactIndentSkin {Skin=new SkinnedMeshRenderer {sharedMesh=source,bones=new[]{bone},LoseUvOnBake=true},Original=source,Bones=new[]{bone}};
            contactIndentSkins.Add(bad);
            bool rejected=false;
            try {ContactBuildIndent(bad);}catch(FormatException ex){rejected=ex.Message.StartsWith("press-missing-attribute");}
            Check(rejected && bad.Skin.sharedMesh==source,"missing baked UV rejects deformation before renderer installation");
            ContactClearIndentations();
            source.InfluencesOverride=new byte[]{5,5,5};
            bad=new ContactIndentSkin {Skin=new SkinnedMeshRenderer {sharedMesh=source,bones=new[]{bone}},Original=source,Bones=new[]{bone}};
            rejected=false;
            try {ContactBuildIndent(bad);}catch(FormatException ex){rejected=ex.Message=="press-variable-influences";}
            Check(rejected && bad.Skin.sharedMesh==source,"more than four authored weights never silently truncate original skin");
            ContactDisposeIndent(bad);
            source.InfluencesOverride=new byte[]{2,2,2};QualitySettings.skinWeights=SkinWeights.OneBone;
            bad=new ContactIndentSkin {Skin=new SkinnedMeshRenderer {sharedMesh=source,bones=new[]{bone}},Original=source,Bones=new[]{bone}};
            rejected=false;
            try {ContactBuildIndent(bad);}catch(FormatException ex){rejected=ex.Message=="press-skin-quality";}
            Check(rejected && bad.Skin.sharedMesh==source,"reduced renderer skinning weights cannot change rest geometry outside the press");
            ContactDisposeIndent(bad);QualitySettings.skinWeights=SkinWeights.FourBones;source.InfluencesOverride=null;
            foreach(string json in new[]{"{\"schemaVersion\":1,\"facePressMeters\":0,\"thighPressMeters\":0.2}","{\"schemaVersion\":1,\"facePressMeters\":0.006,\"thighPressMeters\":0.015}"})
                Check(ContactSettings.Read(System.Text.Encoding.UTF8.GetBytes(json),out _),"pressure config uses physical domain without recommendation clamps");
            Check(!ContactSettings.Read(System.Text.Encoding.UTF8.GetBytes("{\"schemaVersion\":1,\"facePressMeters\":-0.1}"),out _),"negative pressure depth rejected");
            return checks;
        }
    }
}

namespace UnityEngine
{
    internal enum HideFlags {HideAndDontSave}
    internal enum SkinQuality {Auto=0,Bone1=1,Bone2=2,Bone4=4}
    internal enum SkinWeights {OneBone=1,TwoBones=2,FourBones=4,Unlimited=255}
    internal static class QualitySettings {internal static SkinWeights skinWeights=SkinWeights.FourBones;}
    internal struct Vector4 {internal float x,y,z,w;internal Vector4(float x,float y,float z,float w){this.x=x;this.y=y;this.z=z;this.w=w;}}
    internal struct Bounds {internal Vector3 center,size;internal Bounds(Vector3 center,Vector3 size){this.center=center;this.size=size;}}
    internal class Object
    {
        internal string name="test-mesh";
        internal bool Destroyed;
        internal static Object Instantiate(Object value)=>value is Mesh mesh?mesh.Copy():throw new NotSupportedException();
        internal static void Destroy(Object value){if(value!=null)value.Destroyed=true;}
    }
    internal sealed class Mesh:Object
    {
        internal HideFlags hideFlags;
        internal bool isReadable=true,IsCopy;
        internal int vertexCount,subMeshCount=1,blendShapeCount;
        internal Vector3[] RawVertices=Array.Empty<Vector3>(),Normals=Array.Empty<Vector3>();
        internal Vector3[] vertices {get=>isReadable?RawVertices:throw new InvalidOperationException("original CPU vertices unavailable");set {RawVertices=(Vector3[])value.Clone();vertexCount=value.Length;}}
        internal Vector3[] Vertices {get=>RawVertices;set=>vertices=value;}
        internal Vector3[] normals {get=>Normals;set=>Normals=(Vector3[])value.Clone();}
        internal Vector4[] tangents=Array.Empty<Vector4>();
        internal int[] triangles=Array.Empty<int>();
        internal BoneWeight[] boneWeights=Array.Empty<BoneWeight>();
        internal byte[] InfluencesOverride;
        internal byte[] GetBonesPerVertex()=>InfluencesOverride??boneWeights.Select(w=>(byte)((w.weight0>0?1:0)+(w.weight1>0?1:0)+(w.weight2>0?1:0)+(w.weight3>0?1:0))).ToArray();
        internal Matrix4x4[] bindposes=Array.Empty<Matrix4x4>();
        internal Bounds bounds;
        internal HashSet<Rendering.VertexAttribute> Attributes=new(){Rendering.VertexAttribute.Position,Rendering.VertexAttribute.Normal};
        internal List<string> ShapeNames=new();
        internal List<Vector3[]> ShapeDeltas=new();
        internal List<List<float>> ShapeFrames=new();
        internal void GetVertices(List<Vector3> output){output.Clear();output.AddRange(vertices);}
        internal void GetNormals(List<Vector3> output){output.Clear();output.AddRange(normals);}
        internal void GetTangents(List<Vector4> output){output.Clear();output.AddRange(tangents);}
        internal bool HasVertexAttribute(Rendering.VertexAttribute attribute)=>Attributes.Contains(attribute);
        internal int GetVertexAttributeDimension(Rendering.VertexAttribute attribute)=>HasVertexAttribute(attribute)?(attribute==Rendering.VertexAttribute.TexCoord0?2:3):0;
        internal void MarkDynamic(){ }
        internal void ClearBlendShapes(){ShapeNames.Clear();ShapeDeltas.Clear();ShapeFrames.Clear();blendShapeCount=0;}
        internal string GetBlendShapeName(int shape)=>ShapeNames[shape];
        internal int GetBlendShapeFrameCount(int shape)=>ShapeFrames[shape].Count;
        internal float GetBlendShapeFrameWeight(int shape,int frame)=>ShapeFrames[shape][frame];
        internal void AddBlendShapeFrame(string name,float weight,Vector3[] vertices,Vector3[] normals,Vector3[] tangents){
            int index=ShapeNames.IndexOf(name);
            if(index<0){index=ShapeNames.Count;ShapeNames.Add(name);ShapeDeltas.Add((Vector3[])vertices.Clone());ShapeFrames.Add(new());}
            ShapeFrames[index].Add(weight);blendShapeCount=ShapeNames.Count;
        }
        internal Mesh Copy(){
            var result=new Mesh {vertices=RawVertices,normals=Normals,subMeshCount=subMeshCount,triangles=(int[])triangles.Clone(),
                tangents=(Vector4[])tangents.Clone(),bounds=bounds,Attributes=new(Attributes),IsCopy=true};
            return result;
        }
    }
}
namespace UnityEngine.Rendering
{
    internal enum VertexAttribute {Position,Normal,Tangent,Color,TexCoord0,TexCoord1,TexCoord2,TexCoord3,TexCoord4,TexCoord5,TexCoord6,TexCoord7}
}
