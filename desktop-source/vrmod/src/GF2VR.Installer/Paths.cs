using System.Security.Cryptography;

namespace GF2VR.Installer;

internal static class Paths
{
    internal static string Root(string root)
    {
        string full = System.IO.Path.TrimEndingDirectorySeparator(System.IO.Path.GetFullPath(root));
        string exe = Contained(full, "GF2 Game/GF2_Exilium.exe");
        if (!File.Exists(exe)) throw new InstallError("gameMissing");
        return full;
    }
    internal static string Contained(string root, string relative)
    {
        ValidRelative(relative);
        string fullRoot = System.IO.Path.TrimEndingDirectorySeparator(System.IO.Path.GetFullPath(root));
        string full = System.IO.Path.GetFullPath(System.IO.Path.Combine(fullRoot, relative.Replace('/', System.IO.Path.DirectorySeparatorChar)));
        if (!full.StartsWith(fullRoot + System.IO.Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase))
            throw new InstallError("path", relative);
        for (string? part = full; part != null; part = System.IO.Path.GetDirectoryName(part))
            if ((File.Exists(part) || Directory.Exists(part)) && (File.GetAttributes(part) & FileAttributes.ReparsePoint) != 0)
                throw new InstallError("path", relative);
        if (Directory.Exists(full)) throw new InstallError("path", relative);
        return full;
    }
    internal static void ValidRelative(string relative)
    {
        if (string.IsNullOrWhiteSpace(relative) || relative.Contains('\\') || relative.Contains(':') || relative.StartsWith('/') || relative.Length > 200)
            throw new InstallError("path", relative);
        foreach (string part in relative.Split('/'))
        {
            string stem = part.Split('.')[0].ToUpperInvariant();
            if (part is "" or "." or ".." || part.EndsWith('.') || part.EndsWith(' ') ||
                part.IndexOfAny(System.IO.Path.GetInvalidFileNameChars()) >= 0 ||
                stem is "CON" or "PRN" or "AUX" or "NUL" ||
                (stem.Length == 4 && (stem.StartsWith("COM") || stem.StartsWith("LPT")) && stem[3] is >= '1' and <= '9'))
                throw new InstallError("path", relative);
        }
    }
    internal static readonly string[] ToolFiles =
    ["GF2VR.Configurator.exe", "GF2VR.Configurator.dll", "GF2VR.Configurator.deps.json", "GF2VR.Configurator.runtimeconfig.json", "runtime-contract.json"];
    internal static bool Owned(string path)
    {
        ValidRelative(path);
        if (path == "GF2 Game/version.dll" || path == "vrmod/runtime/openxr_loader.dll" ||
            path == "vrmod/runtime/openxr-LICENSE.txt" || path == "vrmod/runtime/minhook-LICENSE.txt") return true;
        const string tools = "vrmod/bin/GF2VR.Configurator/";
        if (path.StartsWith(tools, StringComparison.Ordinal)) return ToolFiles.Contains(path[tools.Length..], StringComparer.Ordinal);
        const string runtime = "vrmod/runtime/product/";
        if (!path.StartsWith(runtime, StringComparison.Ordinal)) return false;
        string file = path[runtime.Length..];
        return !file.Contains('/') && (file.EndsWith(".dll", StringComparison.Ordinal) || file.EndsWith(".bin", StringComparison.Ordinal));
    }
    internal static void HashText(string? hash)
    {
        if (hash == null || hash.Length != 64 || hash.Any(c => !Uri.IsHexDigit(c))) throw new InstallError("badHash");
    }
    internal static string Hash(byte[] bytes) => Convert.ToHexString(SHA256.HashData(bytes));
    internal static byte[]? Bytes(string path, int maximum = Contract.MaxFile)
    {
        if (!File.Exists(path)) return null;
        using var file = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read);
        if (file.Length > maximum) throw new InstallError("size", System.IO.Path.GetFileName(path));
        byte[] bytes = new byte[file.Length]; file.ReadExactly(bytes); return bytes;
    }
    internal static string? FileHash(string path)
    {
        if (!File.Exists(path)) return null;
        using var file = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read);
        return Convert.ToHexString(SHA256.HashData(file));
    }
    internal static bool Equal(string? a, string? b) => string.Equals(a, b, StringComparison.OrdinalIgnoreCase);
    internal static string Transaction(string root, string id)
    {
        if (!Guid.TryParseExact(id, "N", out _)) throw new InstallError("badState");
        return System.IO.Path.GetDirectoryName(Contained(root, "vrmod/installer/transactions/" + id + "/journal.json"))!;
    }
    internal static void WriteNew(string path, byte[] bytes)
    {
        Directory.CreateDirectory(System.IO.Path.GetDirectoryName(path)!);
        using var file = new FileStream(path, FileMode.CreateNew, FileAccess.Write, FileShare.None);
        file.Write(bytes); file.Flush(true);
    }
    internal static void Atomic(string path, byte[] bytes)
    {
        string temp = path + "." + Guid.NewGuid().ToString("N") + ".tmp";
        try
        {
            WriteNew(temp, bytes);
            if (File.Exists(path)) File.Replace(temp, path, null); else File.Move(temp, path);
        }
        finally { if (File.Exists(temp)) File.Delete(temp); }
    }
}
