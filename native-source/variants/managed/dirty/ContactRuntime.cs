#if GF2VR_TOUCH_PRIVATE
using System;
using System.Diagnostics;
using System.Globalization;
using System.Text;
using System.Threading;
using UnityEngine;
using UnityEngine.Events;

namespace GF2.Private
{
    public static partial class ControllerDisplay
    {
        private const int ContactCap = 64;
        private struct ContactTarget
        {
            internal Transform Bone, Parent, Actor, Child;
            internal Animator Animator;
            internal SkinnedMeshRenderer Skin;
            internal Mesh Mesh;
            internal Vector3 Base, LastWritten, Offset, Velocity;
            internal int InsideMask;
            internal long HandTicket;
            internal bool Owned, RotationOwned, ScaleOwned;
            internal Quaternion BaseRotation, LastRotation;
            internal Vector3 BaseScale, LastScale, Tilt, TiltVelocity, PressLog;
            internal ContactMaterial Material;
            internal int Alignment, PressAlignment;
        }
        private static ContactSettings contactSettings;
        private static ContactTarget[] contactTargets;
        private static int contactCount, contactLastFrame = -1, contactHandFrame = -1;
        private static int contactMask, contactStatus, contactPid;
        private static readonly ContactHapticEdges contactHaptics = new ContactHapticEdges();
        private static int contactHapticValid, contactHapticInside;
        private static bool contactConfigured, contactRunning, contactSubscribed, contactNeedsRebind, contactFault;
        private static UnityAction contactAction;
        private static Vector3 contactLeft, contactRight;
        private static Vector3 contactLeftVelocity, contactRightVelocity;
        private static int contactVelocityMask;
        private static float contactUnitScale, contactHandScale;
        private static long contactEpoch, contactGeneration, contactTicket = -1, contactHandStamp, contactStepStamp, contactServiceStamp;
        private static long contactCallbacks, contactFrames, contactWrites, contactRestores, contactExternal, contactFaults, contactBindings;
        private static long contactRotationWrites, contactScaleWrites, contactStiffnessCaps, contactImplicitSteps;
        private static long contactGapHeld, contactGapDecay, contactMaskZero, contactInvalid, contactGenerationReset, contactHardExpiry, contactDtReset, contactImpulses;

        public static void ConfigureContact(byte[] utf8, ref int status)
        {
            status = 0;
            if (!OnOwner()) { status = -1; return; }
            ClearContact();
            using (Process process = Process.GetCurrentProcess()) contactPid = process.Id;
            ContactSettings settings;
            contactConfigured = ContactSettings.Read(utf8, out settings);
            contactSettings = settings;
            contactFault = false;
            contactStatus = status = contactConfigured ? 1 : -70;
        }

        private static bool ContactOwnerValid()
        {
            return Thread.CurrentThread.ManagedThreadId == ownerThread && contactConfigured && !contactFault &&
                contactSettings != null && contactSettings.Enabled && prepared != null && preparedSource != null &&
                contactEpoch == ownerEpoch && contactGeneration == ownerGeneration && Eligible(prepared, preparedSource);
        }

        private static void BeginContact()
        {
            if (!OnOwner() || !contactConfigured || contactSettings == null || !contactSettings.Enabled) return;
            try
            {
                contactEpoch = ownerEpoch; contactGeneration = ownerGeneration;
                if (!ContactOwnerValid()) return;
                contactFault = false; contactRunning = true; contactNeedsRebind = true;
                contactLastFrame = contactHandFrame = -1; contactTicket = -1; contactMask = 0;
                contactHandStamp = contactStepStamp = contactServiceStamp = 0;
                contactVelocityMask = 0; contactLeftVelocity = contactRightVelocity = Vector3.zero;
                if (contactAction == null) contactAction = new UnityAction(ContactBeforeRender);
                if (!contactSubscribed) { Application.onBeforeRender += contactAction; contactSubscribed = true; }
                int status = 0; ServiceContact(ref status);
            }
            catch (Exception) { contactFault = true; ++contactFaults; contactStatus = -71; ClearContact(); }
        }

        // Called only on the existing main owner, never during event iteration.
        private static void ClearContact()
        {
            contactHaptics.Suspend();
            contactRunning = false; contactMask = 0;
            ContactRestoreAll();
            if (contactSubscribed)
            {
                try { Application.onBeforeRender -= contactAction; contactSubscribed = false; }
                catch (Exception) { contactFault = true; ++contactFaults; contactStatus = -72; }
            }
            if (contactTargets != null) Array.Clear(contactTargets, 0, contactTargets.Length);
            contactCount = 0; contactNeedsRebind = false;
            contactTicket = -1; contactLastFrame = contactHandFrame = -1;
            contactEpoch = contactGeneration = 0; contactStepStamp = contactHandStamp = contactServiceStamp = 0;
            contactVelocityMask = 0; contactLeftVelocity = contactRightVelocity = Vector3.zero;
        }

        private static bool ContactExact(Vector3 a, Vector3 b)
        { return a.x == b.x && a.y == b.y && a.z == b.z; }

        private static void ContactRestoreAll()
        {
            if (contactTargets == null) return;
            for (int i = 0; i < contactCount; ++i) ContactRestoreOne(i);
        }

        private static void ContactStop(int status, bool fault)
        {
            contactHaptics.Suspend();
            contactRunning = false; contactMask = 0; contactStatus = status;
            contactVelocityMask = 0; contactLeftVelocity = contactRightVelocity = Vector3.zero;
            if (fault) { contactFault = true; ++contactFaults; }
            ContactRestoreAll();
        }

        // Consume final-eye-aligned positions. Hand scale follows rendered IPD;
        // body/displacement retain the independent character-space unit scale.
        private static void CacheContactHands(Vector3 left, Vector3 right, float radius, float unit, int mask,
            long epoch, long generation, long ticket)
        {
            if (!contactRunning) return;
            try
            {
                if (epoch != contactEpoch || generation != contactGeneration || contactEpoch != ownerEpoch || contactGeneration != ownerGeneration)
                { ++contactGenerationReset; ContactStop(-73, false); return; }
                if (!ContactOwnerValid()) { ++contactInvalid; ContactStop(-73, false); return; }
                long now = Stopwatch.GetTimestamp();
                // Tracking withdrawal clears release eligibility immediately,
                // including stale tickets, but lets the existing spring decay.
                if ((mask & 3) == 0)
                {
                    contactMask = contactVelocityMask = 0;
                    contactLeftVelocity = contactRightVelocity = Vector3.zero;
                    if (ticket > contactTicket) contactTicket = ticket;
                    contactHandFrame = Time.frameCount; contactHandStamp = now;
                    contactHaptics.Suspend(); ContactForgetInside(); return;
                }
                if (ticket <= contactTicket) return;
                if (!Finite(radius) || radius <= 0f || ((mask & 1) != 0 && !Finite(left)) || ((mask & 2) != 0 && !Finite(right)))
                { ++contactInvalid; contactMask = contactVelocityMask = 0; contactHaptics.Suspend(); ContactRestoreAll(); return; }
                float handScale = radius / .035f;
                if (!Finite(unit) || unit < .00001f || unit > 100000f || !Finite(handScale) || handScale <= 0f || handScale > 100000f) { ++contactInvalid; contactMask = contactVelocityMask = 0; contactHaptics.Suspend(); ContactRestoreAll(); return; }
                double elapsed = ((double)now - contactHandStamp) / Stopwatch.Frequency;
                contactVelocityMask = 0; contactLeftVelocity = contactRightVelocity = Vector3.zero;
                // Only two consecutive, same-scale, present-hand samples can
                // produce a velocity. Measured eye-distance float noise is allowed
                // only for velocity eligibility; geometry keeps the exact hand scale.
                // A cache gap/tracking loss or material scale change is no strike.
                if (contactHandStamp != 0 && elapsed >= .000001 && elapsed <= contactSettings.HandHoldSeconds && unit == contactUnitScale && Math.Abs(handScale - contactHandScale) <= Math.Max(handScale, contactHandScale) * .001f)
                {
                    if ((mask & contactMask & 1) != 0)
                    { contactLeftVelocity = ContactSampleVelocity(left, contactLeft, elapsed, unit); contactVelocityMask |= 1; }
                    if ((mask & contactMask & 2) != 0)
                    { contactRightVelocity = ContactSampleVelocity(right, contactRight, elapsed, unit); contactVelocityMask |= 2; }
                }
                contactLeft = left; contactRight = right; contactUnitScale = unit; contactHandScale = handScale;
                contactMask = mask & 3; contactTicket = ticket; contactHandFrame = Time.frameCount;
                contactHandStamp = now;
            }
            catch (Exception) { ContactStop(-74, true); }
        }

        private static Vector3 ContactSampleVelocity(Vector3 current, Vector3 previous, double elapsed, float unit)
        {
            Vector3 velocity = (current - previous) / (float)elapsed;
            // Reject nonfinite differences and bound teleport/timestamp spikes
            // before the existing displacement-derived spring velocity clamp.
            return Finite(velocity) ? ContactClamp(velocity, 4f * unit) : Vector3.zero;
        }

        private static void ContactForgetInside()
        {
            if (contactTargets == null) return;
            for (int i = 0; i < contactCount; ++i)
            { contactTargets[i].InsideMask = 0; contactTargets[i].HandTicket = -1; }
        }

        private static bool ContactValidTarget(ContactTarget target)
        {
            return target.Bone != null && target.Parent != null && target.Actor != null && target.Animator != null &&
                target.Skin != null && target.Mesh != null && target.Bone.parent == target.Parent &&
                target.Animator.isActiveAndEnabled && target.Animator.gameObject.activeInHierarchy &&
                target.Animator.transform == target.Actor && target.Actor.IsChildOf(prepared.Root.transform) &&
                (prepared.Kind != 4 || target.Animator == prepared.ContextActor) &&
                target.Bone.IsChildOf(target.Actor) && target.Skin.transform.IsChildOf(target.Actor) &&
                target.Skin.enabled && target.Skin.gameObject.activeInHierarchy &&
                target.Skin.sharedMesh == target.Mesh && target.Bone.gameObject.activeInHierarchy;
        }

        // Caught at the event boundary because Unity's event iterator is not.
        // No subscribe/unsubscribe, search, allocation, strings or diagnostics.
        private static void ContactBeforeRender()
        {
            try
            {
                // An unexpected emitter thread must not touch Unity objects,
                // including the conditional restoration path.
                if (Thread.CurrentThread.ManagedThreadId != ownerThread) return;
                ++contactCallbacks;
                if (!contactRunning) return;
                if (contactEpoch != ownerEpoch || contactGeneration != ownerGeneration)
                { ++contactGenerationReset; ContactStop(-75, false); return; }
                if (!ContactOwnerValid()) { ++contactInvalid; ContactStop(-75, false); return; }
                int frame = Time.frameCount;
                if (frame == contactLastFrame) return;
                if (frame < 0 || (contactLastFrame >= 0 && frame < contactLastFrame)) { ContactStop(-76, true); return; }
                contactLastFrame = frame; ++contactFrames;
                long now = Stopwatch.GetTimestamp();
                double age = ((double)now - contactHandStamp) / Stopwatch.Frequency;
                if (age < 0 || contactHandFrame > frame)
                { ++contactInvalid; contactMask = contactVelocityMask = 0; contactHaptics.Suspend(); ContactRestoreAll(); contactStepStamp = now; return; }
                if (contactHandStamp == 0 || age > .250)
                { ++contactHardExpiry; contactMask = contactVelocityMask = 0; contactHaptics.Suspend(); ContactRestoreAll(); contactStepStamp = now; return; }
                bool hands = contactMask != 0 && contactHandFrame >= 0 && age <= contactSettings.HandHoldSeconds;
                if (contactMask == 0) ++contactMaskZero;
                else if (!hands) ++contactGapDecay;
                else if (frame > contactHandFrame) ++contactGapHeld;
                double dt = contactStepStamp == 0 ? 1.0 / 90 : ((double)now - contactStepStamp) / Stopwatch.Frequency;
                contactStepStamp = now;
                if (Double.IsNaN(dt) || Double.IsInfinity(dt) || dt <= 0 || dt > .250)
                { ++contactDtReset; contactHaptics.Suspend(); ContactRestoreAll(); return; }
                if (dt > .050) dt = .050;
                contactHapticInside = 0;
                contactHapticValid = hands && contactCount != 0 && !contactNeedsRebind ? contactMask : 0;
                for (int i = 0; i < contactCount; ++i)
                {
                    if (!ContactValidTarget(contactTargets[i]))
                    { ++contactInvalid; contactHapticValid = 0; ContactRestoreOne(i); contactNeedsRebind = true; continue; }
                    ContactApply(i, (float)dt, hands);
                }
                contactHaptics.Observe(contactHapticValid, contactHapticInside, now);
                contactStatus = contactCount == 0 ? 2 : 1;
            }
            catch (Exception) { ContactStop(-77, true); }
        }

        private static bool ContactFiniteScale(Vector3 value)
        { return Finite(value) && value.x != 0f && value.y != 0f && value.z != 0f; }

        private static bool ContactFiniteMatrix(Matrix4x4 value)
        {
            for (int row = 0; row < 4; ++row) for (int col = 0; col < 4; ++col) if (!Finite(value[row, col])) return false;
            return true;
        }

        private static Vector3 ContactClamp(Vector3 value, float maximum)
        {
            float length = value.magnitude;
            if (!Finite(length) || !Finite(value)) return Vector3.zero;
            return length > maximum && length > 0f ? value * (maximum / length) : value;
        }

        private static Vector3 ContactDesired(Vector3 position, Vector3 hand, float radius, float maximum)
        {
            Vector3 away = position - hand; float distance = away.magnitude;
            if (!Finite(distance) || distance >= radius) return Vector3.zero;
            if (distance <= .000001f) return Vector3.zero; // undefined direction: fail closed.
            return away * (Math.Min(radius - distance, maximum) / distance);
        }

        private static bool ContactName(string value)
        {
            if (ContactSettings.Region(value)==0) return false;
            string[] names = contactSettings.Targets;
            for (int i = 0; i < names.Length; ++i) if (String.Equals(value, names[i], StringComparison.Ordinal)) return true;
            return false;
        }

        // Cold bounded renderer/bone lookup; retain no assets except references.
        private static ContactTarget[] ContactFind(out int count)
        {
            count = 0;
            ContactTarget[] selected = new ContactTarget[ContactCap];
            SkinnedMeshRenderer[] renderers = prepared.Root.GetComponentsInChildren<SkinnedMeshRenderer>(false);
            if (renderers == null || renderers.Length > 512) return selected;
            int scannedBones = 0;
            for (int i = 0; i < renderers.Length; ++i)
            {
                SkinnedMeshRenderer skin = renderers[i];
                if (skin == null || !skin.enabled || !skin.gameObject.activeInHierarchy || skin.sharedMesh == null) continue;
                Animator animator = skin.GetComponentInParent<Animator>();
                if (animator == null || !animator.isActiveAndEnabled || !animator.gameObject.activeInHierarchy || !animator.transform.IsChildOf(prepared.Root.transform)) continue;
                if (prepared.Kind == 4 && animator != prepared.ContextActor) continue;
                Transform[] bones = skin.bones;
                if (bones == null) continue;
                // SMRs often repeat the same actor skeleton. Bound each list
                // and the complete lookup without treating repeats as unique
                // targets; 42 renderers x typical skeletons remain usable.
                scannedBones += bones.Length;
                if (bones.Length > 2048 || scannedBones > 65536) { count = 0; return selected; }
                for (int j = 0; j < bones.Length; ++j)
                {
                    Transform bone = bones[j];
                    if (bone == null || bone.parent == null || !bone.gameObject.activeInHierarchy || !bone.IsChildOf(animator.transform) || !ContactName(bone.name)) continue;
                    bool duplicate = false;
                    for (int k = 0; k < count; ++k) if (selected[k].Bone == bone) { duplicate = true; break; }
                    if (duplicate) continue;
                    if (count == ContactCap) { count = 0; return selected; }
                    selected[count].Bone = bone; selected[count].Parent = bone.parent;
                    selected[count].Actor = animator.transform; selected[count].Animator = animator;
                    selected[count].Skin = skin; selected[count].Mesh = skin.sharedMesh;
                    selected[count].Material = contactSettings.ForBone(bone.name);
                    selected[count].Child = bone.childCount>0?bone.GetChild(0):null;
                    ++count;
                }
            }
            // Remove descendants whenever an ancestor is selected, regardless
            // of enumeration order, so a chain never gets compound offsets.
            for (int i = count - 1; i >= 0; --i)
            {
                bool descendant = false;
                for (int j = 0; j < count; ++j) if (i != j && selected[i].Bone.IsChildOf(selected[j].Bone)) { descendant = true; break; }
                if (descendant) { --count; selected[i] = selected[count]; selected[count] = new ContactTarget(); }
            }
            return selected;
        }

        public static void ServiceContact(ref int status)
        {
            status = contactStatus;
            if (!OnOwner()) { status = -1; return; }
            try
            {
                if (!contactRunning || !ContactOwnerValid())
                {
                    if (contactSubscribed || contactCount != 0) ClearContact();
                    status = contactStatus; return;
                }
                // A live binding needs no periodic hierarchy search. Callback
                // validation schedules a cold rebind after target loss/swap.
                if (!contactNeedsRebind && contactCount != 0) { status = contactStatus; return; }
                long now = Stopwatch.GetTimestamp();
                double elapsed = ((double)now - contactServiceStamp) / Stopwatch.Frequency;
                if (contactServiceStamp != 0 && elapsed >= 0 && elapsed < .5) { status = contactStatus; return; }
                contactServiceStamp = now;
                int count;
                ContactTarget[] selected = ContactFind(out count);
                bool same = count == contactCount && contactTargets != null;
                if (same) for (int i = 0; i < count; ++i)
                    if (selected[i].Bone != contactTargets[i].Bone || selected[i].Parent != contactTargets[i].Parent ||
                        selected[i].Skin != contactTargets[i].Skin || selected[i].Mesh != contactTargets[i].Mesh || selected[i].Actor != contactTargets[i].Actor)
                    { same = false; break; }
                if (!same) { ContactRestoreAll(); contactTargets = selected; contactCount = count; ++contactBindings; }
                contactNeedsRebind = false;
                contactStatus = status = count == 0 ? 2 : 1;
            }
            catch (Exception) { ContactStop(-78, true); ClearContact(); status = contactStatus; }
        }

        // Cold F11/mailbox only; no logging/formatting at the callback boundary.
        // Native polls on the existing Unity owner; no OpenXR or native imports here.
        public static void DrainContactHaptics(long epoch, long generation, ref int mask)
        {
            mask = 0;
            if (Thread.CurrentThread.ManagedThreadId != ownerThread) return;
            if (!contactRunning || !ContactOwnerValid() || epoch != contactEpoch || generation != contactGeneration)
            { contactHaptics.Suspend(); return; }
            long now = Stopwatch.GetTimestamp();
            if (contactHandStamp == 0 || now < contactHandStamp ||
                now - contactHandStamp > Stopwatch.Frequency / 10)
            { contactHaptics.Suspend(); return; }
            mask = contactHaptics.Drain(now, Stopwatch.Frequency / 10) & contactMask;
        }

        public static string ContactDiagnostics()
        {
            if (!OnOwner()) return "{\"event\":\"touch_p3_contact\",\"status\":-1}";
            StringBuilder text = new StringBuilder(2048);
            text.Append("{\"event\":\"touch_p3_contact\",\"status\":").Append(contactStatus.ToString(CultureInfo.InvariantCulture));
            text.Append(",\"pid\":").Append(contactPid.ToString(CultureInfo.InvariantCulture));
            text.Append(",\"context\":").Append((prepared == null ? 0 : prepared.Kind).ToString(CultureInfo.InvariantCulture));
            text.Append(",\"running\":").Append(contactRunning ? "true" : "false").Append(",\"subscribed\":").Append(contactSubscribed ? "true" : "false");
            text.Append(",\"configured\":").Append(contactConfigured ? "true" : "false").Append(",\"fault\":").Append(contactFault ? "true" : "false");
            text.Append(",\"targets\":").Append(contactCount.ToString(CultureInfo.InvariantCulture));
            text.Append(",\"callbacks\":").Append(contactCallbacks.ToString(CultureInfo.InvariantCulture)).Append(",\"frames\":").Append(contactFrames.ToString(CultureInfo.InvariantCulture));
            text.Append(",\"writes\":").Append(contactWrites.ToString(CultureInfo.InvariantCulture)).Append(",\"restores\":").Append(contactRestores.ToString(CultureInfo.InvariantCulture));
            text.Append(",\"external_overwrites\":").Append(contactExternal.ToString(CultureInfo.InvariantCulture)).Append(",\"faults\":").Append(contactFaults.ToString(CultureInfo.InvariantCulture));
            text.Append(",\"bindings\":").Append(contactBindings.ToString(CultureInfo.InvariantCulture)).Append(",\"needs_rebind\":").Append(contactNeedsRebind ? "true" : "false");
            text.Append(",\"epoch\":").Append(contactEpoch.ToString(CultureInfo.InvariantCulture)).Append(",\"generation\":").Append(contactGeneration.ToString(CultureInfo.InvariantCulture));
            text.Append(",\"source_id\":").Append((preparedSource == null ? 0 : preparedSource.GetInstanceID()).ToString(CultureInfo.InvariantCulture));
            text.Append(",\"hand_mask\":").Append(contactMask.ToString(CultureInfo.InvariantCulture)).Append(",\"hand_frame\":").Append(contactHandFrame.ToString(CultureInfo.InvariantCulture));
            text.Append(",\"callback_frame\":").Append(contactLastFrame.ToString(CultureInfo.InvariantCulture)).Append(",\"ticket\":").Append(contactTicket.ToString(CultureInfo.InvariantCulture));
            text.Append(",\"hand_radius_m\":").Append((contactSettings == null ? 0 : contactSettings.HandRadius).ToString("R", CultureInfo.InvariantCulture));
            text.Append(",\"character_unit_scale\":").Append(contactUnitScale.ToString("R", CultureInfo.InvariantCulture));
            text.Append(",\"hand_unit_scale\":").Append(contactHandScale.ToString("R", CultureInfo.InvariantCulture));
            text.Append(",\"body_radius_m\":").Append((contactSettings == null ? 0 : contactSettings.BodyRadius).ToString("R", CultureInfo.InvariantCulture));
            text.Append(",\"max_displacement_m\":").Append((contactSettings == null ? 0 : contactSettings.MaxDisplacement).ToString("R", CultureInfo.InvariantCulture));
            text.Append(",\"spring_frequency_hz\":").Append((contactSettings == null ? 0 : contactSettings.Frequency).ToString("R", CultureInfo.InvariantCulture));
            text.Append(",\"damping_ratio\":").Append((contactSettings == null ? 0 : contactSettings.Damping).ToString("R", CultureInfo.InvariantCulture));
            text.Append(",\"hand_hold_seconds\":").Append((contactSettings == null ? 0 : contactSettings.HandHoldSeconds).ToString("R", CultureInfo.InvariantCulture));
            text.Append(",\"release_impulse\":").Append((contactSettings == null ? 0 : contactSettings.ReleaseImpulse).ToString("R", CultureInfo.InvariantCulture));
            text.Append(",\"gap_hold_frames\":").Append(contactGapHeld.ToString(CultureInfo.InvariantCulture)).Append(",\"gap_decay_frames\":").Append(contactGapDecay.ToString(CultureInfo.InvariantCulture));
            text.Append(",\"mask0_decay_frames\":").Append(contactMaskZero.ToString(CultureInfo.InvariantCulture)).Append(",\"invalid_resets\":").Append(contactInvalid.ToString(CultureInfo.InvariantCulture));
            text.Append(",\"generation_resets\":").Append(contactGenerationReset.ToString(CultureInfo.InvariantCulture)).Append(",\"hard_expiry_resets\":").Append(contactHardExpiry.ToString(CultureInfo.InvariantCulture));
            text.Append(",\"dt_resets\":").Append(contactDtReset.ToString(CultureInfo.InvariantCulture)).Append(",\"release_impulses\":").Append(contactImpulses.ToString(CultureInfo.InvariantCulture));
            text.Append(",\"bound_bones\":[");
            for (int i = 0; i < contactCount; ++i)
            {
                if (i != 0) text.Append(',');
                ContactTarget target = contactTargets[i];
                text.Append("{\"id\":").Append((target.Bone == null ? 0 : target.Bone.GetInstanceID()).ToString(CultureInfo.InvariantCulture));
                text.Append(",\"name\":"); Quoted(text, target.Bone == null ? null : target.Bone.name);
                text.Append(",\"axis_status\":").Append(target.Alignment).Append(",\"press_axis_status\":").Append(target.PressAlignment);
                text.Append(",\"actor_id\":").Append((target.Actor == null ? 0 : target.Actor.GetInstanceID()).ToString(CultureInfo.InvariantCulture)).Append('}');
            }
            text.Append(']');
            text.Append(",\"implicit_steps\":").Append(contactImplicitSteps);
            text.Append(",\"rotation_writes\":").Append(contactRotationWrites).Append(",\"scale_writes\":").Append(contactScaleWrites).Append(",\"stiffness_caps\":").Append(contactStiffnessCaps);
            text.Append(",\"timing\":\"Application.onBeforeRender; actual Animator-skinning visibility requires runtime acceptance\"}");
            return text.ToString();
        }
    }
}
#endif
