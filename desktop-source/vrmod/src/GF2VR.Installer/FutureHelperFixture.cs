#if GF2VR_FUTURE_HELPER_FIXTURE
// Independently compiled test helper. Never included in the product assembly.
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Security.Cryptography;
using System.Text.Json;
using System.Text.Json.Nodes;

internal static class FutureHelperFixture
{
    private const string Inert = "INERT INSTALLER FIXTURE — NEVER EXECUTE AS A GAME";
    private static readonly string[] Helpers = { "GF2VR.Installer.exe", "GF2VR.Installer.dll", "GF2VR.Installer.deps.json", "GF2VR.Installer.runtimeconfig.json" };
    private static readonly string[] Owned = {
        "GF2 Game/version.dll", "vrmod/runtime/openxr_loader.dll", "vrmod/runtime/openxr-LICENSE.txt", "vrmod/runtime/minhook-LICENSE.txt",
        "vrmod/bin/GF2VR.Configurator/GF2VR.Configurator.exe", "vrmod/bin/GF2VR.Configurator/GF2VR.Configurator.dll",
        "vrmod/bin/GF2VR.Configurator/GF2VR.Configurator.deps.json", "vrmod/bin/GF2VR.Configurator/GF2VR.Configurator.runtimeconfig.json",
        "vrmod/bin/GF2VR.Configurator/runtime-contract.json", "vrmod/config/future-setting.json", "vrmod/bin/future-tool.txt",
        "vrmod/future/data-a.txt", "vrmod/future/data-b.txt" };
    private static readonly JsonSerializerOptions Json = new() { WriteIndented = true };
    private static string Hash(byte[] b) => Convert.ToHexString(SHA256.HashData(b));
    private static string HashFile(string p) { using var s = File.OpenRead(p); return Convert.ToHexString(SHA256.HashData(s)); }
    private static string Text(JsonElement e, string p) => e.GetProperty(p).GetString() ?? throw new InvalidDataException(p);
    private static void Require(bool value, string why) { if (!value) throw new InvalidDataException(why); }
    private static string Safe(string root, string relative)
    {
        Require(!Path.IsPathRooted(relative) && !relative.Split('/').Any(p => p is "" or "." or "..") && !relative.Contains('\\') && !relative.Contains(':'), "relative path");
        string full = Path.GetFullPath(Path.Combine(root, relative));
        Require(full.StartsWith(Path.GetFullPath(root) + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase), "containment");
        for (string? p = full; p != null; p = Path.GetDirectoryName(p))
            if (File.Exists(p) || Directory.Exists(p)) Require((File.GetAttributes(p) & FileAttributes.ReparsePoint) == 0, "reparse");
        return full;
    }
    private static void Duplicates(JsonElement e)
    {
        if (e.ValueKind == JsonValueKind.Object)
        {
            var names = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (var p in e.EnumerateObject()) { Require(names.Add(p.Name), "duplicate key"); Duplicates(p.Value); }
        }
        else if (e.ValueKind == JsonValueKind.Array) foreach (var p in e.EnumerateArray()) Duplicates(p);
    }
    private static JsonDocument Read(string p) { byte[] b = File.ReadAllBytes(p); Require(b.Length is > 0 and <= 262144, "metadata size"); var d = JsonDocument.Parse(b); Duplicates(d.RootElement); return d; }
    private static void Atomic(string p, byte[] bytes)
    {
        Directory.CreateDirectory(Path.GetDirectoryName(p)!); string temp = p + ".future-fixture-new";
        using (var s = new FileStream(temp, FileMode.CreateNew, FileAccess.Write, FileShare.None)) { s.Write(bytes); s.Flush(true); }
        File.Move(temp, p, true);
    }
    private static int Main(string[] args)
    {
        string folder = "";
        try
        {
            Require(args.Length == 3 && args[0] == "--apply-confirmed-update" && args[1] == "--stage", "helper arguments");
            folder = Path.GetFullPath(args[2]);
            using var hd = Read(Safe(folder, "handoff.json")); var h = hd.RootElement;
            string root = Path.GetFullPath(Text(h, "root"));
            Require(root.Contains("updater-validation-fixture", StringComparison.Ordinal) &&
                File.ReadAllText(Safe(root, "GF2 Game/GF2_Exilium.exe")) == Inert, "isolated inert root only");
            Require(h.GetProperty("schemaVersion").GetInt32() == 1 && h.GetProperty("userConfirmed").GetBoolean(), "consent");
            int pid = h.GetProperty("parentPid").GetInt32(); long ticks = h.GetProperty("parentStartTicks").GetInt64();
            Require(pid > 0 && pid != Environment.ProcessId && ticks > 0, "parent identity");
            try { using var parent = Process.GetProcessById(pid); Require(parent.StartTime.ToUniversalTime().Ticks == ticks && parent.WaitForExit(30000), "parent exit"); } catch (ArgumentException) { }
            string receiptPath = Safe(root, "vrmod/installer/install-state.json");
            byte[] oldReceipt = File.ReadAllBytes(receiptPath);
            Require(Hash(oldReceipt) == Text(h, "receiptHash"), "prior receipt binding");
            using var priorDoc = JsonDocument.Parse(oldReceipt); var prior = priorDoc.RootElement;
            string descriptorPath = Safe(folder, "release-descriptor.json");
            Require(HashFile(descriptorPath) == Text(h, "descriptorHash"), "descriptor binding");
            using var dd = Read(descriptorPath); var d = dd.RootElement;
            Require(Text(d, "product") == "GF2VR" && Text(d, "repository") == "deadpixel134/GF2Exilium-VR" && Text(d, "channel") == "stable", "provenance");
            Require(Version.Parse(Text(d, "version")) > Version.Parse(Text(prior, "version")) && Text(d, "version") == Text(h, "version"), "monotonic version");
            Require(Text(d, "buildFlavor") == Text(prior, "buildFlavor"), "flavor");
            var helpers = d.GetProperty("installerFiles").EnumerateArray().ToArray();
            Require(helpers.Length == 4 && helpers.Select(f => Text(f, "path")).ToHashSet(StringComparer.Ordinal).SetEquals(Helpers), "four helpers");
            foreach (var f in helpers) Require(HashFile(Safe(folder, "package/" + Text(f, "path"))) == Text(f, "sha256"), "executing helper hash");
            Require(Path.GetFullPath(Environment.ProcessPath!).Equals(Safe(folder, "package/GF2VR.Installer.exe"), StringComparison.OrdinalIgnoreCase), "executing helper path");
            string archive = Safe(folder, Text(d, "archiveName")); Require(HashFile(archive) == Text(d, "archiveSha256"), "full ZIP hash");
            using var zip = ZipFile.OpenRead(archive);
            var expected = Owned.Select(p => "payload/" + p).Concat(Helpers).Concat(new[] { "package-manifest.json", "DOTNET-NOTICES.txt", "LOCAL-VR.txt" }).ToHashSet(StringComparer.Ordinal);
            Require(zip.Entries.Count == 20 && zip.Entries.Select(e => e.FullName).ToHashSet(StringComparer.Ordinal).SetEquals(expected), "future twenty-entry policy");
            byte[] ReadEntry(string name)
            {
                var e = zip.Entries.Single(x => x.FullName == name);
                Require(e.Length is > 0 and <= 67108864 && ((e.ExternalAttributes >> 16) & 0xF000) != 0xA000, "bounded ordinary entry");
                using var input = e.Open(); using var output = new MemoryStream(); input.CopyTo(output);
                Require(output.Length == e.Length, "entry length"); return output.ToArray();
            }
            byte[] manifestBytes = ReadEntry("package-manifest.json");
            Require(Hash(manifestBytes) == Text(d, "packageManifestSha256") && Hash(manifestBytes) == Text(h, "manifestHash"), "manifest hash");
            using var md = JsonDocument.Parse(manifestBytes); var m = md.RootElement; Duplicates(m);
            Require(Text(m, "futureExtension") == "owned-future-format-v2", "future schema owner");
            Require(Text(m, "version") == Text(d, "version") && Text(m, "product") == "GF2VR" && Text(m, "scope") == "vrmod", "future manifest identity");
            var files = m.GetProperty("files").EnumerateArray().ToArray();
            Require(files.Length == 13 && files.Select(f => Text(f, "path")).ToHashSet(StringComparer.Ordinal).SetEquals(Owned), "future owned paths");
            var oldOwned = prior.GetProperty("files").EnumerateArray().ToDictionary(f => Text(f, "path"), f => Text(f, "sha256"), StringComparer.Ordinal);
            foreach (var f in oldOwned) Require(HashFile(Safe(root, f.Key)) == f.Value, "prior owned bytes");
            var payload = new Dictionary<string, byte[]>(StringComparer.Ordinal);
            foreach (var f in files)
            {
                string name = Text(f, "path"); byte[] bytes = ReadEntry("payload/" + name);
                Require(bytes.LongLength == f.GetProperty("bytes").GetInt64() && Hash(bytes) == Text(f, "sha256"), "future payload hash");
                Require(oldOwned.ContainsKey(name) || !File.Exists(Safe(root, name)), "unowned preservation");
                if (name.EndsWith("/runtime-contract.json", StringComparison.Ordinal))
                {
                    var contract = JsonNode.Parse(bytes)!; contract["configurationRoot"] = Path.Combine(root, "vrmod", "config");
                    bytes = System.Text.Encoding.UTF8.GetBytes(contract.ToJsonString(Json));
                }
                payload.Add(name, bytes);
            }
            string id = Guid.NewGuid().ToString("N"), backup = Safe(folder, "future-transaction/" + id);
            Directory.CreateDirectory(backup); File.WriteAllBytes(Safe(backup, "prior-receipt.json"), oldReceipt);
            var previous = new Dictionary<string, byte[]?>(StringComparer.Ordinal);
            foreach (var p in payload)
            {
                string target = Safe(root, p.Key); previous.Add(p.Key, File.Exists(target) ? File.ReadAllBytes(target) : null);
                if (previous[p.Key] is byte[] b) { string bp = Safe(backup, p.Key); Directory.CreateDirectory(Path.GetDirectoryName(bp)!); File.WriteAllBytes(bp, b); }
            }
            try
            {
                foreach (var p in payload) Atomic(Safe(root, p.Key), p.Value);
                var result = JsonNode.Parse(oldReceipt)!; result["version"] = Text(d, "version"); result["transactionId"] = id; result["packageSha256"] = Hash(manifestBytes);
                result["files"] = new JsonArray(payload.Select(p => (JsonNode)new JsonObject { ["path"] = p.Key, ["sha256"] = Hash(p.Value), ["bytes"] = p.Value.Length }).ToArray());
                Atomic(receiptPath, System.Text.Encoding.UTF8.GetBytes(result.ToJsonString(Json)));
                File.WriteAllText(Safe(folder, "update-result.json"), JsonSerializer.Serialize(new { passed = true, version = Text(d, "version"), transactionId = id, installedFiles = 13, recognizedFutureField = true, backup }, Json));
            }
            catch
            {
                foreach (var p in previous) { string target = Safe(root, p.Key); if (p.Value == null) File.Delete(target); else Atomic(target, p.Value); }
                Atomic(receiptPath, oldReceipt); throw;
            }
            return 0;
        }
        catch (Exception e) { if (folder.Length != 0) File.WriteAllText(Path.Combine(folder, "future-helper-error.txt"), e.ToString()); return 1; }
    }
}
#endif
