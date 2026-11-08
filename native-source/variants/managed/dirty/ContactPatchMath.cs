#if GF2VR_TOUCH_PRIVATE && GF2VR_DIRTY_CONTACT
using System;
using UnityEngine;

namespace GF2.Private
{
    internal static class ContactPatchMath
    {
        private static void Accumulate(ref Matrix4x4 output, Matrix4x4 matrix, float weight)
        {
            for (int row=0;row<3;++row) for (int column=0;column<4;++column)
                output[row,column]+=matrix[row,column]*weight;
        }

        private static Matrix4x4 Blend(Matrix4x4[] matrices, BoneWeight weight)
        {
            Matrix4x4 result=new Matrix4x4();
            if (weight.weight0>0) Accumulate(ref result,matrices[weight.boneIndex0],weight.weight0);
            if (weight.weight1>0) Accumulate(ref result,matrices[weight.boneIndex1],weight.weight1);
            if (weight.weight2>0) Accumulate(ref result,matrices[weight.boneIndex2],weight.weight2);
            if (weight.weight3>0) Accumulate(ref result,matrices[weight.boneIndex3],weight.weight3);
            // SkinPoint uses affine point multiplication without homogeneous
            // division, even when imported weights have small rounding error.
            result[3,3]=1;
            return result;
        }

        private static bool InvertStable(Matrix4x4 matrix, out Matrix4x4 inverse)
        {
            inverse=new Matrix4x4();
            float determinant=matrix.determinant;
            if (!ContactMotion.Finite(determinant) || determinant==0) return false;
            inverse=matrix.inverse;
            float matrixNorm=0, inverseNorm=0;
            for (int row=0;row<3;++row) {
                float sum=0, inverseSum=0;
                for (int column=0;column<4;++column) {
                    if (!ContactMotion.Finite(matrix[row,column]) || !ContactMotion.Finite(inverse[row,column])) return false;
                    if (column<3) { sum+=Math.Abs(matrix[row,column]); inverseSum+=Math.Abs(inverse[row,column]); }
                }
                matrixNorm=Math.Max(matrixNorm,sum); inverseNorm=Math.Max(inverseNorm,inverseSum);
            }
            return ContactMotion.Finite(matrixNorm*inverseNorm) && matrixNorm*inverseNorm<=10000;
        }

        internal static bool InverseSkin(Matrix4x4[] matrices, BoneWeight weight, out Matrix4x4 inverse)
        { return InvertStable(Blend(matrices,weight),out inverse); }

        // Invert the complete weighted skin transform, not each bone separately.
        // This reconstructs rest geometry from a readable current BakeMesh even
        // when the original mesh has discarded its CPU vertex/index buffers.
        internal static bool RecoverRest(Matrix4x4[] matrices, Matrix4x4[] normalMatrices, BoneWeight weight,
            Vector3 worldPoint, Vector3 worldNormal, out Vector3 point, out Vector3 normal, out float error)
        {
            point=normal=Vector3.zero; error=0;
            Matrix4x4 inversePoint, inverseNormal;
            if (!ContactMotion.Finite(worldPoint) || !ContactMotion.Finite(worldNormal) || worldNormal.sqrMagnitude==0 ||
                !InvertStable(Blend(matrices,weight),out inversePoint) ||
                !InvertStable(Blend(normalMatrices,weight),out inverseNormal)) return false;
            point=inversePoint.MultiplyPoint3x4(worldPoint);
            normal=inverseNormal.MultiplyVector(worldNormal);
            if (!ContactMotion.Finite(point) || !ContactMotion.Finite(normal) || normal.sqrMagnitude==0) return false;
            error=(SkinPoint(matrices,weight,point)-worldPoint).magnitude;
            return ContactMotion.Finite(error);
        }

        // Closest point and the triangle edges containing it. Bit 1 is AB,
        // bit 2 is BC, bit 4 is CA. Interior points have no edge bits.
        internal static Vector3 Closest(Vector3 p, Vector3 a, Vector3 b, Vector3 c, out int edges)
        {
            Vector3 ab=b-a, ac=c-a, ap=p-a;
            float d1=Vector3.Dot(ab,ap), d2=Vector3.Dot(ac,ap);
            if (d1<=0 && d2<=0) { edges=5; return a; }
            Vector3 bp=p-b;
            float d3=Vector3.Dot(ab,bp), d4=Vector3.Dot(ac,bp);
            if (d3>=0 && d4<=d3) { edges=3; return b; }
            float vc=d1*d4-d3*d2;
            if (vc<=0 && d1>=0 && d3<=0) { edges=1; return a+ab*(d1/(d1-d3)); }
            Vector3 cp=p-c;
            float d5=Vector3.Dot(ab,cp), d6=Vector3.Dot(ac,cp);
            if (d6>=0 && d5<=d6) { edges=6; return c; }
            float vb=d5*d2-d1*d6;
            if (vb<=0 && d2>=0 && d6<=0) { edges=4; return a+ac*(d2/(d2-d6)); }
            float va=d3*d6-d5*d4;
            if (va<=0 && d4-d3>=0 && d5-d6>=0) {
                edges=2; return b+(c-b)*((d4-d3)/((d4-d3)+(d5-d6)));
            }
            float divisor=va+vb+vc;
            edges=0;
            return a+ab*(vb/divisor)+ac*(vc/divisor);
        }

        // Close each finite surface triangle toward the unmodified tissue core.
        // Membership is independent of mesh seams, winding and which surface
        // layer happens to contain the nearest point.
        private static bool Contains(Vector3 a, Vector3 b, Vector3 c, Vector3 hand, Vector3 core)
        {
            a-=core; b-=core; c-=core; hand-=core;
            Vector3 bc=Vector3.Cross(b,c);
            float volume=Vector3.Dot(a,bc);
            double scale=(double)a.sqrMagnitude*b.sqrMagnitude*c.sqrMagnitude;
            if (!ContactMotion.Finite(volume) || volume==0 ||
                (double)volume*volume<=scale*1e-12) return false;
            float first=Vector3.Dot(hand,bc)/volume;
            float second=Vector3.Dot(a,Vector3.Cross(hand,c))/volume;
            float third=Vector3.Dot(a,Vector3.Cross(b,hand))/volume;
            const float tolerance=1e-6f;
            return ContactMotion.Finite(first) && ContactMotion.Finite(second) && ContactMotion.Finite(third) &&
                first>=-tolerance && second>=-tolerance && third>=-tolerance && first+second+third<=1+tolerance;
        }

        internal static bool Sample(Vector3[] vertices, Vector3[] normals, int[] triangles,
            Vector3 core, Vector3 minimum, Vector3 maximum, Vector3 hand, float handRadius,
            out Vector3 point, out Vector3 normal, out float penetration)
        {
            return Sample(vertices,normals,triangles,triangles.Length,core,minimum,maximum,hand,handRadius,out point,out normal,out penetration);
        }

        internal static bool Sample(Vector3[] vertices, Vector3[] normals, int[] triangles, int indexCount,
            Vector3 core, Vector3 minimum, Vector3 maximum, Vector3 hand, float handRadius,
            out Vector3 point, out Vector3 normal, out float penetration)
        {
            return SampleVolume(vertices,normals,triangles,indexCount,core,core,false,minimum,maximum,hand,handRadius,out point,out normal,out penetration);
        }

        internal static bool SampleSegment(Vector3[] vertices, Vector3[] normals, int[] triangles, int indexCount,
            Vector3 start, Vector3 end, Vector3 minimum, Vector3 maximum, Vector3 hand, float handRadius,
            out Vector3 point, out Vector3 normal, out float penetration)
        {
            return SampleVolume(vertices,normals,triangles,indexCount,start,end,true,minimum,maximum,hand,handRadius,out point,out normal,out penetration);
        }

        private static bool SampleVolume(Vector3[] vertices, Vector3[] normals, int[] triangles, int indexCount,
            Vector3 core, Vector3 end, bool segment, Vector3 minimum, Vector3 maximum, Vector3 hand, float handRadius,
            out Vector3 point, out Vector3 normal, out float penetration)
        {
            point=normal=Vector3.zero; penetration=0;
            if (!ContactMotion.Finite(hand) || !ContactMotion.Finite(core) || (segment && !ContactMotion.Finite(end)) || !ContactMotion.Finite(handRadius) || handRadius<0 ||
                hand.x<minimum.x-handRadius || hand.y<minimum.y-handRadius || hand.z<minimum.z-handRadius ||
                hand.x>maximum.x+handRadius || hand.y>maximum.y+handRadius || hand.z>maximum.z+handRadius) return false;
            float nearest=Single.PositiveInfinity;
            bool inside=false;
            for (int i=0;i<indexCount;i+=3) {
                int ia=triangles[i], ib=triangles[i+1], ic=triangles[i+2];
                Vector3 a=vertices[ia], b=vertices[ib], c=vertices[ic];
                Vector3 cross=Vector3.Cross(b-a,c-a);
                float length=cross.magnitude;
                if (!ContactMotion.Finite(length) || length<=1e-12f) continue;
                if (!inside) inside=Contains(a,b,c,hand,core);
                // The convex hull of a surface triangle and a finite bone
                // segment is the union of these five tetrahedra. It retains
                // deep contact along the thigh axis without an infinite cone.
                if (!inside && segment) inside=Contains(a,b,c,hand,end) || Contains(a,b,core,hand,end) ||
                    Contains(a,c,core,hand,end) || Contains(b,c,core,hand,end);
                int edges;
                Vector3 candidate=Closest(hand,a,b,c,out edges);
                float squared=(hand-candidate).sqrMagnitude;
                if (!ContactMotion.Finite(squared) || squared>=nearest) continue;
                Vector3 outward=cross/length;
                if (Vector3.Dot(outward,normals[ia]+normals[ib]+normals[ic])<0) outward=outward*-1;
                nearest=squared; point=candidate; normal=outward;
            }
            if (!ContactMotion.Finite(nearest)) return false;
            float distance=(float)Math.Sqrt(nearest);
            // A nearby open seam or inner clothing layer must not turn a
            // contained hand into separation. Outside the closed finite volume,
            // use ordinary distance so sides and the rear remain bounded.
            if (inside) distance=-distance;
            penetration=handRadius-distance;
            return ContactMotion.Finite(penetration) && penetration>0;
        }

        internal static Vector3 SkinPoint(Matrix4x4[] matrices, BoneWeight weight, Vector3 point)
        {
            Vector3 result=Vector3.zero;
            if (weight.weight0>0) result+=matrices[weight.boneIndex0].MultiplyPoint3x4(point)*weight.weight0;
            if (weight.weight1>0) result+=matrices[weight.boneIndex1].MultiplyPoint3x4(point)*weight.weight1;
            if (weight.weight2>0) result+=matrices[weight.boneIndex2].MultiplyPoint3x4(point)*weight.weight2;
            if (weight.weight3>0) result+=matrices[weight.boneIndex3].MultiplyPoint3x4(point)*weight.weight3;
            return result;
        }

        internal static Vector3 SkinNormal(Matrix4x4[] matrices, BoneWeight weight, Vector3 normal)
        {
            Vector3 result=Vector3.zero;
            if (weight.weight0>0) result+=matrices[weight.boneIndex0].MultiplyVector(normal)*weight.weight0;
            if (weight.weight1>0) result+=matrices[weight.boneIndex1].MultiplyVector(normal)*weight.weight1;
            if (weight.weight2>0) result+=matrices[weight.boneIndex2].MultiplyVector(normal)*weight.weight2;
            if (weight.weight3>0) result+=matrices[weight.boneIndex3].MultiplyVector(normal)*weight.weight3;
            return result;
        }
    }
}
#endif
