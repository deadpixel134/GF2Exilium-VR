using System.Diagnostics;
using System.Text;
using System.Text.Json.Nodes;

namespace GF2VR.Installer;

// Real product bytes, inert game prerequisites, no native DLL execution.
internal static class PortablePackageChecks
{
    private static int checks;
    private const string GuiContract = NativeLayout.ToolRoot + "runtime-contract.json";
    private static void Assert(bool ok, string label) { ++checks; if (!ok) throw new InvalidDataException(label); }
    private static void Reject(Action work, string label)
    {
        ++checks; try { work(); } catch (InstallError) { return; }
        throw new InvalidDataException("Expected portable rejection: " + label);
    }
    private static void Manifest(string pkg, Package value) => Paths.Atomic(Paths.Contained(pkg, "package-manifest.json"), Contract.Encode(value));
    private static string Root(string parent, string name)
    {
        string root = Path.Combine(parent, name);
        Paths.WriteNew(Paths.Contained(root, "GF2 Game/GF2_Exilium.exe"), Encoding.UTF8.GetBytes(SelfTest.Inert));
        foreach (string path in NativeLayout.GameFiles)
            Paths.WriteNew(Paths.Contained(root, path), Encoding.UTF8.GetBytes("INERT PREREQUISITE " + path));
        return root;
    }
    private static void Installed(InstallationEngine engine, string root)
    {
        var receipt = engine.Inspect(root)!;
        Assert(receipt.Files.Length == 9, "receipt files");
        foreach (var f in receipt.Files)
        {
            string dest = Paths.Contained(root, f.Path);
            Assert(Paths.Equal(Paths.FileHash(dest), f.Sha256) && new FileInfo(dest).Length == f.Bytes, "installed receipt hash " + f.Path);
        }
        var contract = JsonNode.Parse(File.ReadAllBytes(Paths.Contained(root, GuiContract)))!.AsObject();
        Assert(contract["configurationRoot"]!.GetValue<string>() == Path.Combine(root, "vrmod", "config"), "actual config root");
        Assert(contract["dllSha256"]!.GetValue<string>() == NativeLayout.PortableDllHash, "actual DLL contract");
    }
    private static void Configurator(string root, bool expectedSuccess)
    {
        string exe = Paths.Contained(root, NativeLayout.ToolRoot + "GF2VR.Configurator.exe");
        var info = new ProcessStartInfo(exe) { UseShellExecute = false, CreateNoWindow = true, WindowStyle = ProcessWindowStyle.Hidden };
        info.ArgumentList.Add("--check-installation"); info.ArgumentList.Add("--root"); info.ArgumentList.Add(root);
        using var process = Process.Start(info) ?? throw new InvalidDataException("Configurator did not start");
        Assert(process.WaitForExit(20000), "Configurator check timeout");
        Assert((process.ExitCode == 0) == expectedSuccess, "actual Configurator contract result");
    }
    internal static int Run(string actualPackage)
    {
        string fixture = Path.Combine(AppContext.BaseDirectory, "portable-validation-fixture");
        if (Directory.Exists(fixture)) throw new InvalidDataException("Preserve fixture; use fresh build output.");
        string rootA = Root(fixture, "한글 설치 A"), rootB = Root(fixture, "日本語 games B");
        string pkg = Path.Combine(fixture, "package");
        var source = Contract.Read<Package>(Paths.Bytes(Paths.Contained(actualPackage, "package-manifest.json"), 262144)!);
        Assert(NativeLayout.IsPortable(source) && source.BoundGameRoot == null, "portable source profile");
        foreach (var f in source.Files)
        {
            byte[] bytes = File.ReadAllBytes(Paths.Contained(actualPackage, "payload/" + f.Path));
            Assert(bytes.Length == f.Bytes && Paths.Equal(Paths.Hash(bytes), f.Sha256), "source hash " + f.Path);
            Paths.WriteNew(Paths.Contained(pkg, "payload/" + f.Path), bytes);
        }
        var manifest = source with {
            GameExecutableSha256 = Paths.FileHash(Paths.Contained(rootA, "GF2 Game/GF2_Exilium.exe"))!,
            Native = source.Native! with { GameFiles = NativeLayout.GameFiles.Select(p => new PayloadFile(p,
                Paths.FileHash(Paths.Contained(rootA, p))!, new FileInfo(Paths.Contained(rootA, p)).Length)).ToArray() }
        };
        Manifest(pkg, manifest);
        byte[] template = File.ReadAllBytes(Paths.Contained(pkg, "payload/" + GuiContract));
        var engine = new InstallationEngine();
        var checkedA = engine.CheckPayload(rootA, pkg); var checkedB = engine.CheckPayload(rootB, pkg);
        Assert(checkedA.ManifestHash == checkedB.ManifestHash, "one immutable package, two roots");
        Assert(!checkedA.Payload[GuiContract].SequenceEqual(template) && !checkedA.Payload[GuiContract].SequenceEqual(checkedB.Payload[GuiContract]), "install-root materialization");
        Assert(File.ReadAllBytes(Paths.Contained(pkg, "payload/" + GuiContract)).SequenceEqual(template), "package template changed");
        Assert(!Directory.Exists(Paths.Contained(rootA, "vrmod/installer")), "read-only check wrote receipt");
        var secondA = engine.CheckPayload(rootA, pkg);
        Assert(secondA.Payload[GuiContract].SequenceEqual(checkedA.Payload[GuiContract]), "deterministic materialization");
        Manifest(pkg, manifest with { BoundGameRoot = rootA }); Reject(() => engine.CheckPayload(rootA, pkg), "portable+bound conflict");
        Manifest(pkg, manifest with { Native = manifest.Native! with { ResourceBinding = null } }); Reject(() => engine.CheckPayload(rootA, pkg), "missing resource binding");
        Manifest(pkg, manifest with { Native = manifest.Native! with { ResourceBinding = "GUESS_FROM_CWD" } }); Reject(() => engine.CheckPayload(rootA, pkg), "unknown resource binding");
        Manifest(pkg, manifest with { Native = manifest.Native! with { DllSha256 = NativeLayout.DllHash } }); Reject(() => engine.CheckPayload(rootA, pkg), "legacy DLL as portable");
        Manifest(pkg, manifest with { Native = manifest.Native! with { BuildId = NativeLayout.BuildId } }); Reject(() => engine.CheckPayload(rootA, pkg), "portable DLL as legacy");
        Manifest(pkg, manifest);
        void BadContract(Action<JsonObject> change, string label)
        {
            var c = JsonNode.Parse(template)!.AsObject(); change(c); byte[] bytes = Contract.Encode(c);
            File.WriteAllBytes(Paths.Contained(pkg, "payload/" + GuiContract), bytes);
            Manifest(pkg, manifest with { Files = manifest.Files.Select(f => f.Path == GuiContract ? f with { Sha256 = Paths.Hash(bytes), Bytes = bytes.Length } : f).ToArray() });
            Reject(() => engine.CheckPayload(rootA, pkg), label);
            File.WriteAllBytes(Paths.Contained(pkg, "payload/" + GuiContract), template); Manifest(pkg, manifest);
        }
        BadContract(c => c["configurationRoot"] = Path.Combine(rootA, "vrmod", "config"), "embedded absolute root");
        BadContract(c => c["configurationRoot"] = "../vrmod/config", "arbitrary relative path");
        BadContract(c => c["dllSha256"] = NativeLayout.DllHash, "wrong Configurator DLL");
        BadContract(c => c["additionalTemplate"] = "${ROOT}", "unknown template field");
        BadContract(c => c["files"] = new JsonArray("render_quality.json", "navigation.json", "stereo_depth.json", "../outside.json"), "foreign config file");
        string configA = Paths.Contained(rootA, "vrmod/config/render_quality.json");
        byte[] userConfig = Encoding.UTF8.GetBytes("{\"schemaVersion\":1,\"eyeRenderScale\":0.6}");
        Paths.WriteNew(configA, userConfig);
        string vendorA = Paths.Contained(rootA, "GF2 Game/NEP2.dll"); byte[] vendor = Encoding.UTF8.GetBytes("PRESERVED VENDOR"); Paths.WriteNew(vendorA, vendor);
        var plan = engine.Preview(rootA, pkg);
        File.AppendAllText(Paths.Contained(pkg, "payload/" + GuiContract), " "); Reject(() => engine.Install(plan), "template changed after preview");
        File.WriteAllBytes(Paths.Contained(pkg, "payload/" + GuiContract), template);
        Assert(!File.Exists(Paths.Contained(rootA, "GF2 Game/version.dll")), "preflight mismatch changed DLL");
        // Exercise rollback exactly where generated bytes enter the existing journal.
        try { engine.Install(engine.Preview(rootA, pkg), path => { if (path == GuiContract) throw new IOException("INJECTED generated-contract write boundary"); }); }
        catch (IOException) { }
        Assert(!engine.HasPending(rootA) && engine.Inspect(rootA) == null, "generated-contract failure recovery");
        foreach (var f in manifest.Files) Assert(!File.Exists(Paths.Contained(rootA, f.Path)), "failed install owned file " + f.Path);
        var journalFile = Directory.GetFiles(Path.Combine(rootA, "vrmod", "installer", "transactions"), "journal.json", SearchOption.AllDirectories).Single();
        var recovered = Contract.Read<Journal>(File.ReadAllBytes(journalFile));
        Assert(recovered.State == "recovered" && Paths.Equal(recovered.Changes.Single(c => c.Path == GuiContract).AfterHash, Paths.Hash(checkedA.Payload[GuiContract])), "journal records bound bytes");
        engine.Install(engine.Preview(rootA, pkg)); Installed(engine, rootA);
        Assert(File.ReadAllBytes(configA).SequenceEqual(userConfig) && File.ReadAllBytes(vendorA).SequenceEqual(vendor), "settings/vendor preservation");
        Configurator(rootA, true);
        string configContractA = Paths.Contained(rootA, GuiContract); byte[] boundA = File.ReadAllBytes(configContractA);
        var badInstalled = JsonNode.Parse(boundA)!.AsObject(); badInstalled["configurationRoot"] = Path.Combine(rootB, "vrmod", "config");
        File.WriteAllBytes(configContractA, Contract.Encode(badInstalled)); Configurator(rootA, false);
        Reject(() => engine.Remove(rootA, engine.ReceiptHash(rootA)), "modified bound contract preserved");
        Assert(File.ReadAllBytes(configA).SequenceEqual(userConfig), "refused removal changed settings");
        File.WriteAllBytes(configContractA, boundA);
        string update = Path.Combine(fixture, "update");
        foreach (var f in manifest.Files) Paths.WriteNew(Paths.Contained(update, "payload/" + f.Path), File.ReadAllBytes(Paths.Contained(pkg, "payload/" + f.Path)));
        Manifest(update, manifest with { Version = "0.3.1" }); engine.Install(engine.Preview(rootA, update)); Installed(engine, rootA);
        var removal = engine.Remove(rootA, engine.ReceiptHash(rootA));
        Assert(removal.RestoredPrevious && engine.Inspect(rootA)!.Version == "0.3.0", "restore prior bound receipt");
        Assert(File.ReadAllBytes(configContractA).SequenceEqual(boundA), "restore exact prior bound bytes"); Configurator(rootA, true);
        engine.Remove(rootA, engine.ReceiptHash(rootA));
        foreach (var f in manifest.Files) Assert(!File.Exists(Paths.Contained(rootA, f.Path)), "first-install owned remove " + f.Path);
        Assert(File.ReadAllBytes(configA).SequenceEqual(userConfig) && File.ReadAllBytes(vendorA).SequenceEqual(vendor), "remove preserved settings/vendor");
        // Same unmodified package at a second Unicode root, no root substitution.
        engine.Install(engine.Preview(rootB, pkg)); Installed(engine, rootB); Configurator(rootB, true);
        Assert(!File.ReadAllBytes(Paths.Contained(rootB, GuiContract)).SequenceEqual(boundA), "root B received root A contract");
        foreach (var f in manifest.Native!.GameFiles)
            Assert(Paths.Equal(Paths.FileHash(Paths.Contained(rootB, f.Path)), f.Sha256), "read-only game preserved " + f.Path);
        // Keep B installed only inside this fixture for extracted-package checks.
        using var form = new InstallerForm(rootA, pkg, Path.Combine(AppContext.BaseDirectory, "portable-gui")); Application.Run(form);
        Assert(File.Exists(Path.Combine(AppContext.BaseDirectory, "portable-gui", "gui-validation.json")), "portable GUI missing");
        File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory, "portable-package-validation.json"), Contract.Encode(new {
            passed = true, checks, fixture, rootA, rootB, package = pkg, payloadFiles = 9, readOnlyGameFiles = 10,
            sourceDll = NativeLayout.PortableDllHash, fixtureRuntimeRootSubstitution = false,
            nativeDllsLoaded = 0, actualInstallationWrites = 0, gameLaunches = 0, actualConfiguratorReadOnlyRuns = 4,
            guiLocales = InstallerText.Languages, templateUnchanged = File.ReadAllBytes(Paths.Contained(pkg, "payload/" + GuiContract)).SequenceEqual(template) }));
        return 0;
    }
}
