namespace GF2VR.Installer;

// Not a package/receipt schema. Durable intent is prepared beside the existing
// transaction journal before commit; retries never replace product files.
internal sealed record PostprocessState(int SchemaVersion, string Root, string TransactionId,
    string Operation, string Version, bool CreateDefaults, string[] PendingSteps, string? UpdateStage,
    bool CleanupUpgrade = false, bool SettingsWarning = false, string? LastError = null);

internal sealed partial class InstallationEngine
{
    private static PostprocessState NewPostprocess(string root, string id, string operation, string version,
        bool createDefaults, bool settings, bool cleanup, string? updateStage, bool cleanupUpgrade = false)
        => new(1, root, id, operation, version, createDefaults,
            ["transactionFinalize", .. (cleanup ? new[] { "logCleanup" } : []), .. (settings ? new[] { "settingsProtection" } : []), .. (updateStage == null ? [] : new[] { "updateResult" })], updateStage, cleanupUpgrade);

    private static string PostprocessPath(string root, string id) => Paths.Contained(Paths.Transaction(root, id), "postprocess.json");
    private static PostprocessState? ReadPostprocess(string root, string id)
    {
        byte[]? bytes = Paths.Bytes(PostprocessPath(root, id), 262144);
        if (bytes == null) return null;
        var p = Contract.Read<PostprocessState>(bytes);
        string[] allowed = ["transactionFinalize", "logCleanup", "settingsProtection", "updateResult"];
        if (p.SchemaVersion != 1 || p.Root != root || p.TransactionId != id || p.Operation is not ("install" or "repair" or "remove") ||
            p.PendingSteps == null || p.PendingSteps.Length > 4 || p.PendingSteps.Distinct(StringComparer.Ordinal).Count() != p.PendingSteps.Length ||
            p.PendingSteps.Any(s => !allowed.Contains(s, StringComparer.Ordinal)) ||
            (p.UpdateStage == null && p.PendingSteps.Contains("updateResult"))) throw new InstallError("badState");
        _ = UpdatePolicy.VersionOf(p.Version);
        return p;
    }
    private static void VerifyPostcommit(PostprocessState p)
    {
        var j = ReadJournal(p.Root, p.TransactionId);
        if (j.State != "committed" || !Paths.Equal(Paths.FileHash(Paths.Contained(p.Root, Contract.StatePath)), j.Changes[^1].AfterHash))
            throw new InstallError("postprocessStale");
        foreach (var c in j.Changes[..^1])
            if (!Paths.Equal(Paths.FileHash(Paths.Target(p.Root, c.Path, j.GameDirectory ?? "GF2 Game")), c.AfterHash))
                throw new InstallError("postprocessStale");
    }
    private static OperationResult CompletePostprocessing(PostprocessState state, bool restoredPrevious)
    {
        var pending = state.PendingSteps.ToList();
        try { VerifyPostcommit(state); }
        catch (Exception ex) { return Pending(ex); }
        foreach (string step in state.PendingSteps)
        {
            if (step == "updateResult" && pending.Any(s => s != "updateResult")) continue;
            try
            {
                Stopped();
                switch (step)
                {
                    case "transactionFinalize":
                        string file = Paths.Contained(state.Root, Contract.PendingPath);
                        byte[]? bytes = Paths.Bytes(file, 4096);
                        if (bytes != null)
                        {
                            var active = Contract.Read<Pending>(bytes);
                            if (active.SchemaVersion != 1 || active.TransactionId != state.TransactionId) throw new InstallError("postprocessStale");
                            File.Delete(file);
                        }
                        break;
                    case "logCleanup":
                        string cleanup = RuntimeLogCleanup.Apply(state.Root, state.CleanupUpgrade);
                        if (cleanup is "preserved-settings-busy" or "preserved-unsafe-or-unavailable") throw new InstallError("postprocessPending");
                        break;
                    case "settingsProtection":
                        state = state with { SettingsWarning = UserSettings.Apply(state.Root, state.TransactionId, state.Operation, state.CreateDefaults).Warning }; break;
                    case "updateResult": WriteUpdateResult(state); break;
                }
                var next = pending.Where(s => s != step).ToArray();
                Paths.Atomic(PostprocessPath(state.Root, state.TransactionId), Contract.Encode(state with { PendingSteps = next, LastError = null }));
                pending.Remove(step); state = state with { PendingSteps = next, LastError = null };
            }
            catch (Exception ex) { _ = Pending(ex); }
        }
        return new(state.Version, restoredPrevious, state.TransactionId, state.SettingsWarning, pending.Count == 0 ? null : pending.ToArray());

        OperationResult Pending(Exception ex)
        {
            state = state with { PendingSteps = pending.ToArray(), LastError = ex is InstallError e ? e.Key : "fileError" };
            try { Paths.Atomic(PostprocessPath(state.Root, state.TransactionId), Contract.Encode(state)); }
            catch (Exception) { /* Intent written before commit remains authoritative; pending is returned to the caller. */ }
            return new(state.Version, restoredPrevious, state.TransactionId, state.SettingsWarning, pending.ToArray());
        }
    }
    private static void WriteUpdateResult(PostprocessState p)
    {
        string stage = Path.GetFullPath(p.UpdateStage ?? throw new InstallError("badState"));
        var h = Contract.Read<UpdateHandoff>(Paths.Bytes(Paths.Contained(stage, "handoff.json"), 262144) ?? throw new InstallError("badState"));
        byte[] metadata = Paths.Bytes(Paths.Contained(stage, "release-descriptor.json"), 262144) ?? throw new InstallError("badState");
        var descriptor = Contract.Read<UpdateDescriptor>(metadata); UpdatePolicy.ValidateDescriptor(descriptor);
        var receipt = ReadReceipt(p.Root, StateBytes(p.Root) ?? throw new InstallError("badState"));
        if (h.SchemaVersion != 1 || !h.UserConfirmed || !Paths.Root(h.Root).Equals(p.Root, StringComparison.OrdinalIgnoreCase) || h.Version != p.Version || descriptor.Version != p.Version ||
            !Paths.Equal(Paths.Hash(metadata), h.DescriptorHash) || !Paths.Equal(h.ManifestHash, descriptor.PackageManifestSha256) ||
            receipt.TransactionId != p.TransactionId || !Paths.Equal(receipt.PackageSha256, h.ManifestHash)) throw new InstallError("postprocessStale");
        string output = Paths.Contained(stage, "update-result.json");
        byte[] result = Contract.Encode(new UpdateResult(true, p.Version, p.TransactionId));
        byte[]? existing = Paths.Bytes(output, 262144);
        if (existing != null)
        {
            if (!existing.AsSpan().SequenceEqual(result)) throw new InstallError("postprocessConflict", output);
        }
        else Paths.WriteNew(output, result);
    }
    private sealed record UpdateResult(bool Passed, string Version, string TransactionId);

    internal OperationResult RetryPostprocessing(string root, string id)
    {
        root = Paths.Root(root); using var guard = new OperationLock(root); Stopped();
        var state = ReadPostprocess(root, id) ?? throw new InstallError("noPostprocess");
        VerifyPostcommit(state);
        return CompletePostprocessing(state, false);
    }
    internal string? FindPendingPostprocessing(string root)
    {
        root = Paths.Root(root);
        string transactions = Path.GetDirectoryName(Paths.Contained(root, "vrmod/installer/transactions/index.json"))!;
        if (!Directory.Exists(transactions)) return null;
        // Inspect only our bounded metadata directory, never game assets/configs.
        foreach (var dir in new DirectoryInfo(transactions).EnumerateDirectories().OrderByDescending(d => d.LastWriteTimeUtc).Take(64))
        {
            if (!Guid.TryParseExact(dir.Name, "N", out _)) continue;
            var p = ReadPostprocess(root, dir.Name);
            if (p == null || p.PendingSteps.Length == 0) continue;
            var j = ReadJournal(root, dir.Name);
            if (j.State != "committed" || !Paths.Equal(Paths.FileHash(Paths.Contained(root, Contract.StatePath)), j.Changes[^1].AfterHash)) continue;
            return dir.Name;
        }
        return null;
    }
}
