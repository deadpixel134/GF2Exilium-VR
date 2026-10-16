using System.Diagnostics;
using System.Text;
using System.Text.Json.Nodes;

namespace GF2VR.Installer;

// Exercise real product payloads only against inert, isolated game layouts.
internal static class FlexibleLayoutChecks
{
    private static int checks;
    private static void Assert(bool value, string label)
    { ++checks; if (!value) throw new InvalidDataException(label); }
    private static void Reject(Action action, string label)
    {
        ++checks;
        try { action(); } catch (InstallError) { return; }
        throw new InvalidDataException("Expected rejection: " + label);
    }
    private static string Game(string directory)
    {
        Directory.CreateDirectory(directory);
        string exe = Path.Combine(directory, "GF2_Exilium.exe");
        File.WriteAllText(exe, SelfTest.Inert);
        return exe;
    }
    internal static int Run(string packageRoot)
    {
        string fixture = Path.Combine(AppContext.BaseDirectory, "flexible-validation-fixture");
        if (Directory.Exists(fixture)) throw new InvalidDataException("Preserve fixture; use fresh output.");
        Directory.CreateDirectory(fixture);
        var engine = new InstallationEngine();
        var source = Contract.Read<Package>(File.ReadAllBytes(Path.Combine(packageRoot, "package-manifest.json")));
        Assert(source.Native?.BuildId == NativeLayout.FlexibleBuild, "new flexible native required");
        string package = Path.Combine(fixture, "package");
        foreach (var file in source.Files)
            Paths.WriteNew(Paths.Contained(package, "payload/" + file.Path), File.ReadAllBytes(Paths.Contained(packageRoot, "payload/" + file.Path)));
        Paths.WriteNew(Paths.Contained(package, "package-manifest.json"), Contract.Encode(source));

        foreach (var layout in new[] { ("standard/GF2 Game", true), ("renamed/client", false), ("한글 日本語/deep/client/build", false), ("flat", false) })
        {
            string game = Path.Combine(fixture, layout.Item1.Replace('/', Path.DirectorySeparatorChar));
            string exe = Game(game), root = layout.Item2 ? Path.GetDirectoryName(game)! : game;
            Assert(Paths.Root(exe) == root && Paths.Root(game) == root, "file/directory canonical root");
            if (layout.Item1 != "flat")
                Assert(Paths.Root(Path.GetDirectoryName(game)!) == root, "containing-folder discovery");
            Assert(Paths.GameExecutable(root) == exe, "exact executable location");
            Assert(!Paths.Equal(Paths.FileHash(exe), source.GameExecutableSha256), "fixture original hash differs");
            var plan = engine.Preview(exe, package);
            Assert(plan.Root == root && !Directory.Exists(Path.Combine(root, "vrmod")), "preview is read-only");
            string config = Paths.Contained(root, "vrmod/config/navigation.json");
            byte[] settings = Encoding.UTF8.GetBytes("{\"schemaVersion\":1,\"navigation\":{\"enabled\":false}}");
            Paths.WriteNew(config, settings);
            try { engine.Install(plan, path => { if (path == NativeLayout.ToolRoot + "runtime-contract.json") throw new IOException("INJECTED rollback"); }); }
            catch (IOException) { }
            Assert(!engine.HasPending(root) && engine.Inspect(root) == null, "rollback receipt cleared");
            foreach (var file in source.Files) Assert(!File.Exists(Paths.Target(root, file.Path)), "rollback mapped file " + file.Path);
            Assert(File.ReadAllBytes(config).SequenceEqual(settings), "rollback settings preserved");
            engine.Install(engine.Preview(root, package));
            Assert(File.Exists(Path.Combine(game, "version.dll")), "DLL beside located executable");
            Assert(engine.Inspect(root)!.GameDirectory == (layout.Item2 ? "GF2 Game" : ""), "durable layout ownership");
            foreach (var file in engine.Inspect(root)!.Files)
                Assert(Paths.Equal(Paths.FileHash(Paths.Target(root, file.Path)), file.Sha256), "mapped installed payload " + file.Path);
            var target = SettingsLaunch.Resolve(root, engine);
            Assert(target != null && target.Root == root, "Settings target resolves mapped native");
            Assert(UpdateStaging.ManagedVersion(root) == source.Version, "updater recognizes installed mapped payload");
            // Use valid defaults for the real read-only Configurator process.
            File.Delete(config);
            var info = new ProcessStartInfo(target!.Executable) { UseShellExecute = false, CreateNoWindow = true, WindowStyle = ProcessWindowStyle.Hidden };
            foreach (string arg in new[] { "--check-installation", "--root", root }) info.ArgumentList.Add(arg);
            using (var process = Process.Start(info)!)
                Assert(process.WaitForExit(20000) && process.ExitCode == 0, "actual Configurator layout contract");
            Paths.WriteNew(config, settings);
            File.AppendAllText(exe, " DIFFERENT GAME PATCH");
            Assert(engine.Preview(root, package).Root == root, "changed original remains installable");
            if (layout.Item2)
            {
                // v1.0.0 receipts/journals did not include the new layout field.
                string statePath = Paths.Contained(root, Contract.StatePath);
                var legacyState = JsonNode.Parse(File.ReadAllBytes(statePath))!.AsObject();
                string transaction = legacyState["transactionId"]!.GetValue<string>();
                legacyState.Remove("gameDirectory"); byte[] legacyBytes = Contract.Encode(legacyState);
                string transactionFolder = Paths.Transaction(root, transaction);
                string journalPath = Paths.Contained(transactionFolder, "journal.json");
                var journal = Contract.Read<Journal>(File.ReadAllBytes(journalPath));
                var last = journal.Changes[^1]; File.WriteAllBytes(Paths.Contained(transactionFolder, last.AfterFile!), legacyBytes);
                journal.Changes[^1] = last with { AfterHash = Paths.Hash(legacyBytes) };
                var legacyJournal = JsonNode.Parse(Contract.Encode(journal))!.AsObject(); legacyJournal.Remove("gameDirectory");
                File.WriteAllBytes(journalPath, Contract.Encode(legacyJournal)); File.WriteAllBytes(statePath, legacyBytes);
                Assert(engine.Inspect(root)!.GameDirectory == null && engine.Preview(root, package).Prior != null, "legacy receipt/journal migration");
            }
            bool running = false; var previousRunning = InstallationEngine.GameRunning;
            try
            {
                InstallationEngine.GameRunning = () => running;
                running = true;
                Reject(() => engine.Remove(root, engine.ReceiptHash(root)), "running game guard");
            }
            finally { InstallationEngine.GameRunning = previousRunning; }
            engine.Remove(root, engine.ReceiptHash(root));
            foreach (var file in source.Files) Assert(!File.Exists(Paths.Target(root, file.Path)), "remove mapped file " + file.Path);
            Assert(File.ReadAllBytes(config).SequenceEqual(settings) && File.ReadAllText(exe).EndsWith("DIFFERENT GAME PATCH"), "settings/original preserved");
        }

        string ambiguous = Path.Combine(fixture, "multiple");
        Game(Path.Combine(ambiguous, "one")); Game(Path.Combine(ambiguous, "two"));
        Reject(() => Paths.Root(ambiguous), "multiple executables require narrower selection");
        string stale = Path.Combine(fixture, "stale"), staleExe = Game(stale);
        var shown = engine.Preview(stale, package);
        string relocated = Path.Combine(stale, "moved"); Directory.CreateDirectory(relocated);
        File.Move(staleExe, Path.Combine(relocated, "GF2_Exilium.exe"));
        Reject(() => engine.Install(shown), "changed destination after preview");
        Assert(!Directory.Exists(Path.Combine(relocated, "vrmod")), "stale preview did not redirect writes");
        string tamper = Paths.Contained(package, "payload/vrmod/runtime/openxr-LICENSE.txt");
        byte[] original = File.ReadAllBytes(tamper); File.AppendAllText(tamper, "CORRUPT");
        Reject(() => engine.Preview(relocated, package), "VR package corruption still rejected");
        File.WriteAllBytes(tamper, original);
        string drift = Path.Combine(fixture, "drift"); Game(Path.Combine(drift, "GF2 Game"));
        try
        {
            engine.Install(engine.Preview(drift, package), path =>
            { if (path == "GF2 Game/version.dll") Game(drift); });
        }
        catch (InstallError) { }
        Assert(!File.Exists(Path.Combine(drift, "version.dll")) && !File.Exists(Path.Combine(drift, "GF2 Game", "version.dll")), "layout drift never redirects native write");
        File.Delete(Path.Combine(drift, "GF2_Exilium.exe"));
        engine.Recover(drift);
        Assert(!engine.HasPending(drift), "restore expected layout enables recovery");
        File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory, "flexible-layout-validation.json"), Contract.Encode(new
        { passed = true, checks, layouts = 4, originalGameHashesRequired = false, originalGamePrerequisitesRequired = false,
            actualConfiguratorReadOnlyRuns = 4, gameLaunches = 0, nativeDllsLoaded = 0, actualInstallationWrites = 0 }));
        return 0;
    }
}
