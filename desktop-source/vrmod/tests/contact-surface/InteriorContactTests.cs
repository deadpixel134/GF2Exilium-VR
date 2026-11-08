using GF2.Private;
using UnityEngine;

internal static class InteriorContactTests
{
    private static int checks;
    private static void Check(bool result,string message)
    {
        ++checks;
        if (!result) throw new Exception(message);
    }

    internal static int Run()
    {
        // Duplicated vertices reproduce an open renderer/UV seam along the
        // diagonal. Each triangle used to report this seam as an open edge.
        Vector3[] vertices={new(0,0,0),new(1,0,0),new(0,1,0),new(1,0,0),new(1,1,0),new(0,1,0)};
        Vector3[] normals=Enumerable.Repeat(Vector3.forward,6).ToArray();
        int[] triangles={0,1,2,3,4,5};
        Vector3 core=new(.5f,.5f,-.5f), minimum=new(0,0,-.5f), maximum=new(1,1,0);
        bool Sample(Vector3 hand,out float penetration) => ContactPatchMath.Sample(
            vertices,normals,triangles,core,minimum,maximum,hand,.035f,out _,out _,out penetration);

        for (int step=0;step<=50;++step) {
            float depth=step*.01f;
            Check(Sample(new(.5f,.5f,-depth),out float penetration) &&
                Math.Abs(penetration-(.035f+depth))<1e-5f,"hold through open seam to tissue core");
        }
        Check(!Sample(new(.5f,.5f,.04f),out _),"outward separation releases");
        Check(!Sample(new(.01f,.5f,-.3f),out _),"inside AABB but outside finite side releases");
        Check(!Sample(new(.5f,.5f,-.6f),out _),"behind bounded tissue core releases");
        Check(Sample(new(.5f,.5f,.01f),out _),"reentry after full separation");
        Check(!Sample(new(float.NaN,0,0),out _),"tracking invalid input rejected");

        // An inner clothing layer can be nearer than the exterior. Its positive
        // nearest-face distance must not cancel containment by the outer layer.
        vertices=vertices.Concat(vertices.Select(v=>v+new Vector3(0,0,-.2f))).ToArray();
        normals=Enumerable.Repeat(Vector3.forward,12).ToArray();
        triangles=triangles.Concat(triangles.Select(v=>v+6)).ToArray();
        Check(Sample(new(.5f,.5f,-.15f),out float layered) && Math.Abs(layered-.085f)<1e-5f,
            "outer volume holds beyond nearer inner clothing layer");
        Check(!Sample(new(.5f,.5f,.04f),out _),"layered exterior onset unchanged");
        core=new(float.NaN,0,0);
        Check(!Sample(new(.5f,.5f,-.1f),out _),"invalid core rejected");

        // Volume membership must agree under winding changes, mirroring,
        // translation and scale. Include points beside the finite cone to
        // distinguish containment from merely signing every nearest surface.
        foreach (float scale in new[]{.01f,.5f,1f,10f})
        foreach (float mirror in new[]{-1f,1f})
        foreach (bool reversed in new[]{false,true}) {
            Vector3 Transform(Vector3 p) => new(p.x*scale*mirror+2,p.y*scale-3,p.z*scale+1);
            Vector3 a=Transform(new(0,0,0)), b=Transform(new(1,0,0)), c=Transform(new(0,1,0));
            Vector3[] face={a,b,c}, outward={Vector3.forward,Vector3.forward,Vector3.forward};
            int[] indices=reversed?new[]{2,1,0}:new[]{0,1,2};
            Vector3 center=Transform(new(.25f,.25f,-.5f));
            Vector3 min=Vector3.Min(Vector3.Min(a,b),Vector3.Min(c,center));
            Vector3 max=Vector3.Max(Vector3.Max(a,b),Vector3.Max(c,center));
            for (int step=0;step<=20;++step) {
                float t=step/20f;
                Vector3 hand=Transform(new(.25f,.25f,-.5f*t));
                Check(ContactPatchMath.Sample(face,outward,indices,center,min,max,hand,.005f*scale,
                    out _,out var normal,out float depth) && depth>0 && normal.z>.99f,"transformed inward hold");
            }
            Check(!ContactPatchMath.Sample(face,outward,indices,center,min,max,
                Transform(new(.01f,.25f,-.3f)),.005f*scale,out _,out _,out _),"transformed side exit");
        }
        return checks;
    }
}
