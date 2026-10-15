namespace GF2VR.Installer;

internal sealed class UpdateService
{
    private readonly GithubOwnerClient client = new();
    internal async Task<(ReleaseCandidate? Release, UpdateDescriptor? Descriptor, byte[]? Bytes, string State)> Check(string root, CancellationToken token)
    {
        string? version = await Task.Run(() => UpdateStaging.ManagedVersion(root), token);
        if (version == null) return (null, null, null, "updateUnmanaged");
        if (!client.Available) return (null, null, null, "updateAccess");
        var release = UpdatePolicy.Select(await client.Releases(token), version);
        if (release == null) return (null, null, null, "updateCurrent");
        var bytes = await client.Metadata(release.Descriptor, token);
        return (release, UpdatePolicy.Descriptor(bytes, release), bytes, "updateAvailable");
    }
    internal async Task<PreparedUpdate> Stage(string root, ReleaseCandidate release, UpdateDescriptor descriptor, byte[] bytes, CancellationToken token)
    {
        InstallationEngine.Stopped();
        if (UpdateStaging.ManagedVersion(root) == null) throw new InstallError("updateInvalid");
        string folder = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "GF2VR", "updates", Guid.NewGuid().ToString("N"));
        string archive = Paths.Contained(folder, release.Archive.Name); // Reject linked parents before any write.
        Directory.CreateDirectory(folder);
        await client.Download(release.Archive, archive, token);
        token.ThrowIfCancellationRequested();
        return await Task.Run(() => UpdateStaging.Extract(root, folder, archive, bytes, descriptor), token);
        // Keep a failed stage for inspection; never recursively erase an unknown path.
    }
}
