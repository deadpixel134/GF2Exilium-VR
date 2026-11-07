using System.IO.MemoryMappedFiles;
using System.Runtime.InteropServices;

namespace GF2VR.Configurator;

// Wire layout shared with live_settings_surface.h. Each pixel slot has an atomic
// FREE -> WRITING -> READY -> READING -> FREE owner, independent of source generations.
internal sealed unsafe class VrSettingsSurface : IDisposable
{
    internal const uint Magic = 0x31534647;
    internal const int HeaderBytes = 4096, PixelBytes = VrSettingsEditor.Width * VrSettingsEditor.Height * 4;
    private readonly MemoryMappedFile mapping;
    private readonly MemoryMappedViewAccessor view;
    private byte* memory;
    private bool acquired;
    private ulong inputEpoch;
    private int toggle;
    private int redraw;
    private uint inputVersion;
    internal void InvalidateInput() => inputVersion = 0;
    private ref int I32(int offset) => ref *(int*)(memory + offset);
    private ref long I64(int offset) => ref *(long*)(memory + offset);
    internal VrSettingsSurface(int pid, ulong instance)
    {
        mapping = MemoryMappedFile.OpenExisting("Local\\GF2VR.SettingsSurface." + pid, MemoryMappedFileRights.ReadWrite);
        view = mapping.CreateViewAccessor(0, HeaderBytes + 2L * PixelBytes, MemoryMappedFileAccess.ReadWrite);
        try
        {
            view.SafeMemoryMappedViewHandle.AcquirePointer(ref memory); acquired = true;
            memory += view.PointerOffset;
            if ((uint)Volatile.Read(ref I32(0)) != Magic || I32(4) != 1 || I32(8) != pid ||
                I32(12) != VrSettingsEditor.Width || I32(16) != VrSettingsEditor.Height || I32(20) != VrSettingsEditor.Width * 4 ||
                unchecked((ulong)Interlocked.Read(ref I64(32))) != instance) throw new SettingsError("liveConnectionError");
            toggle = Volatile.Read(ref I32(116));
            redraw = Volatile.Read(ref I32(120));
        }
        catch { Dispose(); throw; }
    }
    internal void Heartbeat(VrSettingsEditor editor)
    {
        Volatile.Write(ref I32(104), (editor.Busy ? 1 : 0) | (editor.Dirty ? 2 : 0));
        Volatile.Write(ref I32(108), editor.Open ? 1 : 0);
        Interlocked.Exchange(ref I64(40), Environment.TickCount64);
    }
    internal void Inputs(VrSettingsEditor editor)
    {
        int nextToggle = Volatile.Read(ref I32(116));
        if (toggle != nextToggle) { toggle = nextToggle; if (Volatile.Read(ref I32(112)) == 1) editor.EnsureOpen(); else editor.Close(); }
        int nextRedraw = Volatile.Read(ref I32(120));
        if (redraw != nextRedraw) { redraw = nextRedraw; editor.Invalidate(); InvalidateInput(); }
        ulong epoch = unchecked((ulong)Interlocked.Read(ref I64(24)));
        if (epoch != inputEpoch) { editor.CancelPointer(); inputEpoch = epoch; }
        uint read = unchecked((uint)Volatile.Read(ref I32(100))), write = unchecked((uint)Volatile.Read(ref I32(96)));
        if (write - read > 64) { editor.CancelPointer(); Volatile.Write(ref I32(100), unchecked((int)write)); return; }
        while (read != write)
        {
            int at = 128 + (int)(read % 64) * 32;
            int type = I32(at), x = I32(at + 4), y = I32(at + 8);
            ulong session = unchecked((ulong)I64(at + 16));
            if (session == unchecked((ulong)Interlocked.Read(ref I64(24))))
            {
                if (type == 4) editor.ToggleOpen();
                else if (type == 6) editor.Close();
                else if (type == 3) editor.CancelPointer();
                else if (editor.Open && !editor.Redraw && inputVersion != 0 && unchecked((uint)I32(at + 12)) == inputVersion) editor.Pointer(type, x, y);
                else editor.CancelPointer();
            }
            ++read; Volatile.Write(ref I32(100), unchecked((int)read));
        }
    }
    internal bool Publish(Bitmap bitmap)
    {
        int slot = -1;
        for (int i = 0; i < 2; ++i)
            if (Interlocked.CompareExchange(ref I32(64 + i * 4), 1, 0) == 0) { slot = i; break; }
        if (slot < 0) return false;
        System.Drawing.Imaging.BitmapData? pixels = null;
        try
        {
            pixels = bitmap.LockBits(new Rectangle(0, 0, bitmap.Width, bitmap.Height), System.Drawing.Imaging.ImageLockMode.ReadOnly,
                System.Drawing.Imaging.PixelFormat.Format32bppArgb);
            if (pixels.Stride != VrSettingsEditor.Width * 4) throw new SettingsError("livePayloadError");
            Buffer.MemoryCopy((void*)pixels.Scan0, memory + HeaderBytes + slot * PixelBytes, PixelBytes, PixelBytes);
            long sequence = Interlocked.Increment(ref I64(88));
            Interlocked.Exchange(ref I64(72 + slot * 8), sequence);
            Volatile.Write(ref I32(64 + slot * 4), 2); inputVersion = unchecked((uint)sequence); return true;
        }
        catch { Volatile.Write(ref I32(64 + slot * 4), 0); throw; }
        finally { if (pixels != null) bitmap.UnlockBits(pixels); }
    }
    public void Dispose()
    {
        if (acquired) { view.SafeMemoryMappedViewHandle.ReleasePointer(); acquired = false; memory = null; }
        view.Dispose(); mapping.Dispose();
    }
}

internal sealed class VrSettingsHost : ApplicationContext
{
#if GF2VR_LIVE_SETTINGS_ENTRY_OBSERVE
    private bool firstPublished;
#endif
    private readonly System.Diagnostics.Process game;
    private readonly VrSettingsSurface surface;
    private readonly VrSettingsEditor editor;
    private readonly System.Windows.Forms.Timer timer = new() { Interval = 67 };
    private Bitmap? unpublished;
    private bool disposed;
    internal VrSettingsHost(string root, int pid)
    {
        game = System.Diagnostics.Process.GetProcessById(pid);
        var store = new SettingsStore(root); store.Load(); LiveSettingsCoordinator.Recover(store);
#if GF2VR_LIVE_SETTINGS_ENTRY_OBSERVE
        VrSettingsTrace.Note("store-loaded");
#endif
        using var runtime = LiveSettingsClient.Connect(root);
#if GF2VR_LIVE_SETTINGS_ENTRY_OBSERVE
        VrSettingsTrace.Note("runtime-connected");
#endif
        if (runtime.State.ProcessId != pid || runtime.State.Phase != 0) throw new SettingsError("liveConnectionError");
        store.BindRuntimeBaseline(runtime.State);
        Texts.Language = store.LoadLanguage();
        surface = new VrSettingsSurface(pid, runtime.State.Instance); editor = new VrSettingsEditor(store);
#if GF2VR_LIVE_SETTINGS_ENTRY_OBSERVE
        VrSettingsTrace.Note("surface-opened");
#endif
        timer.Tick += (_, _) => Tick(); timer.Start();
    }
    private void Tick()
    {
        try
        {
            if (game.HasExited) { ExitThread(); return; }
            surface.Inputs(editor); editor.Tick(); surface.Heartbeat(editor);
            if (editor.Redraw) { surface.InvalidateInput(); unpublished?.Dispose(); unpublished = editor.Render(); }
            if (unpublished != null && surface.Publish(unpublished))
            {
#if GF2VR_LIVE_SETTINGS_ENTRY_OBSERVE
                if (!firstPublished) { VrSettingsTrace.Note("first-frame-published"); firstPublished = true; }
#endif
                unpublished.Dispose(); unpublished = null;
            }
        }
        catch (Exception error)
        {
#if GF2VR_LIVE_SETTINGS_ENTRY_OBSERVE
            VrSettingsTrace.Note("tick-error", error);
#else
            _ = error;
#endif
            ExitThread();
        } // Native heartbeat recovery releases modal ownership if the host dies.
    }
    protected override void Dispose(bool disposing)
    {
        if (disposing && !disposed)
        {
            disposed = true; timer.Stop(); timer.Dispose(); unpublished?.Dispose();
            editor.Dispose(); surface.Dispose(); game.Dispose();
        }
        base.Dispose(disposing);
    }
}
#if GF2VR_LIVE_SETTINGS_ENTRY_OBSERVE
internal static class VrSettingsTrace
{
    private static int gamePid;
    internal static void Start(string? pid)
    {
        _ = int.TryParse(pid, out gamePid); Note("entry");
    }
    internal static void Note(string stage, Exception? error = null)
    {
        try
        {
            var record = new { gamePid, hostPid = Environment.ProcessId, stage, tickMs = Environment.TickCount64, error = error?.ToString() };
            File.WriteAllText(Path.Combine(AppContext.BaseDirectory, $"vr-settings-host-{gamePid}-{Environment.ProcessId}.json"), System.Text.Json.JsonSerializer.Serialize(record));
        }
        catch { } // Observation must never alter host behavior when its output path is unavailable.
    }
}
#endif
