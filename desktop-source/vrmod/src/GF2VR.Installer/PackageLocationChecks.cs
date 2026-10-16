namespace GF2VR.Installer;

internal static class PackageLocationChecks
{
    internal static int Run()
    {
        string fixture = Path.Combine(AppContext.BaseDirectory, "package-location-fixture");
        if (Directory.Exists(fixture)) throw new InvalidDataException("Use a fresh package-location fixture directory");
        Directory.CreateDirectory(fixture);
        int checks = 0;
        void Check(bool ok, string message) { ++checks; if (!ok) throw new InvalidDataException(message); }
        void Missing(string package)
        {
            ++checks;
            try { _ = PackageLocation.Require(package); }
            catch (InstallError ex) when (ex.Key == "bundledPackageMissing") { return; }
            throw new InvalidDataException("Incomplete bundle did not provide extraction guidance");
        }
        string application = Path.Combine(fixture, "설치 프로그램 日本語");
        string working = Path.Combine(fixture, "unrelated-working-directory");
        string staging = Path.Combine(fixture, "explicit-update-stage");
        foreach (string directory in new[] { application, working, staging }) Directory.CreateDirectory(directory);
        foreach (string directory in new[] { working, staging })
        {
            File.WriteAllText(Path.Combine(directory, "package-manifest.json"), "INERT LOCATION FIXTURE; NEVER INSTALL");
            Directory.CreateDirectory(Path.Combine(directory, "payload"));
        }
        string oldWorkingDirectory = Environment.CurrentDirectory;
        try
        {
            Environment.CurrentDirectory = working;
            Check(PackageLocation.Resolve() == Path.GetFullPath(AppContext.BaseDirectory), "Default package depended on working directory");
            Check(PackageLocation.FromApplicationDirectory(application) == application, "Missing application bundle guessed another directory");
            Check(!PackageLocation.IsAvailable(application), "Working-directory package hid incomplete extraction"); Missing(application);
            Check(PackageLocation.FromApplicationDirectory(application, staging) == staging, "Explicit updater package was ignored");
            Check(PackageLocation.Require(staging) == staging, "Explicit updater stage was unavailable");
            File.WriteAllText(Path.Combine(application, "package-manifest.json"), "INERT ADJACENT BUNDLE; NEVER INSTALL");
            Check(!PackageLocation.IsAvailable(application), "Manifest alone passed extraction check"); Missing(application);
            Directory.CreateDirectory(Path.Combine(application, "payload"));
            Check(PackageLocation.Require(PackageLocation.FromApplicationDirectory(application)) == application, "Adjacent Unicode bundle was not selected");
            string manifest = Path.Combine(application, "package-manifest.json"); byte[] before = File.ReadAllBytes(manifest);
            _ = PackageLocation.Resolve(staging); _ = PackageLocation.Require(application);
            Check(File.ReadAllBytes(manifest).AsSpan().SequenceEqual(before), "Package lookup changed bundle files");
            File.Delete(manifest);
            Check(!PackageLocation.IsAvailable(application), "Payload directory alone passed extraction check"); Missing(application);
        }
        finally { Environment.CurrentDirectory = oldWorkingDirectory; }
        File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory, "package-location-validation.json"),
            Contract.Encode(new { passed = true, checks, adjacentPackage = true, workingDirectoryIgnored = true, explicitUpdaterPackage = true, liveInstallationWrites = 0, gameLaunches = 0 }));
        return 0;
    }
}
