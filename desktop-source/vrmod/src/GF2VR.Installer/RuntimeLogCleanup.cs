using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.RegularExpressions;

namespace GF2VR.Installer;

internal sealed record RuntimeLogMigration(int SchemaVersion, string Product, string Migration,
    string Decision, int DeletedFiles, int PreservedFiles);

internal static class RuntimeLogCleanup
{
    internal const string Marker = "vrmod/installer/migrations/runtime-logs-115.json";
    private const string Migration = "runtime-logs-115";

    // Called only after a successful native install, under the Installer lock.
    // Cleanup failure must never undo an already committed product installation.
    internal static string Apply(string root, bool upgrade)
    {
        string mutexName = "Local\\GF2VR.Settings." + Convert.ToHexString(SHA256.HashData(Encoding.UTF8.GetBytes(root.ToUpperInvariant())));
        using var mutex = new Mutex(false, mutexName);
        bool held;
        try { held = mutex.WaitOne(0); } catch (AbandonedMutexException) { held = true; }
        if (!held) return "preserved-settings-busy";
        try { return ApplyLocked(root, upgrade); }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException or InstallError or JsonException)
        { return "preserved-unsafe-or-unavailable"; }
        finally { mutex.ReleaseMutex(); }
    }

    private static string ApplyLocked(string root, bool upgrade)
    {
        InstallationEngine.Stopped();
        string marker = Paths.Contained(root, Marker);
        if (File.Exists(marker))
        {
            var state = Contract.Read<RuntimeLogMigration>(Paths.Bytes(marker, 4096)!);
            if (state.SchemaVersion != 1 || state.Product != Contract.Product || state.Migration != Migration ||
                state.Decision is not ("preserved-logging-on" or "fresh-install" or "cleanup-attempted"))
                throw new InstallError("badState");
            return "already-decided";
        }
        string config = Paths.Contained(root, "vrmod/config/controller_input.json");
        using var input = File.Exists(config) ? new FileStream(config, FileMode.Open, FileAccess.Read, FileShare.Read) : null;
        bool enabled = false;
        if (input != null)
        {
            if (input.Length is < 1 or > 4096) return "preserved-invalid-settings";
            byte[] bytes = new byte[input.Length]; input.ReadExactly(bytes);
            ReadOnlyMemory<byte> data = bytes;
            if (bytes.AsSpan().StartsWith(new byte[] { 239, 187, 191 })) data = data[3..];
            using var document = JsonDocument.Parse(data, new JsonDocumentOptions { MaxDepth = 16 });
            var value = document.RootElement; Contract.Duplicates(value);
            if (value.ValueKind != JsonValueKind.Object) return "preserved-invalid-settings";
            if (value.TryGetProperty("schemaVersion", out var schema) &&
                (schema.ValueKind != JsonValueKind.Number || !schema.TryGetInt32(out int version) || version != 1))
                return "preserved-invalid-settings";
            if (value.TryGetProperty("diagnostics", out var diagnostics))
            {
                if (diagnostics.ValueKind != JsonValueKind.Object) return "preserved-invalid-settings";
                if (diagnostics.TryGetProperty("loggingEnabled", out var logging))
                {
                    if (logging.ValueKind is not (JsonValueKind.True or JsonValueKind.False)) return "preserved-invalid-settings";
                    enabled = logging.GetBoolean();
                }
            }
        }
        var migration = new RuntimeLogMigration(1, Contract.Product, Migration,
            enabled ? "preserved-logging-on" : upgrade ? "cleanup-attempted" : "fresh-install", 0, 0);
        // The once-only decision is durable BEFORE any deletion. ON remains
        // excluded even if a later version changes the setting to OFF.
        Paths.WriteNew(marker, Contract.Encode(migration));
        if (enabled || !upgrade) return migration.Decision;

        string folder = Path.GetDirectoryName(Paths.Contained(root, "vrmod/logs/bootstrap/.cleanup-boundary"))!;
        if (!Directory.Exists(folder)) return migration.Decision;
        int removed = 0, preserved = 0;
        foreach (string file in Directory.EnumerateFiles(folder, "*.jsonl", SearchOption.TopDirectoryOnly))
        {
            string name = Path.GetFileName(file);
            // Published automatic log names only. Historical manual captures,
            // unknown files, subfolders and other mods' logs are not ours.
            if (!Regex.IsMatch(name, @"^(GF2VR-(CLEAN|DIRTY)-[A-Z0-9-]{1,96}|BOOTSTRAP-FLEX-001)(\.inventory)?\.jsonl$",
                    RegexOptions.CultureInvariant)) continue;
            try
            {
                InstallationEngine.Stopped();
                string target = Paths.Contained(root, "vrmod/logs/bootstrap/" + name);
                if ((File.GetAttributes(target) & (FileAttributes.ReparsePoint | FileAttributes.Directory)) != 0)
                    throw new InstallError("path", name);
                using var lease = new FileStream(target, FileMode.Open, FileAccess.Read, FileShare.Delete);
                File.Delete(target); ++removed;
            }
            catch (Exception e) when (e is IOException or UnauthorizedAccessException or InstallError) { ++preserved; }
        }
        Paths.Atomic(marker, Contract.Encode(migration with { DeletedFiles = removed, PreservedFiles = preserved }));
        return migration.Decision;
    }
}
