using GF2.Private;
using UnityEngine;

internal static class RestRecoveryTests
{
    private static int checks;
    private static void Check(bool value,string message) { ++checks; if(!value) throw new Exception(message); }
    private static bool Near(Vector3 a,Vector3 b,float tolerance=.0001f)=>(a-b).magnitude<tolerance;
    internal static int Run()
    {
        var random=new Random(1255352);
        for(int run=0;run<300;++run) {
            var skin=new Matrix4x4[4];var normals=new Matrix4x4[4];var future=new Matrix4x4[4];
            for(int i=0;i<4;++i) {
                float angle=(float)(random.NextDouble()*.8-.4);
                float x=(float)(random.NextDouble()*.5-.25),y=(float)(random.NextDouble()*.5-.25);
                var scale=Matrix4x4.Scale(.5f+(float)random.NextDouble(),.5f+(float)random.NextDouble(),.5f+(float)random.NextDouble());
                skin[i]=Matrix4x4.Translate(x,y,.1f*i)*Matrix4x4.RotateZ(angle)*scale;
                normals[i]=skin[i].inverse.transpose;
                future[i]=Matrix4x4.Translate(x+.3f,y-.2f,0)*Matrix4x4.RotateZ(angle+.7f)*scale;
            }
            var weight=new BoneWeight {boneIndex0=0,boneIndex1=1,boneIndex2=2,boneIndex3=3,weight0=.5f,weight1=.2f,weight2=.2f,weight3=.1f};
            var rest=new Vector3((float)random.NextDouble(),(float)random.NextDouble(),(float)random.NextDouble());
            var restNormal=new Vector3(.2f,.3f,.9f);
            Vector3 bakedPoint=ContactPatchMath.SkinPoint(skin,weight,rest);
            Vector3 bakedNormal=ContactPatchMath.SkinNormal(normals,weight,restNormal);
            Check(ContactPatchMath.RecoverRest(skin,normals,weight,bakedPoint,bakedNormal,out var recovered,out var recoveredNormal,out float error),"recover from current baked pose");
            Check(Near(recovered,rest)&&Near(recoveredNormal,restNormal)&&error<.0001f,"matches independent known rest geometry");
            Check(Near(ContactPatchMath.SkinPoint(future,weight,recovered),ContactPatchMath.SkinPoint(future,weight,rest)),"different animation pose follows original ground truth");
        }
        var one=new BoneWeight {boneIndex0=0,weight0=1,boneIndex1=999,boneIndex2=-1,boneIndex3=999};
        var reflected=new[]{Matrix4x4.Translate(1,2,3)*Matrix4x4.Scale(-2,3,4)};
        var normalMatrices=new[]{reflected[0].inverse.transpose};
        var expected=new Vector3(.1f,.2f,.3f);var expectedNormal=Vector3.forward;
        Check(ContactPatchMath.RecoverRest(reflected,normalMatrices,one,ContactPatchMath.SkinPoint(reflected,one,expected),ContactPatchMath.SkinNormal(normalMatrices,one,expectedNormal),out var point,out var normal,out _)&&Near(point,expected)&&Near(normal,expectedNormal),"mirrored nonuniform and zero-weight invalid slots");
        var rounded=new BoneWeight {boneIndex0=0,weight0=.9995f};
        Check(ContactPatchMath.RecoverRest(reflected,normalMatrices,rounded,ContactPatchMath.SkinPoint(reflected,rounded,expected),ContactPatchMath.SkinNormal(normalMatrices,rounded,expectedNormal),out point,out _,out _)&&Near(point,expected),"rounding in imported weight sums does not add homogeneous division");
        var collapsed=new[]{Matrix4x4.Scale(0,1,1)};
        Check(!ContactPatchMath.RecoverRest(collapsed,normalMatrices,one,new(0,1,1),Vector3.up,out _,out _,out _),"singular transform rejects recovery");
        var cancelled=new[]{Matrix4x4.Scale(1,1,1),Matrix4x4.Scale(-1,-1,1)};
        var half=new BoneWeight {boneIndex0=0,boneIndex1=1,weight0=.5f,weight1=.5f};
        Check(!ContactPatchMath.RecoverRest(cancelled,cancelled,half,new(0,0,1),Vector3.forward,out _,out _,out _),"singular weighted mixture rejected even though individual bones invert");
        var illConditioned=new[]{Matrix4x4.Scale(.0000001f,1,1)};
        Check(!ContactPatchMath.RecoverRest(illConditioned,normalMatrices,one,expected,Vector3.up,out _,out _,out _),"ill-conditioned reconstruction rejected");
        Check(!ContactPatchMath.RecoverRest(reflected,normalMatrices,one,new(float.NaN,0,0),Vector3.up,out _,out _,out _),"nonfinite sample rejected");
        Check(!ContactPatchMath.RecoverRest(reflected,normalMatrices,one,expected,Vector3.zero,out _,out _,out _),"empty surface normal rejected");
        return checks;
    }
}
