#if GF2VR_TOUCH_PRIVATE
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.Text;
using System.Threading;
using UnityEngine;

namespace GF2.Private
{
    // R-P2-4 observation only. No Transform/Animator setters, native imports,
    // scene search, callback subscription, sampling animation or file I/O.
    internal static class BoneReadProbe
    {
        private const int BoneCap = 64, SampleCap = 3, InputCap = 32768;
        private const double TimeoutMilliseconds = 2000;
        private static int owner, context, sourceId, pid, count, matched, scanned, samples;
        private static bool inputLimited, selectionLimited, running, pending;
        private static int lastFrame;
        private static long startedTimestamp;
        private static string outcome;
        private static Camera source;
        private static Transform[] targets, roots;
        private static int[] ids, rootIds, frames;
        private static string[] names;
        private static float[] values;
        private static long[] stamps;

        private static bool OnOwner()
        {
            int current = Thread.CurrentThread.ManagedThreadId;
            if (owner == 0) owner = current;
            return owner == current;
        }

        private static bool Candidate(string name)
        {
            return name == "Chest_L" || name == "Chest_R" ||
                (name != null && (name.StartsWith("Chest_L_", StringComparison.Ordinal) ||
                name.StartsWith("Chest_R_", StringComparison.Ordinal) ||
                name.StartsWith("Butt_", StringComparison.Ordinal)));
        }

        private static void ReleaseEngineReferences()
        { source = null; targets = null; roots = null; }

        private static void Complete(string reason)
        {
            outcome = reason; running = false; pending = true;
            ReleaseEngineReferences();
        }

        private static void ClearData()
        {
            ReleaseEngineReferences();
            ids = null; rootIds = null; frames = null; names = null; values = null; stamps = null;
            running = false; pending = false; outcome = null;
            context = sourceId = pid = count = matched = scanned = samples = 0;
            inputLimited = selectionLimited = false; lastFrame = -1; startedTimestamp = 0;
        }

        public static void Begin(IList<Transform> bones, Camera camera, int manualContext)
        {
            if (!OnOwner()) return;
            ClearData();
            context = manualContext;
            try
            {
                try { using (Process process = Process.GetCurrentProcess()) pid = process.Id; }
                catch (Exception) { pid = 0; }
                if (camera == null || camera.gameObject == null || !camera.gameObject.activeInHierarchy ||
                    bones == null || manualContext < 1 || manualContext > 3)
                { Complete("INVALID_BEGIN_INPUT"); return; }
                source = camera; sourceId = camera.GetInstanceID();
                targets = new Transform[BoneCap]; roots = new Transform[BoneCap];
                ids = new int[BoneCap]; rootIds = new int[BoneCap]; names = new string[BoneCap];
                values = new float[BoneCap * SampleCap * 7];
                frames = new int[SampleCap]; stamps = new long[SampleCap];
                inputLimited = bones.Count > InputCap;
                int limit = Math.Min(bones.Count, InputCap);
                for (int i = 0; i < limit; ++i)
                {
                    ++scanned;
                    Transform bone = bones[i];
                    if (bone == null || !Candidate(bone.name)) continue;
                    bool duplicate = false;
                    for (int j = 0; j < count; ++j) if (targets[j] == bone) { duplicate = true; break; }
                    if (duplicate) continue;
                    ++matched;
                    if (count >= BoneCap) { selectionLimited = true; continue; }
                    Transform root = bone.root;
                    if (root == null || bone.gameObject == null || !bone.gameObject.activeInHierarchy ||
                        root.gameObject == null || !root.gameObject.activeInHierarchy) continue;
                    targets[count] = bone; roots[count] = root;
                    ids[count] = bone.GetInstanceID(); rootIds[count] = root.GetInstanceID();
                    names[count] = bone.name;
                    ++count;
                }
                if (count == 0) { Complete("NO_MATCHING_ACTIVE_BONES"); return; }
                startedTimestamp = Stopwatch.GetTimestamp();
                running = true; lastFrame = -1;
            }
            catch (Exception) { Complete("BEGIN_EXCEPTION"); }
        }

        private static bool Finite(float value)
        { return !Single.IsNaN(value) && !Single.IsInfinity(value); }

        // Called on existing Unity-main tick. Three distinct frameCount values,
        // at most 64*7 float reads per sample; zero owned hot-path allocations.
        public static void Sample()
        {
            if (!OnOwner() || !running) return;
            try
            {
                long now = Stopwatch.GetTimestamp();
                double elapsed = ((double)now - startedTimestamp) * 1000 / Stopwatch.Frequency;
                if (elapsed < 0 || elapsed > TimeoutMilliseconds)
                { Complete("TIMEOUT_OR_CLOCK_DISCONTINUITY"); return; }
                if (source == null || source.gameObject == null || !source.gameObject.activeInHierarchy)
                { Complete("SOURCE_UNAVAILABLE"); return; }
                int frame = Time.frameCount;
                if (frame == lastFrame) return;
                if (frame < 0 || frame < lastFrame) { Complete("UNITY_FRAME_REVERSED"); return; }
                for (int i = 0; i < count; ++i)
                {
                    Transform bone = targets[i], root = roots[i];
                    if (bone == null || root == null || bone.gameObject == null || root.gameObject == null ||
                        !bone.gameObject.activeInHierarchy || !root.gameObject.activeInHierarchy)
                    { Complete("BONE_OR_ROOT_UNAVAILABLE"); return; }
                    Vector3 p = bone.localPosition;
                    Quaternion q = bone.localRotation;
                    if (!Finite(p.x) || !Finite(p.y) || !Finite(p.z) ||
                        !Finite(q.x) || !Finite(q.y) || !Finite(q.z) || !Finite(q.w))
                    { Complete("NONFINITE_LOCAL_VALUE"); return; }
                    int offset = (samples * BoneCap + i) * 7;
                    values[offset] = p.x; values[offset + 1] = p.y; values[offset + 2] = p.z;
                    values[offset + 3] = q.x; values[offset + 4] = q.y;
                    values[offset + 5] = q.z; values[offset + 6] = q.w;
                }
                frames[samples] = frame; stamps[samples] = now;
                lastFrame = frame;
                if (++samples == SampleCap) Complete("THREE_DISTINCT_FRAMES_CAPTURED");
            }
            catch (Exception) { Complete("SAMPLE_EXCEPTION"); }
        }

        private static void Quote(StringBuilder text, string value)
        {
            if (value == null) { text.Append("null"); return; }
            const string hex = "0123456789abcdef";
            text.Append('"');
            for (int i = 0; i < value.Length; ++i)
            {
                int c = value[i];
                if (c >= 0x20 && c <= 0x7e && c != '"' && c != '\\') text.Append((char)c);
                else
                {
                    text.Append("\\u"); text.Append(hex[(c >> 12) & 15]); text.Append(hex[(c >> 8) & 15]);
                    text.Append(hex[(c >> 4) & 15]); text.Append(hex[c & 15]);
                }
            }
            text.Append('"');
        }

        private static bool Varied(int bone, int first, int length)
        {
            for (int sample = 1; sample < samples; ++sample)
                for (int axis = first; axis < first + length; ++axis)
                    if (values[(sample * BoneCap + bone) * 7 + axis] != values[bone * 7 + axis]) return true;
            return false;
        }

        // Called only when the bounded probe has completed/cancelled. JSON is
        // ASCII even for non-ASCII names, and emitted once. Never writes a log.
        public static string Drain()
        {
            if (!OnOwner() || !pending) return null;
            try
            {
                StringBuilder text = new StringBuilder(65536);
                text.Append("{\"event\":\"touch_p2_bone_samples\",\"pid\":").Append(pid.ToString(CultureInfo.InvariantCulture));
                text.Append(",\"context\":").Append(context.ToString(CultureInfo.InvariantCulture)).Append(",\"source_id\":").Append(sourceId.ToString(CultureInfo.InvariantCulture));
                text.Append(",\"reference_epoch\":null,\"source_generation\":null,\"identity_limit\":\"source camera only; epoch/generation not supplied to this managed read probe\"");
                text.Append(",\"outcome\":"); Quote(text, outcome);
                text.Append(",\"requested_samples\":3,\"captured_samples\":").Append(samples.ToString(CultureInfo.InvariantCulture));
                text.Append(",\"bone_cap\":64,\"input_cap\":32768,\"timeout_ms\":2000");
                text.Append(",\"input_scanned\":").Append(scanned.ToString(CultureInfo.InvariantCulture)).Append(",\"candidate_name_matches\":").Append(matched.ToString(CultureInfo.InvariantCulture));
                text.Append(",\"selected_bones\":").Append(count.ToString(CultureInfo.InvariantCulture));
                text.Append(",\"input_limited\":").Append(inputLimited ? "true" : "false");
                text.Append(",\"selection_limited\":").Append(selectionLimited ? "true" : "false");
                text.Append(",\"clip_binding_status\":\"UNSUPPORTED_OBSERVATION\",\"animator_overwrite_proved\":false,\"bone_identity_semantics_accepted\":false,\"engine_writes\":0,\"contact_writes\":0");
                text.Append(",\"variation_limit\":\"sampled local value changes do not identify the writer, clip binding, per-frame overwrite, LateUpdate order or safe restoration\"");
                text.Append(",\"frames\":[");
                for (int s = 0; s < samples; ++s) { if (s != 0) text.Append(','); text.Append(frames[s].ToString(CultureInfo.InvariantCulture)); }
                text.Append("],\"stopwatch_frequency\":").Append(Stopwatch.Frequency.ToString(CultureInfo.InvariantCulture)).Append(",\"sample_timestamps\":[");
                for (int s = 0; s < samples; ++s) { if (s != 0) text.Append(','); text.Append(stamps[s].ToString(CultureInfo.InvariantCulture)); }
                text.Append("],\"bones\":[");
                for (int i = 0; i < count; ++i)
                {
                    if (i != 0) text.Append(',');
                    text.Append("{\"id\":").Append(ids[i].ToString(CultureInfo.InvariantCulture)).Append(",\"root_id\":").Append(rootIds[i].ToString(CultureInfo.InvariantCulture)).Append(",\"name\":"); Quote(text, names[i]);
                    text.Append(",\"local_position_varied\":").Append(Varied(i, 0, 3) ? "true" : "false");
                    text.Append(",\"local_rotation_varied\":").Append(Varied(i, 3, 4) ? "true" : "false");
                    text.Append(",\"samples\":[");
                    for (int s = 0; s < samples; ++s)
                    {
                        if (s != 0) text.Append(',');
                        text.Append("{\"local_position\":[");
                        for (int axis = 0; axis < 7; ++axis)
                        {
                            if (axis == 3) text.Append("],\"local_rotation\":[");
                            else if (axis != 0) text.Append(',');
                            text.Append(values[(s * BoneCap + i) * 7 + axis].ToString("R", CultureInfo.InvariantCulture));
                        }
                        text.Append("]}");
                    }
                    text.Append("]}");
                }
                text.Append("]}");
                return text.ToString();
            }
            finally { ClearData(); }
        }

        public static void Reset()
        {
            if (!OnOwner()) return;
            // Preserve a cancellation record until the next cold Drain; all
            // engine references are released immediately. A later Begin starts
            // a fresh independent observation and discards any old report.
            if (running) Complete("RESET_CANCELLED");
            else ReleaseEngineReferences();
        }
    }
}
#endif
