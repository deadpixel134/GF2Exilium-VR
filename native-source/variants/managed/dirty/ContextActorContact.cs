#if GF2VR_TOUCH_PRIVATE
using System;
using UnityEngine;

namespace GF2.Private
{
    public static partial class ControllerDisplay
    {
        // Source frustum in game world units; no scene, character, camera-name
        // whitelist or arbitrary 32-world-unit range. Projection is not occlusion.
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

        // Optional fallback, cold at source admission and >=1s Prepare retry.
        // Bind only actual configured skin bones belonging to the nearest live
        // Animator. A scene name or hierarchy never grants contact ownership.
        private static Context SelectContextActor(Camera source)
        {
            try
            {
                if (source == null || !source.gameObject.activeInHierarchy) return null;
                UnityEngine.Object[] objects = UnityEngine.Object.FindObjectsOfType(typeof(SkinnedMeshRenderer));
                if (objects == null || objects.Length > 2048) return null;
                Context[] candidates = new Context[16];
                float[] distances = new float[16];
                int[] renderersPerActor = new int[16];
                int count = 0, totalBones = 0;
                for (int i = 0; i < objects.Length; ++i)
                {
                    SkinnedMeshRenderer skin = objects[i] as SkinnedMeshRenderer;
                    if (skin == null || !skin.enabled || !skin.gameObject.activeInHierarchy || skin.sharedMesh == null ||
                        (source.cullingMask & (1 << skin.gameObject.layer)) == 0) continue;
                    Animator actor = skin.GetComponentInParent<Animator>();
                    if (!Live(actor)) continue;
                    Transform[] bones = skin.bones;
                    if (bones == null) continue;
                    if (bones.Length > 2048 || (totalBones += bones.Length) > 65536) return null;
                    Transform anchor = null;
                    float closest = Single.MaxValue;
                    for (int j = 0; j < bones.Length; ++j)
                    {
                        Transform bone = bones[j];
                        float distance;
                        if (bone == null || bone.parent == null || !bone.gameObject.activeInHierarchy ||
                            !bone.IsChildOf(actor.transform) || !ContactName(bone.name) ||
                            !ContextPoint(source, bone, out distance)) continue;
                        if (anchor == null || distance < closest) { anchor = bone; closest = distance; }
                    }
                    if (anchor == null) continue;
                    int index = -1;
                    for (int j = 0; j < count; ++j) if (candidates[j].ContextActor == actor) { index = j; break; }
                    if (index < 0)
                    {
                        if (count == candidates.Length) return null;
                        index = count++;
                        candidates[index] = new Context { Kind = 4, Root = actor.gameObject, OwnerCamera = source,
                            ContextActor = actor, ActorParent = actor.transform.parent };
                        distances[index] = Single.MaxValue;
                    }
                    if (++renderersPerActor[index] > 64) return null;
                    if (closest < distances[index])
                    {
                        distances[index] = closest;
                        candidates[index].AnchorBone = anchor; candidates[index].AnchorSkin = skin;
                        candidates[index].AnchorMesh = skin.sharedMesh;
                    }
                }
                int best = -1;
                float first = Single.MaxValue, second = Single.MaxValue;
                for (int i = 0; i < count; ++i)
                {
                    float distance = distances[i];
                    if (distance < first) { second = first; first = distance; best = i; }
                    else if (distance < second) second = distance;
                }
                // Near ties fail closed; a confidently nearest actor may be
                // chosen even with additional visible characters (R-P3-7).
                if (best < 0 || (count > 1 && second - first <= Math.Max(.000001f, first * .01f))) return null;
                Context selected = candidates[best];
                return ContextActorEligible(selected, source) ? selected : null;
            }
            catch (Exception) { return null; } // optional contact failure preserves accepted display
        }
    }
}
#endif
