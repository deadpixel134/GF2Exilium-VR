using System;
using System.IO;
using System.Linq;
using System.Threading;
using System.Diagnostics;
using System.Reflection;
using System.Security.Cryptography;
using System.Text.Json;

// Test driver only. Runs the released, byte-identical assembly INSIDE its actual
// apphost process; no old source is recompiled and no validation is replaced.
// The product has no offline UI feed, so this is not a public-download UI test.
internal static class StartupHook
{
    private const string Inert = "INERT INSTALLER FIXTURE — NEVER EXECUTE AS A GAME";
    private const BindingFlags Static = BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static;
    public static void Initialize()
    {
        string? file = Environment.GetEnvironmentVariable("GF2VR_RELEASE121_FIXTURE");
        if (string.IsNullOrEmpty(file)) return;
        string? evidence = null;
        try
        {
            using var config = JsonDocument.Parse(File.ReadAllBytes(file));
            string Get(string key) => config.RootElement.GetProperty(key).GetString()!;
            string root = Path.GetFullPath(Get("root"));
            string boundary = Path.GetFullPath(Get("boundary")) + Path.DirectorySeparatorChar;
            if (!root.StartsWith(boundary, StringComparison.OrdinalIgnoreCase) ||
                !root.Contains("updater-validation-fixture", StringComparison.Ordinal) ||
                File.ReadAllText(Path.Combine(root, ".release121-fixture")) != Get("marker") ||
                File.ReadAllText(Path.Combine(root, "GF2 Game", "GF2_Exilium.exe")) != Inert)
                throw new InvalidDataException("Fixture driver refused a non-inert target.");
            evidence = Get("evidence");
            string[] args = Environment.GetCommandLineArgs()[1..];
            string app = Environment.ProcessPath ?? throw new InvalidDataException("Missing apphost path.");
            if (!Path.GetFileName(app).Equals("GF2VR.Installer.exe", StringComparison.OrdinalIgnoreCase))
                throw new InvalidDataException("Unexpected fixture process.");
            var assembly = Assembly.LoadFrom(Path.Combine(AppContext.BaseDirectory, "GF2VR.Installer.dll"));
            string assemblyHash = Hash(assembly.Location);
            if (args.Contains("--apply-confirmed-update"))
            {
                if (assemblyHash != Get("newInstallerSha256")) throw new InvalidDataException("Incoming helper bytes differ.");
                // This existing product flag only suppresses GUI reopening after
                // checking the inert fixture marker. Apply/receipt/hash checks
                // and the real handoff parent identity run unchanged.
                string[] fixtureArgs = [..args, "--fixture-no-reopen"];
                int code = -1;
                Exception? failure = null;
                var thread = new Thread(() =>
                {
                    try { code = (int)assembly.EntryPoint!.Invoke(null, [fixtureArgs])!; }
                    catch (Exception ex) { failure = ex; }
                });
                thread.SetApartmentState(ApartmentState.STA); thread.Start(); thread.Join();
                Write(evidence, "incoming-helper-process.json", new
                {
                    processId = Environment.ProcessId, apphost = app, assemblyHash, originalArgs = args,
                    fixtureArgs, exitCode = code, guiReopenSuppressedByExistingFixtureFlag = true,
                    failure = failure?.ToString(), originalEntryPointExecuted = true
                });
                Environment.Exit(failure == null ? code : 92);
            }
            if (!args.Contains("--release121-official-stage-fixture") || assemblyHash != Get("oldInstallerSha256") ||
                Hash(app) != Get("oldInstallerExeSha256"))
                throw new InvalidDataException("Expected unchanged officially distributed old process.");
            Type TypeOf(string name) => assembly.GetType("GF2VR.Installer." + name, true)!;
            object? Call(string type, string method, params object?[] values) => TypeOf(type).GetMethods(Static)
                .Single(m => m.Name == method && m.GetParameters().Length == values.Length).Invoke(null, values);
            byte[] metadata = File.ReadAllBytes(Get("descriptor"));
            var read = TypeOf("Contract").GetMethod("Read", Static)!.MakeGenericMethod(TypeOf("UpdateDescriptor"));
            object descriptor = read.Invoke(null, [metadata])!;
            string? before = (string?)Call("UpdateStaging", "ManagedVersion", root);
            if (before != Get("oldVersion")) throw new InvalidDataException("Old actual receipt not eligible.");
            object staged = Call("UpdateStaging", "Extract", root, Get("stage"), Get("archive"), metadata, descriptor)!;
            string? after = (string?)Call("UpdateStaging", "ManagedVersion", root);
            if (after != before) throw new InvalidDataException("Staging changed the installation.");
            Write(evidence, "official120-stage.json", new
            {
                passed = true, processId = Environment.ProcessId, processStartTicks = Process.GetCurrentProcess().StartTime.ToUniversalTime().Ticks,
                apphost = app, apphostSha256 = Hash(app), assembly = assembly.Location, assemblyHash,
                versionBefore = before, versionAfterStage = after, staged,
                actualOfficialBinaryCodeExecuted = true, oldSourceRecompiled = false,
                publicDownloadEndToEnd = "NOT_RUN", entryDriver = "isolated DOTNET_STARTUP_HOOKS; existing Extract/Handoff methods"
            });
            Call("UpdateStaging", "Handoff", staged);
            Write(evidence, "official120-handoff.json", new
            {
                passed = true, processId = Environment.ProcessId,
                handoff = JsonDocument.Parse(File.ReadAllBytes(Path.Combine(Get("stage"), "handoff.json"))).RootElement.Clone(),
                actualOldHandoffMethodExecuted = true, parentAndConsentSynthesizedByDriver = false
            });
            Environment.Exit(0);
        }
        catch (Exception ex)
        {
            if (evidence != null) Write(evidence, "fixture-driver-error-" + Environment.ProcessId + ".json", new { error = ex.ToString() });
            Environment.Exit(91); // Never fall through and open the product UI on a failed fixture.
        }
    }
    private static string Hash(string file) => Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(file)));
    private static void Write(string folder, string name, object value)
    {
        Directory.CreateDirectory(folder);
        File.WriteAllText(Path.Combine(folder, name), JsonSerializer.Serialize(value, new JsonSerializerOptions { WriteIndented = true }));
    }
}
