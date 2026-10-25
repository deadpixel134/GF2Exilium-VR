namespace GF2VR.Installer;

internal sealed record UpdateCheckResult(ReleaseCandidate? Release, UpdateDescriptor? Descriptor, byte[]? Bytes,
    string State, string? DiscoveredVersion = null, string? PageUrl = null, InstallError? Refusal = null);

internal sealed class UpdateService
{
    private readonly GithubOwnerClient client;
    internal UpdateService() : this(new GithubOwnerClient()) { }
    internal UpdateService(GithubOwnerClient client) { this.client = client; }
    internal async Task<UpdateCheckResult> Check(string root, CancellationToken token)
    {
        Receipt? receipt = null; InstallError? initial = null;
        try { receipt = new InstallationEngine().Inspect(root); }
        catch (InstallError e) { initial = e; }
        string version = receipt?.Version ?? System.Reflection.Assembly.GetExecutingAssembly().GetName().Version!.ToString(3);
        if (!client.Available) return new(null, null, null, "updateAccess");
        byte[] catalog = await client.Releases(token);
        string? discovered = UpdatePolicy.DiscoverVersion(catalog, version);
        if (discovered == null) return new(null, null, null, "updateCurrent", Refusal: initial);
        string page = UpdatePolicy.ReleasePage(discovered);
        try
        {
            if (initial != null) throw initial;
            string? flavor = receipt?.BuildFlavor;
            var release = UpdatePolicy.Select(catalog, version, flavor) ?? throw new InstallError("updateInvalid");
            var bytes = await client.Metadata(release.Descriptor, token);
            var descriptor = UpdatePolicy.Descriptor(bytes, release);
            if (descriptor.BuildFlavor != flavor) throw new InstallError("updateVariantPending");
            await Task.Run(() =>
            {
                if (UpdateStaging.ManagedVersion(root, out var refusal) == null) throw refusal ?? new InstallError("notInstalled");
            }, token);
            return new(release, descriptor, bytes, "updateAvailable", discovered, page);
        }
        catch (OperationCanceledException) when (token.IsCancellationRequested) { throw; }
        catch (Exception ex)
        {
            return new(null, null, null, "updateManual", discovered, page, ex as InstallError ?? new InstallError("updateInvalid"));
        }
    }
    internal async Task<PreparedUpdate> Stage(string root, ReleaseCandidate release, UpdateDescriptor descriptor, byte[] bytes, CancellationToken token)
    {
        InstallationEngine.Stopped();
        var verified = UpdatePolicy.Descriptor(bytes, release);
        if (!Contract.Encode(verified).AsSpan().SequenceEqual(Contract.Encode(descriptor))) throw new InstallError("updateInvalid");
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
