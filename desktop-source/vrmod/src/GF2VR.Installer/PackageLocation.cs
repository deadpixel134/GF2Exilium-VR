namespace GF2VR.Installer;

internal static class PackageLocation
{
    internal static string Resolve(string? explicitPackage = null) => FromApplicationDirectory(AppContext.BaseDirectory, explicitPackage);

    // The process working directory is unrelated to the extracted release.
    internal static string FromApplicationDirectory(string applicationDirectory, string? explicitPackage = null) =>
        Path.GetFullPath(explicitPackage ?? applicationDirectory);

    internal static bool IsAvailable(string package) =>
        File.Exists(Path.Combine(package, "package-manifest.json")) && Directory.Exists(Path.Combine(package, "payload"));

    internal static string Require(string package) =>
        IsAvailable(package) ? package : throw new InstallError("bundledPackageMissing");
}
