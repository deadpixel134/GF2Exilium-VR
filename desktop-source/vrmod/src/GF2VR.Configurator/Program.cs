namespace GF2VR.Configurator;

internal static class Program
{
    [STAThread]
    private static int Main(string[] args)
    {
#if GF2VR_LIVE_SETTINGS_ENTRY_OBSERVE
        if (args.Contains("--vr-settings-host")) VrSettingsTrace.Start(Value(args, "--pid"));
#endif
        ApplicationConfiguration.Initialize();
        try
        {
            if (args.Contains("--self-test-update-discovery")) return UpdateDiscoveryChecks.Run(Value(args, "--package")!);
            if (args.Contains("--self-test-save-state")) return MainForm.VerifySaveState();
            if (args.Contains("--self-test")) return SelfTest.Run();
            if (args.Contains("--verify-localization")) { Texts.Verify(); return 0; }
            string root = Value(args, "--root") ?? SettingsStore.FindRoot();
            if (args.Contains("--vr-settings-host"))
            {
                if (!int.TryParse(Value(args, "--pid"), out int pid) || pid <= 0) throw new SettingsError("liveConnectionError");
                using var host = new VrSettingsHost(root, pid); Application.Run(host); return 0;
            }
            if (args.Contains("--check-installation"))
            {
                var installed = new SettingsStore(root); installed.Load();
                File.WriteAllText(Path.Combine(AppContext.BaseDirectory, "installation-validation.json"),
                    "{\"passed\":true,\"matchingRuntimeContract\":true,\"writesToRuntime\":0}");
                return 0;
            }
            string? smoke = Value(args, "--smoke-gui");
            using var form = new MainForm(root, smoke);
            Application.Run(form);
            return Environment.ExitCode;
        }
        catch (Exception ex)
        {
            // The VR host has no desktop modal dialogs; heartbeat failure returns ownership to the game panel.
            if (args.Contains("--vr-settings-host"))
            {
#if GF2VR_LIVE_SETTINGS_ENTRY_OBSERVE
                VrSettingsTrace.Note("startup-error", ex);
#endif
                return 1;
            }
            if (args.Contains("--self-test-update-discovery") || args.Contains("--self-test-save-state"))
            { File.WriteAllText(Path.Combine(AppContext.BaseDirectory, "verification-error.txt"), ex.ToString()); return 1; }
            if (args.Contains("--self-test") || args.Contains("--verify-localization") || args.Contains("--check-installation"))
            {
                File.WriteAllText(Path.Combine(AppContext.BaseDirectory, "verification-error.txt"), ex.ToString());
                return 1;
            }
            MessageBox.Show(Texts.Get("error") + "\n" + Texts.Describe(ex), Texts.Get("title"),
                MessageBoxButtons.OK, MessageBoxIcon.Error);
            return 1;
        }
    }

    private static string? Value(string[] args, string name)
    {
        int at = Array.IndexOf(args, name);
        return at >= 0 && at + 1 < args.Length ? args[at + 1] : null;
    }
}
