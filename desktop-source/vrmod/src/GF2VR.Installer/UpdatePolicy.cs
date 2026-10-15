using System.Text.Json;
using System.Text.Json.Serialization;
using System.Text.RegularExpressions;

namespace GF2VR.Installer;

internal sealed record ReleaseAsset(long Id, string Name, long Bytes, string Hash);
internal sealed record ReleaseCandidate(long Id, string Version, ReleaseAsset Descriptor, ReleaseAsset Archive);
internal sealed record UpdateDescriptor(int SchemaVersion, string Product, string Repository, string Version,
    string Channel, string ArchiveName, string ArchiveSha256, string PackageManifestSha256, PayloadFile[] InstallerFiles);
internal sealed record UpdateHandoff(int SchemaVersion, string Root, string Version, string ReceiptHash,
    string ManifestHash, string DescriptorHash, int ParentPid, long ParentStartTicks, bool UserConfirmed);
internal sealed record PreparedUpdate(string Version, string Folder, string Root, string ReceiptHash, string ManifestHash, string DescriptorHash);

internal static class UpdatePolicy
{
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
    internal static string ArchiveName(string version) => "GF2VR-Native-SelfContained-" + version + ".zip";
    internal static ReleaseCandidate? Select(byte[] json, string installedVersion)
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
            if (id < 1 || size < 1 || size > (name == DescriptorName ? 262144 : MaxArchive) ||
                a.GetProperty("url").GetString() != "https://api.github.com/" + Api + "releases/assets/" + id) throw new InstallError("updateInvalid");
            return new(id, name, size, hash);
        }
        long releaseId = highest.Release.GetProperty("id").GetInt64(); if (releaseId < 1) throw new InstallError("updateInvalid");
        return new(releaseId, normalized, Asset(DescriptorName), Asset(ArchiveName(normalized)));
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
        if (d.SchemaVersion != 1 || d.Product != Contract.Product || d.Repository != Repository || d.Channel != "stable" ||
            d.ArchiveName != ArchiveName(d.Version) || d.InstallerFiles == null || d.InstallerFiles.Length != 4 ||
            !d.InstallerFiles.Select(f => f?.Path).ToHashSet(StringComparer.Ordinal).SetEquals(HelperFiles)) throw new InstallError("updateInvalid");
        Paths.HashText(d.ArchiveSha256); Paths.HashText(d.PackageManifestSha256);
        foreach (var f in d.InstallerFiles) { if (f == null || f.Bytes < 1 || f.Bytes > Contract.MaxFile) throw new InstallError("updateInvalid"); Paths.HashText(f.Sha256); }
    }
}
