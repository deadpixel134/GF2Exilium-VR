#if GF2VR_TOUCH_PRIVATE && GF2VR_DIRTY_CONTACT
using UnityEngine;

namespace GF2.Private
{
    internal static class ContactRegionMath
    {
        // Two parallel clipping planes turn a triangle into at most five vertices.
        // Caller-owned six-element buffers keep the frame path allocation-free.
        internal static int ClipTriangle(Vector3 a, Vector3 b, Vector3 c,
            Vector3 na, Vector3 nb, Vector3 nc, Vector3 start, Vector3 end,
            Vector3[] first, Vector3[] second, Vector3[] normalFirst, Vector3[] normalSecond)
        {
            Vector3 axis=end-start;
            float length=axis.sqrMagnitude;
            if (!ContactMotion.Finite(start) || !ContactMotion.Finite(end) || !ContactMotion.Finite(length) || length<=1e-12f) return 0;
            first[0]=a; first[1]=b; first[2]=c;
            normalFirst[0]=na; normalFirst[1]=nb; normalFirst[2]=nc;
            int count=ClipPlane(first,normalFirst,3,second,normalSecond,start,axis,0,false);
            return ClipPlane(second,normalSecond,count,first,normalFirst,start,axis,length,true);
        }

        private static int ClipPlane(Vector3[] input, Vector3[] normals, int count,
            Vector3[] output, Vector3[] outputNormals, Vector3 start, Vector3 axis, float limit, bool upper)
        {
            if (count==0) return 0;
            int written=0;
            Vector3 previous=input[count-1], previousNormal=normals[count-1];
            float previousDistance=Vector3.Dot(previous-start,axis)-limit;
            if (upper) previousDistance=-previousDistance;
            for (int i=0;i<count;++i) {
                Vector3 current=input[i], normal=normals[i];
                float distance=Vector3.Dot(current-start,axis)-limit;
                if (upper) distance=-distance;
                bool inside=distance>=0, previousInside=previousDistance>=0;
                if (inside!=previousInside) {
                    float fraction=previousDistance/(previousDistance-distance);
                    output[written]=previous+(current-previous)*fraction;
                    outputNormals[written++]=previousNormal+(normal-previousNormal)*fraction;
                }
                if (inside) { output[written]=current; outputNormals[written++]=normal; }
                previous=current; previousNormal=normal; previousDistance=distance;
            }
            return written;
        }
    }
}
#endif
