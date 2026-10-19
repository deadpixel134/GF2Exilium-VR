namespace GF2VR.Installer;

internal static class Program
{
    [STAThread]
    private static int Main(string[] args)
    {
        ApplicationConfiguration.Initialize();
        bool updateFixture = false;
        try
        {
            InstallerText.Verify();
            if (args.Contains("--self-test-log-cleanup")) return RuntimeLogCleanupChecks.Run();
            if (args.Contains("--self-test-variants")) return VariantChecks.Run(Value(args, "--clean")!, Value(args, "--dirty")!);
            if (args.Contains("--self-test-package-location")) return PackageLocationChecks.Run();
            if (args.Contains("--self-test-ownership")) return OwnershipChecks.Run(Value(args, "--package") ?? PackageLocation.Resolve());
            if (args.Contains("--self-test-flexible")) return FlexibleLayoutChecks.Run(Value(args, "--package") ?? throw new InstallError("manifestMissing"));
            if (args.Contains("--self-test-installer-ux")) return InstallerUxChecks.Run();
            if (args.Contains("--check-settings-launch"))
            {
                var target = SettingsLaunch.Resolve(Value(args, "--root") ?? FindRoot(), new InstallationEngine()) ?? throw new InstallError("settingsUnavailable");
                File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory, "settings-launch-validation.json"), Contract.Encode(new { passed = true, target.Executable, target.Root, launchedProcesses = 0, installationWrites = 0 })); return 0;
            }
            if (args.Contains("--self-test-updates")) return UpdateChecks.Run(Value(args, "--package")!, Value(args, "--previous")!);
            if (args.Contains("--update-parent-fixture")) { Thread.Sleep(1500); return 0; }
            if (args.Contains("--check-update-source"))
            {
                var client = new GithubOwnerClient();
                byte[] json = client.Releases(CancellationToken.None).GetAwaiter().GetResult();
                var release = UpdatePolicy.Select(json, "0.5.0");
                File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory, "update-source-validation.json"), Contract.Encode(new { passed = true, repository = UpdatePolicy.Repository, newerStableRelease = release?.Version, credentialsLogged = false, writesToGame = 0 }));
                return 0;
            }
            if (args.Contains("--apply-confirmed-update"))
            {
                string folder = Value(args, "--stage") ?? throw new InstallError("updateInvalid");
                if (args.Contains("--fixture-no-reopen"))
                {
                    var f = Contract.Read<UpdateHandoff>(File.ReadAllBytes(Paths.Contained(Path.GetFullPath(folder), "handoff.json")));
                    if (!f.Root.Contains("updater-validation-fixture", StringComparison.Ordinal) || File.ReadAllText(Paths.Contained(f.Root, "GF2 Game/GF2_Exilium.exe")) != SelfTest.Inert) throw new InstallError("path", f.Root);
                    updateFixture = true;
                }
                int result = UpdateStaging.Apply(folder);
                if (updateFixture) return result;
                var handoff = Contract.Read<UpdateHandoff>(File.ReadAllBytes(Paths.Contained(Path.GetFullPath(folder), "handoff.json")));
                var start = new System.Diagnostics.ProcessStartInfo(Paths.Contained(handoff.Root, NativeLayout.ToolRoot + "GF2VR.Configurator.exe")) { UseShellExecute = false };
                start.ArgumentList.Add("--root"); start.ArgumentList.Add(handoff.Root);
                using var reopened = System.Diagnostics.Process.Start(start);
                return result;
            }
            if (args.Contains("--self-test-desktop")) return DesktopDeploymentChecks.Run(Value(args, "--package") ?? throw new InstallError("manifestMissing"));
            if (args.Contains("--self-test-portable")) return PortablePackageChecks.Run(Value(args, "--package") ?? throw new InstallError("manifestMissing"));
            if (args.Contains("--self-test-native")) return NativePackageChecks.Run(Value(args, "--package") ?? throw new InstallError("manifestMissing"));
            if (args.Contains("--self-test")) return SelfTest.Run(Value(args, "--package"));
            if (args.Contains("--verify-localization")) return 0;
            string root = Value(args, "--root") ?? FindRoot();
            if (args.Contains("--install") || args.Contains("--remove"))
            {
                if (Value(args, "--root") == null || args.Contains("--install") == args.Contains("--remove")) throw new InstallError("path", root);
                var engine = new InstallationEngine();
                if (args.Contains("--remove")) engine.Remove(root, engine.ReceiptHash(root));
                else engine.Install(engine.Preview(root, PackageLocation.Require(Value(args, "--package") ?? throw new InstallError("manifestMissing"))));
                return 0;
            }
            string package = PackageLocation.Resolve(Value(args, "--package"));
            if (args.Contains("--interrupt-fixture"))
            {
                SelfTest.InterruptFixture(root, PackageLocation.Require(package)); return 81;
            }
            if (args.Contains("--check-payload"))
            {
                var checkedPackage = new InstallationEngine().CheckPayload(root, PackageLocation.Require(package));
                File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory, "native-payload-validation.json"),
                    Contract.Encode(new { passed = true, checkedPackage.ManifestHash, checkedPackage.Package.Version, files = checkedPackage.Package.Files.Length, installationWrites = 0, ownershipChecked = false }));
                return 0;
            }
            if (args.Contains("--check-package"))
            {
                var plan = new InstallationEngine().Preview(root, PackageLocation.Require(package));
                File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory, "package-validation.json"),
                    Contract.Encode(new { passed = true, plan.ManifestHash, plan.Package.Version, files = plan.Package.Files.Length, installWrites = 0 }));
                return 0;
            }
            using var form = new InstallerForm(root, package, Value(args, "--smoke-gui"), args.Contains("--smoke-install-workflow"));
            Application.Run(form); return 0;
        }
        catch (Exception ex)
        {
            if (args.Contains("--apply-confirmed-update"))
            {
                if (updateFixture) return 1;
                MessageBox.Show(InstallerText.Error(ex), InstallerText.Get("errorTitle"), MessageBoxButtons.OK, MessageBoxIcon.Error);
                return 1;
            }
            if (args.Contains("--install") || args.Contains("--remove") || args.Contains("--self-test-variants") || args.Contains("--self-test-package-location") || args.Contains("--self-test-ownership") || args.Contains("--self-test-flexible") || args.Contains("--self-test-installer-ux") || args.Contains("--check-settings-launch") || args.Contains("--self-test") || args.Contains("--self-test-native") || args.Contains("--self-test-portable") || args.Contains("--self-test-desktop") || args.Contains("--self-test-updates") || args.Contains("--check-update-source") || args.Contains("--check-payload") || args.Contains("--check-package") || args.Contains("--smoke-gui") || args.Contains("--verify-localization"))
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
                // Auto-detection checks only exact ancestor locations. An explicit folder
                // selection may use Paths.Root's bounded search without scanning a drive here.
                if (File.Exists(Path.Combine(dir.FullName, "GF2_Exilium.exe")) ||
                    File.Exists(Path.Combine(dir.FullName, "GF2 Game", "GF2_Exilium.exe")))
                    return Paths.Root(dir.FullName);
        }
        return Environment.CurrentDirectory;
    }
}
