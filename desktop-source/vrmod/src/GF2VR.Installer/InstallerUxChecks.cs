using System.Text;
using System.Text.Json.Nodes;

namespace GF2VR.Installer;

internal static class InstallerUxChecks
{
    private static int checks;
    private static void Check(bool ok, string note) { ++checks; if (!ok) throw new InvalidDataException(note); }
    private static string Game(string root)
    {
        Directory.CreateDirectory(Path.Combine(root, "GF2 Game")); File.WriteAllText(Path.Combine(root, "GF2 Game", "GF2_Exilium.exe"), SelfTest.Inert);
        File.WriteAllText(Path.Combine(root, "GF2 Game", "version.dll"), "INERT NATIVE; NEVER EXECUTE"); return root;
    }
    private static byte[] GuiContract(string root) => Contract.Encode(new { schemaVersion = 1, buildId = "INERT-UX", dllSha256 = Paths.FileHash(Paths.Contained(root, "GF2 Game/version.dll")), configurationRoot = Path.Combine(root, "vrmod", "config"), files = new[] { "navigation.json" }, apply = "GAME_RESTART" });
    private static string PackageAt(string path, string root, string version)
    {
        SelfTest.CreatePackage(path, root, version); string manifest = Path.Combine(path, "package-manifest.json");
        Package p = Contract.Read<Package>(File.ReadAllBytes(manifest)); string contract = "vrmod/bin/GF2VR.Configurator/runtime-contract.json";
        byte[] bytes = GuiContract(root); Paths.Atomic(Paths.Contained(path, "payload/" + contract), bytes);
        p = p with { BoundGameRoot = root, Files = p.Files.Select(f => f.Path == contract ? f with { Sha256 = Paths.Hash(bytes), Bytes = bytes.Length } : f).ToArray() };
        Paths.Atomic(manifest, Contract.Encode(p)); return path;
    }
    internal static int Run()
    {
        string fixture = Path.Combine(AppContext.BaseDirectory, "ux-validation-fixture");
        if (Directory.Exists(fixture)) throw new InvalidDataException("Use a fresh UX fixture directory");
        var engine = new InstallationEngine(); string root = Game(Path.Combine(fixture, "managed"));
        string config = Paths.Contained(root, "vrmod/config/navigation.json"); byte[] preference = Encoding.UTF8.GetBytes("{\"schemaVersion\":1,\"navigation\":{\"cameraFollowEnabled\":false}}"); Paths.WriteNew(config, preference);
        string one = PackageAt(Path.Combine(fixture, "package1"), root, "0.1.0"), two = PackageAt(Path.Combine(fixture, "package2"), root, "0.2.0");
        Check(SettingsLaunch.Resolve(root, engine) == null && !engine.HasPreviousVersion(root), "uninstalled settings must not launch");
        engine.Install(engine.Preview(root, one)); Check(!engine.HasPreviousVersion(root), "first install advertises rollback");
        var target = SettingsLaunch.Resolve(root, engine); Check(target != null, "valid receipt GUI unresolved");
        int starts = 0; var launch = SettingsLaunch.Start;
        try
        {
            SettingsLaunch.Start = info => { ++starts; Check(info.FileName == target!.Executable && !info.UseShellExecute && info.ArgumentList.SequenceEqual(new[] { "--root", root }), "settings launch arguments"); };
            SettingsLaunch.Open(target!, engine); Check(starts == 1, "launch transport absent");
            string dll = Paths.Contained(root, "vrmod/bin/GF2VR.Configurator/GF2VR.Configurator.dll"); byte[] original = File.ReadAllBytes(dll); File.WriteAllText(dll, "MODIFIED");
            Check(SettingsLaunch.Resolve(root, engine) == null, "modified GUI enabled");
            bool refused = false; try { SettingsLaunch.Open(target!, engine); } catch (InstallError) { refused = true; }
            Check(refused && starts == 1, "stale click launched changed GUI"); File.WriteAllBytes(dll, original);
        }
        finally { SettingsLaunch.Start = launch; }
        engine.Install(engine.Preview(root, two)); Check(engine.HasPreviousVersion(root), "upgrade rollback missing");
        Check(engine.Remove(root, engine.ReceiptHash(root)).RestoredPrevious && !engine.HasPreviousVersion(root), "rollback label differs from actual Remove");
        Check(SettingsLaunch.Resolve(root, engine) != null, "restored GUI unresolved"); engine.Remove(root, engine.ReceiptHash(root));
        Check(SettingsLaunch.Resolve(root, engine) == null && !engine.HasPreviousVersion(root), "uninstalled GUI enabled");
        Check(File.ReadAllBytes(config).SequenceEqual(preference), "settings choice changed");
        engine.Install(engine.Preview(root, one)); engine.Install(engine.Preview(root, two));
        string pending = Paths.Contained(root, Contract.PendingPath); Paths.WriteNew(pending, Contract.Encode(new { schemaVersion = 1, transactionId = "INERT" }));
        Check(SettingsLaunch.Resolve(root, engine) == null, "pending recovery allowed settings launch"); File.Delete(pending);
        string workspace = Game(Path.Combine(fixture, "workspace")); const string prefix = "vrmod/build/settings/INERT-GUI/";
        var hashes = new Dictionary<string, string>();
        foreach (string name in Paths.ToolFiles)
        {
            byte[] bytes = name == "runtime-contract.json" ? GuiContract(workspace) : Encoding.UTF8.GetBytes("INERT GUI " + name);
            Paths.WriteNew(Paths.Contained(workspace, prefix + "app/" + name), bytes); hashes[name] = Paths.Hash(bytes);
        }
        string launcher = Paths.Contained(workspace, "vrmod/tools/open_settings.cmd"); byte[] command = Encoding.UTF8.GetBytes("@echo off\r\nset \"GF2_SETTINGS_APP=%GF2_SETTINGS_ROOT%\\vrmod\\build\\settings\\INERT-GUI\\app\\GF2VR.Configurator.exe\"\r\n"); Paths.WriteNew(launcher, command);
        string? native = Paths.FileHash(Paths.Contained(workspace, "GF2 Game/version.dll"));
        string owner = Paths.Contained(workspace, prefix + "owned-launcher.json"), ready = Paths.Contained(workspace, prefix + "gui-ready.json");
        byte[] ownerBytes = Contract.Encode(new { build = "INERT-GUI", state = "INSTALLED", native, launcher = Paths.Hash(command) }); Paths.WriteNew(owner, ownerBytes);
        Paths.WriteNew(ready, Contract.Encode(new { native, launcher = Paths.Hash(command), payload = hashes }));
        Check(SettingsLaunch.Resolve(workspace, engine)?.Root == workspace, "owned workspace GUI unresolved");
        File.AppendAllText(launcher, "REM CHANGED"); Check(SettingsLaunch.Resolve(workspace, engine) == null, "changed launcher accepted"); File.WriteAllBytes(launcher, command);
        var changed = JsonNode.Parse(ownerBytes)!.AsObject(); changed["state"] = "RESTORED"; File.WriteAllBytes(owner, Contract.Encode(changed)); Check(SettingsLaunch.Resolve(workspace, engine) == null, "restored owner accepted"); File.WriteAllBytes(owner, ownerBytes);
        string nativePath = Paths.Contained(workspace, "GF2 Game/version.dll"); byte[] dllBefore = File.ReadAllBytes(nativePath); File.WriteAllText(nativePath, "OTHER"); Check(SettingsLaunch.Resolve(workspace, engine) == null, "other runtime accepted"); File.WriteAllBytes(nativePath, dllBefore);
        var known = SettingsLaunch.Resolve(workspace, engine)!;
        try { SettingsLaunch.Start = info => { ++starts; Check(info.FileName == known.Executable && info.ArgumentList.SequenceEqual(new[] { "--root", workspace }), "workspace arguments"); }; SettingsLaunch.Open(known, engine); }
        finally { SettingsLaunch.Start = launch; }
        Check(starts == 2, "workspace launch transport absent");
        string fresh = Game(Path.Combine(fixture, "fresh")), freshPackage = PackageAt(Path.Combine(fixture, "fresh-package"), fresh, "0.1.0");
        string update = Game(Path.Combine(fixture, "update")), updateOne = PackageAt(Path.Combine(fixture, "update-one"), update, "0.1.0"), updateTwo = PackageAt(Path.Combine(fixture, "update-two"), update, "0.2.0"); engine.Install(engine.Preview(update, updateOne));
        File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory, "installer-ux-validation.json"), Contract.Encode(new { passed = true, checks, launchedProcesses = 0, liveInstallationWrites = 0, managedRoot = root, managedPackage = two, freshRoot = fresh, freshPackage, updateRoot = update, updatePackage = updateTwo, workspaceRoot = workspace })); return 0;
    }
}
