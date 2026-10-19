using System.Diagnostics;
using System.Text;

namespace GF2VR.Installer;

internal static class VariantChecks
{
    private static int checks;
    private static void Assert(bool value, string label) { ++checks; if (!value) throw new InvalidDataException(label); }
    private static void Reject(Action action, string label)
    { ++checks; try { action(); } catch (InstallError) { return; } throw new InvalidDataException("Expected rejection: " + label); }
    internal static int Run(string clean, string dirty)
    {
        string fixture = Path.Combine(AppContext.BaseDirectory, "variants-validation-fixture");
        if (Directory.Exists(fixture)) throw new InvalidDataException("Use a fresh fixture");
        var engine = new InstallationEngine();
        Package Read(string folder) => Contract.Read<Package>(File.ReadAllBytes(Path.Combine(folder, "package-manifest.json")));
        var c = Read(clean); var d = Read(dirty);
        var cn = c.Native ?? throw new InvalidDataException("Clean native missing");
        var dn = d.Native ?? throw new InvalidDataException("Dirty native missing");
        Assert(c.Native?.BuildFlavor == "Clean" && d.Native?.BuildFlavor == "Dirty", "variant identities");
        Assert(c.Version == d.Version && cn.CommonCoreSha256 == dn.CommonCoreSha256, "paired common version/core");
        Assert(c.Files.Select(f => f.Path).SequenceEqual(d.Files.Select(f => f.Path)), "same minimal nine-file ownership");
        foreach (var file in c.Files.Where(f => f.Path is not "GF2 Game/version.dll" && !f.Path.EndsWith("/runtime-contract.json") && !f.Path.EndsWith("/GF2VR.Configurator.dll")))
            Assert(file.Sha256 == d.Files.Single(f => f.Path == file.Path).Sha256, "shared payload byte equality " + file.Path);
        foreach (bool standard in new[] { true, false })
        {
            string root = Path.Combine(fixture, standard ? "standard" : "한글 日本語", "client");
            string game = standard ? Path.Combine(root, "GF2 Game") : root;
            Directory.CreateDirectory(game); File.WriteAllText(Path.Combine(game, "GF2_Exilium.exe"), SelfTest.Inert);
            var preserved = new Dictionary<string, byte[]>();
            foreach (var entry in new[] { ("GameAssembly.dll", "ORIGINAL ASSEMBLY"), ("UnityPlayer.dll", "ORIGINAL PLAYER"), ("assets/sentinel.bin", "ORIGINAL ASSET") })
            {
                string p = Path.Combine(game, entry.Item1); Directory.CreateDirectory(Path.GetDirectoryName(p)!);
                byte[] bytes = Encoding.UTF8.GetBytes(entry.Item2); File.WriteAllBytes(p, bytes); preserved.Add(p, bytes);
            }
            foreach (var entry in new[] { ("controller_hands.json", "{\"schemaVersion\":1,\"enabled\":false,\"future\":42}"),
                ("touch_interaction.json", "{\"schemaVersion\":1,\"enabled\":true,\"privateFuture\":42}"), ("configurator.json", "{\"language\":\"ja\"}") })
            {
                string p = Paths.Contained(root, "vrmod/config/" + entry.Item1); byte[] bytes = Encoding.UTF8.GetBytes(entry.Item2);
                Paths.WriteNew(p, bytes); preserved.Add(p, bytes);
            }
            void Preserve() { foreach (var p in preserved) Assert(File.ReadAllBytes(p.Key).SequenceEqual(p.Value), "preserved " + p.Key); }
            var cleanPlan = engine.Preview(root, clean); Assert(cleanPlan.Prior == null, "fresh preview read-only");
            engine.Install(cleanPlan); Assert(engine.Inspect(root)?.BuildFlavor == "Clean", "Clean durable receipt");
            Preserve();
            var currentBytes = File.ReadAllBytes(Path.Combine(game, "version.dll")); string receipt = engine.ReceiptHash(root)!;
            try { engine.Install(engine.Preview(root, dirty), p => { if (p.EndsWith("runtime-contract.json")) throw new IOException("INJECTED"); }); }
            catch (IOException) { }
            Assert(!engine.HasPending(root) && engine.ReceiptHash(root) == receipt && File.ReadAllBytes(Path.Combine(game, "version.dll")).SequenceEqual(currentBytes), "failed switch restores exact Clean");
            Preserve();
            foreach (var next in new[] { dirty, clean, dirty })
            {
                var package = Read(next); engine.Install(engine.Preview(root, next)); var installed = engine.Inspect(root)!;
                Assert(installed.BuildFlavor == package.Native!.BuildFlavor && installed.CommonCoreSha256 == package.Native.CommonCoreSha256, "switch identity");
                Assert(Paths.FileHash(Path.Combine(game, "version.dll")) == package.Native.DllSha256, "switch DLL");
                var target = SettingsLaunch.Resolve(root, engine); Assert(target != null, "matched common Settings resolves");
                var start = new ProcessStartInfo(target!.Executable) { UseShellExecute = false, CreateNoWindow = true, WindowStyle = ProcessWindowStyle.Hidden };
                foreach (string arg in new[] { "--check-installation", "--root", root }) start.ArgumentList.Add(arg);
                using (var process = Process.Start(start)!) Assert(process.WaitForExit(20000) && process.ExitCode == 0, "real Configurator accepts " + installed.BuildFlavor);
                Assert(UpdateStaging.ManagedVersion(root) == package.Version, "managed update identity after switch");
                Preserve();
            }
            byte[] good = File.ReadAllBytes(Path.Combine(game, "version.dll")); File.AppendAllText(Path.Combine(game, "version.dll"), "MODIFIED");
            Reject(() => engine.Preview(root, clean), "modified DLL switch"); Reject(() => engine.Remove(root, engine.ReceiptHash(root)), "modified DLL remove");
            File.WriteAllBytes(Path.Combine(game, "version.dll"), good);
            while (engine.Inspect(root) != null) engine.Remove(root, engine.ReceiptHash(root));
            foreach (var file in c.Files) Assert(!File.Exists(Paths.Target(root, file.Path)), "remove product ownership " + file.Path);
            Preserve();
            File.WriteAllText(Path.Combine(game, "version.dll"), "FOREIGN DLL");
            Reject(() => engine.Preview(root, clean), "unowned original DLL"); Assert(File.ReadAllText(Path.Combine(game, "version.dll")) == "FOREIGN DLL", "foreign DLL preserved");
            var checkedPackage = engine.CheckPayload(root, clean);
            void Bad(NativePackage native, string label)
            { Reject(() => NativeLayout.Validate(root, c with { Native = native }, c.Files.Select(f => f.Path).ToHashSet(StringComparer.Ordinal)), label); }
            Bad(cn with { BuildFlavor = "Dirty" }, "false Clean flavor");
            Bad(cn with { CommonCoreSha256 = new string('0', 64) }, "wrong common core");
            Bad(cn with { DllSha256 = dn.DllSha256 }, "wrong native hash");
            var helper = UpdatePolicy.HelperFiles.Select(p => new PayloadFile(p, new string('A', 64), 1)).ToArray();
            var descriptor = new UpdateDescriptor(1, Contract.Product, UpdatePolicy.Repository, "1.1.1", "stable", UpdatePolicy.ArchiveName("1.1.1", "Clean"), new string('A', 64), new string('B', 64), helper, "Clean", VariantProfiles.CommonCoreSha256);
            var r = new Receipt(1, Contract.Product, c.Version, "vrmod", root, new string('a', 32), new string('A', 64), c.Files, standard ? "GF2 Game" : "", "Clean", VariantProfiles.CommonCoreSha256);
            UpdatePolicy.ValidateDescriptor(descriptor); UpdateStaging.RequireFlavor(r, descriptor, c); Assert(true, "matching flavor update");
            Reject(() => UpdateStaging.RequireFlavor(r with { BuildFlavor = "Dirty" }, descriptor, c), "Dirty cannot auto-update to Clean");
            Reject(() => UpdateStaging.RequireFlavor(r, descriptor with { BuildFlavor = null }, c), "Clean cannot auto-update to legacy");
            Reject(() => UpdateStaging.RequireFlavor(r, descriptor, d), "descriptor and payload flavor mismatch");
            Reject(() => UpdateStaging.RequireFlavor(r, descriptor with { CommonCoreSha256 = new string('0', 64) }, c), "descriptor and payload core mismatch");
        }
        File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory, "variants-validation.json"), Contract.Encode(new { passed = true, checks, actualConfiguratorRuns = 6, gameLaunches = 0, nativeDllsLoaded = 0, liveInstallationWrites = 0 }));
        return 0;
    }
}
