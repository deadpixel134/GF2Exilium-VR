// Desktop validation driver only; never compiled into a shipped application.
// Replaces HTTPS transport with descriptor-bound local bytes. Product policy,
// consent UI, Stage, Handoff, incoming Apply and GUI reopening run unchanged.
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

internal static class StartupHook
{
    private const string Steam = @"D:\Games\SteamLibrary\steamapps\common\GIRLS' FRONTLINE 2 EXILIUM";
    private const BindingFlags Flags = BindingFlags.Static | BindingFlags.Instance | BindingFlags.NonPublic | BindingFlags.Public;
    private static JsonElement config;
    private static string evidence = "";
    private static bool consent;
    private static int archives;
    private static readonly List<string> requests = new();
    private static string Get(string k) => config.GetProperty(k).GetString()!;
    private static string Hash(string p) { using var s = File.OpenRead(p); return Convert.ToHexString(SHA256.HashData(s)); }
    private static void Require(bool value, string why) { if (!value) throw new InvalidDataException(why); }
    private static void Write(string name, object value) => File.WriteAllText(Path.Combine(evidence, name), JsonSerializer.Serialize(value, new JsonSerializerOptions { WriteIndented = true }));

    public static void Initialize()
    {
        string? cfg = Environment.GetEnvironmentVariable("GF2VR_RELEASE122_DESKTOP_FIXTURE");
        if (cfg == null) return;
        try
        {
            using var doc = JsonDocument.Parse(File.ReadAllBytes(cfg)); config = doc.RootElement.Clone();
            string root = Path.TrimEndingDirectorySeparator(Path.GetFullPath(Get("root")));
            Require(root.Equals(Steam, StringComparison.OrdinalIgnoreCase), "Only the authorized disposable Steam copy");
            evidence = Path.GetFullPath(Get("evidence"));
            Require(evidence.StartsWith(Path.Combine(root, "vrmod", "build", "RELEASE122-T1") + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase), "Evidence boundary");
            Directory.CreateDirectory(evidence);
            Require(File.ReadAllText(Get("markerFile")) == Get("marker"), "Explicit desktop run marker");
            Require(!Process.GetProcessesByName("GF2_Exilium").Any(), "Game must remain stopped");
            string app = Environment.ProcessPath!;
            if (Path.GetFileName(app).Equals("GF2VR.Installer.exe", StringComparison.OrdinalIgnoreCase))
            {
                Require(Hash(Path.Combine(AppContext.BaseDirectory, "GF2VR.Installer.dll")) == Get("helperSha256"), "Actual incoming helper bytes");
                string[] args = Environment.GetCommandLineArgs();
                Require(args.Contains("--apply-confirmed-update"), "Incoming helper mode");
                string stage = args[Array.IndexOf(args, "--stage") + 1];
                Write("incoming-start.json", new { processId = Environment.ProcessId, stage, assemblySha256 = Get("helperSha256"), originalEntryPoint = true });
                AppDomain.CurrentDomain.FirstChanceException += (_, e) =>
                {
                    if (e.Exception.GetType().Name == "InstallError") Write("incoming-exception.json", new { error = e.Exception.ToString() });
                };
                return; // The ordinary product entry point performs Apply and reopens Settings.
            }
            Require(Path.GetFileName(app).Equals("GF2VR.Configurator.exe", StringComparison.OrdinalIgnoreCase), "Configurator process");
            var assembly = Assembly.LoadFrom(Path.Combine(AppContext.BaseDirectory, "GF2VR.Configurator.dll"));
            AppDomain.CurrentDomain.FirstChanceException += (_, e) =>
            {
                if (e.Exception.GetType().Name == "InstallError" || e.Exception is IOException or UnauthorizedAccessException) Write("client-exception.json", new { error = e.Exception.ToString(), stack = e.Exception.StackTrace });
            };
            string hash = Hash(assembly.Location);
            bool reopened = Path.GetFullPath(app).Equals(Path.Combine(root, "vrmod", "bin", "GF2VR.Configurator", "GF2VR.Configurator.exe"), StringComparison.OrdinalIgnoreCase)
                && hash == Get("newGuiSha256") && VersionAt(root) == "1.2.2";
            Require(hash == (reopened ? Get("newGuiSha256") : Get("clientGuiSha256")), "Client assembly bytes");
            Require(VersionAt(root) == (reopened ? "1.2.2" : "1.2.1"), "Actual Installer-created receipt version");
            var shared = (HttpClient)assembly.GetType("GF2VR.Installer.GithubOwnerClient", true)!.GetField("Shared", Flags)!.GetValue(null)!;
            typeof(HttpMessageInvoker).GetField("_handler", Flags)!.SetValue(shared, new LocalTransport(reopened));
            Exception? failure = null;
            var thread = new Thread(() => { try { Gui(assembly, root, reopened, hash); } catch (Exception e) { failure = e; } });
            thread.SetApartmentState(ApartmentState.STA); thread.Start(); thread.Join();
            if (failure != null) throw failure;
            Environment.Exit(0);
        }
        catch (Exception e)
        {
            if (evidence.Length != 0) Write("driver-error-" + Environment.ProcessId + ".json", new { error = e.ToString() });
            Environment.Exit(91);
        }
    }
    private static string VersionAt(string root)
    {
        using var d = JsonDocument.Parse(File.ReadAllBytes(Path.Combine(root, "vrmod", "installer", "install-state.json")));
        return d.RootElement.GetProperty("version").GetString()!;
    }
    private static void Gui(Assembly assembly, string root, bool reopened, string hash)
    {
        Application.SetHighDpiMode(HighDpiMode.SystemAware); Application.EnableVisualStyles(); Application.SetCompatibleTextRenderingDefault(false);
        var type = assembly.GetType("GF2VR.Configurator.MainForm", true)!;
        using var form = (Form)Activator.CreateInstance(type, Flags, null, new object?[] { root, null }, null)!;
        var update = (Button)type.GetField("updateButton", Flags)!.GetValue(form)!;
        var status = (Label)type.GetField("updateStatus", Flags)!.GetValue(form)!;
        form.StartPosition = FormStartPosition.Manual; form.Location = new Point(-20000, -20000); form.ShowInTaskbar = false;
        using var tick = new System.Windows.Forms.Timer { Interval = 100 };
        using var dialog = new System.Windows.Forms.Timer { Interval = 50 };
        var watch = Stopwatch.StartNew(); bool clicked = false; bool done = false; Exception? failed = null;
        void Capture(string name) { using var b = new Bitmap(form.Width, form.Height); form.DrawToBitmap(b, new Rectangle(0, 0, b.Width, b.Height)); b.Save(Path.Combine(evidence, name)); }
        tick.Tick += (_, _) =>
        {
            try
            {
                if (watch.ElapsedMilliseconds > 150000) throw new TimeoutException("Desktop GUI update");
                if (clicked && (string?)type.GetField("updateKey", Flags)!.GetValue(form) == "updateFailed")
                {
                    Capture("failed.png");
                    throw new InvalidDataException("Product refused update: " + status.AccessibleName);
                }
                if (reopened && watch.ElapsedMilliseconds > 2000)
                {
                    Require(!update.Visible, "Already-current GUI must not apply again");
                    Capture("03-reopened.png"); Write("reopened.json", new { passed = true, processId = Environment.ProcessId, version = VersionAt(root), assemblySha256 = hash, status = status.Text, gameLaunches = 0 }); done = true; form.Close(); return;
                }
                if (!reopened && !clicked && update.Visible)
                {
                    Require(archives == 0 && !consent, "No archive download before consent");
                    Capture("01-discovered.png"); Write("discovered.json", new { passed = true, version = "1.2.2", status = status.Text, archiveDownloads = archives, source = "explicit local fixture transport, not public GitHub" });
                    clicked = true; dialog.Start(); update.PerformClick();
                }
            }
            catch (Exception e) { failed = e; tick.Stop(); dialog.Stop(); form.Close(); }
        };
        dialog.Tick += (_, _) =>
        {
            EnumThreadWindows(GetCurrentThreadId(), (hwnd, _) =>
            {
                var cls = new StringBuilder(64); GetClassName(hwnd, cls, cls.Capacity);
                if (cls.ToString() != "#32770" || GetDlgItem(hwnd, 6) == IntPtr.Zero) return true;
                try
                {
                    Require(!consent && archives == 0, "Exactly one consent before archive");
                    if (GetWindowRect(hwnd, out var rect))
                    {
                        using var b = new Bitmap(rect.Right - rect.Left, rect.Bottom - rect.Top);
                        using (var graphics = Graphics.FromImage(b)) { var hdc = graphics.GetHdc(); try { Require(PrintWindow(hwnd, hdc, 2), "Confirmation screenshot"); } finally { graphics.ReleaseHdc(hdc); } }
                        b.Save(Path.Combine(evidence, "02-consent.png"));
                    }
                    consent = true; dialog.Stop(); SendMessage(GetDlgItem(hwnd, 6), 0x00F5, IntPtr.Zero, IntPtr.Zero); // Actual Yes button.
                }
                catch (Exception e) { failed = e; dialog.Stop(); SendMessage(GetDlgItem(hwnd, 7), 0x00F5, IntPtr.Zero, IntPtr.Zero); form.Close(); }
                return false;
            }, IntPtr.Zero);
        };
        form.Shown += (_, _) => tick.Start();
        Application.Run(form); tick.Stop(); dialog.Stop();
        if (failed != null) throw failed;
        if (!reopened)
        {
            Require(clicked && consent && archives == 1, "One real consent, one archive download, product form closed after handoff");
            Write("client-completed.json", new { passed = true, processId = Environment.ProcessId, clientAssemblySha256 = hash, clientVersion = Get("clientVersion"), userConfirmedOnce = true, archives, requests, productionApplyUpdateStageHandoff = true, publicEndToEnd = "NOT_RUN" });
        }
        else Require(done, "Ordinary installed GUI reopened");
    }
    private sealed class LocalTransport(bool reopened) : HttpMessageHandler
    {
        protected override Task<HttpResponseMessage> SendAsync(HttpRequestMessage request, CancellationToken cancellationToken)
        {
            string url = request.RequestUri!.AbsoluteUri;
            lock (requests) requests.Add(url);
            Write("transport-progress.json", new { url, consent, archives, requests });
            string file; string prefix = "https://api.github.com/repos/deadpixel134/GF2Exilium-VR/";
            if (url == prefix + "releases?per_page=20") file = Get("catalog");
            else if (url == prefix + "releases/assets/102") file = Get("descriptorAsset");
            else if (url == prefix + "releases/assets/103") file = Get("archiveAsset");
            else if (url == Get("descriptorUrl")) file = Get("descriptor");
            else if (url == Get("archiveUrl"))
            {
                Require(!reopened && consent, "No automatic download/execution without consent"); archives++; file = Get("archive");
            }
            else throw new InvalidDataException("Unexpected external request " + url);
            return Task.FromResult(new HttpResponseMessage(HttpStatusCode.OK) { Content = new StreamContent(File.OpenRead(file)), RequestMessage = request });
        }
    }
    private delegate bool EnumCallback(IntPtr hwnd, IntPtr parameter);
    [StructLayout(LayoutKind.Sequential)] private struct Rect { public int Left, Top, Right, Bottom; }
    [DllImport("kernel32.dll")] private static extern uint GetCurrentThreadId();
    [DllImport("user32.dll")] private static extern bool EnumThreadWindows(uint threadId, EnumCallback callback, IntPtr parameter);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] private static extern int GetClassName(IntPtr hwnd, StringBuilder name, int length);
    [DllImport("user32.dll")] private static extern IntPtr GetDlgItem(IntPtr hwnd, int id);
    [DllImport("user32.dll")] private static extern IntPtr SendMessage(IntPtr hwnd, uint message, IntPtr wparam, IntPtr lparam);
    [DllImport("user32.dll")] private static extern bool GetWindowRect(IntPtr hwnd, out Rect rect);
    [DllImport("user32.dll")] private static extern bool PrintWindow(IntPtr hwnd, IntPtr dc, uint flags);
}
