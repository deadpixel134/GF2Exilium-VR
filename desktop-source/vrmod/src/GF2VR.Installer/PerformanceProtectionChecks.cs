using System.Text;
using System.Text.Json;

namespace GF2VR.Installer;

internal static class PerformanceProtectionChecks
{
    internal static int Run(string actualPackage)
    {
        int checks = 0;
        void Check(bool ok, string label) { ++checks; if (!ok) throw new InvalidDataException(label); }
        void Reject(Action work, string label) { ++checks; try { work(); } catch (InstallError) { return; } throw new InvalidDataException("Expected refusal: " + label); }
        string fixture = Path.Combine(AppContext.BaseDirectory, "performance-protection-fixture");
        if (Directory.Exists(fixture)) throw new InvalidDataException("Preserve previous protection fixture");
        var source = Contract.Read<Package>(File.ReadAllBytes(Paths.Contained(actualPackage, "package-manifest.json")));
        string first = Path.Combine(fixture, "first"), next = Path.Combine(fixture, "next");
        void Package(string folder, Package manifest)
        {
            foreach (var f in source.Files)
            {
                byte[] bytes = File.ReadAllBytes(Paths.Contained(actualPackage, "payload/" + f.Path));
                Check(Paths.Equal(Paths.Hash(bytes), f.Sha256), "verified product input " + f.Path);
                Paths.WriteNew(Paths.Contained(folder, "payload/" + f.Path), bytes);
            }
            Paths.WriteNew(Paths.Contained(folder, "package-manifest.json"), Contract.Encode(manifest));
        }
        Package(first, source with { UserSettings = UserSettings.Policies });
        var version = Version.Parse(source.Version);
        Package(next, source with { Version = new Version(version.Major, version.Minor, version.Build + 1).ToString(3), UserSettings = UserSettings.Policies });
        string Game(string name)
        {
            string root = Path.Combine(fixture, name);
            Paths.WriteNew(Paths.Contained(root, "GF2 Game/GF2_Exilium.exe"), Encoding.UTF8.GetBytes(SelfTest.Inert));
            return root;
        }
        var engine = new InstallationEngine();
        string missing = Game("missing");
        string file = Paths.Contained(missing, UserSettings.PerformancePath);
        _ = engine.Preview(missing, first);
        Check(!File.Exists(file), "preview never creates settings");
        engine.Install(engine.Preview(missing, first));
        using (var json = JsonDocument.Parse(File.ReadAllBytes(file)))
        {
            var pc = json.RootElement.GetProperty("pcOutput");
            Check(pc.GetProperty("mode").GetString() == "NATIVE" && pc.GetProperty("scope").GetString() == "IMMERSIVE_ONLY", "missing creates native/immersive defaults");
            Check(!json.RootElement.TryGetProperty("dynamicEyeScale", out _), "default leaves dynamic absent/OFF");
        }
        var installed = engine.Inspect(missing)!;
        Check(installed.Files.All(f => f.Path != UserSettings.PerformancePath), "settings excluded from owned hashes");
        Check(!File.ReadAllText(Paths.Contained(missing, Contract.StatePath)).Contains("userSettings", StringComparison.Ordinal), "product receipt schema stays compatible with installed GUI");
        var protection = Contract.Read<SettingsProtectionReceipt>(File.ReadAllBytes(Paths.Contained(missing, UserSettings.ReceiptPath)));
        Check(protection.UserSettings[0].Disposition == "CREATED_DEFAULT" && !protection.UserSettings[0].Warning, "creation receipt");
        byte[] defaults = File.ReadAllBytes(file);
        engine.Remove(missing, engine.ReceiptHash(missing));
        Check(File.ReadAllBytes(file).SequenceEqual(defaults), "remove preserves installer-created user default");

        byte[] opaque = Encoding.UTF8.GetBytes("\uFEFF{\r\n \"schemaVersion\":77, \"pcOutput\":{\"mode\":\"FUTURE\",\"width\":\"opaque\",\"renderScale\":0.731,\"future\":[null,1]},\"dynamicEyeScale\":{\"enabled\":false,\"future\":{\"all\":[true,9]}},\"unknownRoot\":42}\r\n");
        foreach (var item in new[] { (Name: "unknown", Bytes: opaque, Warning: false), (Name: "malformed", Bytes: Encoding.UTF8.GetBytes("{ BROKEN original\r\n"), Warning: true) })
        {
            string root = Game(item.Name), config = Paths.Contained(root, UserSettings.PerformancePath);
            Paths.WriteNew(config, item.Bytes);
            var install = engine.Install(engine.Preview(root, first));
            Check(File.ReadAllBytes(config).SequenceEqual(item.Bytes), item.Name + " install byte preservation");
            Check(install.SettingsWarning == item.Warning, item.Name + " syntax-only warning");
            engine.Install(engine.Preview(root, next));
            Check(File.ReadAllBytes(config).SequenceEqual(item.Bytes), item.Name + " update byte preservation");
            engine.Remove(root, engine.ReceiptHash(root));
            Check(File.ReadAllBytes(config).SequenceEqual(item.Bytes), item.Name + " rollback byte preservation");
            engine.Remove(root, engine.ReceiptHash(root));
            Check(File.ReadAllBytes(config).SequenceEqual(item.Bytes), item.Name + " final remove preservation");
            var record = Contract.Read<SettingsProtectionReceipt>(File.ReadAllBytes(Paths.Contained(root, UserSettings.ReceiptPath)));
            Check(record.Operation == "remove" && record.UserSettings[0].Warning == item.Warning, item.Name + " remove receipt");
        }

        string reverify = Game("reverify"); engine.Install(engine.Preview(reverify, first));
        string cfg = Paths.Contained(reverify, UserSettings.PerformancePath);
        File.WriteAllBytes(cfg, opaque);
        string dll = Paths.Contained(reverify, "GF2 Game/version.dll");
        File.AppendAllText(dll, "INERT PRIVATE OVERRIDE");
        var beforeFiles = engine.Inspect(reverify)!.Files.ToDictionary(f => f.Path, f => Paths.FileHash(Paths.Target(reverify, f.Path)));
        string priorReceipt = engine.ReceiptHash(reverify);
        engine.ReverifySettings(reverify);
        Check(engine.ReceiptHash(reverify) == priorReceipt, "reverify leaves product receipt unchanged");
        Check(beforeFiles.All(f => Paths.Equal(f.Value, Paths.FileHash(Paths.Target(reverify, f.Key)))), "reverify cannot replace changed product files");
        Check(File.ReadAllBytes(cfg).SequenceEqual(opaque), "reverify existing opaque file exact");
        Reject(() => engine.Preview(reverify, next), "normal update still refuses modified native");
        var gameRunning = InstallationEngine.GameRunning;
        try { InstallationEngine.GameRunning = () => true; Reject(() => engine.ReverifySettings(reverify), "running game guard"); }
        finally { InstallationEngine.GameRunning = gameRunning; }
        File.Delete(cfg); engine.ReverifySettings(reverify);
        Check(File.ReadAllBytes(cfg).SequenceEqual(UserSettings.DefaultBytes), "reverify creates only missing default");

        string noReceipt = Game("no-receipt");
        Reject(() => engine.ReverifySettings(noReceipt), "no receipt cannot claim installed protection");
        Check(!File.Exists(Paths.Contained(noReceipt, UserSettings.PerformancePath)), "refused reverify creates no setting");
        UserSettings.ValidatePolicy(null); Check(true, "legacy missing policy accepted");
        foreach (var policy in new[] { new UserSettingPolicy("vrmod/config/../outside", "PRESERVE_EXISTING_CREATE_IF_MISSING", "USER_DATA_NO_HASH"), UserSettings.Policies[0] with { Policy = "OVERWRITE" }, UserSettings.Policies[0] with { Integrity = "PAYLOAD_HASH" } })
            Reject(() => UserSettings.ValidatePolicy([policy]), "invalid policy cannot grant ownership");
        var valid = engine.CheckPayload(missing, first);
        var tampered = valid.Package with { Files = [.. valid.Package.Files, new PayloadFile(UserSettings.PerformancePath, new string('0', 64), 1)] };
        string mf = Paths.Contained(first, "package-manifest.json"); byte[] saved = File.ReadAllBytes(mf);
        File.WriteAllBytes(mf, Contract.Encode(tampered));
        Reject(() => engine.CheckPayload(missing, first), "user data cannot become hash-owned payload");
        File.WriteAllBytes(mf, saved);
        foreach (string locale in InstallerText.Languages)
        {
            InstallerText.Language = locale;
            Check(InstallerText.Get("settingsPreservedWarning").Contains("performance.json"), "warning locale " + locale);
            Check(InstallerText.Get("settingsMaintenanceHelp").Contains("NATIVE"), "default help locale " + locale);
        }
        InstallerText.Verify();
        File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory, "performance-protection-validation.json"), Contract.Encode(new { passed = true, checks, fixture, nativeDllsLoaded = 0, gameLaunches = 0, guiBuilds = 0, liveWrites = 0 }));
        return 0;
    }
}
