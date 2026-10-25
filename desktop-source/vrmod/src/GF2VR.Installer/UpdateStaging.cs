using System.Diagnostics;
using System.IO.Compression;

namespace GF2VR.Installer;

internal static class UpdateStaging
{
    internal static OperationResult? LastApplyResult { get; private set; }
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
            if (hash != null && !Paths.Equal(hash, f.Sha256))
            { refusal = new InstallError("modified", f.Path); return null; }
        }
        _ = UpdatePolicy.VersionOf(receipt.Version); return receipt.Version;
    }
    internal static PreparedUpdate Extract(string root, string folder, string archive, byte[] descriptorBytes, UpdateDescriptor descriptor)
    {
        UpdatePolicy.ValidateDescriptor(descriptor);
        var encoded = Contract.Read<UpdateDescriptor>(descriptorBytes);
        if (!Contract.Encode(encoded).AsSpan().SequenceEqual(Contract.Encode(descriptor))) throw new InstallError("updateInvalid");
        var engine = new InstallationEngine();
        RequireFlavor(engine.Inspect(root), descriptor);
        string? installedVersion = ManagedVersion(root);
        if (installedVersion == null || UpdatePolicy.VersionOf(descriptor.Version) <= UpdatePolicy.VersionOf(installedVersion))
            throw new InstallError("updateInvalid");
        string receiptHash = engine.ReceiptHash(root);
        if (new FileInfo(archive).Length is < 1 or > UpdatePolicy.MaxArchive) throw new InstallError("updateInvalid");
        if (!Paths.Equal(Paths.FileHash(archive), descriptor.ArchiveSha256)) throw new InstallError("updateInvalid");
        string package = Paths.Contained(folder, "package");
        if (Directory.Exists(package)) throw new InstallError("updateInvalid");
        // Keep the verified archive for the incoming helper. No payload schema,
        // manifest, runtime contract or global entry count belongs to this client.
        string cachedArchive = Paths.Contained(folder, descriptor.ArchiveName);
        if (!Path.GetFullPath(archive).Equals(cachedArchive, StringComparison.OrdinalIgnoreCase))
        {
            if (File.Exists(cachedArchive)) throw new InstallError("updateInvalid");
            Directory.CreateDirectory(folder); File.Copy(archive, cachedArchive);
        }
        if (!Paths.Equal(Paths.FileHash(cachedArchive), descriptor.ArchiveSha256)) throw new InstallError("updateInvalid");
        using var zip = ZipFile.OpenRead(cachedArchive);
        foreach (var f in descriptor.InstallerFiles)
        {
            var matches = zip.Entries.Where(e => e.FullName == f.Path).ToArray();
            if (matches.Length != 1) throw new InstallError("updateInvalid");
            ExtractEntry(matches[0], Paths.Contained(package, f.Path), f.Bytes);
            if (!Paths.Equal(Paths.FileHash(Paths.Contained(package, f.Path)), f.Sha256)) throw new InstallError("updateInvalid");
        }
        if (ManagedVersion(root) != installedVersion || !Paths.Equal(engine.ReceiptHash(root), receiptHash)) throw new InstallError("updateInvalid");
        Paths.WriteNew(Paths.Contained(folder, "release-descriptor.json"), descriptorBytes);
        return new(descriptor.Version, folder, root, receiptHash, descriptor.PackageManifestSha256, Paths.Hash(descriptorBytes));
    }

    private static void ExtractEntry(ZipArchiveEntry entry, string destination, long expectedBytes, bool reuse = false)
    {
        if (entry.Length != expectedBytes || entry.Length is < 1 or > Contract.MaxFile ||
            (((entry.ExternalAttributes >> 16) & 0xF000) == 0xA000)) throw new InstallError("updateInvalid");
        using var input = entry.Open();
        if (reuse && File.Exists(destination))
        {
            string hash = Convert.ToHexString(System.Security.Cryptography.SHA256.HashData(input));
            if (new FileInfo(destination).Length != expectedBytes || !Paths.Equal(Paths.FileHash(destination), hash)) throw new InstallError("updateInvalid");
            return;
        }
        Directory.CreateDirectory(Path.GetDirectoryName(destination)!);
        using var output = new FileStream(destination, FileMode.CreateNew, FileAccess.Write, FileShare.None);
        byte[] buffer = new byte[65536]; long written = 0; int read;
        while ((read = input.Read(buffer)) != 0)
        {
            if ((written += read) > expectedBytes) throw new InstallError("updateInvalid");
            output.Write(buffer, 0, read);
        }
        if (written != expectedBytes) throw new InstallError("updateInvalid");
        output.Flush(true);
    }

    // Called by the incoming executable, after helper/consent/parent validation.
    // This is its current package policy; future helpers own their future policy.
    internal static string MaterializeForHelper(string folder, UpdateDescriptor descriptor)
    {
        string archive = Paths.Contained(folder, descriptor.ArchiveName);
        if (!File.Exists(archive))
            return Path.GetDirectoryName(Paths.Contained(folder, "package/package-manifest.json"))!; // Legacy client already extracted it.
        if (new FileInfo(archive).Length is < 1 or > UpdatePolicy.MaxArchive ||
            !Paths.Equal(Paths.FileHash(archive), descriptor.ArchiveSha256)) throw new InstallError("updateInvalid");
        using var zip = ZipFile.OpenRead(archive);
        var expected = NativeLayout.Files.Select(f => "payload/" + f).Concat(UpdatePolicy.HelperFiles)
            .Concat(["package-manifest.json", "DOTNET-NOTICES.txt", "LOCAL-VR.txt"]).ToHashSet(StringComparer.Ordinal);
        if (zip.Entries.Count != expected.Count || !zip.Entries.Select(e => e.FullName).ToHashSet(StringComparer.Ordinal).SetEquals(expected)) throw new InstallError("updateInvalid");
        long total = 0;
        string package = Path.GetDirectoryName(Paths.Contained(folder, "incoming-package/package-manifest.json"))!;
        foreach (var e in zip.Entries)
        {
            if ((total += e.Length) > UpdatePolicy.MaxArchive) throw new InstallError("updateInvalid");
            ExtractEntry(e, Paths.Contained(package, e.FullName), e.Length, reuse: true);
        }
        foreach (var f in descriptor.InstallerFiles)
            if (!Paths.Equal(Paths.FileHash(Paths.Contained(package, f.Path)), f.Sha256) || new FileInfo(Paths.Contained(package, f.Path)).Length != f.Bytes) throw new InstallError("updateInvalid");
        if (!Paths.Equal(Paths.FileHash(Paths.Contained(package, "package-manifest.json")), descriptor.PackageManifestSha256)) throw new InstallError("updateInvalid");
        return package;
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
        string helper = Paths.Contained(folder, "package/GF2VR.Installer.exe");
        foreach (var f in descriptor.InstallerFiles)
            if (!Paths.Equal(Paths.FileHash(Paths.Contained(folder, "package/" + f.Path)), f.Sha256)) throw new InstallError("updateInvalid");
        if (!Path.GetFullPath(Environment.ProcessPath!).Equals(helper, StringComparison.OrdinalIgnoreCase)) throw new InstallError("updateInvalid");
        string package = MaterializeForHelper(folder, descriptor);
        var engine = new InstallationEngine(); var plan = engine.Preview(h.Root, package);
        RequireFlavor(plan.Prior, descriptor, plan.Package);
        if (plan.Package.Native == null || plan.Package.BoundGameRoot != null || plan.Package.RequiresInstalledDllSha256 != null ||
            plan.Package.Native.ResourceBinding != NativeLayout.PortableBinding ||
            plan.Prior == null || !Paths.Equal(plan.PriorHash, h.ReceiptHash) || !Paths.Equal(plan.ManifestHash, h.ManifestHash) ||
            !Paths.Equal(plan.ManifestHash, descriptor.PackageManifestSha256) || plan.Package.Version != descriptor.Version || h.Version != descriptor.Version ||
            UpdatePolicy.VersionOf(plan.Package.Version) <= UpdatePolicy.VersionOf(plan.Prior.Version)) throw new InstallError("updateInvalid");
        LastApplyResult = engine.Install(plan, updateStage: folder);
        return 0;
    }
    internal static void RequireFlavor(Receipt? receipt, UpdateDescriptor descriptor, Package? package = null)
    {
        if (receipt == null || receipt.BuildFlavor != descriptor.BuildFlavor ||
            (package != null && (package.Native?.BuildFlavor != descriptor.BuildFlavor ||
                !Paths.Equal(package.Native?.CommonCoreSha256, descriptor.CommonCoreSha256)))) throw new InstallError("updateInvalid");
    }
}
