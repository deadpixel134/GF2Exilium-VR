#if GF2VR_TOUCH_PRIVATE && GF2VR_DIRTY_CONTACT
using System;
using System.Text;
using UnityEngine;

namespace GF2.Private
{
    public static partial class ControllerDisplay
    {
        private static void ContactMarkBindingReport(bool requireFreshBinding=true)
        {
            if (requireFreshBinding || !contactBindingReportPending) contactBindingReportFrame=Time.frameCount;
            contactBindingReportPending=true;
        }

        private static int ContactServiceStatus()
        {
            // Initial targets0 before callbacks cannot establish target absence.
            return contactBindingReportPending && contactLastFrame>contactBindingReportFrame?3:contactStatus;
        }

        private static bool ContactBindingHint(string name)
        {
            return name!=null && (name.StartsWith("Chest",StringComparison.OrdinalIgnoreCase) ||
                name.StartsWith("Butt",StringComparison.OrdinalIgnoreCase) ||
                name.StartsWith("Breast",StringComparison.OrdinalIgnoreCase) ||
                name.StartsWith("Bust",StringComparison.OrdinalIgnoreCase) ||
                name=="Root_M" || name=="Hip_L" || name=="Hip_R");
        }

        // Called only for cold diagnostics. Bound output to fit the existing
        // native mailbox together with the per-target surface result records.
        private static void ContactAppendBindingEvidence(StringBuilder text)
        {
            var evidence=new StringBuilder(2048);
            var actors=new Transform[16];
            int actorCount=0;
            bool truncated=false;
            evidence.Append('[');
            try {
                if (contactSurfaceSkins!=null) foreach (SkinnedMeshRenderer skin in contactSurfaceSkins) {
                    if (skin==null || !skin.enabled || !skin.gameObject.activeInHierarchy || ContactSourceMesh(skin)==null) continue;
                    Animator animator=skin.GetComponentInParent<Animator>();
                    if (animator==null || !animator.isActiveAndEnabled || !animator.gameObject.activeInHierarchy) continue;
                    Transform actor=animator.transform;
                    bool duplicate=false;
                    for (int i=0;i<actorCount;++i) duplicate|=actors[i]==actor;
                    if (duplicate) continue;
                    if (actorCount==actors.Length) { truncated=true; break; }
                    actors[actorCount++]=actor;
                    var entry=new StringBuilder(512);
                    entry.Append("{\"actor_id\":").Append(actor.GetInstanceID()).Append(",\"actor_name\":");
                    Quoted(entry,actor.name);
                    entry.Append(",\"source_eligible\":").Append(prepared!=null &&
                        (prepared.Kind==5 || actor.IsChildOf(prepared.Root.transform)) &&
                        (prepared.Kind!=4 || animator==prepared.ContextActor)?"true":"false");
                    entry.Append(",\"candidate_bones\":[");
                    var seen=new Transform[32];
                    int boneCount=0, writtenBones=0;
                    bool boneLimit=false;
                    foreach (SkinnedMeshRenderer candidate in contactSurfaceSkins) {
                        if (candidate==null || !candidate.enabled || !candidate.gameObject.activeInHierarchy ||
                            ContactSourceMesh(candidate)==null || candidate.GetComponentInParent<Animator>()!=animator) continue;
                        Transform[] bones=candidate.bones;
                        if (bones==null || bones.Length>2048) { boneLimit=true; continue; }
                        foreach (Transform bone in bones) {
                            if (bone==null || !ContactBindingHint(bone.name)) continue;
                            duplicate=false;
                            for (int i=0;i<boneCount;++i) duplicate|=seen[i]==bone;
                            if (duplicate) continue;
                            if (boneCount==seen.Length || bone.name.Length>64) { boneLimit=true; continue; }
                            seen[boneCount++]=bone;
                            var candidateEntry=new StringBuilder(128);
                            candidateEntry.Append("{\"name\":"); Quoted(candidateEntry,bone.name);
                            candidateEntry.Append(",\"parent\":"); Quoted(candidateEntry,bone.parent==null?null:bone.parent.name);
                            candidateEntry.Append(",\"configured\":").Append(ContactName(bone.name)?"true":"false");
                            candidateEntry.Append(",\"visible\":").Append(prepared!=null &&
                                (prepared.Kind!=5 || ContactVisiblePoint(bone.position,candidate.gameObject.layer))?"true":"false");
                            candidateEntry.Append('}');
                            if (entry.Length+candidateEntry.Length+64>1800) { boneLimit=true; continue; }
                            if (writtenBones++>0) entry.Append(',');
                            entry.Append(candidateEntry);
                        }
                    }
                    entry.Append("],\"bone_limit\":").Append(boneLimit?"true":"false").Append('}');
                    if (evidence.Length+entry.Length+2>2048) { truncated=true; break; }
                    if (evidence.Length>1) evidence.Append(',');
                    evidence.Append(entry);
                }
            }
            catch (Exception) { truncated=true; }
            evidence.Append(']');
            text.Append(",\"binding_actors\":").Append(evidence);
            text.Append(",\"binding_evidence_limited\":").Append(truncated?"true":"false");
        }
    }
}
#endif
