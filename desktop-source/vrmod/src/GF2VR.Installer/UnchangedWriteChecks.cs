using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;

namespace GF2VR.Installer;

internal static class UnchangedWriteChecks
{
    internal static int Run(string package)
    {
        int checks = 0;
        void Check(bool ok, string why) { ++checks; if (!ok) throw new InvalidDataException(why); }
        void Reject(Action action, string why)
        {
            ++checks;
            try { action(); }
            catch (Exception ex) when (ex is InstallError or IOException or UnauthorizedAccessException) { return; }
            throw new InvalidDataException("Expected rejection: " + why);
        }
        string fixture = Path.Combine(AppContext.BaseDirectory, "unchanged-validation-fixture");
        if (Directory.Exists(fixture)) throw new InvalidDataException("Preserve prior fixture");
        string root = Path.Combine(fixture, "game");
        Paths.WriteNew(Paths.Contained(root, "GF2 Game/GF2_Exilium.exe"), Encoding.UTF8.GetBytes("INERT FIXTURE; NEVER EXECUTE"));
        var engine = new InstallationEngine();
        engine.Install(engine.Preview(root, package));
        var files = engine.Inspect(root)!.Files;
        string Target(string p) => Paths.Target(root, p);
        string Status(string id) => Paths.Contained(Paths.Transaction(root, id), "write-status.json");
        Journal JournalAt(string id) => Contract.Read<Journal>(File.ReadAllBytes(Paths.Contained(Paths.Transaction(root, id), "journal.json")));
        Dictionary<string, DateTime> Stamp() => files.ToDictionary(f => f.Path, f => File.GetLastWriteTimeUtc(Target(f.Path)));
        var calls = new List<string>();
        var oldObserver = InstallationEngine.TargetWriteObserved;
        var oldRunning = InstallationEngine.GameRunning;
        InstallationEngine.TargetWriteObserved = p => calls.Add(Path.GetRelativePath(root, p).Replace('\\', '/'));
        try
        {
            foreach (var f in files) File.SetLastWriteTimeUtc(Target(f.Path), new DateTime(2020, 1, 2, 3, 4, 5, DateTimeKind.Utc));
            var stamps = Stamp(); string oldReceipt = engine.ReceiptHash(root);
            // Read-only leases would reject File.Replace: this independently proves
            // that unchanged files are not merely rewritten with restored timestamps.
            var leases = files.Select(f => new FileStream(Target(f.Path), FileMode.Open, FileAccess.Read, FileShare.Read)).ToArray();
            OperationResult same;
            try { same = engine.Repair(engine.PreviewRepair(root, package), true); }
            finally { foreach (var lease in leases) lease.Dispose(); }
            Check(calls.SequenceEqual(new[] { Contract.StatePath }), "all same: only real receipt target written");
            Check(stamps.All(f => File.GetLastWriteTimeUtc(Target(f.Key)) == f.Value), "all same: payload mtime preserved");
            Check(engine.ReceiptHash(root) != oldReceipt, "new receipt really written");
            var journal = JournalAt(same.TransactionId);
            Check(journal.Changes.Length == files.Length + 1, "all changes retained");
            Check(journal.Changes[..^1].Select(c => c.Path).ToHashSet().SetEquals(files.Select(f => f.Path)), "receipt/journal path sets equal");
            foreach (var c in journal.Changes[..^1])
            {
                Check(c.BeforeHash == c.AfterHash, "same bytes identifiable in legacy fields");
                Check(Paths.FileHash(Paths.Contained(Paths.Transaction(root, same.TransactionId), c.BeforeFile!)) == c.BeforeHash, "before image retained");
                Check(Paths.FileHash(Paths.Contained(Paths.Transaction(root, same.TransactionId), c.AfterFile!)) == c.AfterHash, "after image retained");
            }
            using (var status = JsonDocument.Parse(File.ReadAllBytes(Status(same.TransactionId))))
            {
                Check(status.RootElement.EnumerateObject().Count() == 3, "optional status shape");
                Check(status.RootElement.GetProperty("transactionId").GetString() == same.TransactionId, "status transaction binding");
                var entries = status.RootElement.GetProperty("changes").EnumerateArray().ToArray();
                Check(entries.Length == journal.Changes.Length, "status index matches journal");
                for (int i = 0; i < files.Length; ++i)
                    Check(entries[i].GetProperty("path").GetString() == journal.Changes[i].Path && !entries[i].GetProperty("written").GetBoolean() && entries[i].GetProperty("reason").GetString() == "unchanged", "explicit unchanged status");
                Check(entries[^1].GetProperty("written").GetBoolean(), "receipt status written");
            }
            calls.Clear(); stamps = Stamp();
            engine.Install(engine.Preview(root, package));
            Check(calls.SequenceEqual(new[] { Contract.StatePath }), "ordinary update skips all identical payloads");
            Check(stamps.All(f => File.GetLastWriteTimeUtc(Target(f.Key)) == f.Value), "ordinary update preserves payload mtime");

            byte[] original0 = File.ReadAllBytes(Target(files[0].Path));
            byte[] original1 = File.ReadAllBytes(Target(files[1].Path));
            byte[] changed = Encoding.UTF8.GetBytes("INERT USER CHANGED BYTES");
            File.WriteAllBytes(Target(files[0].Path), changed); File.WriteAllBytes(Target(files[1].Path), changed);
            calls.Clear(); Reject(() => engine.Preview(root, package), "present different bytes still refused by normal install");
            Check(calls.Count == 0, "refusal did not write");
            stamps = Stamp(); calls.Clear();
            var mixed = engine.Repair(engine.PreviewRepair(root, package), true);
            Check(calls.SequenceEqual(new[] { files[0].Path, files[1].Path, Contract.StatePath }), "partial change: only changed payloads and receipt written");
            Check(files.Skip(2).All(f => File.GetLastWriteTimeUtc(Target(f.Path)) == stamps[f.Path]), "partial change: same files retain mtime");
            Check(File.ReadAllBytes(Target(files[0].Path)).SequenceEqual(original0) && File.ReadAllBytes(Target(files[1].Path)).SequenceEqual(original1), "changed payload restored");

            File.Delete(Target(files[0].Path)); calls.Clear();
            engine.Install(engine.Preview(root, package));
            Check(calls.SequenceEqual(new[] { files[0].Path, Contract.StatePath }), "absent owned file recreated, not skipped");
            Check(File.ReadAllBytes(Target(files[0].Path)).SequenceEqual(original0), "missing file exact bytes");

            var shown = engine.Preview(root, package); oldReceipt = engine.ReceiptHash(root);
            File.WriteAllBytes(Target(files[0].Path), changed); calls.Clear();
            Reject(() => engine.Install(shown), "external edit after preview");
            Check(calls.Count == 0 && engine.ReceiptHash(root) == oldReceipt && File.ReadAllBytes(Target(files[0].Path)).SequenceEqual(changed), "preview race preserves external bytes and receipt");
            File.WriteAllBytes(Target(files[0].Path), original0);

            File.WriteAllBytes(Target(files[0].Path), changed); File.WriteAllBytes(Target(files[1].Path), changed);
            oldReceipt = engine.ReceiptHash(root); calls.Clear();
            Reject(() => engine.Repair(engine.PreviewRepair(root, package), true, p => { if (p == files[1].Path) throw new IOException("INERT mid-write failure"); }), "mid-write failure");
            Check(calls.SequenceEqual(new[] { files[0].Path }), "one forward write completed before failure");
            Check(engine.ReceiptHash(root) == oldReceipt && !engine.HasPending(root) && File.ReadAllBytes(Target(files[0].Path)).SequenceEqual(changed) && File.ReadAllBytes(Target(files[1].Path)).SequenceEqual(changed), "existing rollback restores modified before images and receipt");
            File.WriteAllBytes(Target(files[0].Path), original0); File.WriteAllBytes(Target(files[1].Path), original1);

            var current = engine.Inspect(root)!; string journalPath = Paths.Contained(Paths.Transaction(root, current.TransactionId), "journal.json");
            byte[] raw = File.ReadAllBytes(journalPath); var node = JsonNode.Parse(raw)!.AsObject();
            node["unexpected"] = true; Reject(() => Contract.Read<Journal>(Encoding.UTF8.GetBytes(node.ToJsonString())), "strict journal still rejects unknown key");
            node = JsonNode.Parse(raw)!.AsObject(); node["changes"]!.AsArray().RemoveAt(0);
            File.WriteAllText(journalPath, node.ToJsonString()); Reject(() => engine.Preview(root, package), "receipt/journal set mismatch still refused"); File.WriteAllBytes(journalPath, raw);
            string statusPath = Status(current.TransactionId);
            File.WriteAllText(statusPath, "{"); Check(engine.Preview(root, package).Prior != null, "corrupt optional sidecar ignored");
            File.Delete(statusPath); Check(engine.Preview(root, package).Prior != null, "absent optional sidecar ignored");

            calls.Clear(); FileStream? lockProtection = null;
            OperationResult post;
            try
            {
                post = engine.Install(engine.Preview(root, package), p =>
                {
                    if (p == Contract.StatePath) lockProtection = new FileStream(Paths.Contained(root, UserSettings.ReceiptPath), FileMode.Open, FileAccess.Read, FileShare.Read);
                });
            }
            finally { lockProtection?.Dispose(); }
            Check(post.PendingPostprocess?.SequenceEqual(new[] { "settingsProtection" }) == true, "postprocess failure remains separately retryable");
            Check(calls.SequenceEqual(new[] { Contract.StatePath }), "postprocess failure did not repeat identical payload writes");
            oldReceipt = engine.ReceiptHash(root); calls.Clear(); File.WriteAllText(Status(post.TransactionId), "CORRUPT OPTIONAL DATA");
            var retried = engine.RetryPostprocessing(root, post.TransactionId);
            Check(retried.PendingPostprocess == null && calls.Count == 0 && engine.ReceiptHash(root) == oldReceipt, "postprocess retry ignores corrupt sidecar and does not replace targets");

            calls.Clear(); OperationResult optional = engine.Install(engine.Preview(root, package), p =>
            {
                if (p != Contract.StatePath) return;
                var pending = Contract.Read<Pending>(File.ReadAllBytes(Paths.Contained(root, Contract.PendingPath)));
                string s = Status(pending.TransactionId);
                File.Move(s, s + ".preserved"); Directory.CreateDirectory(s);
            });
            Check(optional.PendingPostprocess == null && engine.Inspect(root)!.TransactionId == optional.TransactionId, "unwritable optional sidecar never fails product commit");
            Check(calls.SequenceEqual(new[] { Contract.StatePath }), "optional metadata failure does not change target plan");
            calls.Clear(); InstallationEngine.GameRunning = () => true;
            Reject(() => engine.Repair(engine.PreviewRepair(root, package), true), "running game guard even when all bytes identical");
            Check(calls.Count == 0, "running game rejection has zero target writes");
            InstallationEngine.GameRunning = oldRunning;
            calls.Clear(); engine.Remove(root, engine.ReceiptHash(root));
            Check(calls.Count == files.Length + 1, "Remove restoration writes retain their previous behavior");
        }
        finally { InstallationEngine.TargetWriteObserved = oldObserver; InstallationEngine.GameRunning = oldRunning; }
        File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory, "unchanged-write-validation.json"), Contract.Encode(new { passed = true, checks, gameLaunches = 0, fixture, journalSchemaChanged = false }));
        return 0;
    }
}
