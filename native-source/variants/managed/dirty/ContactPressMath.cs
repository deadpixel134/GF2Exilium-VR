#if GF2VR_TOUCH_PRIVATE && GF2VR_DIRTY_CONTACT
using System;
using UnityEngine;
namespace GF2.Private
{
    internal static class ContactPressMath
    {
        internal static Vector3 Goal(Vector3 vertex, Vector3 normal, Vector3 point, Vector3 outward,
            float penetration, float radius, float maximum)
        {
            if (!ContactMotion.Finite(vertex) || !ContactMotion.Finite(normal) || !ContactMotion.Finite(point) ||
                !ContactMotion.Finite(outward) || !ContactMotion.Finite(penetration) ||
                !ContactMotion.Finite(radius) || !ContactMotion.Finite(maximum) || radius<=0 || maximum<=0 || penetration<=0) return Vector3.zero;
            float distance=(vertex-point).sqrMagnitude/(radius*radius);
            if (!ContactMotion.Finite(distance) || distance>=1 || Vector3.Dot(normal,outward)<=0) return Vector3.zero;
            float falloff=1-distance;
            float depth=(float)((double)maximum*penetration/((double)maximum+penetration));
            return outward*(-depth*falloff*falloff);
        }

        internal static Vector3 Relax(Vector3 current, Vector3 goal, float dt, float unit)
        {
            Vector3 result=current+(goal-current)*(float)(1-Math.Exp(-45*dt));
            if (goal.sqrMagnitude==0 && result.sqrMagnitude<1e-14f*unit*unit) return Vector3.zero;
            return result;
        }
    }
}
#endif
