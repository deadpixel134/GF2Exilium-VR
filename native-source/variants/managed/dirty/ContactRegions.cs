#if GF2VR_TOUCH_PRIVATE && GF2VR_DIRTY_CONTACT
using System;
using System.Collections.Generic;
using System.Diagnostics;
using UnityEngine;

namespace GF2.Private
{
    public static partial class ControllerDisplay
    {
        private static bool ContactFaceSource(Transform head, Transform[] bones)
        {
            if (head==null || head.parent==null || head.parent.name!="Neck_M" || bones==null || bones.Length>32) return false;
            bool left=false, right=false, root=false;
            foreach (Transform bone in bones) {
                if (bone==null) continue;
                if (bone!=head && !bone.IsChildOf(head)) return false;
                root|=bone==head;
                left|=bone.name=="Face_Eye_L";
                right|=bone.name=="Face_Eye_R";
            }
            return root && left && right;
        }

        private static Transform ContactThighEnd(Transform hip, Transform[] bones, int region)
        {
            string part=region==4?"HipPart1_L":"HipPart1_R";
            string knee=region==4?"Knee_L":"Knee_R";
            foreach (Transform bone in bones)
                if (bone!=null && bone.name==knee && bone.parent!=null && bone.parent.name==part && bone.parent.parent==hip)
                    return bone;
            return null;
        }

        private static bool ContactRelatedBone(ContactTarget target, Transform bone)
        {
            if (bone==null) return false;
            if (target.Region==4 || target.Region==5)
                return bone==target.Bone || (bone.parent==target.Bone && bone.name==(target.Region==4?"HipPart1_L":"HipPart1_R"));
            return bone==target.Bone || bone.IsChildOf(target.Bone);
        }

        private static ContactSurface ContactBuildRegionSurface(ContactTarget target)
        {
            long started=Stopwatch.GetTimestamp();
            ++contactSurfaceBuilds;
            var surface=new ContactSurface {State=2};
            var sources=new List<ContactSurfaceSource>();
            try {
                if (contactSurfaceSkins==null) return surface;
                foreach (SkinnedMeshRenderer skin in contactSurfaceSkins) {
                    if (skin==null || !skin.enabled || !skin.gameObject.activeInHierarchy || ContactSourceMesh(skin)==null ||
                        skin.GetComponentInParent<Animator>()!=target.Animator) continue;
                    if (sources.Count>=128) throw new FormatException("region-source-limit");
                    Transform[] bones=skin.bones;
                    if (bones==null || bones.Length>2048) throw new FormatException("region-bone-limit");
                    var source=new ContactSurfaceSource {Skin=skin,Mesh=ContactSourceMesh(skin),Actor=target.Actor,Bones=bones,Shapes=new float[0]};
                    sources.Add(source);
                    if (target.Region==3) continue; // Current face geometry includes current morph weights.
                    bool related=false;
                    foreach (Transform bone in bones) related|=ContactRelatedBone(target,bone);
                    if (!related) continue;
                    if (source.Mesh.blendShapeCount>256) throw new FormatException("region-shape-limit");
                    source.Shapes=new float[source.Mesh.blendShapeCount];
                    for (int i=0;i<source.Shapes.Length;++i) source.Shapes[i]=skin.GetBlendShapeWeight(i);
                }
                surface.Sources=sources.ToArray();
                surface.Patch=ContactBuildPatch(target,surface);
                if (surface.Patch!=null) surface.Samples=surface.Patch.Vertices.Length;
            }
            catch (Exception ex) { surface.Failure=ex is FormatException?ex.Message:ex.GetType().Name; ++contactSurfaceFailures; }
            finally {
                if (surface.Sources==null) surface.Sources=sources.ToArray();
                contactSurfaceTicks+=Stopwatch.GetTimestamp()-started;
            }
            return surface;
        }

        private static void ContactApplyRegion(int index, bool hands)
        {
            ContactTarget target=contactTargets[index];
            if (target.SurfaceState!=5 || target.ContactFrame!=Time.frameCount ||
                target.Surface==null || target.Surface.Patch==null || target.Surface.Patch.Frame!=Time.frameCount) {
                target.UnobservedMask|=target.InsideMask;
                contactHapticValid&=~target.UnobservedMask;
                target.InsideMask=0; target.HandTicket=-1;
                contactTargets[index]=target;
                return;
            }
            target.UnobservedMask=0;
            Vector3 desired=Vector3.zero, pressure=Vector3.zero;
            float penetration=0;
            int inside=0;
            if (hands) {
                if ((contactMask&1)!=0) ContactPatchPressure(ref target,contactLeft,0,ref desired,ref pressure,ref penetration,ref inside,1);
                if ((contactMask&2)!=0) ContactPatchPressure(ref target,contactRight,0,ref desired,ref pressure,ref penetration,ref inside,2);
            }
            target.InsideMask=inside; target.HandTicket=hands?contactTicket:-1;
            contactHapticInside|=inside;
            contactTargets[index]=target;
        }

        private static bool ContactUpdateFaceSkin(ContactPatchSkin skin, ContactPatch patch,
            ref Vector3 minimum, ref Vector3 maximum)
        {
            if (skin.DynamicSkin==null || ContactSourceMesh(skin.DynamicSkin)!=skin.DynamicSource ||
                !skin.DynamicSkin.enabled || !skin.DynamicSkin.gameObject.activeInHierarchy) return false;
            ContactBakeOriginal(skin.DynamicSkin,skin.Snapshot);
            skin.Snapshot.GetVertices(skin.SampledVertices);
            skin.Snapshot.GetNormals(skin.SampledNormals);
            if (skin.SampledVertices.Count!=skin.DynamicSource.vertexCount || skin.SampledNormals.Count!=skin.SampledVertices.Count) return false;
            Matrix4x4 world=skin.DynamicSkin.transform.localToWorldMatrix;
            if (!ContactFiniteMatrix(world) || !Finite(world.determinant) || world.determinant==0) return false;
            Matrix4x4 normalWorld=world.inverse.transpose;
            for (int v=0;v<skin.SampleIndices.Length;++v) {
                int index=skin.SampleIndices[v];
                Vector3 point=world.MultiplyPoint3x4(skin.SampledVertices[index]);
                Vector3 normal=normalWorld.MultiplyVector(skin.SampledNormals[index]);
                if (!Finite(point) || !Finite(normal) || normal.sqrMagnitude<1e-12f) return false;
                patch.Vertices[skin.Start+v]=point; patch.Normals[skin.Start+v]=normal;
                minimum=Vector3.Min(minimum,point); maximum=Vector3.Max(maximum,point);
            }
            return true;
        }

        private static bool ContactClipThigh(ContactPatch patch, ref Vector3 minimum, ref Vector3 maximum)
        {
            if (patch.RegionStart==null || patch.RegionEnd==null || patch.RegionEnd.parent==null ||
                patch.RegionEnd.parent.parent!=patch.RegionStart || !patch.RegionEnd.gameObject.activeInHierarchy) return false;
            Vector3 start=patch.RegionStart.position, end=patch.RegionEnd.position;
            if (!Finite(start) || !Finite(end) || (end-start).sqrMagnitude<1e-12f) return false;
            int written=0;
            minimum=new Vector3(Single.PositiveInfinity,Single.PositiveInfinity,Single.PositiveInfinity);
            maximum=new Vector3(Single.NegativeInfinity,Single.NegativeInfinity,Single.NegativeInfinity);
            for (int i=0;i<patch.Triangles.Length;i+=3) {
                int a=patch.Triangles[i], b=patch.Triangles[i+1], c=patch.Triangles[i+2];
                int count=ContactRegionMath.ClipTriangle(patch.Vertices[a],patch.Vertices[b],patch.Vertices[c],
                    patch.Normals[a],patch.Normals[b],patch.Normals[c],start,end,
                    patch.ClipA,patch.ClipB,patch.ClipNormalA,patch.ClipNormalB);
                for (int t=1;t<count-1;++t) for (int corner=0;corner<3;++corner) {
                    int index=corner==0?0:corner==1?t:t+1;
                    Vector3 point=patch.ClipA[index], normal=patch.ClipNormalA[index];
                    if (!Finite(point) || !Finite(normal)) return false;
                    patch.RegionVertices[written]=point; patch.RegionNormals[written]=normal;
                    minimum=Vector3.Min(minimum,point); maximum=Vector3.Max(maximum,point);
                    ++written;
                }
            }
            patch.RegionIndexCount=written;
            patch.SegmentStart=start; patch.SegmentFinish=end;
            patch.Core=(start+end)*.5f;
            minimum=Vector3.Min(minimum,Vector3.Min(start,end));
            maximum=Vector3.Max(maximum,Vector3.Max(start,end));
            return written>0;
        }
    }
}
#endif
