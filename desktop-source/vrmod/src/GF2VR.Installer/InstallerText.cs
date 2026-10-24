using System.Globalization;
using System.Reflection;
using System.Text.Json;
using System.Text.Json.Nodes;

namespace GF2VR.Installer;

internal static class InstallerText
{
    internal static readonly string[] Languages = ["ko", "en", "ja"];
    private static readonly Dictionary<string, Dictionary<string, string>> Resources = Load();
    internal static string Language = Normalize(CultureInfo.CurrentUICulture.TwoLetterISOLanguageName);
    internal static string Normalize(string? value) => value != null && Languages.Contains(value) ? value : "ko";
    private static Dictionary<string, Dictionary<string, string>> Load()
    {
        var assembly = Assembly.GetExecutingAssembly(); var resources = new Dictionary<string, Dictionary<string, string>>();
        foreach (string lang in Languages)
        {
            string name = assembly.GetManifestResourceNames().Single(n => n.EndsWith(".Locales." + lang + ".json", StringComparison.Ordinal));
            using var stream = assembly.GetManifestResourceStream(name)!;
            resources.Add(lang, JsonSerializer.Deserialize<Dictionary<string, string>>(stream)!);
        }
        return resources;
    }
    internal static string Get(string key)
    {
        if (Resources[Normalize(Language)].TryGetValue(key, out var v) && !string.IsNullOrWhiteSpace(v)) return v;
        return Resources["ko"].TryGetValue(key, out v) ? v : Resources["ko"]["unavailable"];
    }
    internal static string Format(string key, params object[] arguments) => string.Format(CultureInfo.InvariantCulture, Get(key), arguments);
    internal static string Error(Exception ex) => (ex is InstallError e ? Format(e.Key, e.Arguments) : Get("fileError") + "\n" + ex.Message) + "\n\n" + Get("updateNextAction");
    internal static void Verify()
    {
        foreach (var r in Resources)
            if (!Resources["ko"].Keys.ToHashSet(StringComparer.Ordinal).SetEquals(r.Value.Keys) || r.Value.Values.Any(string.IsNullOrWhiteSpace))
                throw new InvalidDataException("Incomplete packaged locale: " + r.Key);
    }
    internal static string LoadPreference(string root)
    {
        try
        {
            string path = Paths.Contained(Paths.Root(root), "vrmod/config/installer-ui.json");
            byte[]? raw = Paths.Bytes(path, 4096);
            if (raw == null) return Normalize(CultureInfo.CurrentUICulture.TwoLetterISOLanguageName);
            using var doc = JsonDocument.Parse(raw);
            Contract.Duplicates(doc.RootElement);
            if (doc.RootElement.GetProperty("schemaVersion").GetInt32() != 1) return "ko";
            return Normalize(doc.RootElement.GetProperty("language").GetString());
        }
        catch { return "ko"; }
    }
    internal static void SavePreference(string root, string locale)
    {
        string path = Paths.Contained(Paths.Root(root), "vrmod/config/installer-ui.json");
        byte[]? raw = Paths.Bytes(path, 4096);
        if (raw != null) { using var json = JsonDocument.Parse(raw); Contract.Duplicates(json.RootElement); }
        JsonObject doc = raw == null ? new() : JsonNode.Parse(raw)!.AsObject();
        if (doc.ContainsKey("schemaVersion") && doc["schemaVersion"]?.ToJsonString() != "1") throw new InstallError("preference");
        doc["schemaVersion"] = 1; doc["language"] = Normalize(locale);
        byte[] bytes = System.Text.Encoding.UTF8.GetBytes(doc.ToJsonString(new JsonSerializerOptions { WriteIndented = true }));
        if (bytes.Length > 4096) throw new InstallError("preference");
        if (!Paths.Equal(raw == null ? null : Paths.Hash(raw), Paths.FileHash(path))) throw new InstallError("previewChanged");
        Paths.Atomic(path, bytes);
    }
}
