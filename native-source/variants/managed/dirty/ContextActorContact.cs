#if GF2VR_TOUCH_PRIVATE
using System;
using UnityEngine;

namespace GF2.Private
{
    public static partial class ControllerDisplay
    {
        // Ownership comes from the native-admitted source. Each bound target
        // separately proves its active Animator, parent, renderer, mesh and bone.
        // No character is selected by helper ancestry or distance ranking.
        private static Context SelectContextActor(Camera source)
        {
            return source != null && source.gameObject.activeInHierarchy
                ? new Context { Kind = 5, Root = source.gameObject, OwnerCamera = source }
                : null;
        }

        // Kept only for existing legacy Kind4 predicates; new source admission
        // produces Kind5 and does not depend on an arbitrary anchor bone.
        private static bool ContextPoint(Camera source, Transform bone, out float distanceSquared)
        {
            distanceSquared = 0f;
            Vector3 p = bone.position, camera = source.transform.position;
            float near = source.nearClipPlane, far = source.farClipPlane;
            if (!Finite(p) || !Finite(camera) || !Finite(near) || !Finite(far) || near < 0f || far <= near) return false;
            float x = p.x - camera.x, y = p.y - camera.y, z = p.z - camera.z;
            distanceSquared = x * x + y * y + z * z;
            if (!Finite(distanceSquared)) return false;
            Vector3 view = source.WorldToViewportPoint(p);
            return Finite(view) && view.z > 0f && view.z >= near && view.z <= far &&
                view.x >= 0f && view.x <= 1f && view.y >= 0f && view.y <= 1f;
        }

        private static bool ContextActorEligible(Context value, Camera source)
        {
            if (value.OwnerCamera != source || !Live(value.ContextActor) || value.Root != value.ContextActor.gameObject ||
                value.Root.transform.parent != value.ActorParent) return false;
            float distance;
            return value.AnchorBone != null && value.AnchorBone.gameObject.activeInHierarchy &&
                value.AnchorBone.IsChildOf(value.ContextActor.transform) && value.AnchorSkin != null &&
                value.AnchorSkin.enabled && value.AnchorSkin.gameObject.activeInHierarchy &&
                value.AnchorSkin.sharedMesh == value.AnchorMesh && value.AnchorMesh != null &&
                value.AnchorSkin.GetComponentInParent<Animator>() == value.ContextActor &&
                (source.cullingMask & (1 << value.AnchorSkin.gameObject.layer)) != 0 &&
                ContextPoint(source, value.AnchorBone, out distance);
        }

        private static bool ContactPointInView(Camera camera, Vector3 point, int layer)
        {
            if (camera == null || !Finite(point) || (camera.cullingMask & (1 << layer)) == 0) return false;
            float near = camera.nearClipPlane, far = camera.farClipPlane;
            if (!Finite(near) || !Finite(far) || near < 0f || far <= near) return false;
            Vector3 view = camera.WorldToViewportPoint(point);
            return Finite(view) && view.z >= near && view.z > 0f && view.z <= far &&
                view.x >= 0f && view.x <= 1f && view.y >= 0f && view.y <= 1f;
        }

        // The most recently drawn eye pair belongs to this prepared generation;
        // Clear() resets the pair table. A head/navigation move need not match
        // the original authored camera frustum. Before the first valid Draw,
        // admission falls back to that source, then refreshes on the cold path.
        private static bool ContactVisiblePoint(Vector3 point, int layer)
        {
            int pair = -1;
            long ticket = -1;
            for (int i = 0; i < BuildContract.EyeCapacity; ++i)
                if (lastPair[i] > ticket && leftEyes[i] != null && rightEyes[i] != null)
                { pair = i; ticket = lastPair[i]; }
            if (pair >= 0)
                return ContactPointInView(leftEyes[pair], point, layer) ||
                    ContactPointInView(rightEyes[pair], point, layer);
            return ContactPointInView(preparedSource, point, layer);
        }
    }
}
#endif
