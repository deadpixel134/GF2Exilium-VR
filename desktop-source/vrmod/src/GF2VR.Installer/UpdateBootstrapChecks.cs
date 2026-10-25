using System.Diagnostics;
using System.IO.Compression;
using System.Text;
using System.Text.Json.Nodes;

namespace GF2VR.Installer;

internal static class UpdateBootstrapChecks
{
    internal static int Parent(string folder)
    {
        folder = Path.GetFullPath(folder);
        var prepared = Contract.Read<PreparedUpdate>(File.ReadAllBytes(Paths.Contained(folder, "bootstrap-parent.json")));
        if (!prepared.Root.Contains("updater-validation-fixture", StringComparison.Ordinal) ||
            File.ReadAllText(Paths.Contained(prepared.Root, "GF2 Game/GF2_Exilium.exe")) != SelfTest.Inert ||
            !Path.GetFullPath(prepared.Folder).Equals(folder, StringComparison.OrdinalIgnoreCase)) throw new InstallError("path", prepared.Root);
        UpdateStaging.Handoff(prepared); // Actual production handoff; this parent exits immediately.
        return 0;
    }

    internal static int Run(string candidate, string previous, string futureHelper)
    {
        int checks = 0;
        var cases = new List<object>();
        void Check(bool value, string why) { ++checks; if (!value) throw new InvalidDataException(why); }
        void Reject(Action action, string why)
        {
            ++checks;
            try { action(); } catch (Exception e) when (e is InstallError or InvalidDataException) { cases.Add(new { name = why, rejected = true }); return; }
            throw new InvalidDataException("Expected rejection: " + why);
        }
        // Preserve the real current-format install/rollback and all prior checks.
        Check(UpdateChecks.Run(candidate, previous) == 0, "existing current package update/rollback suite");
        string fixture = Path.Combine(AppContext.BaseDirectory, "updater-validation-fixture");
        string root = Path.Combine(fixture, "게임"), current = Path.Combine(fixture, "next");
        var engine = new InstallationEngine();
        engine.Install(engine.Preview(root, current));
        var prior = engine.Inspect(root)!;
        byte[] beforeReceipt = File.ReadAllBytes(Paths.Contained(root, Contract.StatePath));
        byte[] beforeSettings = File.ReadAllBytes(Paths.Contained(root, "vrmod/config/render_quality.json"));
        byte[] beforePerformance = File.ReadAllBytes(Paths.Contained(root, UserSettings.PerformancePath));
        var manifest = JsonNode.Parse(File.ReadAllBytes(Paths.Contained(current, "package-manifest.json")))!.AsObject();
        var version = UpdatePolicy.VersionOf(prior.Version);
        string futureVersion = new Version(version.Major, version.Minor, checked(version.Build + 1)).ToString(3);
        manifest["version"] = futureVersion; manifest["futureExtension"] = "owned-future-format-v2";
        string[] extra = ["vrmod/config/future-setting.json", "vrmod/bin/future-tool.txt", "vrmod/future/data-a.txt", "vrmod/future/data-b.txt"];
        var extras = extra.ToDictionary(p => p, p => Encoding.UTF8.GetBytes(p.EndsWith(".json", StringComparison.Ordinal) ? "{\"future\":true}" : "OWNED FUTURE FIXTURE " + p), StringComparer.Ordinal);
        foreach (var p in extras) manifest["files"]!.AsArray().Add(new JsonObject { ["path"] = p.Key, ["sha256"] = Paths.Hash(p.Value), ["bytes"] = p.Value.Length });
        byte[] manifestBytes = Encoding.UTF8.GetBytes(manifest.ToJsonString(Contract.Json));
        string archive = Path.Combine(fixture, UpdatePolicy.ArchiveName(futureVersion, prior.BuildFlavor));
        using (var zip = ZipFile.Open(archive, ZipArchiveMode.Create))
        {
            foreach (var f in Contract.Read<Package>(File.ReadAllBytes(Paths.Contained(current, "package-manifest.json"))).Files)
                zip.CreateEntryFromFile(Paths.Contained(current, "payload/" + f.Path), "payload/" + f.Path);
            foreach (var p in extras) { using var stream = zip.CreateEntry("payload/" + p.Key).Open(); stream.Write(p.Value); }
            foreach (string helper in UpdatePolicy.HelperFiles) zip.CreateEntryFromFile(Paths.Contained(futureHelper, helper), helper);
            using (var stream = zip.CreateEntry("package-manifest.json").Open()) stream.Write(manifestBytes);
            foreach (string name in new[] { "DOTNET-NOTICES.txt", "LOCAL-VR.txt" }) { using var w = new StreamWriter(zip.CreateEntry(name).Open()); w.Write("OWNED FUTURE FIXTURE"); }
        }
        using (var zip = ZipFile.OpenRead(archive)) Check(zip.Entries.Count == 20, "future fixture contains twenty entries");
        var descriptor = new UpdateDescriptor(1, Contract.Product, UpdatePolicy.Repository, futureVersion, "stable", Path.GetFileName(archive), Paths.FileHash(archive)!, Paths.Hash(manifestBytes),
            UpdatePolicy.HelperFiles.Select(p => new PayloadFile(p, Paths.FileHash(Paths.Contained(futureHelper, p))!, new FileInfo(Paths.Contained(futureHelper, p)).Length)).ToArray(), prior.BuildFlavor, prior.CommonCoreSha256);
        byte[] bytes = Contract.Encode(descriptor);
        var release = new ReleaseCandidate(101, futureVersion, new(102, UpdatePolicy.DescriptorFor(prior.BuildFlavor), bytes.Length, Paths.Hash(bytes)), new(103, descriptor.ArchiveName, new FileInfo(archive).Length, descriptor.ArchiveSha256));
        Check(UpdatePolicy.Descriptor(bytes, release).Version == futureVersion, "future descriptor keeps legacy schema");
        string Stage(string name) { string p = Path.Combine(fixture, name); Directory.CreateDirectory(p); return p; }
        Reject(() => UpdatePolicy.Descriptor(Contract.Encode(descriptor with { Repository = "other/repository" }), release), "descriptor tamper");
        string badZip = Path.Combine(fixture, "tampered-archive.zip"); File.Copy(archive, badZip);
        using (var stream = new FileStream(badZip, FileMode.Append)) stream.WriteByte(1);
        Reject(() => UpdateStaging.Extract(root, Stage("bad-archive"), badZip, bytes, descriptor), "ZIP tamper");
        string badHelper = Path.Combine(fixture, "tampered-helper.zip"); File.Copy(archive, badHelper);
        using (var zip = ZipFile.Open(badHelper, ZipArchiveMode.Update))
        {
            zip.GetEntry("GF2VR.Installer.dll")!.Delete(); using var w = new StreamWriter(zip.CreateEntry("GF2VR.Installer.dll").Open()); w.Write("TAMPERED HELPER");
        }
        var changedArchive = descriptor with { ArchiveSha256 = Paths.FileHash(badHelper)! };
        Reject(() => UpdateStaging.Extract(root, Stage("bad-helper"), badHelper, Contract.Encode(changedArchive), changedArchive), "helper tamper despite valid archive hash");
        Check(File.ReadAllBytes(Paths.Contained(root, Contract.StatePath)).SequenceEqual(beforeReceipt), "all tamper refusals preserve prior receipt");
        string stage = Stage("future-stage");
        var prepared = UpdateStaging.Extract(root, stage, archive, bytes, descriptor);
        Check(Directory.GetFiles(Path.Combine(stage, "package"), "*", SearchOption.AllDirectories).Select(p => Path.GetFileName(p)!).ToHashSet(StringComparer.Ordinal).SetEquals(UpdatePolicy.HelperFiles), "only four helpers are extracted");
        Check(!File.Exists(Path.Combine(stage, "package", "package-manifest.json")), "client never extracts/parses future manifest");
        Check(!Directory.Exists(Path.Combine(stage, "package", "payload")), "client never extracts future payload");
        Check(File.ReadAllBytes(Paths.Contained(root, Contract.StatePath)).SequenceEqual(beforeReceipt), "future staging performs no installation");
        Paths.WriteNew(Paths.Contained(stage, "bootstrap-parent.json"), Contract.Encode(prepared));
        var start = new ProcessStartInfo(Path.Combine(AppContext.BaseDirectory, "GF2VR.Installer.exe")) { UseShellExecute = false, CreateNoWindow = true, WindowStyle = ProcessWindowStyle.Hidden };
        foreach (string arg in new[] { "--bootstrap-parent-fixture", "--stage", stage }) start.ArgumentList.Add(arg);
        using (var parent = Process.Start(start)!) Check(parent.WaitForExit(30000) && parent.ExitCode == 0, "real parent issues production handoff then exits");
        string resultPath = Paths.Contained(stage, "update-result.json"); var watch = Stopwatch.StartNew();
        while (!File.Exists(resultPath) && watch.ElapsedMilliseconds < 45000) Thread.Sleep(20);
        Check(File.Exists(resultPath), "independent future helper completed install");
        var result = JsonNode.Parse(File.ReadAllBytes(resultPath))!;
        Check(result["passed"]!.GetValue<bool>() && result["installedFiles"]!.GetValue<int>() == 13 && result["recognizedFutureField"]!.GetValue<bool>(), "future helper applies its own schema");
        var receipt = JsonNode.Parse(File.ReadAllBytes(Paths.Contained(root, Contract.StatePath)))!;
        Check(receipt["version"]!.GetValue<string>() == futureVersion && receipt["files"]!.AsArray().Count == 13, "actual future installation receipt");
        foreach (var f in receipt["files"]!.AsArray()) Check(Paths.FileHash(Paths.Contained(root, f!["path"]!.GetValue<string>())) == f["sha256"]!.GetValue<string>(), "future installed byte hash");
        foreach (var p in extras) Check(File.ReadAllBytes(Paths.Contained(root, p.Key)).SequenceEqual(p.Value), "new future config/tool/data installed");
        Check(File.ReadAllBytes(Paths.Contained(root, "vrmod/config/render_quality.json")).SequenceEqual(beforeSettings), "future install preserves current user settings");
        Check(File.ReadAllBytes(Paths.Contained(root, UserSettings.PerformancePath)).SequenceEqual(beforePerformance), "future install preserves opaque performance settings");
        cases.Add(new { name = "future20", handedOff = true, installed = true, files = 13 });
        File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory, "bootstrap-validation.json"), Contract.Encode(new { passed = true, checks, cases, currentPackage = prior.Version, futureVersion, clientManifestParses = 0, clientPayloadExtractions = 0, gameLaunches = 0, nativeLoads = 0, fixture }));
        return 0;
    }
}
