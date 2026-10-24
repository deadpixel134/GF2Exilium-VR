namespace GF2VR.Installer;

internal sealed class UpdateService
{
    private readonly GithubOwnerClient client = new();
    internal async Task<(ReleaseCandidate? Release, UpdateDescriptor? Descriptor, byte[]? Bytes, string State)> Check(string root, CancellationToken token)
    {
        string version = await Task.Run(() =>
        {
            string? managed = UpdateStaging.ManagedVersion(root, out var refusal);
            if (managed == null) throw refusal ?? new InstallError("notInstalled");
            return managed;
        }, token);
        if (!client.Available) return (null, null, null, "updateAccess");
        string? flavor = new InstallationEngine().Inspect(root)?.BuildFlavor;
        var release = UpdatePolicy.Select(await client.Releases(token), version, flavor);
        if (release == null) return (null, null, null, "updateCurrent");
        var bytes = await client.Metadata(release.Descriptor, token);
        var descriptor = UpdatePolicy.Descriptor(bytes, release);
        if (descriptor.BuildFlavor != flavor) return (null, null, null, "updateVariantPending");
        return (release, descriptor, bytes, "updateAvailable");
    }
    internal async Task<PreparedUpdate> Stage(string root, ReleaseCandidate release, UpdateDescriptor descriptor, byte[] bytes, CancellationToken token)
    {
        InstallationEngine.Stopped();
        if (UpdateStaging.ManagedVersion(root, out var refusal) == null) throw refusal ?? new InstallError("notInstalled");
        string folder = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "GF2VR", "updates", Guid.NewGuid().ToString("N"));
        string archive = Paths.Contained(folder, release.Archive.Name); // Reject linked parents before any write.
        Directory.CreateDirectory(folder);
        await client.Download(release.Archive, archive, token);
        token.ThrowIfCancellationRequested();
        return await Task.Run(() => UpdateStaging.Extract(root, folder, archive, bytes, descriptor), token);
        // Keep a failed stage for inspection; never recursively erase an unknown path.
    }
}
