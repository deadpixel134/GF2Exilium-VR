using System.Security.AccessControl;
using System.Security.Principal;
using System.Text;

namespace GF2VR.Installer;

internal static class RepairChecks
{
    internal static int Run(string package)
    {
        int checks = 0;
        void Check(bool ok, string why) { ++checks; if (!ok) throw new InvalidDataException(why); }
        void Reject(Action work, string why)
        {
            ++checks;
            try { work(); }
            catch (Exception ex) when (ex is InstallError or IOException or UnauthorizedAccessException) { return; }
            throw new InvalidDataException("Expected rejection: " + why);
        }
        string fixture = Path.Combine(AppContext.BaseDirectory, "repair-validation-fixture");
        if (Directory.Exists(fixture)) throw new InvalidDataException("Preserve old repair fixture");
        string Game(string name)
        {
            string r = Path.Combine(fixture, name);
            Paths.WriteNew(Paths.Contained(r, "GF2 Game/GF2_Exilium.exe"), Encoding.UTF8.GetBytes(SelfTest.Inert));
            return r;
        }
        var engine = new InstallationEngine();
        string root = Game("owned"), dll = Paths.Target(root, "GF2 Game/version.dll");
        engine.Install(engine.Preview(root, package));
        byte[] original = File.ReadAllBytes(dll);
        File.Move(dll, dll + ".quarantine");
        Check(UpdateStaging.ManagedVersion(root) != null, "verified receipt permits absent owned file");
        Check(!File.Exists(dll), "eligibility never reinstalls");
        engine.Install(engine.Preview(root, package));
        Check(File.ReadAllBytes(dll).SequenceEqual(original), "ordinary reinstall restores absence");
        Check(File.ReadAllBytes(dll + ".quarantine").SequenceEqual(original), "quarantine preserved");
        string loaderFolder = Path.Combine(root, "vrmod", "runtime");
        Directory.Move(loaderFolder, loaderFolder + ".quarantine");
        Check(UpdateStaging.ManagedVersion(root) != null, "DirectoryNotFound also permits owned absence");
        engine.Install(engine.Preview(root, package));
        Check(File.Exists(Paths.Target(root, "vrmod/runtime/openxr_loader.dll")), "missing directory reinstalls");
        byte[] changed = Encoding.UTF8.GetBytes("INERT USER MODIFIED BYTES"); File.WriteAllBytes(dll, changed);
        Reject(() => engine.Preview(root, package), "normal update still refuses changed bytes");
        var repair = engine.PreviewRepair(root, package);
        var before = repair.Files.Single(f => f.Path == "GF2 Game/version.dll");
        Check(before.BeforeHash == Paths.Hash(changed) && before.AfterHash == Paths.Hash(original) && before.Selected, "per-file repair hashes");
        string receiptBefore = engine.ReceiptHash(root);
        Reject(() => engine.Repair(repair, false), "no consent");
        Reject(() => engine.Install(repair.Plan), "repair cannot use ordinary install");
        Check(engine.ReceiptHash(root) == receiptBefore && File.ReadAllBytes(dll).SequenceEqual(changed), "rejections are read-only");
        File.AppendAllText(dll, "race"); Reject(() => engine.Repair(repair, true), "changed preview"); File.WriteAllBytes(dll, changed);
        string config = Paths.Contained(root, UserSettings.PerformancePath), log = Paths.Contained(root, "vrmod/logs/bootstrap/GF2VR-DIRTY-REPAIR.jsonl");
        byte[] opaque = Encoding.UTF8.GetBytes("{\"unknown\":{\"renderScale\":0.731},\"dynamicEyeScale\":{\"enabled\":false}}");
        File.WriteAllBytes(config, opaque); Paths.WriteNew(log, changed);
        string other = Paths.Target(root, "GF2 Game/other-mod.dll"), backup = Paths.Contained(root, "vrmod/user-backups/keep.bin");
        Paths.WriteNew(other, changed); Paths.WriteNew(backup, changed);
        var fixedResult = engine.Repair(engine.PreviewRepair(root, package), true);
        var journal = Contract.Read<Journal>(File.ReadAllBytes(Paths.Contained(Paths.Transaction(root, fixedResult.TransactionId), "journal.json")));
        var c = journal.Changes.Single(f => f.Path == "GF2 Game/version.dll");
        Check(File.ReadAllBytes(Paths.Contained(Paths.Transaction(root, fixedResult.TransactionId), c.BeforeFile!)).SequenceEqual(changed), "changed original backed up");
        Check(journal.Changes[^1].BeforeHash == receiptBefore, "real prior receipt backed up");
        Check(new[] { log, other, backup }.All(p => File.ReadAllBytes(p).SequenceEqual(changed)) && File.ReadAllBytes(config).SequenceEqual(opaque), "logs/settings/other mods/backups preserved by repair");
        engine.Remove(root, engine.ReceiptHash(root));
        Check(File.ReadAllBytes(dll).SequenceEqual(changed) && engine.ReceiptHash(root) == receiptBefore, "repair rollback restores exact changed bytes and receipt");
        File.WriteAllBytes(dll, original);
        var snapshot = engine.Inspect(root)!.Files.ToDictionary(f => f.Path, f => Paths.FileHash(Paths.Target(root, f.Path)));
        Reject(() => engine.Repair(engine.PreviewRepair(root, package), true, p => { if (p == snapshot.Keys.Skip(1).First()) throw new IOException("INERT partial failure"); }), "mid-install failure");
        Check(snapshot.All(f => f.Value == Paths.FileHash(Paths.Target(root, f.Key))) && engine.ReceiptHash(root) == receiptBefore && !engine.HasPending(root), "partial failure rollback exact");
        using (var locked = new FileStream(dll, FileMode.Open, FileAccess.Read, FileShare.None))
        { Reject(() => engine.Preview(root, package), "sharing violation is not absence"); Reject(() => engine.PreviewRepair(root, package), "repair cannot bypass lock"); }
        var fileInfo = new FileInfo(dll); var acl = fileInfo.GetAccessControl();
        var denied = fileInfo.GetAccessControl(); denied.AddAccessRule(new FileSystemAccessRule(WindowsIdentity.GetCurrent().User!, FileSystemRights.ReadData, AccessControlType.Deny));
        try { fileInfo.SetAccessControl(denied); Reject(() => engine.Preview(root, package), "access denied is not absence"); }
        finally { fileInfo.SetAccessControl(acl); }
        string linked = Game("reparse"), outside = Path.Combine(fixture,"outside-link");
        Directory.CreateDirectory(outside); File.WriteAllBytes(Path.Combine(outside,"keep.bin"),changed);
        var linkStart = new System.Diagnostics.ProcessStartInfo("cmd.exe", "/d /c mklink /J \""+Path.Combine(linked,"vrmod")+"\" \""+outside+"\"")
        { UseShellExecute=false, CreateNoWindow=true, WindowStyle=System.Diagnostics.ProcessWindowStyle.Hidden, RedirectStandardOutput=true, RedirectStandardError=true };
        using(var linker=System.Diagnostics.Process.Start(linkStart)!) { linker.WaitForExit(); Check(linker.ExitCode==0,"isolated junction fixture created"); }
        Reject(()=>engine.PreviewRepair(linked,package),"repair rejects reparse target");
        Check(File.ReadAllBytes(Path.Combine(outside,"keep.bin")).SequenceEqual(changed),"reparse destination preserved");
        var running = InstallationEngine.GameRunning;
        try { InstallationEngine.GameRunning = () => true; Reject(() => engine.Repair(engine.PreviewRepair(root, package), true), "running game guard"); }
        finally { InstallationEngine.GameRunning = running; }
        string stateFile = Paths.Contained(root, Contract.StatePath); byte[] stateBytes = File.ReadAllBytes(stateFile);
        File.WriteAllText(stateFile, "{"); Reject(() => engine.PreviewRepair(root, package), "corrupt receipt grants no repair authority"); File.WriteAllBytes(stateFile, stateBytes);
        string foreign = Game("foreign"), foreignDll = Paths.Target(foreign, "GF2 Game/version.dll"); Paths.WriteNew(foreignDll, changed);
        var foreignPlan = engine.PreviewRepair(foreign, package);
        Check(!foreignPlan.CanCommit && foreignPlan.Files.Single(f => f.Path == "GF2 Game/version.dll") is { Unowned: true, Selected: false }, "unknown ownership excluded by default");
        Reject(() => engine.Repair(foreignPlan, true), "no implicit foreign overwrite");
        var included = engine.PreviewRepair(foreign, package, true); var imported = engine.Repair(included, true);
        engine.Remove(foreign, engine.ReceiptHash(foreign));
        Check(File.ReadAllBytes(foreignDll).SequenceEqual(changed) && engine.Inspect(foreign) == null, "explicit foreign replacement backs up and restores original");
        foreach (string step in new[] { "transactionFinalize", "settingsProtection", "updateResult" })
        {
            string r = Game("post-" + step); engine.Install(engine.Preview(r, package));
            FileStream? lease = null;
            string? stage = null;
            if (step == "updateResult")
            {
                stage = Path.Combine(fixture, "update-stage");
                var manifest = Contract.Read<Package>(File.ReadAllBytes(Paths.Contained(package, "package-manifest.json")));
                var descriptor = new UpdateDescriptor(1, Contract.Product, UpdatePolicy.Repository, manifest.Version, "stable", UpdatePolicy.ArchiveName(manifest.Version, manifest.Native!.BuildFlavor), new string('A',64), Paths.FileHash(Paths.Contained(package,"package-manifest.json"))!,
                    UpdatePolicy.HelperFiles.Select(f => new PayloadFile(f,Paths.FileHash(Path.Combine(AppContext.BaseDirectory,f))!,new FileInfo(Path.Combine(AppContext.BaseDirectory,f)).Length)).ToArray(), manifest.Native.BuildFlavor, manifest.Native.CommonCoreSha256);
                byte[] metadata = Contract.Encode(descriptor);
                Paths.WriteNew(Paths.Contained(stage,"release-descriptor.json"), metadata);
                Paths.WriteNew(Paths.Contained(stage,"handoff.json"), Contract.Encode(new UpdateHandoff(1,r,manifest.Version,engine.ReceiptHash(r),descriptor.PackageManifestSha256,Paths.Hash(metadata),0,0,true)));
                // An occupied destination is a real postcommit write failure.
                Directory.CreateDirectory(Path.Combine(stage,"update-result.json"));
            }
            OperationResult pending;
            try
            {
                pending = engine.Install(engine.Preview(r, package), p =>
                {
                    if (p == Contract.StatePath && step != "updateResult") lease = new FileStream(Paths.Contained(r, step == "transactionFinalize" ? Contract.PendingPath : UserSettings.ReceiptPath), FileMode.Open, FileAccess.Read, FileShare.Read);
                }, updateStage: stage);
            }
            finally { lease?.Dispose(); }
            if (stage != null) Directory.Move(Path.Combine(stage,"update-result.json"),Path.Combine(stage,"blocked-result-preserved"));
            Check(pending.PendingPostprocess?.SequenceEqual(new[] { step }) == true, "commit separated from failed " + step);
            Check(engine.Inspect(r)!.TransactionId == pending.TransactionId, "product receipt committed despite " + step);
            Check(engine.FindPendingPostprocessing(r) == pending.TransactionId, "durable unfinished step discoverable");
            var product = engine.Inspect(r)!.Files.ToDictionary(f => f.Path, f => Paths.FileHash(Paths.Target(r, f.Path)));
            byte[] protectionBefore = File.ReadAllBytes(Paths.Contained(r, UserSettings.ReceiptPath));
            if (stage != null)
            {
                string handoff = Paths.Contained(stage,"handoff.json");byte[] stable = File.ReadAllBytes(handoff);
                var malformed = Contract.Read<UpdateHandoff>(stable) with { SchemaVersion = 2 };
                try
                {
                    File.WriteAllBytes(handoff,Contract.Encode(malformed));
                    Check(engine.RetryPostprocessing(r,pending.TransactionId).PendingPostprocess?.SequenceEqual(new[]{"updateResult"})==true&&!File.Exists(Path.Combine(stage,"update-result.json")),"retry still validates stable handoff schema");
                    Check(engine.Inspect(r)!.TransactionId==pending.TransactionId,"bad handoff never changes committed product");
                }
                finally { File.WriteAllBytes(handoff,stable); }
            }
            var retried = engine.RetryPostprocessing(r, pending.TransactionId);
            Check(retried.PendingPostprocess == null && !engine.HasPending(r), "retry completes only unfinished step");
            Check(engine.Inspect(r)!.TransactionId == pending.TransactionId && product.All(f => f.Value == Paths.FileHash(Paths.Target(r, f.Key))), "retry never reinstalls product");
            if (step == "transactionFinalize") Check(File.ReadAllBytes(Paths.Contained(r, UserSettings.ReceiptPath)).SequenceEqual(protectionBefore), "successful protection step not repeated");
            File.AppendAllText(Paths.Target(r, "GF2 Game/version.dll"), "later change");
            Reject(() => engine.RetryPostprocessing(r, pending.TransactionId), "stale product prevents retry");
        }
        foreach (string lang in InstallerText.Languages)
        {
            InstallerText.Language = lang;
            Check(InstallerText.Get("repair") != "repair" && InstallerText.Get("postprocessPending").Length > 20, "packaged repair/final-step language " + lang);
        }
        File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory, "repair-validation.json"), Contract.Encode(new { passed = true, checks, fixture, actualGameLaunches = 0, nativeDllsLoaded = 0, aclTest = true, partialRollback = true, postcommitFailures = 3, syntheticActualReceipt = false }));
        return 0;
    }
}
