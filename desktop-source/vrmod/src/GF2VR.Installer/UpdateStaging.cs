using System.Diagnostics;
using System.IO.Compression;

namespace GF2VR.Installer;

internal static class UpdateStaging
{
    internal static string? ManagedVersion(string root)
        => ManagedVersion(root, out _);
    internal static string? ManagedVersion(string root, out InstallError? refusal)
    {
        refusal = null;
        root = Paths.Root(root);
        var engine = new InstallationEngine();
        if (engine.HasPending(root)) { refusal = new InstallError("pending"); return null; }
        var receipt = engine.Inspect(root);
        if (receipt == null) { refusal = new InstallError("notInstalled"); return null; }
        if (receipt.Scope != "vrmod" || !receipt.Files.Select(f => f.Path).ToHashSet(StringComparer.Ordinal).SetEquals(NativeLayout.Files))
        { refusal = new InstallError("layoutChanged"); return null; }
        foreach (var f in receipt.Files)
        {
            string? hash = Paths.FileHash(Paths.Target(root, f.Path, receipt.GameDirectory ?? "GF2 Game"));
            if (!Paths.Equal(hash, f.Sha256))
            { refusal = new InstallError(hash == null ? "ownedMissing" : "modified", f.Path); return null; }
        }
        _ = UpdatePolicy.VersionOf(receipt.Version); return receipt.Version;
    }
    internal static PreparedUpdate Extract(string root, string folder, string archive, byte[] descriptorBytes, UpdateDescriptor descriptor)
    {
        UpdatePolicy.ValidateDescriptor(descriptor);
        RequireFlavor(new InstallationEngine().Inspect(root), descriptor);
        if (!Paths.Equal(Paths.FileHash(archive), descriptor.ArchiveSha256)) throw new InstallError("updateInvalid");
        string package = Paths.Contained(folder, "package");
        if (Directory.Exists(package)) throw new InstallError("updateInvalid");
        using var zip = ZipFile.OpenRead(archive);
        var expected = NativeLayout.Files.Select(f => "payload/" + f).Concat(UpdatePolicy.HelperFiles)
            .Concat(["package-manifest.json", "DOTNET-NOTICES.txt", "LOCAL-VR.txt"]).ToHashSet(StringComparer.Ordinal);
        if (zip.Entries.Count != expected.Count || !zip.Entries.Select(e => e.FullName).ToHashSet(StringComparer.Ordinal).SetEquals(expected)) throw new InstallError("updateInvalid");
        long total = 0;
        foreach (var e in zip.Entries)
        {
            if (e.Length < 1 || e.Length > Contract.MaxFile || (total += e.Length) > UpdatePolicy.MaxArchive ||
                (((e.ExternalAttributes >> 16) & 0xF000) == 0xA000)) throw new InstallError("updateInvalid");
            string destination = Paths.Contained(package, e.FullName); Directory.CreateDirectory(Path.GetDirectoryName(destination)!);
            using var input = e.Open(); using var output = new FileStream(destination, FileMode.CreateNew, FileAccess.Write, FileShare.None);
            byte[] buffer = new byte[65536]; long written = 0; int read;
            while ((read = input.Read(buffer)) != 0) { if ((written += read) > e.Length) throw new InstallError("updateInvalid"); output.Write(buffer, 0, read); }
            if (written != e.Length) throw new InstallError("updateInvalid"); output.Flush(true);
        }
        foreach (var f in descriptor.InstallerFiles)
            if (!Paths.Equal(Paths.FileHash(Paths.Contained(package, f.Path)), f.Sha256) || new FileInfo(Paths.Contained(package, f.Path)).Length != f.Bytes) throw new InstallError("updateInvalid");
        if (!Paths.Equal(Paths.FileHash(Paths.Contained(package, "package-manifest.json")), descriptor.PackageManifestSha256)) throw new InstallError("updateInvalid");
        // Old clients verify provenance and bytes; the verified incoming helper
        // knows the future native profile and owns final preflight/installation.
        var incoming = Contract.Read<Package>(Paths.Bytes(Paths.Contained(package, "package-manifest.json"), 262144)!);
        var engine = new InstallationEngine(); var prior = engine.Inspect(root);
        string? installedVersion = ManagedVersion(root);
        RequireFlavor(prior, descriptor, incoming);
        if (incoming.SchemaVersion != 1 || incoming.Product != Contract.Product || incoming.Scope != "vrmod" ||
            incoming.Version != descriptor.Version || incoming.Native == null || incoming.BoundGameRoot != null ||
            incoming.RequiresInstalledDllSha256 != null || incoming.Native.ResourceBinding != NativeLayout.PortableBinding ||
            installedVersion == null || UpdatePolicy.VersionOf(incoming.Version) <= UpdatePolicy.VersionOf(installedVersion) ||
            incoming.Files == null || incoming.Files.Length != NativeLayout.Files.Length ||
            !incoming.Files.Select(f => f?.Path).ToHashSet(StringComparer.Ordinal).SetEquals(NativeLayout.Files))
            throw new InstallError("updateInvalid");
        foreach (var f in incoming.Files)
        {
            if (f == null || !Paths.Owned(f.Path) || f.Bytes is < 1 or > Contract.MaxFile) throw new InstallError("updateInvalid");
            Paths.HashText(f.Sha256); string payload = Paths.Contained(package, "payload/" + f.Path);
            if (new FileInfo(payload).Length != f.Bytes || !Paths.Equal(Paths.FileHash(payload), f.Sha256)) throw new InstallError("updateInvalid");
        }
        Paths.HashText(incoming.Native.DllSha256); Paths.HashText(incoming.Native.LoaderSha256);
        if (!Paths.Equal(incoming.Files.Single(f => f.Path == "GF2 Game/version.dll").Sha256, incoming.Native.DllSha256) ||
            !Paths.Equal(incoming.Files.Single(f => f.Path == "vrmod/runtime/openxr_loader.dll").Sha256, incoming.Native.LoaderSha256))
            throw new InstallError("updateInvalid");
        string receiptHash = engine.ReceiptHash(root);
        if (ManagedVersion(root) != installedVersion || !Paths.Equal(engine.ReceiptHash(root), receiptHash)) throw new InstallError("updateInvalid");
        Paths.WriteNew(Paths.Contained(folder, "release-descriptor.json"), descriptorBytes);
        return new(descriptor.Version, folder, root, receiptHash, descriptor.PackageManifestSha256, Paths.Hash(descriptorBytes));
    }
    internal static void Handoff(PreparedUpdate update)
    {
        InstallationEngine.Stopped(); using var parent = Process.GetCurrentProcess();
        byte[] descriptorBytes = Paths.Bytes(Paths.Contained(update.Folder, "release-descriptor.json"), 262144) ?? throw new InstallError("updateInvalid");
        if (!Paths.Equal(Paths.Hash(descriptorBytes), update.DescriptorHash)) throw new InstallError("updateInvalid");
        var descriptor = Contract.Read<UpdateDescriptor>(descriptorBytes); UpdatePolicy.ValidateDescriptor(descriptor);
        foreach (var f in descriptor.InstallerFiles)
            if (!Paths.Equal(Paths.FileHash(Paths.Contained(update.Folder, "package/" + f.Path)), f.Sha256)) throw new InstallError("updateInvalid");
        var handoff = new UpdateHandoff(1, update.Root, update.Version, update.ReceiptHash, update.ManifestHash,
            update.DescriptorHash, parent.Id, parent.StartTime.ToUniversalTime().Ticks, true);
        Paths.WriteNew(Paths.Contained(update.Folder, "handoff.json"), Contract.Encode(handoff));
        string helper = Paths.Contained(update.Folder, "package/GF2VR.Installer.exe");
        var start = new ProcessStartInfo(helper) { UseShellExecute = false, CreateNoWindow = true, WindowStyle = ProcessWindowStyle.Hidden };
        start.ArgumentList.Add("--apply-confirmed-update"); start.ArgumentList.Add("--stage"); start.ArgumentList.Add(update.Folder);
        using var child = Process.Start(start) ?? throw new InstallError("updateInvalid");
    }
    internal static int Apply(string folder)
    {
        folder = Path.GetFullPath(folder);
        var h = Contract.Read<UpdateHandoff>(File.ReadAllBytes(Paths.Contained(folder, "handoff.json")));
        if (h.SchemaVersion != 1 || !h.UserConfirmed || h.ParentPid <= 0 || h.ParentPid == Environment.ProcessId || h.ParentStartTicks <= 0) throw new InstallError("updateInvalid");
        try
        {
            using var parent = Process.GetProcessById(h.ParentPid);
            if (parent.StartTime.ToUniversalTime().Ticks != h.ParentStartTicks || !parent.WaitForExit(30000)) throw new InstallError("updateParent");
        }
        catch (ArgumentException) { } // Already exited; never kill the parent/game.
        InstallationEngine.Stopped();
        byte[] descriptorBytes = Paths.Bytes(Paths.Contained(folder, "release-descriptor.json"), 262144) ?? throw new InstallError("updateInvalid");
        if (!Paths.Equal(Paths.Hash(descriptorBytes), h.DescriptorHash)) throw new InstallError("updateInvalid");
        var descriptor = Contract.Read<UpdateDescriptor>(descriptorBytes); UpdatePolicy.ValidateDescriptor(descriptor);
        string package = Path.GetDirectoryName(Paths.Contained(folder, "package/package-manifest.json"))!;
        foreach (var f in descriptor.InstallerFiles) if (!Paths.Equal(Paths.FileHash(Paths.Contained(package, f.Path)), f.Sha256)) throw new InstallError("updateInvalid");
        if (!Path.GetFullPath(Environment.ProcessPath!).Equals(Paths.Contained(package, "GF2VR.Installer.exe"), StringComparison.OrdinalIgnoreCase)) throw new InstallError("updateInvalid");
        var engine = new InstallationEngine(); var plan = engine.Preview(h.Root, package);
        RequireFlavor(plan.Prior, descriptor, plan.Package);
        if (plan.Prior == null || !Paths.Equal(plan.PriorHash, h.ReceiptHash) || !Paths.Equal(plan.ManifestHash, h.ManifestHash) ||
            !Paths.Equal(plan.ManifestHash, descriptor.PackageManifestSha256) || plan.Package.Version != descriptor.Version || h.Version != descriptor.Version ||
            UpdatePolicy.VersionOf(plan.Package.Version) <= UpdatePolicy.VersionOf(plan.Prior.Version)) throw new InstallError("updateInvalid");
        var result = engine.Install(plan);
        Paths.WriteNew(Paths.Contained(folder, "update-result.json"), Contract.Encode(new { passed = true, result.Version, result.TransactionId }));
        return 0;
    }
    internal static void RequireFlavor(Receipt? receipt, UpdateDescriptor descriptor, Package? package = null)
    {
        if (receipt == null || receipt.BuildFlavor != descriptor.BuildFlavor ||
            (package != null && (package.Native?.BuildFlavor != descriptor.BuildFlavor ||
                !Paths.Equal(package.Native?.CommonCoreSha256, descriptor.CommonCoreSha256)))) throw new InstallError("updateInvalid");
    }
}
