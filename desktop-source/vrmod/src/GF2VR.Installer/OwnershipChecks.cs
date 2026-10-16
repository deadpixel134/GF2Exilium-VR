using System.Text;
using System.Text.Json.Nodes;

namespace GF2VR.Installer;

// Real verified product bytes with inert game fixtures; never loads a native DLL.
internal static class OwnershipChecks
{
    private const string GuiContract = NativeLayout.ToolRoot + "runtime-contract.json";
    private static int checks;
    private static void Assert(bool ok, string label) { ++checks; if (!ok) throw new InvalidDataException(label); }
    private static void Reject(Action work, string key, string label)
    {
        ++checks;
        try { work(); }
        catch (InstallError error) when (error.Key == key) { return; }
        throw new InvalidDataException("Expected " + key + ": " + label);
    }
    private static string Game(string fixture, string name, bool direct)
    {
        string root = Path.Combine(fixture, name);
        Paths.WriteNew(Paths.Contained(root, direct ? "GF2_Exilium.exe" : "GF2 Game/GF2_Exilium.exe"), Encoding.UTF8.GetBytes(SelfTest.Inert));
        Paths.WriteNew(Paths.Contained(root, "vrmod/config/navigation.json"), Encoding.UTF8.GetBytes("PRESERVE USER CONFIGURATION"));
        Paths.WriteNew(Paths.Target(root, "GF2 Game/unrelated-mod.dll"), Encoding.UTF8.GetBytes("PRESERVE UNRELATED MOD"));
        return root;
    }
    private static Dictionary<string, string> Snapshot(string root) => Directory.GetFiles(root, "*", SearchOption.AllDirectories)
        .ToDictionary(p => Path.GetRelativePath(root, p), p => Paths.FileHash(p)!, StringComparer.Ordinal);
    private static void Unchanged(string root, Dictionary<string, string> before, string label)
    {
        var after = Snapshot(root);
        Assert(before.Count == after.Count && before.All(f => after.TryGetValue(f.Key, out string? hash) && Paths.Equal(hash, f.Value)), label);
    }
    private static Dictionary<string, byte[]?> ProductBefore(string root, Package package) => package.Files
        .ToDictionary(f => f.Path, f => Paths.Bytes(Paths.Target(root, f.Path)), StringComparer.Ordinal);
    private static void Restored(InstallationEngine engine, string root, IReadOnlyDictionary<string, byte[]?> before)
    {
        foreach (var f in before)
        {
            byte[]? actual = Paths.Bytes(Paths.Target(root, f.Key));
            Assert(f.Value == null ? actual == null : actual != null && f.Value.SequenceEqual(actual), "restored exact preexisting bytes/absence " + f.Key);
        }
        Assert(engine.Inspect(root) == null && !File.Exists(Paths.Contained(root, Contract.StatePath)), "missing receipt restored");
        Assert(!engine.HasPending(root), "no pending authority after restoration");
        Assert(File.ReadAllText(Paths.Contained(root, "vrmod/config/navigation.json")) == "PRESERVE USER CONFIGURATION", "user configuration preserved");
        Assert(File.ReadAllText(Paths.Target(root, "GF2 Game/unrelated-mod.dll")) == "PRESERVE UNRELATED MOD", "unrelated mod preserved");
        Assert(File.ReadAllText(Paths.GameExecutable(root)) == SelfTest.Inert, "original executable preserved");
    }
    private static void Seed(string root, CheckedPackage package, IEnumerable<string> paths)
    {
        foreach (string path in paths) Paths.WriteNew(Paths.Target(root, path), package.Payload[path]);
    }
    private static void Installed(InstallationEngine engine, string root, CheckedPackage package)
    {
        var receipt = engine.Inspect(root)!;
        Assert(receipt.Files.Length == package.Package.Files.Length, "complete installed receipt");
        foreach (var f in receipt.Files)
        {
            byte[] expected = package.Payload[f.Path];
            Assert(Paths.Equal(f.Sha256, Paths.Hash(expected)) && f.Bytes == expected.Length &&
                File.ReadAllBytes(Paths.Target(root, f.Path)).SequenceEqual(expected), "verified final installed bytes " + f.Path);
        }
        var contract = JsonNode.Parse(File.ReadAllBytes(Paths.Target(root, GuiContract)))!.AsObject();
        Assert(contract["configurationRoot"]!.GetValue<string>() == Path.Combine(root, "vrmod", "config"), "runtime contract bound to actual root");
    }
    internal static int Run(string actualPackage)
    {
        InstallerText.Verify();
        string fixture = Path.Combine(AppContext.BaseDirectory, "ownership-validation-fixture");
        if (Directory.Exists(fixture)) throw new InvalidDataException("Preserve fixture; use fresh ownership output.");
        var sourceSnapshot = Snapshot(actualPackage);
        var source = Contract.Read<Package>(File.ReadAllBytes(Paths.Contained(actualPackage, "package-manifest.json")));
        Assert(source.Native?.BuildId == NativeLayout.FlexibleBuild && source.BoundGameRoot == null, "verified flexible portable package required");
        var engine = new InstallationEngine();
        string[] partial = ["GF2 Game/version.dll", "vrmod/runtime/openxr_loader.dll", GuiContract];
        int completedLayouts = 0;
        foreach (bool direct in new[] { false, true })
        foreach (string scenario in new[] { "native-only", "contract-only", "partial", "full" })
        {
            string root = Game(fixture, (direct ? "direct-" : "standard-") + scenario, direct);
            var package = engine.CheckPayload(root, actualPackage);
            IEnumerable<string> preexisting = scenario switch
            {
                "native-only" => ["GF2 Game/version.dll"],
                "contract-only" => [GuiContract],
                "partial" => partial,
                _ => source.Files.Select(f => f.Path)
            };
            Seed(root, package, preexisting);
            var before = ProductBefore(root, source);
            var wholeBefore = Snapshot(root);
            Plan plan = engine.Preview(root, actualPackage);
            Unchanged(root, wholeBefore, "preview is read-only " + scenario);
            Assert(plan.Prior == null && plan.PriorHash == null && plan.BeforeHashes.Count == source.Files.Length, "no synthetic ownership in preview");
            foreach (var f in before) Assert(Paths.Equal(plan.BeforeHashes[f.Key], f.Value == null ? null : Paths.Hash(f.Value)), "frozen preview hash " + f.Key);
            Assert(!Directory.Exists(Path.Combine(root, "vrmod", "installer")), "preview created no state directory");
            var installed = engine.Install(plan);
            Installed(engine, root, package);
            Assert(!engine.HasPreviousVersion(root), "retained identical files do not invent a previous receipt");
            string transaction = Paths.Transaction(root, installed.TransactionId);
            var journal = Contract.Read<Journal>(File.ReadAllBytes(Paths.Contained(transaction, "journal.json")));
            foreach (var f in before)
            {
                var change = journal.Changes.Single(c => c.Path == f.Key);
                Assert(Paths.Equal(change.BeforeHash, f.Value == null ? null : Paths.Hash(f.Value)), "journal original hash " + f.Key);
                Assert(f.Value == null ? change.BeforeFile == null : change.BeforeFile != null &&
                    File.ReadAllBytes(Paths.Contained(transaction, change.BeforeFile)).SequenceEqual(f.Value), "durable original bytes " + f.Key);
            }
            Assert(journal.Changes[^1].Path == Contract.StatePath && journal.Changes[^1].BeforeHash == null, "journal preserves prior receipt absence");
            engine.Remove(root, engine.ReceiptHash(root));
            Restored(engine, root, before);
            ++completedLayouts;
        }
        // Neither an arbitrary loader nor an unbound package contract is a match.
        foreach (string path in new[] { "GF2 Game/version.dll", "vrmod/runtime/openxr_loader.dll", GuiContract })
        {
            string root = Game(fixture, "unknown-" + Path.GetFileName(path), true);
            byte[] unknown = path == GuiContract ? File.ReadAllBytes(Paths.Contained(actualPackage, "payload/" + path)) : Encoding.UTF8.GetBytes("UNOWNED CONTENT MUST SURVIVE");
            Paths.WriteNew(Paths.Target(root, path), unknown);
            var before = Snapshot(root);
            Reject(() => engine.Preview(root, actualPackage), "unowned", "unknown bytes " + path);
            Unchanged(root, before, "unowned refusal preserved every file");
            Assert(!Directory.Exists(Path.Combine(root, "vrmod", "installer")), "unowned refusal wrote no state");
        }
        // Freeze existence too: a new or removed exact matching file still
        // changes the reviewed before-state and requires a fresh preview.
        foreach (string scenario in new[] { "appeared", "disappeared", "modified" })
        {
            string root = Game(fixture, "race-" + scenario, true);
            var package = engine.CheckPayload(root, actualPackage);
            string dll = Paths.Target(root, "GF2 Game/version.dll");
            if (scenario != "appeared") Paths.WriteNew(dll, package.Payload["GF2 Game/version.dll"]);
            Plan plan = engine.Preview(root, actualPackage);
            if (scenario == "appeared") Paths.WriteNew(dll, package.Payload["GF2 Game/version.dll"]);
            else if (scenario == "disappeared") File.Delete(dll);
            else File.AppendAllText(dll, "EXTERNAL CHANGE");
            var changed = Snapshot(root);
            Reject(() => engine.Install(plan), scenario == "modified" ? "unowned" : "previewChanged", "preview-to-install race " + scenario);
            Unchanged(root, changed, "race refused without writes");
        }
        // An intact receipt keeps existing ownership rules, even for the same
        // package. A modified owned file is never re-adopted through this path.
        {
            string root = Game(fixture, "owned-modified", false);
            var package = engine.CheckPayload(root, actualPackage);
            engine.Install(engine.Preview(root, actualPackage));
            Installed(engine, root, package);
            string dll = Paths.Target(root, "GF2 Game/version.dll");
            byte[] original = File.ReadAllBytes(dll); File.AppendAllText(dll, "EXTERNAL CHANGE");
            var changed = Snapshot(root);
            Reject(() => engine.Preview(root, actualPackage), "modified", "modified owned reinstall");
            Reject(() => engine.Remove(root, engine.ReceiptHash(root)), "modified", "modified owned removal");
            Unchanged(root, changed, "owned modification preserved");
            File.WriteAllBytes(dll, original);
            string firstReceipt = engine.ReceiptHash(root);
            engine.Install(engine.Preview(root, actualPackage));
            Assert(engine.HasPreviousVersion(root), "intact owned reinstall records previous version");
            Assert(engine.Remove(root, engine.ReceiptHash(root)).RestoredPrevious && Paths.Equal(engine.ReceiptHash(root), firstReceipt), "owned reinstall removal restores exact prior receipt");
            engine.Remove(root, engine.ReceiptHash(root));
            Restored(engine, root, source.Files.ToDictionary(f => f.Path, _ => (byte[]?)null, StringComparer.Ordinal));
        }
        // Failure after writes must restore both retained exact files and
        // previously missing files, without manufacturing a receipt.
        {
            string root = Game(fixture, "partial-rollback", true);
            var package = engine.CheckPayload(root, actualPackage);
            Seed(root, package, partial);
            var before = ProductBefore(root, source);
            int writes = 0; bool failed = false;
            try
            {
                engine.Install(engine.Preview(root, actualPackage), _ => { if (++writes == 4) throw new IOException("INERT OWNERSHIP ROLLBACK FIXTURE"); });
            }
            catch (IOException) { failed = true; }
            Assert(failed && writes == 4, "real partial transaction failed after earlier writes");
            Restored(engine, root, before);
            string journalPath = Directory.GetFiles(Path.Combine(root, "vrmod", "installer", "transactions"), "journal.json", SearchOption.AllDirectories).Single();
            Assert(Contract.Read<Journal>(File.ReadAllBytes(journalPath)).State == "recovered", "transaction rollback recorded");
        }
        foreach (string language in InstallerText.Languages)
        {
            InstallerText.Language = language;
            Assert(InstallerText.Get("unowned") != InstallerText.Get("modified"), "distinct actionable ownership messages " + language);
            Assert(InstallerText.Get("bundledPackageMissing") != InstallerText.Get("unavailable") &&
                InstallerText.Get("bundledPackageHelp") != InstallerText.Get("unavailable"), "packaged automatic-discovery guidance " + language);
        }
        Unchanged(actualPackage, sourceSnapshot, "verified source package unchanged");
        File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory, "ownership-validation.json"), Contract.Encode(new
        {
            passed = true, checks, completedLayouts, packageVersion = source.Version, nativeDllSha256 = source.Native!.DllSha256,
            exactUnownedOnly = true, boundContractVerified = true, previewRacesRefused = 3, ownedModifiedRefused = true,
            removeRestoresPreexistingBytesAndReceiptAbsence = true, partialTransactionRollback = true,
            locales = InstallerText.Languages, nativeDllsLoaded = 0, gameLaunches = 0, actualInstallationWrites = 0, fixture
        }));
        return 0;
    }
}
