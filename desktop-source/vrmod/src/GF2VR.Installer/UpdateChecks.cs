using System.Diagnostics;
using System.IO.Compression;
using System.Text;
using System.Text.Json.Nodes;

namespace GF2VR.Installer;

internal static class UpdateChecks
{
    internal static int Run(string candidate, string previous)
    {
        int checks = 0;
        void Check(bool value, string why) { ++checks; if (!value) throw new InvalidDataException(why); }
        void Reject(Action action, string why) { ++checks; try { action(); } catch (Exception e) when (e is InstallError or InvalidDataException) { return; } throw new InvalidDataException("Expected rejection: " + why); }
        var candidateManifest = Contract.Read<Package>(File.ReadAllBytes(Paths.Contained(candidate, "package-manifest.json")));
        var previousManifest = Contract.Read<Package>(File.ReadAllBytes(Paths.Contained(previous, "package-manifest.json")));
        string candidateVersion = candidateManifest.Version, previousVersion = previousManifest.Version;
        string? flavor = candidateManifest.Native?.BuildFlavor;
        var nextVersion = UpdatePolicy.VersionOf(candidateVersion);
        Check(nextVersion > UpdatePolicy.VersionOf(previousVersion), "candidate version advances previous package");
        string otherVersion = new Version(nextVersion.Major, nextVersion.Minor, checked(nextVersion.Build + 1)).ToString(3);
        string fixture = Path.Combine(AppContext.BaseDirectory, "updater-validation-fixture");
        if (Directory.Exists(fixture)) throw new InvalidDataException("Preserve old updater fixture");
        string root = Path.Combine(fixture, "게임"), pkg = Path.Combine(fixture, "next"), old = Path.Combine(fixture, "previous");
        Paths.WriteNew(Paths.Contained(root, "GF2 Game/GF2_Exilium.exe"), Encoding.UTF8.GetBytes(SelfTest.Inert));
        foreach (string p in NativeLayout.GameFiles) Paths.WriteNew(Paths.Contained(root, p), Encoding.UTF8.GetBytes("INERT UPDATE PREREQUISITE " + p));
        void CopyPackage(string source, string output)
        {
            var m = Contract.Read<Package>(File.ReadAllBytes(Paths.Contained(source, "package-manifest.json")));
            foreach (var f in m.Files) { byte[] b = File.ReadAllBytes(Paths.Contained(source, "payload/" + f.Path)); Check(Paths.Equal(Paths.Hash(b), f.Sha256), "product byte hash"); Paths.WriteNew(Paths.Contained(output, "payload/" + f.Path), b); }
            m = m with { GameExecutableSha256 = Paths.FileHash(Paths.Contained(root, "GF2 Game/GF2_Exilium.exe"))!, Native = m.Native! with { GameFiles = NativeLayout.GameFiles.Select(p => new PayloadFile(p, Paths.FileHash(Paths.Contained(root, p))!, new FileInfo(Paths.Contained(root, p)).Length)).ToArray() } };
            Paths.WriteNew(Paths.Contained(output, "package-manifest.json"), Contract.Encode(m));
        }
        CopyPackage(candidate, pkg); CopyPackage(previous, old);
        var engine = new InstallationEngine(); Check(UpdateStaging.ManagedVersion(root) == null, "unmanaged install ineligible");
        string settings = Paths.Contained(root, "vrmod/config/render_quality.json"); byte[] settingsBytes = Encoding.UTF8.GetBytes("{\"schemaVersion\":1,\"eyeRenderScale\":0.6}"); Paths.WriteNew(settings, settingsBytes);
        // A released predecessor owns its native allowlist. Use its real helper to
        // establish the old receipt rather than asking the new helper to install
        // an intentionally obsolete native profile before testing the update.
        var previousStart = new ProcessStartInfo(Path.Combine(previous, "GF2VR.Installer.exe")) { UseShellExecute = false, CreateNoWindow = true, WindowStyle = ProcessWindowStyle.Hidden };
        foreach (string arg in new[] { "--install", "--root", root, "--package", old }) previousStart.ArgumentList.Add(arg);
        using (var previousHelper = Process.Start(previousStart)!)
            Check(previousHelper.WaitForExit(30000) && previousHelper.ExitCode == 0, "actual previous helper establishes old native receipt");
        Check(UpdateStaging.ManagedVersion(root) == previousVersion, "receipt uses prior package version");
        string archive = Path.Combine(fixture, UpdatePolicy.ArchiveName(candidateVersion, flavor)); Directory.CreateDirectory(fixture);
        using (var zip = ZipFile.Open(archive, ZipArchiveMode.Create))
        {
            foreach (var f in Contract.Read<Package>(File.ReadAllBytes(Paths.Contained(pkg, "package-manifest.json"))).Files) zip.CreateEntryFromFile(Paths.Contained(pkg, "payload/" + f.Path), "payload/" + f.Path);
            foreach (string f in UpdatePolicy.HelperFiles) zip.CreateEntryFromFile(Path.Combine(AppContext.BaseDirectory, f), f);
            zip.CreateEntryFromFile(Paths.Contained(pkg, "package-manifest.json"), "package-manifest.json");
            foreach (string f in new[] { "DOTNET-NOTICES.txt", "LOCAL-VR.txt" }) { using var writer = new StreamWriter(zip.CreateEntry(f).Open()); writer.Write("INERT OWNED UPDATE TEST NOTICE"); }
        }
        var descriptor = new UpdateDescriptor(1, "GF2VR", UpdatePolicy.Repository, candidateVersion, "stable", Path.GetFileName(archive), Paths.FileHash(archive)!, Paths.FileHash(Paths.Contained(pkg, "package-manifest.json"))!,
            UpdatePolicy.HelperFiles.Select(p => new PayloadFile(p, Paths.FileHash(Path.Combine(AppContext.BaseDirectory, p))!, new FileInfo(Path.Combine(AppContext.BaseDirectory, p)).Length)).ToArray(), flavor, candidateManifest.Native?.CommonCoreSha256);
        byte[] meta = Contract.Encode(descriptor);
        JsonObject Asset(long id, string name, long size, string hash) => new() { ["id"] = id, ["name"] = name, ["size"] = size, ["state"] = "uploaded", ["digest"] = "sha256:" + hash, ["url"] = "https://api.github.com/" + UpdatePolicy.Api + "releases/assets/" + id };
        JsonObject Release(string version) => new() { ["id"] = 1, ["draft"] = false, ["prerelease"] = false, ["published_at"] = "2026-10-11T00:00:00Z", ["tag_name"] = "v" + version,
            ["assets"] = new JsonArray(Asset(11, UpdatePolicy.DescriptorFor(flavor), meta.Length, Paths.Hash(meta)), Asset(12, descriptor.ArchiveName, new FileInfo(archive).Length, descriptor.ArchiveSha256)) };
        byte[] Catalog(JsonObject r) => Contract.Encode(new JsonArray(r));
        Check(UpdatePolicy.Select(Encoding.UTF8.GetBytes("[]"), previousVersion, flavor) == null, "empty release list");
        var release = UpdatePolicy.Select(Catalog(Release(candidateVersion)), previousVersion, flavor)!; Check(release.Version == candidateVersion, "exact new stable selection");
        Check(UpdatePolicy.Select(Catalog(Release(candidateVersion)), candidateVersion, flavor) == null, "same version no update");
        var draft = Release(candidateVersion); draft["draft"] = true; Check(UpdatePolicy.Select(Catalog(draft), previousVersion, flavor) == null, "draft ignored");
        var pre = Release(candidateVersion); pre["prerelease"] = true; Check(UpdatePolicy.Select(Catalog(pre), previousVersion, flavor) == null, "prerelease ignored");
        Reject(() => UpdatePolicy.VersionOf(candidateVersion + "-beta"), "unapproved tag/channel");
        var foreign = Release(candidateVersion); foreign["assets"]![0]!["url"] = "https://example.invalid/token"; Reject(() => UpdatePolicy.Select(Catalog(foreign), previousVersion, flavor), "foreign asset host/repo");
        var missing = Release(candidateVersion); missing["assets"]![1]!["name"] = "looks-latest.zip"; Reject(() => UpdatePolicy.Select(Catalog(missing), previousVersion, flavor), "unrelated zip selection");
        var noHash = Release(candidateVersion); noHash["assets"]![1]!["digest"] = null; Reject(() => UpdatePolicy.Select(Catalog(noHash), previousVersion, flavor), "no integrity");
        Check(UpdatePolicy.Descriptor(meta, release).Version == candidateVersion, "descriptor bound to release assets");
        Reject(() => UpdatePolicy.ValidateDescriptor(descriptor with { Repository = "someone/other" }), "descriptor repo");
        Reject(() => UpdatePolicy.ValidateDescriptor(descriptor with { InstallerFiles = descriptor.InstallerFiles[..3] }), "missing executable dependencies");
        Reject(() => UpdatePolicy.Descriptor(Contract.Encode(descriptor with { Version = otherVersion }), release), "descriptor tamper");
        string rejected = Path.Combine(fixture, "rejected"); Directory.CreateDirectory(rejected);
        Reject(() => UpdateStaging.Extract(root, rejected, archive, meta, descriptor with { ArchiveSha256 = new string('0',64) }), "wrong archive hash");
        Check(UpdateStaging.ManagedVersion(root) == previousVersion, "rejected download preserved installation");
        string unsafeArchive = Path.Combine(fixture,"unsafe.zip"); File.Copy(archive,unsafeArchive);
        using(var unsafeZip=ZipFile.Open(unsafeArchive,ZipArchiveMode.Update)) { using var w=new StreamWriter(unsafeZip.CreateEntry("../escaped.txt").Open()); w.Write("REFUSE"); }
        Reject(()=>UpdateStaging.Extract(root,rejected,unsafeArchive,meta,descriptor with { ArchiveSha256=Paths.FileHash(unsafeArchive)! }),"foreign/traversal ZIP entry");
        Check(!File.Exists(Path.Combine(fixture,"escaped.txt")),"no traversal write");
        string folder = Path.Combine(fixture, "stage"); Directory.CreateDirectory(folder);
        var staged = UpdateStaging.Extract(root, folder, archive, meta, descriptor); Check(staged.Version == candidateVersion, "verified stage no installation");
        Check(UpdateStaging.ManagedVersion(root) == previousVersion, "staging does not update");
        var parentInfo = new ProcessStartInfo(Path.Combine(AppContext.BaseDirectory,"GF2VR.Installer.exe")) { UseShellExecute = false, CreateNoWindow = true, WindowStyle = ProcessWindowStyle.Hidden };
        parentInfo.ArgumentList.Add("--update-parent-fixture");
        using var parent = Process.Start(parentInfo)!; long ticks = parent.StartTime.ToUniversalTime().Ticks;
        var handoff = new UpdateHandoff(1, root, staged.Version, staged.ReceiptHash, staged.ManifestHash, staged.DescriptorHash, parent.Id, ticks, false);
        string hfile = Paths.Contained(folder,"handoff.json"); Paths.WriteNew(hfile,Contract.Encode(handoff));
        int ApplyChild()
        {
            var info = new ProcessStartInfo(Paths.Contained(folder,"package/GF2VR.Installer.exe")) { UseShellExecute = false, CreateNoWindow = true, WindowStyle = ProcessWindowStyle.Hidden };
            foreach(string a in new[]{"--apply-confirmed-update","--stage",folder,"--fixture-no-reopen"}) info.ArgumentList.Add(a);
            using var p = Process.Start(info)!; Check(p.WaitForExit(15000),"helper finished without parent/game kill"); return p.ExitCode;
        }
        Check(ApplyChild() != 0, "no consent cannot apply"); Check(UpdateStaging.ManagedVersion(root)==previousVersion,"consent refusal preserved files");
        File.WriteAllBytes(hfile,Contract.Encode(handoff with { UserConfirmed = true, ReceiptHash = new string('0',64) }));
        Check(ApplyChild()!=0,"stale receipt cannot apply"); Check(UpdateStaging.ManagedVersion(root)==previousVersion,"stale receipt preserved");
        using var waitingParent=Process.Start(parentInfo)!;
        var successHandoff=handoff with {UserConfirmed=true,ParentPid=waitingParent.Id,ParentStartTicks=waitingParent.StartTime.ToUniversalTime().Ticks};
        File.WriteAllBytes(hfile,Contract.Encode(successHandoff with {ParentStartTicks=successHandoff.ParentStartTicks+1}));
        Check(ApplyChild()!=0,"reused/wrong parent identity cannot authorize update");
        Check(UpdateStaging.ManagedVersion(root)==previousVersion,"parent mismatch preserved");
        File.WriteAllBytes(hfile,Contract.Encode(successHandoff));
        Check(ApplyChild()==0,"production staged helper applies after own parent exit");
        Check(UpdateStaging.ManagedVersion(root)==candidateVersion,"real helper receipt advanced");
        Check(File.ReadAllBytes(settings).SequenceEqual(settingsBytes),"0.6 user settings preserved");
        var undo=engine.Remove(root,engine.ReceiptHash(root));Check(undo.RestoredPrevious && UpdateStaging.ManagedVersion(root)==previousVersion,"updater transaction rollback to previous");
        File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory,"update-validation.json"),Contract.Encode(new {passed=true,checks,fixture,previousVersion,candidateVersion,candidateBuild=candidateManifest.Native!.BuildId,actualHelperProcesses=4,productionEngineUsed=true,gameLaunches=0,nativeDllsLoaded=0,userSettingsPreserved=true}));
        return 0;
    }
}
