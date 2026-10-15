using System;
using System.Threading;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;

namespace GF2.Performance
{
    // Native scalar notifications only. No Submit, CommandBuffer or XR calls.
    // Native owner validates the original Submit, empty pending context and
    // exact child queue before scheduling its existing ordered plugin event.
    public sealed class ProducerScopeBridge
    {
        private static ProducerScopeBridge instance;
        // Cold-install observation only. Never changes Install's result or
        // subscription cleanup. Read by the native owner once on failure.
        private static int installProgress;
        private static string installFailure = "";
        public static int InstallProgress() { return installProgress; }
        public static string InstallFailure() { return installFailure; }
        // This mailbox is owned primitive state, accessed by the native
        // adapter through public exact field APIs on this managed owner only.
        private static long engineSerial, engineNonce, engineAck;
        private static int engineOperation, engineCamera, engineResult;
        private static IntPtr engineCallback;
        private static int engineThread;
        private static long engineSequence;
        private static bool engineBusy;
        private static int NativeNotify(long epoch, int operation, int camera)
        {
            if (engineCallback == IntPtr.Zero || engineThread != Thread.CurrentThread.ManagedThreadId ||
                engineBusy || engineSequence == Int64.MaxValue) return 0;
            engineBusy = true;
            long sequence = ++engineSequence;
            try
            {
                engineSerial = epoch; engineOperation = operation; engineCamera = camera;
                engineResult = 0;
                Interlocked.Exchange(ref engineAck, 0);
                Interlocked.Exchange(ref engineNonce, sequence);
                // Existing engine wrapper, not a new external declaration.
                // Native interception consumes only the exact reserved callback/ID.
                GL.IssuePluginEvent(engineCallback, 0x65315031);
                return Interlocked.Read(ref engineAck) == sequence ? engineResult : 0;
            }
            finally { engineBusy = false; }
        }

        private readonly int owner;
        private bool beginSubscribed, endSubscribed;
        private long serial, lastSerial, failedSerial;
        private ScriptableRenderContext context;
        private Camera left, right;
        private int[] originalIds;
        private int leftId, rightId, openId, started, ended, tailAttempted, tailAccepted, notifying;
        private Camera[][] groups;
        private int[] progress;

        private ProducerScopeBridge()
        {
            owner = Thread.CurrentThread.ManagedThreadId;
        }

        public static int Install(IntPtr callback)
        {
            if (instance != null) { installProgress = 6; return 0; }
            if (callback == IntPtr.Zero) { installProgress = 7; return 0; }
            installFailure = "";
            ProducerScopeBridge bridge = null;
            try
            {
                // Existing engine owner and primitive-field handshake. Reject
                // missing interception/wrong owner/cookie before subscriptions.
                installProgress = 1;
                engineCallback = callback;
                engineThread = Thread.CurrentThread.ManagedThreadId;
                if (NativeNotify(callback.ToInt64(), 0, 0x50534231) != 0x50534231)
                { installFailure = "Engine owner handshake rejected"; return 0; }
                installProgress = 2;
                bridge = new ProducerScopeBridge();
                instance = bridge;
                installProgress = 3;
                RenderPipelineManager.beginCameraRendering += bridge.Begin;
                bridge.beginSubscribed = true;
                installProgress = 4;
                RenderPipelineManager.endCameraRendering += bridge.End;
                bridge.endSubscribed = true;
                installProgress = 5;
                return 1;
            }
            catch (Exception exception)
            {
                // Each diagnostic operation has its own guard; unsupported
                // exception introspection cannot prevent original cleanup.
                try { installFailure = exception.GetType().FullName; } catch (Exception) { }
                try { installFailure = installFailure + ": " + exception.Message; } catch (Exception) { }
                if (bridge != null) bridge.RemoveSubscriptions();
                return 0;
            }
        }

        private bool OnOwner() { return Thread.CurrentThread.ManagedThreadId == owner; }
        private bool Failed(long epoch) { return Interlocked.Read(ref failedSerial) >= epoch; }
        private void Fail(long epoch)
        {
            long old;
            do
            {
                old = Interlocked.Read(ref failedSerial);
                if (old >= epoch) break;
            }
            while (Interlocked.CompareExchange(ref failedSerial, epoch, old) != old);
            if (OnOwner() && notifying == 0 && Interlocked.Read(ref serial) == epoch)
            {
                notifying = 1;
                try { NativeNotify(epoch, 4, 0); } catch (Exception) { }
                finally { notifying = 0; }
            }
        }

        private int Send(long epoch, int operation, int id)
        {
            if (notifying != 0) { Fail(epoch); return 0; }
            int result = 0;
            notifying = 1;
            try { result = NativeNotify(epoch, operation, id); }
            catch (Exception) { Fail(epoch); }
            finally { notifying = 0; }
            // A callback may reenter the managed events. Such a root remains
            // failed even if native issuance has already queued a real event.
            if (Interlocked.Read(ref serial) != epoch || Failed(epoch))
            { Fail(epoch); return 0; }
            return result;
        }

        private static bool EmptyPrivate(Camera camera)
        {
            if (camera == null) return false;
            UniversalAdditionalCameraData data = camera.GetComponent(typeof(UniversalAdditionalCameraData)) as UniversalAdditionalCameraData;
            if (data == null || data.renderType != CameraRenderType.Base) return false;
            var cameras = data.cameras;
            var stack = data.cameraStack;
            return (cameras == null || cameras.Count == 0) && (stack == null || stack.Count == 0);
        }

        private static Camera[] PrivateGroup(Camera camera)
        {
            if (EmptyPrivate(camera)) return new Camera[] { camera };
            if (camera == null) return null;
            var data = camera.GetComponent(typeof(UniversalAdditionalCameraData)) as UniversalAdditionalCameraData;
            if (data == null || data.renderType != CameraRenderType.Base || data.cameras == null ||
                data.cameraStack == null || data.cameras.Count == 0 || data.cameras.Count > 7 ||
                data.cameras.Count != data.cameraStack.Count) return null;
            var result = new Camera[data.cameras.Count + 1]; result[0] = camera;
            for (int i = 1; i < result.Length; ++i)
            {
                Camera member = data.cameras[i - 1];
                if (member == null || member != data.cameraStack[i - 1] || !member.isActiveAndEnabled) return null;
                var overlay = member.GetComponent(typeof(UniversalAdditionalCameraData)) as UniversalAdditionalCameraData;
                // Both stack aliases warn and return null on Overlay in the
                // actual URP. The Base list owns these ordered contributors.
                if (overlay == null || overlay.renderType != CameraRenderType.Overlay) return null;
                int id = member.GetInstanceID();
                if (id == 0) return null;
                for (int j = 0; j < i; ++j) if (result[j].GetInstanceID() == id) return null;
                result[i] = member;
            }
            return result;
        }

        private bool GroupUnchanged(int eye)
        {
            Camera[] expected = groups[eye];
            Camera camera = expected[0];
            if (camera == null) return false;
            var data = camera.GetComponent(typeof(UniversalAdditionalCameraData)) as UniversalAdditionalCameraData;
            if (data == null || data.renderType != CameraRenderType.Base) return false;
            if (expected.Length == 1) return EmptyPrivate(camera);
            if (data.cameras == null || data.cameraStack == null || data.cameras.Count != expected.Length - 1 ||
                data.cameraStack.Count != expected.Length - 1) return false;
            for (int i = 1; i < expected.Length; ++i)
            {
                Camera member = expected[i];
                if (member == null || !member.isActiveAndEnabled || data.cameras[i - 1] != member ||
                    data.cameraStack[i - 1] != member) return false;
                var overlay = member.GetComponent(typeof(UniversalAdditionalCameraData)) as UniversalAdditionalCameraData;
                if (overlay == null || overlay.renderType != CameraRenderType.Overlay) return false;
            }
            return true;
        }

        public static int Arm(long epoch, ScriptableRenderContext context, Camera left, Camera right, Camera[] originals)
        {
            ProducerScopeBridge b = instance;
            if (b == null || !b.OnOwner() || !b.beginSubscribed || !b.endSubscribed ||
                Interlocked.Read(ref b.serial) != 0 || epoch <= b.lastSerial ||
                originals == null || originals.Length == 0 || originals.Length > 32) return 0;
            try
            {
                Camera[] lg = PrivateGroup(left), rg = PrivateGroup(right);
                if (lg == null || rg == null || lg.Length != rg.Length) return 0;
                int l = left.GetInstanceID(), r = right.GetInstanceID();
                if (l == 0 || r == 0 || l == r) return 0;
                int[] ids = new int[originals.Length];
                for (int i = 0; i < originals.Length; ++i)
                {
                    if (originals[i] == null) return 0;
                    int id = originals[i].GetInstanceID();
                    if (id == 0 || id == l || id == r) return 0;
                    for (int j = 0; j < i; ++j)
                        if (ids[j] == id) return 0;
                    ids[i] = id;
                }
                for (int eye = 0; eye < 2; ++eye)
                {
                    Camera[] group = eye == 0 ? lg : rg;
                    for (int n = 0; n < group.Length; ++n)
                    {
                        int id = group[n].GetInstanceID();
                        if (id == 0) return 0;
                        for (int j = 0; j < ids.Length; ++j) if (id == ids[j]) return 0;
                        if (eye == 1) for (int j = 0; j < lg.Length; ++j)
                            if (id == lg[j].GetInstanceID()) return 0;
                    }
                }
                b.groups = new Camera[][] { lg, rg }; b.progress = new int[2];
                b.context = context; b.left = left; b.right = right; b.originalIds = ids;
                b.leftId = l; b.rightId = r; b.openId = b.started = b.ended = 0;
                b.tailAttempted = b.tailAccepted = 0; b.lastSerial = epoch;
                Interlocked.Exchange(ref b.serial, epoch);
                return 1;
            }
            catch (Exception) { return 0; }
        }

        private int Eye(int id)
        {
            for (int e = 0; e < 2; ++e) for (int n = 0; n < groups[e].Length; ++n)
                if (groups[e][n].GetInstanceID() == id) return 1 << e;
            return 0;
        }
        private bool Original(int id)
        {
            for (int i = 0; i < originalIds.Length; ++i)
                if (originalIds[i] == id) return true;
            return false;
        }
        private bool Enter(long epoch, ScriptableRenderContext c, Camera camera)
        {
            if (!OnOwner()) { Fail(epoch); return false; }
            if (Failed(epoch)) return false;
            if (notifying != 0) { Fail(epoch); return false; }
            if (!context.Equals(c) || camera == null) { Fail(epoch); return false; }
            return true;
        }

        private void Begin(ScriptableRenderContext c, Camera camera)
        {
            long epoch = Interlocked.Read(ref serial);
            if (epoch == 0 || !Enter(epoch, c, camera)) return;
            try
            {
                int id = camera.GetInstanceID(), eye = Eye(id);
                if (openId != 0) { Fail(epoch); return; }
                if (eye != 0)
                {
                    int e = eye == 1 ? 0 : 1;
                    if ((ended & eye) != 0 || tailAttempted != 0 || !GroupUnchanged(e) ||
                        progress[e] >= groups[e].Length || groups[e][progress[e]].GetInstanceID() != id)
                    { Fail(epoch); return; }
                    if (progress[e] == 0)
                    {
                        if ((started & eye) != 0 || (started != ended) || Send(epoch, 1, e == 0 ? leftId : rightId) != 1)
                        { Fail(epoch); return; }
                        started |= eye;
                    }
                    if (groups[e].Length > 1 && Send(epoch, 5, id) != 1) { Fail(epoch); return; }
                }
                else
                {
                    if (!Original(id) || started != ended) { Fail(epoch); return; }
                    // A later Begin is after the preceding End multicast has
                    // returned. Native rejection is a harmless fallback, not
                    // permission to retry tails on every following camera.
                    if (ended == 3 && tailAttempted == 0)
                    {
                        tailAttempted = 1;
                        tailAccepted = Send(epoch, 3, id) == 1 ? 1 : 0;
                    }
                }
                if (Failed(epoch)) return;
                openId = id;
            }
            catch (Exception) { Fail(epoch); }
        }
        private void End(ScriptableRenderContext c, Camera camera)
        {
            long epoch = Interlocked.Read(ref serial);
            if (epoch == 0 || !Enter(epoch, c, camera)) return;
            try
            {
                int id = camera.GetInstanceID(), eye = Eye(id);
                if (openId != id || id == 0) { Fail(epoch); return; }
                if (eye != 0)
                {
                    int e = eye == 1 ? 0 : 1;
                    if ((started & eye) == 0 || (ended & eye) != 0 || !GroupUnchanged(e) ||
                        progress[e] >= groups[e].Length || groups[e][progress[e]].GetInstanceID() != id)
                    { Fail(epoch); return; }
                    if (groups[e].Length > 1 && Send(epoch, 6, id) != 1) { Fail(epoch); return; }
                    ++progress[e];
                    if (progress[e] == groups[e].Length)
                    {
                        if (Send(epoch, 2, e == 0 ? leftId : rightId) != 1) { Fail(epoch); return; }
                        ended |= eye;
                    }
                }
                openId = 0;
            }
            catch (Exception) { Fail(epoch); }
        }

        // 1 valid/no early tail, 2 valid/early tail accepted, 0 invalid.
        // Native root CPU success/currentness and real GPU drains remain
        // independent. These flags cannot authorize publication or release.
        public static int Finish(long epoch, int rootSucceeded)
        {
            ProducerScopeBridge b = instance;
            if (b == null || !b.OnOwner() || epoch <= 0 || Interlocked.Read(ref b.serial) != epoch) return 0;
            if (b.notifying != 0) { b.Fail(epoch); return 0; }
            if (rootSucceeded == 0 || b.openId != 0 || b.started != 3 || b.ended != 3) b.Fail(epoch);
            int result = b.Failed(epoch) ? 0 : b.tailAccepted != 0 ? 2 : 1;
            Interlocked.Exchange(ref b.serial, 0);
            b.left = b.right = null; b.originalIds = null; b.groups = null; b.progress = null;
            b.context = default(ScriptableRenderContext);
            return result;
        }
        public static int Status(long epoch)
        {
            ProducerScopeBridge b = instance;
            if (b == null || epoch <= 0 || Interlocked.Read(ref b.serial) != epoch) return -1;
            if (!b.OnOwner()) { b.Fail(epoch); return -1; }
            return b.Failed(epoch) ? -1 : b.tailAccepted != 0 ? 2 : 1;
        }
        private bool RemoveSubscriptions()
        {
            bool ok = true;
            try { if (beginSubscribed) { RenderPipelineManager.beginCameraRendering -= Begin; beginSubscribed = false; } }
            catch (Exception) { ok = false; }
            try { if (endSubscribed) { RenderPipelineManager.endCameraRendering -= End; endSubscribed = false; } }
            catch (Exception) { ok = false; }
            return ok;
        }
        public static int Detach()
        {
            ProducerScopeBridge b = instance;
            if (b == null || !b.OnOwner() || Interlocked.Read(ref b.serial) != 0) return 0;
            // Keep instance and native callback process-pinned after unsubscription.
            return b.RemoveSubscriptions() ? 1 : 0;
        }
    }
}
