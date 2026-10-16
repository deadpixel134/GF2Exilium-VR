using System.Text.Json;
using System.Text.Json.Serialization;

namespace GF2VR.Installer;

internal sealed class InstallError(string key, params object[] arguments) : Exception(key)
{
    internal string Key { get; } = key;
    internal object[] Arguments { get; } = arguments;
}

internal sealed record PayloadFile(string Path, string Sha256, long Bytes);
internal sealed record Package(int SchemaVersion, string Product, string Version, string Scope,
    string GameExecutableSha256, string? BoundGameRoot, string? RequiresInstalledDllSha256,
    PayloadFile[] Files, NativePackage? Native = null);
internal sealed record NativePackage(string BuildId, string DllSha256, string LoaderSha256, PayloadFile[] GameFiles,
    string? ResourceBinding = null);
internal sealed record CheckedPackage(string Root, string PackageRoot, Package Package, string ManifestHash, IReadOnlyDictionary<string, byte[]> Payload);
internal sealed record Receipt(int SchemaVersion, string Product, string Version, string Scope,
    string Root, string TransactionId, string PackageSha256, PayloadFile[] Files, string? GameDirectory = null);
internal sealed record Change(string Path, string? BeforeHash, string? AfterHash, string? BeforeFile, string? AfterFile);
internal sealed record Journal(int SchemaVersion, string Product, string Root, string TransactionId,
    string State, Change[] Changes, string? GameDirectory = null);
internal sealed record Pending(int SchemaVersion, string TransactionId);
internal sealed record Plan(string Root, string PackageRoot, Package Package, string ManifestHash,
    Receipt? Prior, string? PriorHash, IReadOnlyDictionary<string, byte[]> Payload, string GameDirectory);
internal sealed record OperationResult(string Version, bool RestoredPrevious, string TransactionId);

internal static class Contract
{
    internal const string Product = "GF2VR";
    internal const string StatePath = "vrmod/installer/install-state.json";
    internal const string PendingPath = "vrmod/installer/pending.json";
    internal const int MaxFile = 64 * 1024 * 1024;
    internal static readonly JsonSerializerOptions Json = new()
    {
        PropertyNamingPolicy = JsonNamingPolicy.CamelCase, WriteIndented = true,
        UnmappedMemberHandling = JsonUnmappedMemberHandling.Disallow, MaxDepth = 16
    };
    internal static byte[] Encode<T>(T item) => JsonSerializer.SerializeToUtf8Bytes(item, Json);
    internal static T Read<T>(byte[] bytes)
    {
        if (bytes.Length is < 1 or > 262144) throw new InstallError("badState");
        try
        {
            using var doc = JsonDocument.Parse(bytes, new JsonDocumentOptions { MaxDepth = 16 });
            Duplicates(doc.RootElement);
            return JsonSerializer.Deserialize<T>(bytes, Json) ?? throw new InstallError("badState");
        }
        catch (JsonException) { throw new InstallError("badState"); }
    }
    internal static void Duplicates(JsonElement element)
    {
        if (element.ValueKind == JsonValueKind.Object)
        {
            var keys = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (var p in element.EnumerateObject())
            {
                if (!keys.Add(p.Name)) throw new InstallError("duplicate", p.Name);
                Duplicates(p.Value);
            }
        }
        else if (element.ValueKind == JsonValueKind.Array)
            foreach (var v in element.EnumerateArray()) Duplicates(v);
    }
}
