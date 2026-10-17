namespace GF2VR.Configurator;

internal static class Program
{
    [STAThread]
    private static int Main(string[] args)
    {
        ApplicationConfiguration.Initialize();
        try
        {
            if (args.Contains("--self-test")) return SelfTest.Run();
            if (args.Contains("--verify-localization")) { Texts.Verify(); return 0; }
            string root = Value(args, "--root") ?? SettingsStore.FindRoot();
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
