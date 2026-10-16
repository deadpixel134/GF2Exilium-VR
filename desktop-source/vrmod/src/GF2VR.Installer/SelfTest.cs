using System.Diagnostics;
using System.Text;
using System.Text.Json.Nodes;

namespace GF2VR.Installer;

internal static class SelfTest
{
    private static int checks;
    internal const string Inert = "INERT INSTALLER FIXTURE — NEVER EXECUTE AS A GAME";
    private static void Assert(bool result, string message) { ++checks; if (!result) throw new InvalidDataException(message); }
    private static void Reject(Action work, string message)
    {
        ++checks; try { work(); } catch (InstallError) { return; }
        throw new InvalidDataException("Expected rejection: " + message);
    }
    private static string CreateGame(string folder)
    {
        Directory.CreateDirectory(Path.Combine(folder, "GF2 Game"));
        File.WriteAllText(Path.Combine(folder, "GF2 Game", "GF2_Exilium.exe"), Inert);
        return folder;
    }
    internal static string CreatePackage(string folder, string root, string version, string scope = "tools")
    {
        var files = new List<PayloadFile>();
        IEnumerable<string> paths = scope == "tools" ? Paths.ToolFiles.Select(f => "vrmod/bin/GF2VR.Configurator/" + f) : ["GF2 Game/version.dll", "vrmod/runtime/product/bridge.dll"];
        foreach (string path in paths)
        {
            byte[] bytes = Encoding.UTF8.GetBytes("INERT FIXTURE PAYLOAD " + version + " " + path);
            Paths.WriteNew(Paths.Contained(folder, "payload/" + path), bytes); files.Add(new(path, Paths.Hash(bytes), bytes.Length));
        }
        var package = new Package(1, Contract.Product, version, scope, Paths.FileHash(Path.Combine(root, "GF2 Game", "GF2_Exilium.exe"))!,
            scope == "vrmod" ? root : null, null, files.ToArray());
        Paths.WriteNew(Paths.Contained(folder, "package-manifest.json"), Contract.Encode(package));
        return folder;
    }
    private static void ReplaceManifest(string folder, Package manifest)
        => Paths.Atomic(Paths.Contained(folder, "package-manifest.json"), Contract.Encode(manifest));
    internal static int Run(string? actualPackage = null)
    {
        InstallerText.Verify();
        string fixture = Path.Combine(AppContext.BaseDirectory, "validation-fixture");
        if (Directory.Exists(fixture)) throw new InvalidDataException("Use a fresh validation-fixture directory.");
        string root = CreateGame(Path.Combine(fixture, "game"));
        string one = CreatePackage(Path.Combine(fixture, "package1"), root, "0.1.0");
        string two = CreatePackage(Path.Combine(fixture, "package2"), root, "0.2.0");
        string cfg = Paths.Contained(root, "vrmod/config/navigation.json");
        Paths.WriteNew(cfg, Encoding.UTF8.GetBytes("PRESERVE ALL USER CONFIG BYTES"));
        string foreign = Paths.Contained(root, "GF2 Game/other-mod.dll");
        Paths.WriteNew(foreign, Encoding.UTF8.GetBytes("UNRELATED MOD"));
        string cfgHash = Paths.FileHash(cfg)!; string otherHash = Paths.FileHash(foreign)!;
        var engine = new InstallationEngine(); var preview = engine.Preview(root, one);
        Assert(!File.Exists(Paths.Contained(root, Contract.StatePath)), "preview created ownership");
        Assert(!Directory.Exists(Path.Combine(root, "vrmod", "installer")), "preview wrote state directory");
        var runningBefore = InstallationEngine.GameRunning; InstallationEngine.GameRunning = () => true;
        Reject(() => engine.Install(preview), "running-game install"); InstallationEngine.GameRunning = runningBefore;
        Assert(!File.Exists(Paths.Contained(root, preview.Package.Files[0].Path)), "guard created product file");
        engine.Install(preview); var owned = engine.Inspect(root)!;
        Assert(owned.Version == "0.1.0", "clean install receipt");
        foreach (var f in owned.Files) Assert(Paths.Equal(Paths.FileHash(Paths.Contained(root, f.Path)), f.Sha256), "clean installed hash");
        Assert(Paths.Equal(Paths.FileHash(cfg), cfgHash) && Paths.Equal(Paths.FileHash(foreign), otherHash), "settings/foreign preservation");
        string first = Paths.Contained(root, owned.Files[0].Path);
        byte[] good = File.ReadAllBytes(first); File.AppendAllText(first, "USER MODIFIED");
        Reject(() => engine.Preview(root, two), "modified update");
        Reject(() => engine.Remove(root, engine.ReceiptHash(root)), "modified uninstall");
        Assert(File.ReadAllText(first).EndsWith("USER MODIFIED"), "modified file overwritten"); File.WriteAllBytes(first, good);
        // Real file-sharing conflict on the third destination; earlier new files must be rolled back.
        byte[] stateBefore = File.ReadAllBytes(Paths.Contained(root, Contract.StatePath));
        var oldHashes = owned.Files.ToDictionary(f => f.Path, f => Paths.FileHash(Paths.Contained(root, f.Path)));
        using (var locked = new FileStream(Paths.Contained(root, owned.Files[2].Path), FileMode.Open, FileAccess.Read, FileShare.Read))
        {
            bool failed = false; try { engine.Install(engine.Preview(root, two)); } catch (IOException) { failed = true; }
            Assert(failed, "locked third file did not fail");
        }
        foreach (var f in owned.Files) Assert(Paths.Equal(Paths.FileHash(Paths.Contained(root, f.Path)), oldHashes[f.Path]), "partial update rollback " + f.Path);
        Assert(stateBefore.SequenceEqual(File.ReadAllBytes(Paths.Contained(root, Contract.StatePath))), "partial update changed receipt");
        Assert(!engine.HasPending(root), "successful rollback left pending authority");
        engine.Install(engine.Preview(root, two));
        Assert(engine.Inspect(root)!.Version == "0.2.0", "update receipt");
        var updated = engine.Inspect(root)!;
        string backup = Paths.Contained(Paths.Transaction(root, updated.TransactionId), "files/0.before");
        byte[] backupGood = File.ReadAllBytes(backup); File.AppendAllText(backup, "BAD");
        Reject(() => engine.Remove(root, engine.ReceiptHash(root)), "corrupt restore backup");
        Assert(engine.Inspect(root)!.Version == "0.2.0", "corrupt backup changed current receipt"); File.WriteAllBytes(backup, backupGood);
        InstallationEngine.GameRunning = () => true;
        Reject(() => engine.Remove(root, engine.ReceiptHash(root)), "running-game remove"); InstallationEngine.GameRunning = runningBefore;
        var removal = engine.Remove(root, engine.ReceiptHash(root));
        Assert(removal.RestoredPrevious && engine.Inspect(root)!.Version == "0.1.0", "previous version restoration");
        foreach (var f in owned.Files) Assert(Paths.Equal(Paths.FileHash(Paths.Contained(root, f.Path)), f.Sha256), "restored prior hash");
        var clean = engine.Remove(root, engine.ReceiptHash(root));
        Assert(!clean.RestoredPrevious && engine.Inspect(root) == null, "first version removal");
        foreach (var f in owned.Files) Assert(!File.Exists(Paths.Contained(root, f.Path)), "owned file remains after remove");
        Assert(Paths.Equal(Paths.FileHash(cfg), cfgHash) && Paths.Equal(Paths.FileHash(foreign), otherHash), "remove changed settings/foreign");
        Reject(() => engine.Remove(root, ""), "unowned uninstall");
        Paths.WriteNew(first, Encoding.UTF8.GetBytes("FOREIGN EVEN IF SAME FILENAME"));
        Reject(() => engine.Preview(root, one), "unowned existing file");
        Assert(File.ReadAllText(first).StartsWith("FOREIGN"), "unowned file changed"); File.Delete(first);
        var manifest = Contract.Read<Package>(File.ReadAllBytes(Path.Combine(one, "package-manifest.json")));
        foreach (string bad in new[] { "../outside.dll", "GF2 Game/../version.dll", "GF2 Game/CON.dll", "GF2 Game/version.dll.", "vrmod/config/navigation.json", "GF2 Game/UnityPlayer.dll", "GF2 Game/NEP2.dll", "vrmod/bin/GF2VR.Configurator/../../config/a.json" })
        {
            ReplaceManifest(one, manifest with { Files = [manifest.Files[0] with { Path = bad }] });
            Reject(() => engine.Preview(root, one), "forbidden path " + bad);
        }
        ReplaceManifest(one, manifest with { Files = [manifest.Files[0], manifest.Files[0]] }); Reject(() => engine.Preview(root, one), "duplicate target");
        ReplaceManifest(one, manifest with { Files = manifest.Files[..^1] }); Reject(() => engine.Preview(root, one), "incomplete tool package");
        ReplaceManifest(one, manifest with { GameExecutableSha256 = new string('0', 64) });
        Assert(engine.Preview(root, one).Root == root, "reference game fingerprint is nonblocking");
        ReplaceManifest(one, manifest with { BoundGameRoot = fixture }); Reject(() => engine.Preview(root, one), "wrong absolute binding");
        ReplaceManifest(one, manifest with { RequiresInstalledDllSha256 = new string('0', 64) }); Reject(() => engine.Preview(root, one), "required native build missing");
        ReplaceManifest(one, manifest);
        string payloadFirst = Paths.Contained(one, "payload/" + manifest.Files[0].Path); byte[] payloadGood = File.ReadAllBytes(payloadFirst);
        File.AppendAllText(payloadFirst, "CORRUPT"); Reject(() => engine.Preview(root, one), "payload tamper"); File.WriteAllBytes(payloadFirst, payloadGood);
        var stale = engine.Preview(root, one); ReplaceManifest(one, manifest with { Version = "0.1.1" });
        Reject(() => engine.Install(stale), "stale manifest preview"); ReplaceManifest(one, manifest);
        File.WriteAllText(Path.Combine(one, "package-manifest.json"), "{\"schemaVersion\":1,\"schemaVersion\":1}");
        Reject(() => engine.Preview(root, one), "duplicate manifest property"); ReplaceManifest(one, manifest);
        // Native fixture is explicitly non-executable, proving loader coexistence/ownership only.
        string nativeRoot = CreateGame(Path.Combine(fixture, "native-game"));
        string native = CreatePackage(Path.Combine(fixture, "native-package"), nativeRoot, "0.1.0", "vrmod");
        Reject(() => engine.Preview(nativeRoot, native), "incomplete native package without profile");
        var nativeManifest = Contract.Read<Package>(File.ReadAllBytes(Path.Combine(native, "package-manifest.json")));
        ReplaceManifest(native, nativeManifest with { BoundGameRoot = null }); Reject(() => engine.Preview(nativeRoot, native), "unproved portability");
        ReplaceManifest(native, nativeManifest);
        // A real installer child exits between file replacements, preserving on-disk pending authority.
        string crashRoot = CreateGame(Path.Combine(fixture, "crash-game"));
        string crashPackage = CreatePackage(Path.Combine(fixture, "crash-package"), crashRoot, "0.1.0");
        var psi = new ProcessStartInfo(Path.Combine(AppContext.BaseDirectory, "GF2VR.Installer.exe")) { UseShellExecute = false, CreateNoWindow = true, WindowStyle = ProcessWindowStyle.Hidden };
        foreach (string arg in new[] { "--interrupt-fixture", "--root", crashRoot, "--package", crashPackage }) psi.ArgumentList.Add(arg);
        using var child = Process.Start(psi)!; if (!child.WaitForExit(30000)) throw new InvalidDataException("Fixture child timeout");
        Assert(child.ExitCode == 81 && engine.HasPending(crashRoot), "real interruption journal missing");
        Reject(() => engine.Preview(crashRoot, crashPackage), "pending install blocked");
        engine.Recover(crashRoot);
        Assert(!engine.HasPending(crashRoot) && engine.Inspect(crashRoot) == null, "interrupted receipt recovery");
        foreach (string file in Paths.ToolFiles) Assert(!File.Exists(Paths.Contained(crashRoot, "vrmod/bin/GF2VR.Configurator/" + file)), "interrupted payload not restored");
        // Repeat a real interrupted transaction, then prove foreign changes block destructive recovery.
        using (var externalChild = Process.Start(psi)!)
        {
            if (!externalChild.WaitForExit(30000)) throw new InvalidDataException("Fixture child timeout");
            Assert(externalChild.ExitCode == 81, "second interruption did not occur");
        }
        string conflicted = Paths.Contained(crashRoot, "vrmod/bin/GF2VR.Configurator/" + Paths.ToolFiles[0]);
        byte[] expectedCrash = File.ReadAllBytes(conflicted); File.AppendAllText(conflicted, "EXTERNAL CHANGE");
        string crashSecond = Paths.Contained(crashRoot, "vrmod/bin/GF2VR.Configurator/" + Paths.ToolFiles[1]);
        string crashSecondHash = Paths.FileHash(crashSecond)!;
        Reject(() => engine.Recover(crashRoot), "recovery refuses externally changed file");
        Assert(engine.HasPending(crashRoot) && File.ReadAllText(conflicted).EndsWith("EXTERNAL CHANGE") &&
            Paths.Equal(Paths.FileHash(crashSecond), crashSecondHash), "recovery preflight changed an unrelated/foreign file");
        File.WriteAllBytes(conflicted, expectedCrash); engine.Recover(crashRoot);
        Assert(!engine.HasPending(crashRoot), "resolved conflict recovery failed");
        if (actualPackage != null)
        {
            // Actual Configurator payload, copied into a distinct fixture manifest. Never execute a fixture game.
            string realRoot = CreateGame(Path.Combine(fixture, "real-payload-game"));
            string localPackage = Path.Combine(fixture, "real-payload-package");
            var actual = Contract.Read<Package>(Paths.Bytes(Paths.Contained(actualPackage, "package-manifest.json"), 262144)!);
            if (actual.Scope != "tools" || !actual.Files.Select(f => f.Path).ToHashSet().SetEquals(Paths.ToolFiles.Select(f => "vrmod/bin/GF2VR.Configurator/" + f)))
                throw new InvalidDataException("Expected actual five-file Configurator package");
            foreach (var f in actual.Files)
            {
                byte[] bytes = Paths.Bytes(Paths.Contained(actualPackage, "payload/" + f.Path))!;
                Assert(bytes.Length == f.Bytes && Paths.Equal(Paths.Hash(bytes), f.Sha256), "actual packaged settings hash");
                Paths.WriteNew(Paths.Contained(localPackage, "payload/" + f.Path), bytes);
            }
            var local = actual with { GameExecutableSha256 = Paths.FileHash(Paths.Contained(realRoot, "GF2 Game/GF2_Exilium.exe"))!, BoundGameRoot = realRoot, RequiresInstalledDllSha256 = null };
            Paths.WriteNew(Paths.Contained(localPackage, "package-manifest.json"), Contract.Encode(local));
            engine.Install(engine.Preview(realRoot, localPackage));
            foreach (var f in actual.Files) Assert(Paths.Equal(Paths.FileHash(Paths.Contained(realRoot, f.Path)), f.Sha256), "actual installed settings hash");
            engine.Remove(realRoot, engine.ReceiptHash(realRoot));
            foreach (var f in actual.Files) Assert(!File.Exists(Paths.Contained(realRoot, f.Path)), "actual settings removal");
        }
        foreach (string lang in InstallerText.Languages)
        {
            InstallerText.SavePreference(root, lang); Assert(InstallerText.LoadPreference(root) == lang, "locale persistence " + lang);
            InstallerText.Language = lang; Assert(InstallerText.Get("installed").Length > 0, "packaged locale " + lang);
        }
        string preference = Paths.Contained(root, "vrmod/config/installer-ui.json");
        File.WriteAllText(preference, "{\"schemaVersion\":1,\"language\":\"future\",\"retain\":42}");
        Assert(InstallerText.LoadPreference(root) == "ko", "locale fallback"); InstallerText.SavePreference(root, "ja");
        Assert(JsonNode.Parse(File.ReadAllText(preference))!["retain"]!.GetValue<int>() == 42, "preference preservation");
        byte[] preferenceFuture = Encoding.UTF8.GetBytes("{\"schemaVersion\":2,\"language\":\"ko\"}"); File.WriteAllBytes(preference, preferenceFuture);
        Reject(() => InstallerText.SavePreference(root, "en"), "future locale preference");
        Assert(preferenceFuture.SequenceEqual(File.ReadAllBytes(preference)), "future preference overwritten");
        File.WriteAllText(preference, "{\"schemaVersion\":1,\"schemaVersion\":1,\"language\":\"ko\"}");
        Reject(() => InstallerText.SavePreference(root, "en"), "duplicate locale preference");
        File.WriteAllText(preference, "{\"schemaVersion\":1,\"language\":\"ko\"}");
        InstallerText.Language = "future"; Assert(InstallerText.Get("title") == "GF2 VR 설치 및 관리", "unknown locale lookup");
        Assert(InstallerText.Get("unrecorded-key") == InstallerText.Get("unavailable"), "missing key fallback");
        InstallerText.SavePreference(root, "ko");
        File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory, "installer-validation.json"), Contract.Encode(new
        {
            passed = true, checks, fixture, locales = InstallerText.Languages, realInstallationWrites = 0, gameLaunches = 0,
            actualConfiguratorPayloadFixture = actualPackage != null,
            interruption = "Actual installer child exits81 between replacements; durable pending recovery succeeds",
            scope = "Inert loader/package fixtures; no clean-game/HMD native portability claim"
        }));
        return 0;
    }
    internal static void InterruptFixture(string root, string package)
    {
        string allowed = Path.Combine(AppContext.BaseDirectory, "validation-fixture", "crash-game");
        if (!Path.GetFullPath(root).Equals(allowed, StringComparison.OrdinalIgnoreCase) ||
            File.ReadAllText(Path.Combine(root, "GF2 Game", "GF2_Exilium.exe")) != Inert)
            throw new InstallError("path", root);
        var engine = new InstallationEngine(); int files = 0;
        engine.Install(engine.Preview(root, package), _ => { if (++files == 3) Environment.Exit(81); });
        throw new InvalidDataException("Fixture did not interrupt");
    }
}
