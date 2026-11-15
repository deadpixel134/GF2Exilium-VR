using Microsoft.Win32;
using System.Text;
using System.Text.RegularExpressions;

namespace GF2VR.Installer;

internal sealed record GameLocations(string[] Roots, string? PreferredRoot);
internal sealed record RegisteredGame(string? Location, string? Icon);

internal static class GameLocationDiscovery
{
    // Observed supported Steam distribution. Registry InstallLocation also covers
    // matching product registrations without depending on a particular app ID.
    internal const string SteamAppId = "3347400";
    private static readonly TimeSpan RegexTimeout = TimeSpan.FromMilliseconds(200);
    internal static GameLocations Discover(string[] starts, CancellationToken token = default)
    {
        var games = new List<RegisteredGame>(); var steam = new List<string>();
        foreach (var hive in new[] { RegistryHive.CurrentUser, RegistryHive.LocalMachine })
        foreach (var view in new[] { RegistryView.Registry64, RegistryView.Registry32 })
        {
            token.ThrowIfCancellationRequested();
            try
            {
                using var registry = RegistryKey.OpenBaseKey(hive, view);
                using var valve = registry.OpenSubKey(@"Software\Valve\Steam", false);
                foreach (string name in new[] { "SteamPath", "InstallPath" })
                    if (valve?.GetValue(name) is string home) steam.Add(home);
                using var uninstall = registry.OpenSubKey(@"Software\Microsoft\Windows\CurrentVersion\Uninstall", false);
                foreach (string name in (uninstall?.GetSubKeyNames() ?? []).Take(2048))
                {
                    token.ThrowIfCancellationRequested();
                    try
                    {
                        using var entry = uninstall!.OpenSubKey(name, false);
                        string display = entry?.GetValue("DisplayName") as string ?? "";
                        if (!name.Equals("GF2Exilium", StringComparison.OrdinalIgnoreCase) &&
                            !name.Equals("Steam App " + SteamAppId, StringComparison.OrdinalIgnoreCase) &&
                            !display.Contains("EXILIUM", StringComparison.OrdinalIgnoreCase) &&
                            !display.Contains("소녀전선2", StringComparison.Ordinal) &&
                            !display.Contains("少女前線2", StringComparison.Ordinal) &&
                            !display.Contains("少女前线2", StringComparison.Ordinal)) continue;
                        games.Add(new(entry?.GetValue("InstallLocation") as string, entry?.GetValue("DisplayIcon") as string));
                    }
                    catch (Exception ex) when (OptionalFailure(ex)) { }
                }
            }
            catch (Exception ex) when (OptionalFailure(ex)) { }
        }
        return Probe(starts, games, steam, token);
    }
    internal static GameLocations Probe(string[] starts, IEnumerable<RegisteredGame> games, IEnumerable<string> steamHomes, CancellationToken token = default)
    {
        var roots = new HashSet<string>(StringComparer.OrdinalIgnoreCase); string? contextual = null;
        void Add(string? candidate, bool context = false)
        {
            token.ThrowIfCancellationRequested();
            string? root = ExactRoot(candidate);
            if (root == null) return;
            roots.Add(root); if (context) contextual ??= root;
        }
        foreach (string start in starts.Take(2))
        {
            try
            {
                if (!LocalPath(start)) continue;
                DirectoryInfo? dir = new(start);
                for (int i = 0; i < 8 && dir != null; ++i, dir = dir.Parent) Add(dir.FullName, true);
            }
            catch (Exception ex) when (OptionalFailure(ex)) { }
        }
        foreach (var game in games.Take(128))
        {
            Add(game.Location);
            try
            {
                string icon = Regex.Replace(game.Icon?.Trim() ?? "", @",\s*-?\d+\s*$", "", RegexOptions.CultureInvariant, RegexTimeout).Trim('"');
                if (LocalPath(icon) && (Path.GetFileName(icon).Equals("PCLauncher.exe", StringComparison.OrdinalIgnoreCase) ||
                    Path.GetFileName(icon).Equals("GF2_Exilium.exe", StringComparison.OrdinalIgnoreCase))) Add(Path.GetDirectoryName(icon));
            }
            catch (Exception ex) when (OptionalFailure(ex)) { }
        }
        var libraries = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (string home in steamHomes.Distinct(StringComparer.OrdinalIgnoreCase).Take(8))
        {
            token.ThrowIfCancellationRequested();
            if (!LocalPath(home)) continue;
            libraries.Add(Path.GetFullPath(home));
            string text = ReadSmall(Path.Combine(home, "steamapps", "libraryfolders.vdf"));
            try
            {
                // Current path fields and old numbered path entries; never walk a drive.
                foreach (Match match in Regex.Matches(text, "\"(?:path|[0-9]+)\"\\s+\"((?:\\\\.|[^\"\\\\])*)\"", RegexOptions.CultureInvariant, RegexTimeout))
                {
                    string library = Unescape(match.Groups[1].Value);
                    if (LocalPath(library) && libraries.Count < 32) libraries.Add(Path.GetFullPath(library));
                }
            }
            catch (RegexMatchTimeoutException) { }
        }
        foreach (string library in libraries.Take(32))
        {
            token.ThrowIfCancellationRequested();
            string manifest = ReadSmall(Path.Combine(library, "steamapps", "appmanifest_" + SteamAppId + ".acf"));
            string? appId = VdfValue(manifest, "appid"), directory = VdfValue(manifest, "installdir");
            if (appId != SteamAppId || string.IsNullOrWhiteSpace(directory) || directory is "." or ".." ||
                directory.IndexOfAny(Path.GetInvalidFileNameChars()) >= 0) continue;
            Add(Path.Combine(library, "steamapps", "common", directory));
        }
        string[] result = roots.OrderBy(p => p, StringComparer.OrdinalIgnoreCase).ToArray();
        return new(result, contextual ?? (result.Length == 1 ? result[0] : null));
    }
    private static bool LocalPath(string? value) => !string.IsNullOrWhiteSpace(value) && value.Length < 1024 &&
        value.Length >= 3 && char.IsAsciiLetter(value[0]) && value[1] == ':' && value[2] is '\\' or '/' &&
        !value.Any(char.IsControl) && value.IndexOfAny(Path.GetInvalidPathChars()) < 0;
    private static string? ExactRoot(string? candidate)
    {
        try
        {
            if (!LocalPath(candidate)) return null;
            string folder = Path.GetFullPath(candidate!);
            if (Path.GetFileName(folder).Equals("GF2_Exilium.exe", StringComparison.OrdinalIgnoreCase)) folder = Path.GetDirectoryName(folder)!;
            // Check only exact direct/Haoplay layouts before calling Paths.Root;
            // its broader search is reserved for explicit manual selection.
            if (!File.Exists(Path.Combine(folder, "GF2_Exilium.exe")) && !File.Exists(Path.Combine(folder, "GF2 Game", "GF2_Exilium.exe"))) return null;
            string root = Paths.Root(folder); string game = Path.GetDirectoryName(Paths.GameExecutable(root))!;
            return File.Exists(Path.Combine(game, "GameAssembly.dll")) && File.Exists(Path.Combine(game, "UnityPlayer.dll")) ? root : null;
        }
        catch (Exception ex) when (OptionalFailure(ex)) { return null; }
    }
    private static string ReadSmall(string file)
    {
        try { return Encoding.UTF8.GetString(Paths.Bytes(file, 1048576) ?? []); }
        catch (Exception ex) when (OptionalFailure(ex)) { return ""; }
    }
    private static string Unescape(string text) => text.Replace("\\\\", "\\").Replace("\\\"", "\"");
    private static string? VdfValue(string text, string key)
    {
        try
        {
            var matches = Regex.Matches(text, "\"" + Regex.Escape(key) + "\"\\s+\"((?:\\\\.|[^\"\\\\])*)\"", RegexOptions.CultureInvariant, RegexTimeout);
            return matches.Count == 1 ? Unescape(matches[0].Groups[1].Value) : null;
        }
        catch (RegexMatchTimeoutException) { return null; }
    }
    private static bool OptionalFailure(Exception ex) => ex is IOException or UnauthorizedAccessException or ArgumentException or
        NotSupportedException or System.Security.SecurityException or InstallError or RegexMatchTimeoutException;
}
