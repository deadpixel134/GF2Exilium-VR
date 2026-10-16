using System.Text;
using System.Text.Json.Nodes;

namespace GF2VR.Installer;

// All game files are inert synthetic prerequisites. Actual product DLLs are copied, hashed and never loaded.
internal static class NativePackageChecks
{
    private static int checks;
    private static void Assert(bool ok, string label) { ++checks; if (!ok) throw new InvalidDataException(label); }
    private static void Reject(Action work, string label)
    {
        ++checks; try { work(); } catch (InstallError) { return; }
        throw new InvalidDataException("Expected native-package rejection: " + label);
    }
    private static void Manifest(string folder, Package value)
        => Paths.Atomic(Paths.Contained(folder, "package-manifest.json"), Contract.Encode(value));
    internal static int Run(string actualPackage)
    {
        string fixture = Path.Combine(AppContext.BaseDirectory, "native-validation-fixture");
        if (Directory.Exists(fixture)) throw new InvalidDataException("Preserve old fixture; use a fresh output directory.");
        string root = Path.Combine(fixture, "game");
        Paths.WriteNew(Paths.Contained(root, "GF2 Game/GF2_Exilium.exe"), Encoding.UTF8.GetBytes(SelfTest.Inert));
        var actual = Contract.Read<Package>(Paths.Bytes(Paths.Contained(actualPackage, "package-manifest.json"), 262144)!);
        if (actual.Scope != "vrmod" || actual.Native == null) throw new InstallError("incompletePackage");
        var gameFiles = new List<PayloadFile>();
        foreach (string path in NativeLayout.GameFiles)
        {
            byte[] fake = Encoding.UTF8.GetBytes("INERT GAME PREREQUISITE " + path);
            Paths.WriteNew(Paths.Contained(root, path), fake); gameFiles.Add(new(path, Paths.Hash(fake), fake.Length));
        }
        string pkg = Path.Combine(fixture, "package");
        var files = new List<PayloadFile>();
        foreach (var file in actual.Files)
        {
            byte[] bytes = Paths.Bytes(Paths.Contained(actualPackage, "payload/" + file.Path))!;
            if (bytes.Length != file.Bytes || !Paths.Equal(Paths.Hash(bytes), file.Sha256)) throw new InstallError("payloadHash", file.Path);
            if (file.Path == NativeLayout.ToolRoot + "runtime-contract.json")
            {
                var c = JsonNode.Parse(bytes)!.AsObject(); c["configurationRoot"] = Path.Combine(root, "vrmod", "config");
                bytes = Encoding.UTF8.GetBytes(c.ToJsonString());
            }
            Paths.WriteNew(Paths.Contained(pkg, "payload/" + file.Path), bytes); files.Add(new(file.Path, Paths.Hash(bytes), bytes.Length));
        }
        var manifest = actual with { BoundGameRoot = root,
            GameExecutableSha256 = Paths.FileHash(Paths.Contained(root, "GF2 Game/GF2_Exilium.exe"))!, Files = files.ToArray(),
            Native = actual.Native with { GameFiles = gameFiles.ToArray() } };
        Manifest(pkg, manifest);
        string config = Paths.Contained(root, "vrmod/config/render_quality.json");
        byte[] configBytes = Encoding.UTF8.GetBytes("{\"schemaVersion\":1,\"eyeRenderScale\":0.6,\"future\":\"PRESERVE\"}");
        Paths.WriteNew(config, configBytes);
        string foreign = Paths.Contained(root, "GF2 Game/NEP2.dll");
        byte[] foreignBytes = Encoding.UTF8.GetBytes("INERT FOREIGN VENDOR FILE"); Paths.WriteNew(foreign, foreignBytes);
        string savedRoot = NativeLayout.BoundRuntimeRoot;
        try
        {
            // Root change is confined to this in-process test, with an inert game and no DLL execution.
            NativeLayout.BoundRuntimeRoot = root;
            var engine = new InstallationEngine(); var plan = engine.Preview(root, pkg);
            Assert(plan.Package.Files.Length == 9, "incomplete actual native fixture");
            Assert(!Directory.Exists(Paths.Contained(root, "vrmod/installer")), "read-only preview created state");
            Manifest(pkg, manifest with { Native = null }); Reject(() => engine.Preview(root, pkg), "no native profile");
            Manifest(pkg, manifest with { BoundGameRoot = null }); Reject(() => engine.Preview(root, pkg), "portable claim");
            Manifest(pkg, manifest with { Native = manifest.Native! with { DllSha256 = new string('0', 64) } });
            Reject(() => engine.Preview(root, pkg), "unverified runtime build");
            Manifest(pkg, manifest with { Native = manifest.Native! with { LoaderSha256 = new string('0', 64) } });
            Reject(() => engine.Preview(root, pkg), "unverified OpenXR loader");
            foreach (var f in manifest.Files)
            {
                Manifest(pkg, manifest with { Files = manifest.Files.Where(v => v.Path != f.Path).ToArray() });
                Reject(() => engine.Preview(root, pkg), "missing mandatory native file " + f.Path);
            }
            Manifest(pkg, manifest with { Native = manifest.Native! with { GameFiles = gameFiles.Take(9).ToArray() } });
            Reject(() => engine.Preview(root, pkg), "missing game prerequisite");
            Manifest(pkg, manifest with { Native = manifest.Native! with { GameFiles = [.. gameFiles.Take(9), gameFiles[0]] } });
            Reject(() => engine.Preview(root, pkg), "duplicate game prerequisite");
            Manifest(pkg, manifest with { Native = manifest.Native! with { GameFiles = [gameFiles[0] with { Path = "vrmod/config/render_quality.json" }, .. gameFiles.Skip(1)] } });
            Reject(() => engine.Preview(root, pkg), "unrelated read-only path");
            Manifest(pkg, manifest);
            var checkedPackage = engine.CheckPayload(root, pkg);
            Assert(checkedPackage.Payload.Count == 9, "payload inventory");
            string gameDependency = Paths.Contained(root, gameFiles[0].Path); byte[] dependencyBytes = File.ReadAllBytes(gameDependency);
            File.AppendAllText(gameDependency, "NEW GAME BUILD");
            Assert(engine.Preview(root, pkg).Root == root, "changed original game prerequisite is nonblocking");
            Assert(!File.Exists(Paths.Contained(root, "GF2 Game/version.dll")), "read-only compatibility preview wrote loader");
            File.WriteAllBytes(gameDependency, dependencyBytes);
            string contractPath = Paths.Contained(pkg, "payload/" + NativeLayout.ToolRoot + "runtime-contract.json");
            byte[] contractBytes = File.ReadAllBytes(contractPath);
            var changed = JsonNode.Parse(contractBytes)!.AsObject(); changed["dllSha256"] = new string('0', 64);
            byte[] changedBytes = Encoding.UTF8.GetBytes(changed.ToJsonString()); File.WriteAllBytes(contractPath, changedBytes);
            Manifest(pkg, manifest with { Files = manifest.Files.Select(f => f.Path == NativeLayout.ToolRoot + "runtime-contract.json" ? f with { Sha256 = Paths.Hash(changedBytes), Bytes = changedBytes.Length } : f).ToArray() });
            Reject(() => engine.Preview(root, pkg), "Configurator runtime mismatch");
            File.WriteAllBytes(contractPath, contractBytes); Manifest(pkg, manifest);
            string dllPath = Paths.Contained(root, "GF2 Game/version.dll");
            byte[] dllBytes = File.ReadAllBytes(Paths.Contained(pkg, "payload/GF2 Game/version.dll"));
            Paths.WriteNew(dllPath, dllBytes);
            Assert(Paths.Equal(engine.Preview(root, pkg).BeforeHashes["GF2 Game/version.dll"], Paths.Hash(dllBytes)),
                "exact verified loader without receipt is frozen in preview");
            Assert(File.ReadAllBytes(dllPath).SequenceEqual(dllBytes), "matching loader changed by preview"); File.Delete(dllPath);
            string xrPath = Paths.Contained(root, "vrmod/runtime/openxr_loader.dll");
            Paths.WriteNew(xrPath, Encoding.UTF8.GetBytes("FOREIGN OPENXR LOADER"));
            Reject(() => engine.Preview(root, pkg), "unowned external loader");
            Assert(File.ReadAllText(xrPath) == "FOREIGN OPENXR LOADER", "foreign helper changed"); File.Delete(xrPath);
            engine.Install(engine.Preview(root, pkg));
            foreach (var f in manifest.Files) Assert(Paths.Equal(Paths.FileHash(Paths.Contained(root, f.Path)), f.Sha256), "actual native installed hash " + f.Path);
            Assert(File.ReadAllBytes(config).SequenceEqual(configBytes), "native install changed user settings");
            Assert(File.ReadAllBytes(foreign).SequenceEqual(foreignBytes), "native install changed vendor file");
            foreach (var f in gameFiles) Assert(Paths.Equal(Paths.FileHash(Paths.Contained(root, f.Path)), f.Sha256), "native install changed game prerequisite " + f.Path);
            string updatePkg = Path.Combine(fixture, "update");
            foreach (var f in manifest.Files) Paths.WriteNew(Paths.Contained(updatePkg, "payload/" + f.Path), File.ReadAllBytes(Paths.Contained(pkg, "payload/" + f.Path)));
            Manifest(updatePkg, manifest with { Version = "0.2.1" });
            engine.Install(engine.Preview(root, updatePkg)); Assert(engine.Inspect(root)!.Version == "0.2.1", "native update receipt");
            File.AppendAllText(xrPath, "USER CHANGED"); Reject(() => engine.Remove(root, engine.ReceiptHash(root)), "modified installed helper removal");
            Assert(File.ReadAllBytes(config).SequenceEqual(configBytes), "refused removal changed user settings");
            File.WriteAllBytes(xrPath, checkedPackage.Payload["vrmod/runtime/openxr_loader.dll"]);
            var previous = engine.Remove(root, engine.ReceiptHash(root));
            Assert(previous.RestoredPrevious && engine.Inspect(root)!.Version == "0.2.0", "native previous-version restore");
            engine.Remove(root, engine.ReceiptHash(root));
            foreach (var f in manifest.Files) Assert(!File.Exists(Paths.Contained(root, f.Path)), "native owned file remained " + f.Path);
            Assert(File.ReadAllBytes(config).SequenceEqual(configBytes) && File.ReadAllBytes(foreign).SequenceEqual(foreignBytes), "native remove preservation");
            // Same GUI, now showing the actual nine-file native profile in all three packaged languages.
            using var form = new InstallerForm(root, pkg, Path.Combine(AppContext.BaseDirectory, "native-gui"));
            Application.Run(form);
            Assert(File.Exists(Path.Combine(AppContext.BaseDirectory, "native-gui", "gui-validation.json")), "native GUI failed");
            File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory, "native-package-validation.json"),
                Contract.Encode(new { passed = true, checks, payloadFiles = 9, readOnlyGameFiles = 10, sourceDll = NativeLayout.DllHash,
                    sourceLoader = NativeLayout.LoaderHash, fixtureGameExecutable = SelfTest.Inert, fixtureRuntimeRootSubstitution = true,
                    nativeDllsLoaded = 0, actualInstallationWrites = 0, gameLaunches = 0, fixture, allThreeGuiLocales = true }));
            return 0;
        }
        finally { NativeLayout.BoundRuntimeRoot = savedRoot; }
    }
}
