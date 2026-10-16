using System.Diagnostics;
using System.Text;

namespace GF2VR.Installer;

// Changed deployment only: real private-runtime Configurator payload, inert game
// prerequisites, existing receipt/journal engine. No native DLL is ever loaded.
internal static class DesktopDeploymentChecks
{
    internal static int Run(string packageRoot)
    {
        int checks = 0;
        void Check(bool ok, string label) { ++checks; if (!ok) throw new InvalidDataException(label); }
        string fixture = Path.Combine(AppContext.BaseDirectory, "desktop-validation-fixture");
        if (Directory.Exists(fixture)) throw new InvalidDataException("Preserve deployment fixture; use a fresh output.");
        string root = Path.Combine(fixture, "한글 desktop game"); string pkg = Path.Combine(fixture, "package");
        Paths.WriteNew(Paths.Contained(root, "GF2 Game/GF2_Exilium.exe"), Encoding.UTF8.GetBytes(SelfTest.Inert));
        var original = Contract.Read<Package>(File.ReadAllBytes(Paths.Contained(packageRoot, "package-manifest.json")));
        Check(NativeLayout.IsPortable(original) && original.BoundGameRoot == null, "recognized portable native provider");
        string expectedDll = NativeLayout.ExpectedHash(original);
        Check(Paths.Equal(original.Native!.DllSha256, expectedDll), "exact native provider hash");
        foreach (var f in original.Files)
        {
            byte[] bytes = File.ReadAllBytes(Paths.Contained(packageRoot, "payload/" + f.Path));
            Check(Paths.Equal(Paths.Hash(bytes), f.Sha256) && bytes.Length == f.Bytes, "declared deployment bytes " + f.Path);
            Paths.WriteNew(Paths.Contained(pkg, "payload/" + f.Path), bytes);
        }
        foreach (string p in NativeLayout.GameFiles) Paths.WriteNew(Paths.Contained(root, p), Encoding.UTF8.GetBytes("INERT DESKTOP PREREQUISITE " + p));
        var manifest = original with { GameExecutableSha256 = Paths.FileHash(Paths.Contained(root, "GF2 Game/GF2_Exilium.exe"))!,
            Native = original.Native! with { GameFiles = NativeLayout.GameFiles.Select(p => new PayloadFile(p,
                Paths.FileHash(Paths.Contained(root, p))!, new FileInfo(Paths.Contained(root, p)).Length)).ToArray() } };
        Paths.WriteNew(Paths.Contained(pkg, "package-manifest.json"), Contract.Encode(manifest));
        var engine = new InstallationEngine(); var plan = engine.Preview(root, pkg);
        Check(plan.Payload.Count == 9, "unchanged owned-file layout");
        engine.Install(plan); var receipt = engine.Inspect(root)!;
        foreach (var f in receipt.Files) Check(Paths.Equal(Paths.FileHash(Paths.Contained(root, f.Path)), f.Sha256), "private deployment receipt " + f.Path);
        string guiExe = Paths.Contained(root, NativeLayout.ToolRoot + "GF2VR.Configurator.exe");
        string trace = Path.Combine(fixture, "configurator-host-trace.txt");
        var info = new ProcessStartInfo(guiExe) { UseShellExecute = false, CreateNoWindow = true, WindowStyle = ProcessWindowStyle.Hidden };
        info.ArgumentList.Add("--check-installation"); info.ArgumentList.Add("--root"); info.ArgumentList.Add(root);
        info.Environment["DOTNET_HOST_TRACE"] = "1"; info.Environment["DOTNET_HOST_TRACEFILE"] = trace;
        info.Environment["DOTNET_HOST_TRACE_VERBOSITY"] = "4";
        using (var gui = Process.Start(info) ?? throw new InvalidDataException("Configurator did not start"))
        { Check(gui.WaitForExit(20000) && gui.ExitCode == 0, "installed private-runtime Configurator root/contract read"); }
        Check(File.Exists(trace), "installed Configurator host trace");
        string contractPath = Paths.Contained(root, NativeLayout.ToolRoot + "runtime-contract.json");
        byte[] contract = File.ReadAllBytes(contractPath);
        // Update from an explicit prior package, then verify removal restores
        // its exact owned bytes, including its native and desktop host versions.
        string legacyPackage = Path.Combine(fixture, "legacy-package");
        // Use the immutable prior release manifest supplied only by this test.
        string legacySource = Path.Combine(AppContext.BaseDirectory, "legacy-fixture-source.txt");
        if (!File.Exists(legacySource)) throw new InvalidDataException("Explicit prior fixture package path missing");
        string oldRoot = File.ReadAllText(legacySource);
        var oldManifest = Contract.Read<Package>(File.ReadAllBytes(Paths.Contained(oldRoot, "package-manifest.json")));
        Check(UpdatePolicy.VersionOf(oldManifest.Version) < UpdatePolicy.VersionOf(manifest.Version), "prior package version precedes candidate");
        foreach (var f in oldManifest.Files) Paths.WriteNew(Paths.Contained(legacyPackage, "payload/" + f.Path), File.ReadAllBytes(Paths.Contained(oldRoot, "payload/" + f.Path)));
        var old = oldManifest with { GameExecutableSha256 = manifest.GameExecutableSha256,
            Native = oldManifest.Native! with { GameFiles = manifest.Native!.GameFiles } };
        Paths.WriteNew(Paths.Contained(legacyPackage, "package-manifest.json"), Contract.Encode(old));
        engine.Remove(root, engine.ReceiptHash(root));
        engine.Install(engine.Preview(root, legacyPackage));
        string oldExeHash = Paths.FileHash(guiExe)!;
        engine.Install(engine.Preview(root, pkg)); Check(engine.Inspect(root)!.Version == manifest.Version, "same-layout desktop update");
        var removed = engine.Remove(root, engine.ReceiptHash(root));
        var restored = engine.Inspect(root)!;
        Check(removed.RestoredPrevious && restored.Version == oldManifest.Version && Paths.Equal(Paths.FileHash(guiExe), oldExeHash), "previous host exact restoration");
        foreach (var f in restored.Files)
            Check(Paths.Equal(Paths.FileHash(Paths.Contained(root, f.Path)), f.Sha256), "previous receipt bytes restored " + f.Path);
        engine.Remove(root, engine.ReceiptHash(root));
        foreach (var f in receipt.Files) Check(!File.Exists(Paths.Contained(root, f.Path)), "private deployment remove " + f.Path);
        Check(contract.Length != 0, "bound contract retained in journal evidence");
        File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory, "desktop-deployment-validation.json"), Contract.Encode(new {
            passed = true, checks, fixture, previousVersion = oldManifest.Version, candidateVersion = manifest.Version,
            candidateBuild = manifest.Native!.BuildId, installedConfiguratorHostTrace = trace, sameLayoutMigration = true,
            nativeDllsLoaded = 0, gameLaunches = 0, actualInstallationWrites = 0, privateConfiguratorReadOnlyRuns = 1 }));
        return 0;
    }
}
