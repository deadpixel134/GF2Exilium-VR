using System.Globalization;
using System.Reflection;
using System.Text.Json;

namespace GF2VR.Configurator;

internal static class Texts
{
    internal static readonly string[] Languages = ["ko", "en", "ja"];
    private static readonly Dictionary<string, Dictionary<string, string>> Resources = Load();
    internal static string Language = Normalize(CultureInfo.CurrentUICulture.TwoLetterISOLanguageName);
    internal static string Normalize(string? locale) => locale != null && Languages.Contains(locale) ? locale : "ko";

    private static Dictionary<string, Dictionary<string, string>> Load()
    {
        var assembly = Assembly.GetExecutingAssembly(); var resources = new Dictionary<string, Dictionary<string, string>>();
        foreach (string language in Languages)
        {
            string name = assembly.GetManifestResourceNames().Single(n => n.EndsWith(".Locales." + language + ".json", StringComparison.Ordinal));
            using var stream = assembly.GetManifestResourceStream(name)!;
            resources[language] = JsonSerializer.Deserialize<Dictionary<string, string>>(stream)!;
#if GF2VR_DIRTY_CONTACT
            string overlay = assembly.GetManifestResourceNames().Single(n => n.EndsWith(".Locales.Dirty." + language + ".json", StringComparison.Ordinal));
            using var dirtyStream = assembly.GetManifestResourceStream(overlay)!;
            foreach (var p in JsonSerializer.Deserialize<Dictionary<string, string>>(dirtyStream)!) resources[language][p.Key] = p.Value;
#endif
        }
        return resources;
    }

    internal static string Get(string key)
    {
        if (Resources[Normalize(Language)].TryGetValue(key, out var value) && !string.IsNullOrWhiteSpace(value)) return value;
        if (Resources["ko"].TryGetValue(key, out value) && !string.IsNullOrWhiteSpace(value)) return value;
        return Resources["ko"]["unavailable"];
    }
    internal static string Format(string key, params object[] args) => string.Format(CultureInfo.InvariantCulture, Get(key), args);
    internal static string Describe(Exception ex) => ex is SettingsError e ? Format(e.Key, e.Arguments) : Get("ioError") + "\n" + ex.Message;
    internal static void Verify()
    {
        foreach (var p in Resources)
        {
            if (!Resources["ko"].Keys.ToHashSet().SetEquals(p.Value.Keys) || p.Value.Values.Any(string.IsNullOrWhiteSpace))
                throw new InvalidDataException("Incomplete packaged locale: " + p.Key);
        }
        foreach (var f in Fields.All)
            if (!Resources["ko"].ContainsKey(f.Key) || (f.Help.Length != 0 && !Resources["ko"].ContainsKey(f.Help)))
                throw new InvalidDataException("Field text missing: " + f.Key);
    }
}
