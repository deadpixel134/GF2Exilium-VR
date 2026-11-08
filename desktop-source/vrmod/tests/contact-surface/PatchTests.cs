using GF2.Private;
using UnityEngine;

internal static class PatchTests
{
    private static int checks;
    private static void Check(bool result,string message) { ++checks; if(!result) throw new Exception(message); }
    private static bool Near(Vector3 a,Vector3 b) => (a-b).magnitude<1e-5f;

    internal static int Run()
    {
        Vector3 a=new(0,0,0), b=new(1,0,0), c=new(0,1,0);
        foreach(var test in new[] {
            (new Vector3(.2f,.3f,1),new Vector3(.2f,.3f,0),0),
            (new Vector3(-1,-1,1),a,5), (new Vector3(2,-1,1),b,3),
            (new Vector3(-1,2,1),c,6), (new Vector3(.5f,-1,1),new Vector3(.5f,0,0),1),
            (new Vector3(-1,.5f,1),new Vector3(0,.5f,0),4),
            (new Vector3(1,1,1),new Vector3(.5f,.5f,0),2)
        }) {
            Vector3 closest=ContactPatchMath.Closest(test.Item1,a,b,c,out int edge);
            Check(Near(closest,test.Item2) && edge==test.Item3,"closest-point Voronoi region");
        }
        Vector3[] vertices={a,b,c}; Vector3[] normals={Vector3.forward,Vector3.forward,Vector3.forward};
        int[] indices={0,1,2};
        Vector3 core=new(.25f,.25f,-.12f);
        Vector3 minimum=new(0,0,-.12f),maximum=new(1,1,0);
        bool Sample(Vector3 hand,out Vector3 point,out Vector3 normal,out float depth) =>
            ContactPatchMath.Sample(vertices,normals,indices,core,minimum,maximum,hand,.035f,out point,out normal,out depth);
        Check(Sample(new(.2f,.2f,.02f),out var point,out var normal,out float depth) && Math.Abs(depth-.015f)<1e-6f && Near(point,new(.2f,.2f,0)) && Near(normal,Vector3.forward),"outer surface entry and inward force normal");
        Check(!Sample(new(.2f,.2f,.04f),out _,out _,out _),"separation beyond visible hand radius");
        Check(Sample(new(.2f,.2f,-.08f),out _,out _,out depth)&&Math.Abs(depth-.115f)<1e-6f,"deep tissue contact to bounded core");
        Check(!Sample(new(-.02f,.5f,-.08f),out _,out _,out _),"open boundary does not extend inward halfspace");
        Check(!Sample(new(.2f,.2f,-1f),out _,out _,out _),"outside tissue volume");
        Check(!Sample(new(float.NaN,0,0),out _,out _,out _),"invalid hand rejected");
        // Reverse winding while retaining outward normals, including a mirrored
        // world triangle. The actual outward pressure direction must survive.
        indices=new[]{2,1,0};
        Check(Sample(new(.2f,.2f,.02f),out _,out normal,out _)&&Near(normal,Vector3.forward),"winding correction");
        vertices=new[]{new Vector3(0,0,0),new Vector3(-2,0,0),new Vector3(0,3,0)};
        minimum=new(-2,0,-.12f); maximum=new(0,3,0); core=new(-.5f,.75f,-.12f); indices=new[]{0,1,2};
        Check(Sample(new(-.2f,.3f,.02f),out point,out normal,out depth)&&Near(normal,Vector3.forward)&&Math.Abs(depth-.015f)<1e-6f,"mirrored nonuniform world-space distance");
        vertices=new[]{a,a,a};
        Check(!Sample(new(0,0,.01f),out _,out _,out _),"degenerate triangle has no contact");

        var matrices=new[]{Matrix4x4.Translate(1,0,0),Matrix4x4.Translate(0,2,0),Matrix4x4.Translate(0,0,3),Matrix4x4.Translate(-1,0,0)};
        var weights=new BoneWeight {boneIndex0=0,boneIndex1=1,boneIndex2=2,boneIndex3=3,weight0=.1f,weight1=.2f,weight2=.3f,weight3=.4f};
        Check(Near(ContactPatchMath.SkinPoint(matrices,weights,new(4,5,6)),new(3.7f,5.4f,6.9f)),"actual four-weight skin point");
        Check(Near(ContactPatchMath.SkinNormal(matrices,weights,Vector3.up),Vector3.up),"normal excludes translation");
        weights=new BoneWeight {boneIndex0=0,weight0=1,boneIndex1=999,boneIndex2=-1,boneIndex3=999};
        Check(Near(ContactPatchMath.SkinPoint(matrices,weights,a),new(1,0,0)),"zero-weight slots never read invalid bones");

        // A curved surface whose residual exceeds the old sphere guard still
        // has a direct triangle-level contact point at its authored surface.
        vertices=new[]{new Vector3(0,0,.09f),new Vector3(.04f,0,.07f),new Vector3(0,.03f,.08f)};
        normals=new[]{Vector3.forward,Vector3.forward,Vector3.forward};indices=new[]{0,1,2};
        point=(vertices[0]+vertices[1]+vertices[2])/3;
        normal=Vector3.Cross(vertices[1]-vertices[0],vertices[2]-vertices[0]); normal=normal/normal.magnitude;
        if(normal.z<0) normal=normal*-1;
        var hand=point+normal*.01f;
        Check(ContactPatchMath.Sample(vertices,normals,indices,new(.01f,.01f,0),new(0,0,0),new(.04f,.03f,.09f),hand,.02f,out var picked,out _,out depth)&&Near(picked,point)&&Math.Abs(depth-.01f)<1e-6f,"surface contact independent of bone-center sphere fit");
        return checks;
    }
}
