using System.Diagnostics;
using System.Text.Json.Nodes;

namespace GF2VR.Installer;

internal static class BootstrapContractChecks
{
    internal static int Run(string candidate)
    {
        int checks = 0;
        void Check(bool value, string why) { ++checks; if (!value) throw new InvalidDataException(why); }
        void Reject(Action action, string key, string why)
        {
            ++checks;
            try { action(); } catch (InstallError e) when (e.Key == key) { return; }
            throw new InvalidDataException("Expected rejection: " + why);
        }
        string[] names = ["GF2VR.Installer.exe", "GF2VR.Installer.dll", "GF2VR.Installer.deps.json", "GF2VR.Installer.runtimeconfig.json"];
        string[] keys = ["schemaVersion", "product", "repository", "version", "channel", "archiveName", "archiveSha256", "packageManifestSha256", "installerFiles", "buildFlavor", "commonCoreSha256"];
        Check(UpdatePolicy.Repository == "deadpixel134/GF2Exilium-VR", "stable repository");
        Check(UpdatePolicy.HelperFiles.SequenceEqual(names), "four exact ZIP-root helpers");
        foreach (string flavor in new[] { "Clean", "Dirty" })
        {
            Check(UpdatePolicy.ArchiveName("1.2.2", flavor) == "GF2VR-Native-SelfContained-" + flavor + "-1.2.2.zip", "stable ZIP asset name");
            Check(UpdatePolicy.DescriptorFor(flavor) == "GF2VR-" + flavor + "-update.json", "stable descriptor asset name");
        }
        string fixture = Path.Combine(AppContext.BaseDirectory, "bootstrap-contract-fixture");
        if (Directory.Exists(fixture)) throw new InvalidDataException("Preserve prior contract fixture");
        string root = Path.Combine(fixture, "game");
        Paths.WriteNew(Paths.Contained(root, "GF2 Game/GF2_Exilium.exe"), System.Text.Encoding.UTF8.GetBytes(SelfTest.Inert));
        var package = Contract.Read<Package>(File.ReadAllBytes(Paths.Contained(candidate, "package-manifest.json")));
        var descriptor = new UpdateDescriptor(1, "GF2VR", "deadpixel134/GF2Exilium-VR", "1.2.2", "stable",
            UpdatePolicy.ArchiveName("1.2.2", package.Native!.BuildFlavor), new string('A', 64), new string('B', 64),
            names.Select(n => new PayloadFile(n, Paths.FileHash(Path.Combine(AppContext.BaseDirectory, n))!, new FileInfo(Path.Combine(AppContext.BaseDirectory, n)).Length)).ToArray(),
            package.Native.BuildFlavor, package.Native.CommonCoreSha256);
        var node = JsonNode.Parse(Contract.Encode(descriptor))!.AsObject();
        Check(node.Count == 11 && node.Select(p => p.Key).ToHashSet(StringComparer.Ordinal).SetEquals(keys), "exact descriptor eleven-key envelope");
        UpdatePolicy.ValidateDescriptor(Contract.Read<UpdateDescriptor>(Contract.Encode(node))); ++checks;
        var unknown = node.DeepClone().AsObject(); unknown["archives"] = new JsonArray();
        Reject(() => Contract.Read<UpdateDescriptor>(Contract.Encode(unknown)), "badState", "second archive/protocol extension");
        Reject(() => UpdatePolicy.ValidateDescriptor(descriptor with { InstallerFiles = [.. descriptor.InstallerFiles, new("extra-runtime.dll", new string('C', 64), 1)] }), "updateInvalid", "fifth helper dependency");
        Reject(() => UpdatePolicy.ValidateDescriptor(descriptor with { ArchiveSha256 = "" }), "badHash", "missing whole archive SHA");
        Reject(() => UpdatePolicy.ValidateDescriptor(descriptor with { Repository = "other/project" }), "updateInvalid", "repository change");
        Reject(() => UpdatePolicy.ValidateDescriptor(descriptor with { ArchiveName = "latest.zip" }), "updateInvalid", "asset naming change");
        foreach (string owned in new[] { "GF2 Game/version.dll", "vrmod/runtime/openxr_loader.dll" })
        {
            string bad = Path.Combine(fixture, owned.Contains("version.dll", StringComparison.Ordinal) ? "bad-dll" : "bad-loader");
            var mismatch = package with { Files = package.Files.Select(f => f.Path == owned ? f with { Sha256 = new string('0', 64) } : f).ToArray() };
            Paths.WriteNew(Paths.Contained(bad, "package-manifest.json"), Contract.Encode(mismatch));
            Reject(() => new InstallationEngine().CheckPayload(root, bad), "nativeBuild", "payload/native metadata SHA cross-check " + owned);
        }
        string only = Path.Combine(fixture, "helper-only"); Directory.CreateDirectory(only);
        foreach (string name in names) File.Copy(Path.Combine(AppContext.BaseDirectory, name), Path.Combine(only, name));
        var start = new ProcessStartInfo(Path.Combine(only, names[0])) { UseShellExecute = false, CreateNoWindow = true, WindowStyle = ProcessWindowStyle.Hidden };
        start.ArgumentList.Add("--verify-localization"); start.Environment.Remove("DOTNET_STARTUP_HOOKS");
        using (var child = Process.Start(start)!) Check(child.WaitForExit(30000) && child.ExitCode == 0, "four-file helper starts with bundled private runtime/resources");
        Check(Directory.GetFiles(only).Length == 4, "no fifth file supplied to helper");
        Paths.WriteNew(Path.Combine(AppContext.BaseDirectory, "bootstrap-contract-validation.json"), Contract.Encode(new { passed = true, checks, descriptorKeys = keys, helpers = names, crossChecks = 2, fourFileHelperProcess = true, gameLaunches = 0 }));
        return 0;
    }
}
