using System.Text.Json;
using System.Text.Json.Serialization;
using System.Text.RegularExpressions;

namespace GF2VR.Installer;

internal sealed record ReleaseAsset(long Id, string Name, long Bytes, string Hash);
internal sealed record ReleaseCandidate(long Id, string Version, ReleaseAsset Descriptor, ReleaseAsset Archive);
internal sealed record UpdateDescriptor(int SchemaVersion, string Product, string Repository, string Version,
    string Channel, string ArchiveName, string ArchiveSha256, string PackageManifestSha256, PayloadFile[] InstallerFiles,
    string? BuildFlavor = null, string? CommonCoreSha256 = null);
internal sealed record UpdateHandoff(int SchemaVersion, string Root, string Version, string ReceiptHash,
    string ManifestHash, string DescriptorHash, int ParentPid, long ParentStartTicks, bool UserConfirmed);
internal sealed record PreparedUpdate(string Version, string Folder, string Root, string ReceiptHash, string ManifestHash, string DescriptorHash);

internal static class UpdatePolicy
{
    // Stable updater bootstrap v1 (docs/release-notes/update-bootstrap-contract.md):
    // Clean/Dirty descriptors keep these eleven fields and one ZIP/ArchiveSha256.
    // The fixed repository and exact per-flavor asset names below are protocol.
    // ZIP-root HelperFiles are an independently executable, self-contained unit:
    // never require a fifth helper/runtime file. Manifest/layout policy belongs to
    // the incoming helper; changing this envelope requires a separately approved
    // client/protocol migration, not a payload-schema exception in old clients.
    internal const string Repository = "deadpixel134/GF2Exilium-VR";
    internal const string Api = "repos/" + Repository + "/";
    internal const string DescriptorName = "GF2VR-update.json";
    internal static readonly string[] HelperFiles = ["GF2VR.Installer.exe", "GF2VR.Installer.dll", "GF2VR.Installer.deps.json", "GF2VR.Installer.runtimeconfig.json"];
    internal const long MaxArchive = 256L * 1024 * 1024;
    internal static Version VersionOf(string text)
    {
        if (!Regex.IsMatch(text, @"^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$", RegexOptions.CultureInvariant) ||
            !Version.TryParse(text, out var version) || version.Build < 0 || version.Revision != -1) throw new InstallError("updateInvalid");
        return version;
    }
    internal static string ArchiveName(string version, string? flavor = null) => "GF2VR-Native-SelfContained-" + (flavor == null ? "" : flavor + "-") + version + ".zip";
    internal static string DescriptorFor(string? flavor) => flavor == null ? DescriptorName : "GF2VR-" + flavor + "-update.json";
    internal static string ReleasePage(string version) { _ = VersionOf(version); return "https://github.com/" + Repository + "/releases/tag/v" + version; }
    // Discovery needs only a published stable version. Applicability continues
    // through the unchanged strict asset/descriptor policy in Select/Descriptor.
    internal static string? DiscoverVersion(byte[] json, string installedVersion)
    {
        if (json.Length > 262144) throw new InstallError("updateInvalid");
        using var doc = JsonDocument.Parse(json); Contract.Duplicates(doc.RootElement);
        if (doc.RootElement.ValueKind != JsonValueKind.Array || doc.RootElement.GetArrayLength() > 20) throw new InstallError("updateInvalid");
        Version current = VersionOf(installedVersion); Version? newest = null;
        foreach (var r in doc.RootElement.EnumerateArray())
        {
            if (r.GetProperty("draft").GetBoolean() || r.GetProperty("prerelease").GetBoolean() || r.GetProperty("published_at").ValueKind != JsonValueKind.String) continue;
            string tag = r.GetProperty("tag_name").GetString() ?? "";
            if (!tag.StartsWith('v')) continue;
            Version v; try { v = VersionOf(tag[1..]); } catch (InstallError) { continue; }
            if (v > current && (newest == null || v > newest)) newest = v;
        }
        return newest?.ToString(3);
    }
    internal static ReleaseCandidate? Select(byte[] json, string installedVersion, string? flavor = null)
    {
        if (json.Length > 262144) throw new InstallError("updateInvalid");
        using var doc = JsonDocument.Parse(json); Contract.Duplicates(doc.RootElement);
        if (doc.RootElement.ValueKind != JsonValueKind.Array || doc.RootElement.GetArrayLength() > 20) throw new InstallError("updateInvalid");
        Version current = VersionOf(installedVersion); var newer = new List<(JsonElement Release, Version Version)>();
        foreach (var r in doc.RootElement.EnumerateArray())
        {
            if (r.GetProperty("draft").GetBoolean() || r.GetProperty("prerelease").GetBoolean() || r.GetProperty("published_at").ValueKind != JsonValueKind.String) continue;
            string tag = r.GetProperty("tag_name").GetString()!;
            if (!tag.StartsWith('v')) continue;
            Version version; try { version = VersionOf(tag[1..]); } catch (InstallError) { continue; }
            if (version > current) newer.Add((r, version));
        }
        if (newer.Count == 0) return null;
        var highest = newer.OrderByDescending(r => r.Version).First();
        if (newer.Count(r => r.Version == highest.Version) != 1) throw new InstallError("updateInvalid");
        string normalized = highest.Version.ToString(3); var assets = highest.Release.GetProperty("assets");
        ReleaseAsset Asset(string name)
        {
            var matches = assets.EnumerateArray().Where(a => a.GetProperty("name").GetString() == name).ToArray();
            if (matches.Length != 1) throw new InstallError("updateInvalid");
            var a = matches[0]; string digest = a.GetProperty("digest").GetString() ?? "";
            if (!digest.StartsWith("sha256:", StringComparison.Ordinal) || a.GetProperty("state").GetString() != "uploaded") throw new InstallError("updateInvalid");
            string hash = digest[7..]; Paths.HashText(hash);
            long id = a.GetProperty("id").GetInt64(), size = a.GetProperty("size").GetInt64();
            if (id < 1 || size < 1 || size > (name == DescriptorFor(flavor) ? 262144 : MaxArchive) ||
                a.GetProperty("url").GetString() != "https://api.github.com/" + Api + "releases/assets/" + id) throw new InstallError("updateInvalid");
            return new(id, name, size, hash);
        }
        long releaseId = highest.Release.GetProperty("id").GetInt64(); if (releaseId < 1) throw new InstallError("updateInvalid");
        return new(releaseId, normalized, Asset(DescriptorFor(flavor)), Asset(ArchiveName(normalized, flavor)));
    }
    internal static UpdateDescriptor Descriptor(byte[] bytes, ReleaseCandidate release)
    {
        if (bytes.LongLength != release.Descriptor.Bytes || !Paths.Equal(Paths.Hash(bytes), release.Descriptor.Hash)) throw new InstallError("updateInvalid");
        var d = Contract.Read<UpdateDescriptor>(bytes); ValidateDescriptor(d);
        if (d.Version != release.Version || d.ArchiveName != release.Archive.Name || !Paths.Equal(d.ArchiveSha256, release.Archive.Hash)) throw new InstallError("updateInvalid");
        return d;
    }
    internal static void ValidateDescriptor(UpdateDescriptor d)
    {
        _ = VersionOf(d.Version);
        if (d.BuildFlavor is not (null or "Clean" or "Dirty") ||
            (d.BuildFlavor == null ? d.CommonCoreSha256 != null : d.CommonCoreSha256 == null)) throw new InstallError("updateInvalid");
        if (d.CommonCoreSha256 != null) Paths.HashText(d.CommonCoreSha256);
        if (d.SchemaVersion != 1 || d.Product != Contract.Product || d.Repository != Repository || d.Channel != "stable" ||
            d.ArchiveName != ArchiveName(d.Version, d.BuildFlavor) || d.InstallerFiles == null || d.InstallerFiles.Length != 4 ||
            !d.InstallerFiles.Select(f => f?.Path).ToHashSet(StringComparer.Ordinal).SetEquals(HelperFiles)) throw new InstallError("updateInvalid");
        Paths.HashText(d.ArchiveSha256); Paths.HashText(d.PackageManifestSha256);
        foreach (var f in d.InstallerFiles) { if (f == null || f.Bytes < 1 || f.Bytes > Contract.MaxFile) throw new InstallError("updateInvalid"); Paths.HashText(f.Sha256); }
    }
}
