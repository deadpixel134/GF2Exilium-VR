using System.Diagnostics;
using System.Text;
using System.Text.Json;

namespace GF2VR.Installer;

internal sealed partial class InstallationEngine
{
    // Fixture observation only; never replaces the target write or its guards.
    internal static Action<string>? TargetWriteObserved = null;
    internal static Func<bool> GameRunning = () =>
    {
        var processes = Process.GetProcessesByName("GF2_Exilium");
        try { return processes.Length != 0; } finally { foreach (var p in processes) p.Dispose(); }
    };
    internal static void Stopped() { if (GameRunning()) throw new InstallError("running"); }
    private static void NoPending(string root)
    {
        if (Paths.Bytes(Paths.Contained(root, Contract.PendingPath), 4096) != null) throw new InstallError("pending");
    }
    private static byte[]? StateBytes(string root) => Paths.Bytes(Paths.Contained(root, Contract.StatePath), 262144);
    internal Receipt? Inspect(string root)
    {
        root = Paths.Root(root); byte[]? bytes = StateBytes(root);
        return bytes == null ? null : ReadReceipt(root, bytes);
    }
    internal bool HasPending(string root) => Paths.Bytes(Paths.Contained(Paths.Root(root), Contract.PendingPath), 4096) != null;
    internal bool HasPreviousVersion(string root)
    {
        root = Paths.Root(root); byte[]? state = StateBytes(root);
        return state != null && InstallationJournal(root, ReadReceipt(root, state), state).Changes[^1].BeforeHash != null;
    }

    private static void ValidateFiles(PayloadFile[] files)
    {
        if (files == null || files.Length is < 1 or > 128) throw new InstallError("badPackage");
        var paths = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        long total = 0;
        foreach (var f in files)
        {
            if (f == null || !Paths.Owned(f.Path) || !paths.Add(f.Path)) throw new InstallError("foreignPath", f?.Path ?? "");
            Paths.HashText(f.Sha256);
            if (f.Bytes is < 1 or > Contract.MaxFile || (total += f.Bytes) > 256 * 1024 * 1024) throw new InstallError("size", f.Path);
        }
    }
    private static void Identity(int schema, string product, string version, string scope)
    {
        if (schema != 1 || product != Contract.Product || !Version.TryParse(version, out _) || scope is not ("tools" or "vrmod"))
            throw new InstallError("badPackage");
    }
    private static Receipt ReadReceipt(string root, byte[] bytes)
    {
        var receipt = Contract.Read<Receipt>(bytes); Identity(receipt.SchemaVersion, receipt.Product, receipt.Version, receipt.Scope);
        if (!string.Equals(root, receipt.Root, StringComparison.OrdinalIgnoreCase)) throw new InstallError("badState");
        Paths.Layout(root, receipt.GameDirectory ?? "GF2 Game");
        _ = Paths.Transaction(root, receipt.TransactionId); Paths.HashText(receipt.PackageSha256); ValidateFiles(receipt.Files);
        return receipt;
    }
    internal CheckedPackage CheckPayload(string root, string packageRoot)
    {
        root = Paths.Root(root);
        packageRoot = Path.TrimEndingDirectorySeparator(Path.GetFullPath(packageRoot));
        byte[] bytes = Paths.Bytes(Paths.Contained(packageRoot, "package-manifest.json"), 262144) ?? throw new InstallError("manifestMissing");
        var package = Contract.Read<Package>(bytes); Identity(package.SchemaVersion, package.Product, package.Version, package.Scope);
        ValidateFiles(package.Files); Paths.HashText(package.GameExecutableSha256);
        UserSettings.ValidatePolicy(package.UserSettings);
        // Original game fingerprints describe the packaging baseline. They do
        // not authorize writes and are deliberately not installation gates.
        if (package.BoundGameRoot != null && !Path.TrimEndingDirectorySeparator(Path.GetFullPath(package.BoundGameRoot)).Equals(root, StringComparison.OrdinalIgnoreCase))
            throw new InstallError("boundRoot");
        if (package.RequiresInstalledDllSha256 != null)
        {
            Paths.HashText(package.RequiresInstalledDllSha256);
            if (!Paths.Equal(Paths.FileHash(Paths.Target(root, "GF2 Game/version.dll")), package.RequiresInstalledDllSha256))
                throw new InstallError("runtimeRequired");
        }
        var set = package.Files.Select(f => f.Path).ToHashSet(StringComparer.Ordinal);
        if (package.Scope == "tools")
        {
            if (package.Native != null || !set.SetEquals(Paths.ToolFiles.Select(f => "vrmod/bin/GF2VR.Configurator/" + f)))
                throw new InstallError("incompletePackage");
        }
        else NativeLayout.Validate(root, package, set);
        var payload = new Dictionary<string, byte[]>(StringComparer.Ordinal);
        foreach (var f in package.Files)
        {
            byte[] data = Paths.Bytes(Paths.Contained(packageRoot, "payload/" + f.Path)) ?? throw new InstallError("payloadMissing", f.Path);
            if (data.Length != f.Bytes || !Paths.Equal(Paths.Hash(data), f.Sha256)) throw new InstallError("payloadHash", f.Path);
            payload.Add(f.Path, data);
        }
        if (package.Scope == "vrmod") NativeLayout.ConfiguratorContract(root, package, payload);
        return new CheckedPackage(root, packageRoot, package, Paths.Hash(bytes), payload);
    }
    internal Plan Preview(string root, string packageRoot) => PreviewCore(root, packageRoot, false, false);
    internal RepairPlan PreviewRepair(string root, string packageRoot, bool includeUnowned = false)
    {
        var plan = PreviewCore(root, packageRoot, true, includeUnowned);
        return new(plan, plan.Package.Files.Select(f =>
        {
            bool unowned = plan.Prior == null && plan.BeforeHashes[f.Path] != null && !Paths.Equal(plan.BeforeHashes[f.Path], Paths.Hash(plan.Payload[f.Path]));
            return new RepairFile(f.Path, plan.BeforeHashes[f.Path], Paths.Hash(plan.Payload[f.Path]), unowned, !unowned || includeUnowned);
        }).ToArray());
    }
    private Plan PreviewCore(string root, string packageRoot, bool repair, bool includeUnowned)
    {
        root = Paths.Root(root); NoPending(root); string gameDirectory = Paths.GameDirectory(root);
        var checkedPackage = CheckPayload(root, packageRoot);
        var package = checkedPackage.Package;
        var set = package.Files.Select(f => f.Path).ToHashSet(StringComparer.Ordinal);
        byte[]? priorBytes = StateBytes(root); Receipt? prior = priorBytes == null ? null : ReadReceipt(root, priorBytes);
        if (prior != null)
        {
            if (prior.Scope != package.Scope || !set.SetEquals(prior.Files.Select(f => f.Path))) throw new InstallError("layoutChanged");
            if (Version.Parse(package.Version) < Version.Parse(prior.Version)) throw new InstallError("downgrade");
            _ = InstallationJournal(root, prior, priorBytes!);
        }
        var beforeHashes = new Dictionary<string, string?>(StringComparer.Ordinal);
        foreach (var f in package.Files)
        {
            string? hash = Paths.FileHash(Paths.Target(root, f.Path, gameDirectory));
            var owned = prior?.Files.SingleOrDefault(p => p.Path == f.Path);
            if (prior == null)
            {
                // A missing receipt grants no authority over unknown files.
                // Only exact, verified final installation bytes may be retained;
                // the Configurator contract has already been bound by CheckPayload.
                if (!repair && hash != null && !Paths.Equal(hash, Paths.Hash(checkedPackage.Payload[f.Path])))
                    throw new InstallError("unowned", f.Path);
            }
            else if (owned == null || (!repair && hash != null && !Paths.Equal(hash, owned.Sha256))) throw new InstallError("modified", f.Path);
            beforeHashes.Add(f.Path, hash);
        }
        return new Plan(root, checkedPackage.PackageRoot, package, checkedPackage.ManifestHash, prior,
            priorBytes == null ? null : Paths.Hash(priorBytes), checkedPackage.Payload, gameDirectory, beforeHashes, repair, includeUnowned);
    }

    internal OperationResult Install(Plan shown, Action<string>? progress = null, string? updateStage = null)
    {
        if (shown.IsRepair) throw new InstallError("repairConsent");
        return InstallCore(shown, progress, updateStage);
    }
    internal OperationResult Repair(RepairPlan shown, bool userConfirmed, Action<string>? progress = null)
    {
        if (!userConfirmed || !shown.Plan.IsRepair || !shown.CanCommit) throw new InstallError("repairConsent");
        return InstallCore(shown.Plan, progress, null);
    }
    private OperationResult InstallCore(Plan shown, Action<string>? progress, string? updateStage)
    {
        using var guard = new OperationLock(shown.Root); Stopped(); NoPending(shown.Root);
        // The selected immutable preview is revalidated at the actual write boundary.
        Plan plan;
        if (shown.IsRepair)
        {
            var reviewed = PreviewRepair(shown.Root, shown.PackageRoot, shown.IncludeUnowned);
            if (!reviewed.CanCommit) throw new InstallError("repairConsent");
            plan = reviewed.Plan;
        }
        else plan = Preview(shown.Root, shown.PackageRoot);
        if (!string.Equals(plan.Root, shown.Root, StringComparison.OrdinalIgnoreCase) || plan.GameDirectory != shown.GameDirectory ||
            !Paths.Equal(plan.ManifestHash, shown.ManifestHash) || !Paths.Equal(plan.PriorHash, shown.PriorHash) ||
            plan.BeforeHashes.Count != shown.BeforeHashes.Count || plan.BeforeHashes.Any(f =>
                !shown.BeforeHashes.TryGetValue(f.Key, out string? hash) || !Paths.Equal(f.Value, hash))) throw new InstallError("previewChanged");
        string id = Guid.NewGuid().ToString("N");
        // The verified package may contain an install-root contract template.
        // Receipts/journals must bind the actual materialized bytes, not its template hash.
        var installedFiles = plan.Package.Files.Select(f => new PayloadFile(f.Path,
            Paths.Hash(plan.Payload[f.Path]), plan.Payload[f.Path].Length)).ToArray();
        var receipt = new Receipt(1, Contract.Product, plan.Package.Version, plan.Package.Scope, plan.Root, id, plan.ManifestHash, installedFiles, plan.GameDirectory,
            plan.Package.Native?.BuildFlavor, plan.Package.Native?.CommonCoreSha256);
        var changes = new List<(string Path, byte[]? Before, byte[]? After)>();
        foreach (var f in plan.Package.Files)
        {
            byte[]? before = Paths.Bytes(Paths.Target(plan.Root, f.Path, plan.GameDirectory));
            if (!Paths.Equal(before == null ? null : Paths.Hash(before), plan.BeforeHashes[f.Path]))
                throw new InstallError("previewChanged");
            changes.Add((f.Path, before, plan.Payload[f.Path]));
        }
        byte[]? priorState = StateBytes(plan.Root);
        if (!Paths.Equal(priorState == null ? null : Paths.Hash(priorState), plan.PriorHash)) throw new InstallError("previewChanged");
        changes.Add((Contract.StatePath, priorState, Contract.Encode(receipt)));
        var post = NewPostprocess(plan.Root, id, plan.IsRepair ? "repair" : "install", receipt.Version, true,
            plan.Package.Scope == "vrmod", !plan.IsRepair && plan.Package.Native != null && Version.Parse(plan.Package.Version) >= new Version(1, 1, 5), updateStage, plan.Prior != null);
        Execute(plan.Root, id, changes, progress, plan.GameDirectory, post);
        return CompletePostprocessing(post, false);
    }

    private static Journal ReadJournal(string root, string id)
    {
        string folder = Paths.Transaction(root, id);
        var j = Contract.Read<Journal>(Paths.Bytes(Paths.Contained(folder, "journal.json"), 262144) ?? throw new InstallError("badState"));
        Paths.Layout(root, j.GameDirectory ?? "GF2 Game");
        if (j.SchemaVersion != 1 || j.Product != Contract.Product || j.Root != root || j.TransactionId != id ||
            j.State is not ("prepared" or "committed" or "recovered") || j.Changes == null || j.Changes.Length is < 2 or > 129)
            throw new InstallError("badState");
        var paths = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        for (int i = 0; i < j.Changes.Length; i++)
        {
            var c = j.Changes[i];
            if (!paths.Add(c.Path) || (c.Path != Contract.StatePath && !Paths.Owned(c.Path)) ||
                (c.Path == Contract.StatePath && i != j.Changes.Length - 1)) throw new InstallError("badState");
            VerifyImage(folder, c.BeforeHash, c.BeforeFile, "files/" + i + ".before");
            VerifyImage(folder, c.AfterHash, c.AfterFile, "files/" + i + ".after");
        }
        if (j.Changes[^1].Path != Contract.StatePath) throw new InstallError("badState");
        return j;
    }
    private static void VerifyImage(string folder, string? hash, string? file, string expected)
    {
        if (hash == null) { if (file != null) throw new InstallError("badState"); return; }
        Paths.HashText(hash);
        if (file != expected || !Paths.Equal(hash, Paths.FileHash(Paths.Contained(folder, expected)))) throw new InstallError("backupMissing", expected);
    }
    private static Journal InstallationJournal(string root, Receipt receipt, byte[] bytes)
    {
        var journal = ReadJournal(root, receipt.TransactionId);
        if (journal.State != "committed" || !Paths.Equal(journal.Changes[^1].AfterHash, Paths.Hash(bytes)) ||
            !receipt.Files.Select(f => f.Path).ToHashSet().SetEquals(journal.Changes[..^1].Select(c => c.Path))) throw new InstallError("badState");
        foreach (var f in receipt.Files)
            if (!Paths.Equal(f.Sha256, journal.Changes.Single(c => c.Path == f.Path).AfterHash)) throw new InstallError("badState");
        return journal;
    }

    internal OperationResult Remove(string root, string expectedReceiptHash, Action<string>? progress = null)
    {
        root = Paths.Root(root); using var guard = new OperationLock(root); Stopped(); NoPending(root);
        byte[] state = StateBytes(root) ?? throw new InstallError("notInstalled");
        if (!Paths.Equal(Paths.Hash(state), expectedReceiptHash)) throw new InstallError("previewChanged");
        var receipt = ReadReceipt(root, state); var original = InstallationJournal(root, receipt, state);
        string folder = Paths.Transaction(root, receipt.TransactionId);
        var changes = new List<(string Path, byte[]? Before, byte[]? After)>();
        foreach (var c in original.Changes)
        {
            byte[]? current = Paths.Bytes(Paths.Target(root, c.Path, original.GameDirectory ?? "GF2 Game"));
            if (!Paths.Equal(current == null ? null : Paths.Hash(current), c.AfterHash)) throw new InstallError("modified", c.Path);
            byte[]? restore = c.BeforeFile == null ? null : Paths.Bytes(Paths.Contained(folder, c.BeforeFile));
            changes.Add((c.Path, current, restore));
        }
        byte[]? priorState = changes[^1].After;
        if (priorState != null)
        {
            var previous = ReadReceipt(root, priorState); _ = InstallationJournal(root, previous, priorState);
        }
        string id = Guid.NewGuid().ToString("N");
        var post = NewPostprocess(root, id, "remove", receipt.Version, false, receipt.Scope == "vrmod", false, null);
        Execute(root, id, changes, progress, original.GameDirectory ?? "GF2 Game", post);
        return CompletePostprocessing(post, priorState != null);
    }

    // Settings-only maintenance never grants authority to replace product files.
    internal OperationResult ReverifySettings(string root)
    {
        root = Paths.Root(root); using var guard = new OperationLock(root); Stopped(); NoPending(root);
        var receipt = Inspect(root) ?? throw new InstallError("notInstalled");
        string id = Guid.NewGuid().ToString("N");
        var result = UserSettings.Apply(root, id, "reverify", true);
        return new OperationResult(receipt.Version, false, id, result.Warning);
    }

    private static void Execute(string root, string id, List<(string Path, byte[]? Before, byte[]? After)> requested, Action<string>? progress, string gameDirectory, PostprocessState postprocess)
    {
        string folder = Paths.Transaction(root, id); var changes = new List<Change>();
        for (int i = 0; i < requested.Count; i++)
        {
            var item = requested[i];
            string? beforeFile = item.Before == null ? null : "files/" + i + ".before";
            string? afterFile = item.After == null ? null : "files/" + i + ".after";
            if (beforeFile != null) Paths.WriteNew(Paths.Contained(folder, beforeFile), item.Before!);
            if (afterFile != null) Paths.WriteNew(Paths.Contained(folder, afterFile), item.After!);
            changes.Add(new Change(item.Path, item.Before == null ? null : Paths.Hash(item.Before),
                item.After == null ? null : Paths.Hash(item.After), beforeFile, afterFile));
        }
        var journal = new Journal(1, Contract.Product, root, id, "prepared", changes.ToArray(), gameDirectory);
        string journalFile = Paths.Contained(folder, "journal.json"); Paths.WriteNew(journalFile, Contract.Encode(journal));
        Paths.WriteNew(Paths.Contained(folder, "postprocess.json"), Contract.Encode(postprocess));
        _ = ReadJournal(root, id); // Check durable backup and stage integrity before creating pending/write authority.
        foreach (var c in changes)
            if (!Paths.Equal(Paths.FileHash(Paths.Target(root, c.Path, gameDirectory)), c.BeforeHash)) throw new InstallError("previewChanged");
        Stopped();
        string pending = Paths.Contained(root, Contract.PendingPath);
        Paths.WriteNew(pending, Contract.Encode(new Pending(1, id)));
        var writeStatus = changes.Select(c => new TargetWriteEntry(c.Path, false, "not-attempted")).ToArray();
        SaveTargetWriteStatus(root, id, writeStatus);
        try
        {
            for (int index = 0; index < changes.Count; ++index)
            {
                var c = changes[index];
                Stopped(); progress?.Invoke(c.Path);
                string dest = Paths.Target(root, c.Path, gameDirectory);
                if (!Paths.Equal(Paths.FileHash(dest), c.BeforeHash)) throw new InstallError("modified", c.Path);
                // Keep every image/change/receipt entry. Only Install/Repair's
                // forward target write is optional; Remove/Recover/RollBack keep
                // their existing restoration behavior and authority checks.
                if (postprocess.Operation is "install" or "repair" && Paths.Equal(c.BeforeHash, c.AfterHash))
                {
                    writeStatus[index] = new(c.Path, false, "unchanged");
                    continue;
                }
                try
                {
                    TargetWriteObserved?.Invoke(dest);
                    SetFile(dest, c.AfterFile == null ? null : Paths.Bytes(Paths.Contained(folder, c.AfterFile)), c.AfterHash);
                    writeStatus[index] = new(c.Path, true, "written");
                }
                catch
                {
                    // False here means no completed forward write, not proof
                    // of zero partial I/O. Recovery uses the journal, never this.
                    writeStatus[index] = new(c.Path, false, "write-not-completed");
                    throw;
                }
            }
            Paths.Atomic(journalFile, Contract.Encode(journal with { State = "committed" }));
        }
        catch
        {
            try { RollBack(root, journal); }
            catch { throw new InstallError("recoveryNeeded", folder); }
            throw;
        }
        finally { SaveTargetWriteStatus(root, id, writeStatus); }
    }
    private sealed record TargetWriteEntry(string Path, bool Written, string Reason);
    private sealed record TargetWriteStatus(int SchemaVersion, string TransactionId, TargetWriteEntry[] Changes);
    private static void SaveTargetWriteStatus(string root, string id, TargetWriteEntry[] changes)
    {
        // Optional diagnostics outside the strict, legacy-compatible journal.
        // Absence/corruption is harmless: no installation/recovery path reads it.
        try
        {
            Paths.Atomic(Paths.Contained(Paths.Transaction(root, id), "write-status.json"),
                Contract.Encode(new TargetWriteStatus(1, id, changes)));
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or InstallError or System.Security.SecurityException) { }
    }
    private static void SetFile(string dest, byte[]? bytes, string? expected)
    {
        if (!Paths.Equal(bytes == null ? null : Paths.Hash(bytes), expected)) throw new InstallError("backupMissing", dest);
        if (bytes == null) { if (File.Exists(dest)) File.Delete(dest); }
        else Paths.Atomic(dest, bytes);
        if (!Paths.Equal(Paths.FileHash(dest), expected)) throw new InstallError("writeHash", dest);
    }
    private static void RollBack(string root, Journal journal)
    {
        Stopped(); string folder = Paths.Transaction(root, journal.TransactionId);
        _ = ReadJournal(root, journal.TransactionId); // Never restore unverified/corrupted backup bytes.
        // Preflight every current file before rollback touches even one file.
        foreach (var c in journal.Changes)
        {
            string? hash = Paths.FileHash(Paths.Target(root, c.Path, journal.GameDirectory ?? "GF2 Game"));
            if (!Paths.Equal(hash, c.BeforeHash) && !Paths.Equal(hash, c.AfterHash)) throw new InstallError("modified", c.Path);
        }
        foreach (var c in journal.Changes.Reverse())
        {
            Stopped(); string dest = Paths.Target(root, c.Path, journal.GameDirectory ?? "GF2 Game");
            if (Paths.Equal(Paths.FileHash(dest), c.BeforeHash)) continue;
            if (!Paths.Equal(Paths.FileHash(dest), c.AfterHash)) throw new InstallError("modified", c.Path);
            SetFile(dest, c.BeforeFile == null ? null : Paths.Bytes(Paths.Contained(folder, c.BeforeFile)), c.BeforeHash);
        }
        Paths.Atomic(Paths.Contained(folder, "journal.json"), Contract.Encode(journal with { State = "recovered" }));
        File.Delete(Paths.Contained(root, Contract.PendingPath));
    }
    internal void Recover(string root)
    {
        root = Paths.Root(root); using var guard = new OperationLock(root); Stopped();
        var pending = Contract.Read<Pending>(Paths.Bytes(Paths.Contained(root, Contract.PendingPath), 4096) ?? throw new InstallError("noPending"));
        if (pending.SchemaVersion != 1) throw new InstallError("badState");
        var journal = ReadJournal(root, pending.TransactionId);
        if (journal.State is "committed" or "recovered")
        {
            foreach (var c in journal.Changes)
                if (!Paths.Equal(Paths.FileHash(Paths.Target(root, c.Path, journal.GameDirectory ?? "GF2 Game")), journal.State == "committed" ? c.AfterHash : c.BeforeHash))
                    throw new InstallError("modified", c.Path);
            File.Delete(Paths.Contained(root, Contract.PendingPath)); return;
        }
        RollBack(root, journal);
    }
    internal string ReceiptHash(string root) => Paths.Hash(StateBytes(Paths.Root(root)) ?? throw new InstallError("notInstalled"));

    private sealed class OperationLock : IDisposable
    {
        private readonly Mutex mutex;
        internal OperationLock(string root)
        {
            mutex = new Mutex(false, "Local\\GF2VR.Installer." + Paths.Hash(Encoding.UTF8.GetBytes(root.ToUpperInvariant())));
            bool held; try { held = mutex.WaitOne(0); } catch (AbandonedMutexException) { held = true; }
            if (!held) { mutex.Dispose(); throw new InstallError("busy"); }
        }
        public void Dispose() { mutex.ReleaseMutex(); mutex.Dispose(); }
    }
}

// User data never enters product payload hashes or rollback ownership.
internal sealed record UserSettingPolicy(string Path, string Policy, string Integrity);
internal sealed record UserSettingResult(string Path, string Disposition, bool Warning);
internal sealed record SettingsProtectionReceipt(int SchemaVersion, string Product, string Root,
    string Operation, string OperationId, string? ProductReceiptSha256, UserSettingResult[] UserSettings);

internal static class UserSettings
{
    internal const string PerformancePath = "vrmod/config/performance.json";
    internal const string ReceiptPath = "vrmod/installer/settings-protection-receipt.json";
    internal static readonly UserSettingPolicy[] Policies =
        [new(PerformancePath, "PRESERVE_EXISTING_CREATE_IF_MISSING", "USER_DATA_NO_HASH")];
    internal static readonly byte[] DefaultBytes = Encoding.UTF8.GetBytes(
        "{\n  \"schemaVersion\": 1,\n  \"pcOutput\": { \"mode\": \"NATIVE\", \"scope\": \"IMMERSIVE_ONLY\" }\n}\n");

    internal static void ValidatePolicy(UserSettingPolicy[]? policies)
    {
        // Legacy absence is accepted without granting any payload exemption.
        if (policies == null) return;
        if (policies.Length != 1 || policies[0] != Policies[0]) throw new InstallError("settingsPolicy");
    }

    internal static UserSettingResult Apply(string root, string id, string operation, bool createMissing)
    {
        string file = Paths.Contained(root, PerformancePath);
        string disposition = "PRESERVED";
        if (!File.Exists(file) && createMissing)
        {
            Directory.CreateDirectory(Path.GetDirectoryName(file)!);
            string stage = Paths.Contained(root, "vrmod/config/.performance-default-" + Guid.NewGuid().ToString("N") + ".tmp");
            try
            {
                using (var stream = new FileStream(stage, FileMode.CreateNew, FileAccess.Write, FileShare.None))
                { stream.Write(DefaultBytes); stream.Flush(true); }
                InstallationEngine.Stopped();
                try { File.Move(stage, Paths.Contained(root, PerformancePath), false); disposition = "CREATED_DEFAULT"; }
                catch (IOException) when (File.Exists(Paths.Contained(root, PerformancePath))) { }
            }
            finally { if (File.Exists(stage)) File.Delete(stage); }
        }
        // Only JSON syntax is inspected for a warning. Never interpret keys or
        // feature values. Oversize input is preserved without parsing it.
        bool warning = false;
        file = Paths.Contained(root, PerformancePath);
        if (!File.Exists(file)) disposition = "ABSENT_PRESERVED";
        else
        {
            using var input = new FileStream(file, FileMode.Open, FileAccess.Read, FileShare.Read);
            if (input.Length > 1024 * 1024) warning = true;
            else
            {
                try
                {
                    using var copy = new MemoryStream(); input.CopyTo(copy);
                    var bytes = copy.ToArray().AsMemory();
                    if (bytes.Span.StartsWith(new byte[] { 239, 187, 191 })) bytes = bytes[3..];
                    using var json = JsonDocument.Parse(bytes);
                    warning = json.RootElement.ValueKind != JsonValueKind.Object;
                }
                catch (JsonException) { warning = true; }
            }
        }
        var result = new UserSettingResult(PerformancePath, disposition, warning);
        var receipt = new SettingsProtectionReceipt(1, Contract.Product, root, operation, id,
            Paths.FileHash(Paths.Contained(root, Contract.StatePath)), [result]);
        Paths.Atomic(Paths.Contained(root, ReceiptPath), Contract.Encode(receipt));
        return result;
    }
}
