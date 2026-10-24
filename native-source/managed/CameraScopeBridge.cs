using System;
using System.Threading;
using UnityEngine;
using UnityEngine.Rendering;

namespace GF2.Composition
{
    // One process-lifetime plan. Native owner must publish all marker identities
    // before Install, bracket one original render, and retain callback/plan storage
    // through independent callback retirement AND GPU completion after Detach.
    // This observes camera scopes only. It supplies no pass/resource attribution.
    // Diagnostic-only Flush explicitly submits the last context after original
    // render returns and Seal succeeds. This changes submission timing and must
    // be enabled only for the connected diagnostic, never ordinary product use.
    public sealed class CameraScopeBridge
    {
        // 0 installing, 1 between cameras, 2 camera open, 3 awaiting Seal,
        // 4 sealed, 5 flush returned, 6 flushing, -1 invalid.
        // Detach/Flush are not GPU fences or callback cancellation.
        private static CameraScopeBridge instance;
        private readonly int owner;
        private readonly long epoch;
        private readonly IntPtr callback;
        private readonly int[] cameras, beginIds, endIds;
        private int state, next;
        private bool beginSubscribed, endSubscribed;
        private ScriptableRenderContext lastContext;
        private bool haveContext;

        private CameraScopeBridge(IntPtr callback, long epoch, int[] cameras,
                                  int[] beginIds, int[] endIds)
        {
            owner = Thread.CurrentThread.ManagedThreadId;
            this.callback = callback;
            this.epoch = epoch;
            this.cameras = (int[])cameras.Clone();
            this.beginIds = (int[])beginIds.Clone();
            this.endIds = (int[])endIds.Clone();
        }

        public static int Install(IntPtr callback, long epoch, int[] cameras,
                                  int[] beginIds, int[] endIds)
        {
            if (instance != null || callback == IntPtr.Zero || epoch <= 0 ||
                cameras == null || beginIds == null || endIds == null ||
                cameras.Length == 0 || cameras.Length > 8 ||
                cameras.Length != beginIds.Length || cameras.Length != endIds.Length)
                return 0;
            int last = 0;
            for (int i = 0; i < cameras.Length; ++i)
            {
                if (cameras[i] == 0 || beginIds[i] <= last || endIds[i] <= beginIds[i] ||
                    endIds[i] == Int32.MaxValue) return 0;
                for (int j = 0; j < i; ++j) if (cameras[i] == cameras[j]) return 0;
                last = endIds[i];
            }
            CameraScopeBridge bridge = new CameraScopeBridge(callback, epoch, cameras, beginIds, endIds);
            instance = bridge; // Never reused, including partial subscription failure.
            try
            {
                RenderPipelineManager.beginCameraRendering += bridge.Begin;
                bridge.beginSubscribed = true;
                RenderPipelineManager.endCameraRendering += bridge.End;
                bridge.endSubscribed = true;
                if (Interlocked.CompareExchange(ref bridge.state, 1, 0) != 0)
                {
                    bridge.Fail();
                    bridge.RemoveSubscriptions();
                    return 0;
                }
                return 1;
            }
            catch (Exception)
            {
                bridge.Fail();
                bridge.RemoveSubscriptions();
                return 0;
            }
        }

        private void Fail() { Interlocked.Exchange(ref state, -1); }
        private bool OnOwner()
        {
            if (Thread.CurrentThread.ManagedThreadId == owner) return true;
            Fail();
            return false;
        }
        private int ReadState() { return Interlocked.CompareExchange(ref state, 0, 0); }

        private void Emit(ScriptableRenderContext context, int id)
        {
            // Engine copies/enqueues the command before this local wrapper retires.
            // This does not retire the native callback or its separately owned plan.
            using (CommandBuffer command = new CommandBuffer())
            {
                command.IssuePluginEvent(callback, id);
                context.ExecuteCommandBuffer(command);
            }
        }
        private void Begin(ScriptableRenderContext context, Camera camera)
        {
            if (!OnOwner() || ReadState() == -1) return;
            try
            {
                if (ReadState() != 1 || next >= cameras.Length || camera == null ||
                    camera.GetInstanceID() != cameras[next]) { Fail(); return; }
                if (haveContext && !lastContext.Equals(context)) { Fail(); return; }
                lastContext = context;
                haveContext = true;
                // CAS preserves a concurrent foreign-thread failure.
                if (Interlocked.CompareExchange(ref state, 2, 1) != 1) { Fail(); return; }
                Emit(context, beginIds[next]);
            }
            catch (Exception) { Fail(); }
        }
        private void End(ScriptableRenderContext context, Camera camera)
        {
            if (!OnOwner() || ReadState() == -1) return;
            try
            {
                if (ReadState() != 2 || next >= cameras.Length || camera == null ||
                    camera.GetInstanceID() != cameras[next] || !haveContext ||
                    !lastContext.Equals(context)) { Fail(); return; }
                Emit(context, endIds[next]);
                lastContext = context;
                ++next;
                if (Interlocked.CompareExchange(ref state, next == cameras.Length ? 3 : 1, 2) != 2) Fail();
            }
            catch (Exception) { Fail(); }
        }
        public static int Status(long epoch)
        {
            CameraScopeBridge bridge = instance;
            return bridge != null && bridge.epoch == epoch ? bridge.ReadState() : -1;
        }
        public static int Seal(long epoch)
        {
            CameraScopeBridge bridge = instance;
            if (bridge == null) return 0;
            if (!bridge.OnOwner()) return 0;
            if (bridge.epoch != epoch || bridge.next != bridge.cameras.Length ||
                Interlocked.CompareExchange(ref bridge.state, 4, 3) != 3) { bridge.Fail(); return 0; }
            return 1;
        }
        public static int Flush(long epoch)
        {
            CameraScopeBridge bridge = instance;
            if (bridge == null || !bridge.OnOwner()) return 0;
            if (bridge.epoch != epoch ||
                Interlocked.CompareExchange(ref bridge.state, 6, 4) != 4) { bridge.Fail(); return 0; }
            try
            {
                // A native tail callback runs after every queued camera marker
                // and any command already on this context. It closes CPU-side
                // observation only; it is not a GPU completion fence.
                bridge.Emit(bridge.lastContext, bridge.endIds[bridge.endIds.Length - 1] + 1);
                bridge.lastContext.Submit();
                if (Interlocked.CompareExchange(ref bridge.state, 5, 6) == 6) return 1;
            }
            catch (Exception) { }
            bridge.Fail();
            return 0;
        }
        private bool RemoveSubscriptions()
        {
            bool ok = true;
            try { if (beginSubscribed) { RenderPipelineManager.beginCameraRendering -= Begin; beginSubscribed = false; } }
            catch (Exception) { ok = false; Fail(); }
            try { if (endSubscribed) { RenderPipelineManager.endCameraRendering -= End; endSubscribed = false; } }
            catch (Exception) { ok = false; Fail(); }
            return ok;
        }
        public static int Detach(long epoch)
        {
            CameraScopeBridge bridge = instance;
            if (bridge == null || !bridge.OnOwner()) return 0;
            if (bridge.epoch != epoch) { bridge.Fail(); return 0; }
            if (bridge.ReadState() != 5) bridge.Fail();
            return bridge.RemoveSubscriptions() ? 1 : 0;
        }
    }
}
