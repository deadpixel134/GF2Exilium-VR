using System.Diagnostics;
using System.Text.Json;
using System.Text.RegularExpressions;

namespace GF2VR.Installer;

internal sealed record SettingsTarget(string Executable, string Root);

internal static class SettingsLaunch
{
    internal static Action<ProcessStartInfo> Start = info => { using var process = Process.Start(info); };

    internal static SettingsTarget? Resolve(string root, InstallationEngine engine)
    {
        root = Paths.Root(root);
        if (engine.HasPending(root)) return null;
        Receipt? receipt = engine.Inspect(root);
        if (receipt != null)
        {
            string folder = "vrmod/bin/GF2VR.Configurator/";
            foreach (string name in Paths.ToolFiles)
            {
                var file = receipt.Files.SingleOrDefault(f => f.Path == folder + name);
                if (file == null || !Paths.Equal(Paths.FileHash(Paths.Contained(root, file.Path)), file.Sha256)) return null;
            }
            return Target(root, folder);
        }
        // The current workspace uses an owned GUI launcher rather than a
        // release-install receipt. Resolve its reviewed payload directly;
        // never execute a batch file or search for arbitrary executables.
        const string launcher = "vrmod/tools/open_settings.cmd";
        byte[]? raw = Paths.Bytes(Paths.Contained(root, launcher), 16384);
        if (raw == null) return null;
        var match = Regex.Match(System.Text.Encoding.UTF8.GetString(raw),
            "^set \\\"GF2_SETTINGS_APP=%GF2_SETTINGS_ROOT%\\\\(?<path>vrmod\\\\build\\\\settings\\\\(?<build>[A-Z0-9-]+)\\\\app\\\\GF2VR\\.Configurator\\.exe)\\\"\\r?$",
            RegexOptions.Multiline | RegexOptions.CultureInvariant);
        if (!match.Success) return null;
        string prefix = "vrmod/build/settings/" + match.Groups["build"].Value + "/";
        using var owner = JsonDocument.Parse(Paths.Bytes(Paths.Contained(root, prefix + "owned-launcher.json"), 16384) ?? throw new InstallError("badState"));
        using var ready = JsonDocument.Parse(Paths.Bytes(Paths.Contained(root, prefix + "gui-ready.json"), 262144) ?? throw new InstallError("badState"));
        Contract.Duplicates(owner.RootElement); Contract.Duplicates(ready.RootElement);
        var o = owner.RootElement; var g = ready.RootElement;
        string? native = Paths.FileHash(Paths.Contained(root, "GF2 Game/version.dll")); Paths.HashText(native);
        if (o.GetProperty("state").GetString() != "INSTALLED" || o.GetProperty("build").GetString() != match.Groups["build"].Value ||
            !Paths.Equal(o.GetProperty("launcher").GetString(), Paths.Hash(raw)) ||
            !Paths.Equal(g.GetProperty("launcher").GetString(), Paths.Hash(raw)) ||
            !Paths.Equal(o.GetProperty("native").GetString(), native) || !Paths.Equal(g.GetProperty("native").GetString(), native)) return null;
        foreach (string name in Paths.ToolFiles)
        {
            string? expected = g.GetProperty("payload").GetProperty(name).GetString(); Paths.HashText(expected);
            if (!Paths.Equal(Paths.FileHash(Paths.Contained(root, prefix + "app/" + name)), expected)) return null;
        }
        return Target(root, prefix + "app/");
    }

    private static SettingsTarget? Target(string root, string folder)
    {
        using var contract = JsonDocument.Parse(Paths.Bytes(Paths.Contained(root, folder + "runtime-contract.json"), 16384) ?? throw new InstallError("badState"));
        var c = contract.RootElement; Contract.Duplicates(c);
        Paths.HashText(c.GetProperty("dllSha256").GetString());
        if (c.GetProperty("schemaVersion").GetInt32() != 1 ||
            !Path.GetFullPath(c.GetProperty("configurationRoot").GetString()!).Equals(Path.Combine(root, "vrmod", "config"), StringComparison.OrdinalIgnoreCase) ||
            !Paths.Equal(c.GetProperty("dllSha256").GetString(), Paths.FileHash(Paths.Contained(root, "GF2 Game/version.dll")))) return null;
        return new(Paths.Contained(root, folder + "GF2VR.Configurator.exe"), root);
    }

    internal static void Open(SettingsTarget target, InstallationEngine engine)
    {
        SettingsTarget? fresh = Resolve(target.Root, engine);
        if (fresh == null || fresh != target) throw new InstallError("settingsUnavailable");
        var info = new ProcessStartInfo(fresh.Executable) { UseShellExecute = false, WorkingDirectory = fresh.Root };
        info.ArgumentList.Add("--root"); info.ArgumentList.Add(fresh.Root); Start(info);
    }
}
