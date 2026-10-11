namespace GF2VR.Installer;

internal static class Program
{
    [STAThread]
    private static int Main(string[] args)
    {
        ApplicationConfiguration.Initialize();
        try
        {
            InstallerText.Verify();
            if (args.Contains("--self-test-desktop")) return DesktopDeploymentChecks.Run(Value(args, "--package") ?? throw new InstallError("manifestMissing"));
            if (args.Contains("--self-test-portable")) return PortablePackageChecks.Run(Value(args, "--package") ?? throw new InstallError("manifestMissing"));
            if (args.Contains("--self-test-native")) return NativePackageChecks.Run(Value(args, "--package") ?? throw new InstallError("manifestMissing"));
            if (args.Contains("--self-test")) return SelfTest.Run(Value(args, "--package"));
            if (args.Contains("--verify-localization")) return 0;
            string root = Value(args, "--root") ?? FindRoot();
            string? package = Value(args, "--package") ??
                (File.Exists(Path.Combine(AppContext.BaseDirectory, "package-manifest.json")) ? AppContext.BaseDirectory : null);
            if (args.Contains("--interrupt-fixture"))
            {
                SelfTest.InterruptFixture(root, package ?? throw new InstallError("manifestMissing")); return 81;
            }
            if (args.Contains("--check-payload"))
            {
                var checkedPackage = new InstallationEngine().CheckPayload(root, package ?? throw new InstallError("manifestMissing"));
                File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory, "native-payload-validation.json"),
                    Contract.Encode(new { passed = true, checkedPackage.ManifestHash, checkedPackage.Package.Version, files = checkedPackage.Package.Files.Length, installationWrites = 0, ownershipChecked = false }));
                return 0;
            }
            if (args.Contains("--check-package"))
            {
                var plan = new InstallationEngine().Preview(root, package ?? throw new InstallError("manifestMissing"));
                File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory, "package-validation.json"),
                    Contract.Encode(new { passed = true, plan.ManifestHash, plan.Package.Version, files = plan.Package.Files.Length, installWrites = 0 }));
                return 0;
            }
            using var form = new InstallerForm(root, package, Value(args, "--smoke-gui"));
            Application.Run(form); return 0;
        }
        catch (Exception ex)
        {
            if (args.Contains("--self-test") || args.Contains("--self-test-native") || args.Contains("--self-test-portable") || args.Contains("--self-test-desktop") || args.Contains("--check-payload") || args.Contains("--check-package") || args.Contains("--smoke-gui") || args.Contains("--verify-localization"))
                File.WriteAllText(Path.Combine(AppContext.BaseDirectory, "verification-error.txt"), ex.ToString());
            else MessageBox.Show(InstallerText.Error(ex), InstallerText.Get("errorTitle"), MessageBoxButtons.OK, MessageBoxIcon.Error);
            return 1;
        }
    }
    private static string? Value(string[] args, string key)
    {
        int i = Array.IndexOf(args, key); return i >= 0 && i + 1 < args.Length ? args[i + 1] : null;
    }
    private static string FindRoot()
    {
        foreach (string start in new[] { AppContext.BaseDirectory, Environment.CurrentDirectory })
        {
            DirectoryInfo? dir = new(start);
            for (int i = 0; i < 8 && dir != null; ++i, dir = dir.Parent)
                if (File.Exists(Path.Combine(dir.FullName, "GF2 Game", "GF2_Exilium.exe"))) return dir.FullName;
        }
        return Environment.CurrentDirectory;
    }
}
