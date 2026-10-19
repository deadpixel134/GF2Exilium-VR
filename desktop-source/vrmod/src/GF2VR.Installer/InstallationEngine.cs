using System.Diagnostics;
using System.Text;

namespace GF2VR.Installer;

internal sealed class InstallationEngine
{
    internal static Func<bool> GameRunning = () =>
    {
        var processes = Process.GetProcessesByName("GF2_Exilium");
        try { return processes.Length != 0; } finally { foreach (var p in processes) p.Dispose(); }
    };
    internal static void Stopped() { if (GameRunning()) throw new InstallError("running"); }
    private static void NoPending(string root)
    {
        if (File.Exists(Paths.Contained(root, Contract.PendingPath))) throw new InstallError("pending");
    }
    private static byte[]? StateBytes(string root) => Paths.Bytes(Paths.Contained(root, Contract.StatePath), 262144);
    internal Receipt? Inspect(string root)
    {
        root = Paths.Root(root); byte[]? bytes = StateBytes(root);
        return bytes == null ? null : ReadReceipt(root, bytes);
    }
    internal bool HasPending(string root) => File.Exists(Paths.Contained(Paths.Root(root), Contract.PendingPath));
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
    internal Plan Preview(string root, string packageRoot)
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
                if (hash != null && !Paths.Equal(hash, Paths.Hash(checkedPackage.Payload[f.Path])))
                    throw new InstallError("unowned", f.Path);
            }
            else if (owned == null || !Paths.Equal(hash, owned.Sha256)) throw new InstallError("modified", f.Path);
            beforeHashes.Add(f.Path, hash);
        }
        return new Plan(root, checkedPackage.PackageRoot, package, checkedPackage.ManifestHash, prior,
            priorBytes == null ? null : Paths.Hash(priorBytes), checkedPackage.Payload, gameDirectory, beforeHashes);
    }

    internal OperationResult Install(Plan shown, Action<string>? progress = null)
    {
        using var guard = new OperationLock(shown.Root); Stopped(); NoPending(shown.Root);
        // The selected immutable preview is revalidated at the actual write boundary.
        Plan plan = Preview(shown.Root, shown.PackageRoot);
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
        Execute(plan.Root, id, changes, progress, plan.GameDirectory);
        if (plan.Package.Scope == "vrmod" && plan.Package.Native != null && Version.Parse(plan.Package.Version) >= new Version(1, 1, 5))
            _ = RuntimeLogCleanup.Apply(plan.Root, plan.Prior != null);
        return new OperationResult(receipt.Version, false, id);
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
        string id = Guid.NewGuid().ToString("N"); Execute(root, id, changes, progress, original.GameDirectory ?? "GF2 Game");
        return new OperationResult(receipt.Version, priorState != null, id);
    }

    private static void Execute(string root, string id, List<(string Path, byte[]? Before, byte[]? After)> requested, Action<string>? progress, string gameDirectory)
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
        _ = ReadJournal(root, id); // Check durable backup and stage integrity before creating pending/write authority.
        foreach (var c in changes)
            if (!Paths.Equal(Paths.FileHash(Paths.Target(root, c.Path, gameDirectory)), c.BeforeHash)) throw new InstallError("previewChanged");
        Stopped();
        string pending = Paths.Contained(root, Contract.PendingPath);
        Paths.WriteNew(pending, Contract.Encode(new Pending(1, id)));
        try
        {
            foreach (var c in changes)
            {
                Stopped(); progress?.Invoke(c.Path);
                string dest = Paths.Target(root, c.Path, gameDirectory);
                if (!Paths.Equal(Paths.FileHash(dest), c.BeforeHash)) throw new InstallError("modified", c.Path);
                SetFile(dest, c.AfterFile == null ? null : Paths.Bytes(Paths.Contained(folder, c.AfterFile)), c.AfterHash);
            }
            Paths.Atomic(journalFile, Contract.Encode(journal with { State = "committed" }));
            File.Delete(pending);
        }
        catch
        {
            try { RollBack(root, journal); }
            catch { throw new InstallError("recoveryNeeded", folder); }
            throw;
        }
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
