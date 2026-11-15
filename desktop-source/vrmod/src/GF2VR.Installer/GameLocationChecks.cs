using System.Reflection;

namespace GF2VR.Installer;

internal static class GameLocationChecks
{
    internal static int Run()
    {
        int checks = 0;
        void Check(bool value, string why) { ++checks; if (!value) throw new InvalidDataException(why); }
        string fixture = Path.Combine(AppContext.BaseDirectory, "location-fixture-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(fixture);
        string Game(string folder, bool standard = false)
        {
            string game = standard ? Path.Combine(folder, "GF2 Game") : folder;
            Directory.CreateDirectory(game);
            foreach (string name in new[] { "GF2_Exilium.exe", "GameAssembly.dll", "UnityPlayer.dll" })
                File.WriteAllText(Path.Combine(game, name), SelfTest.Inert);
            return folder;
        }
        string haoplay = Game(Path.Combine(fixture, "한글 Haoplay"), true);
        string steam = Game(Path.Combine(fixture, "Steam direct"));
        var none = GameLocationDiscovery.Probe([], [], []);
        Check(none.Roots.Length == 0 && none.PreferredRoot == null, "missing metadata leaves manual selection");
        var icon = new RegisteredGame(null, "\"" + Path.Combine(haoplay, "PCLauncher.exe") + "\",0");
        var unique = GameLocationDiscovery.Probe([], [icon], []);
        Check(unique.Roots.SequenceEqual([haoplay]) && unique.PreferredRoot == haoplay, "quoted Haoplay launcher path resolves exact child layout");
        var direct = new RegisteredGame(steam, null);
        var both = GameLocationDiscovery.Probe([], [icon, direct], []);
        Check(both.Roots.Length == 2 && both.PreferredRoot == null, "multiple products do not choose arbitrarily");
        Check(GameLocationDiscovery.Probe([], [icon, icon, new(haoplay.ToUpperInvariant(), null)], []).Roots.Length == 1, "case-insensitive deduplication");
        string tools = Path.Combine(haoplay, "vrmod", "bin"); Directory.CreateDirectory(tools);
        Check(GameLocationDiscovery.Probe([tools], [direct], []).PreferredRoot == haoplay, "nearby current game retains precedence");
        Check(GameLocationDiscovery.Probe([], [new(Path.Combine(fixture, "missing"), null), new(@"\\server\game", null)], []).Roots.Length == 0, "stale/network paths ignored");
        string bad = Path.Combine(fixture, "incomplete"); Directory.CreateDirectory(bad);
        File.WriteAllText(Path.Combine(bad, "GF2_Exilium.exe"), SelfTest.Inert);
        Check(GameLocationDiscovery.Probe([], [new(bad, null)], []).Roots.Length == 0, "executable alone is not a game installation");
        string ambiguous = Game(Path.Combine(fixture, "ambiguous")); Game(ambiguous, true);
        Check(GameLocationDiscovery.Probe([], [new(ambiguous, null)], []).Roots.Length == 0, "ambiguous direct/child layout rejected");
        Check(GameLocationDiscovery.Probe([], [new(fixture, null)], []).Roots.Length == 0, "registered parent is not recursively scanned");

        string home = Path.Combine(fixture, "Steam client"), library = Path.Combine(fixture, "別の library");
        Directory.CreateDirectory(Path.Combine(home, "steamapps")); Directory.CreateDirectory(Path.Combine(library, "steamapps"));
        string installed = Game(Path.Combine(library, "steamapps", "common", "GF2 Exilium"));
        string Escape(string p) => p.Replace("\\", "\\\\");
        string libraries = Path.Combine(home, "steamapps", "libraryfolders.vdf");
        string manifest = Path.Combine(library, "steamapps", "appmanifest_" + GameLocationDiscovery.SteamAppId + ".acf");
        void Manifest(string appId, string directory) => File.WriteAllText(manifest, "\"AppState\" { \"appid\" \"" + appId + "\" \"installdir\" \"" + directory + "\" }");
        File.WriteAllText(libraries, "\"libraryfolders\" { \"1\" { \"path\" \"" + Escape(library) + "\" } }");
        Manifest(GameLocationDiscovery.SteamAppId, "GF2 Exilium");
        Check(GameLocationDiscovery.Probe([], [], [home]).PreferredRoot == installed, "Steam secondary library and manifest");
        File.WriteAllText(libraries, "\"LibraryFolders\" { \"1\" \"" + Escape(library) + "\" }");
        Check(GameLocationDiscovery.Probe([], [], [home]).PreferredRoot == installed, "legacy numbered Steam library path");
        Manifest("1", "GF2 Exilium");
        Check(GameLocationDiscovery.Probe([], [], [home]).Roots.Length == 0, "other Steam app ignored");
        Manifest(GameLocationDiscovery.SteamAppId, "../GF2 Exilium");
        Check(GameLocationDiscovery.Probe([], [], [home]).Roots.Length == 0, "manifest cannot redirect outside common");
        File.WriteAllText(manifest, "{ malformed");
        Check(GameLocationDiscovery.Probe([], [icon], [home]).PreferredRoot == haoplay, "malformed Steam metadata preserves other candidates");
        File.WriteAllText(libraries, new string('x', 1048577));
        Check(GameLocationDiscovery.Probe([], [icon], [home]).PreferredRoot == haoplay, "oversized optional metadata ignored");
        using (var cancel = new CancellationTokenSource())
        {
            cancel.Cancel(); bool cancelled = false;
            try { GameLocationDiscovery.Probe([fixture], [], [], cancel.Token); } catch (OperationCanceledException) { cancelled = true; }
            Check(cancelled, "closing installer cancels discovery");
        }

        string oldLanguage = InstallerText.Language;
        foreach (string locale in InstallerText.Languages)
        {
            using var form = new InstallerForm("", fixture);
            InstallerText.Language = locale;
            typeof(InstallerForm).GetMethod("Translate", BindingFlags.Instance | BindingFlags.NonPublic)!.Invoke(form, null);
            form.ApplyDetectedLocations(both);
            form.StartPosition = FormStartPosition.Manual; form.Location = new Point(-20000, -20000); form.ShowInTaskbar = false;
            form.Show(); Application.DoEvents();
            var combo = form.Controls.Find("detectedGameLocations", true).OfType<ComboBox>().Single();
            Check(combo.Items.Count == 2 && combo.SelectedIndex == -1 && form.SelectedGameRoot == "", "multiple locations require choice " + locale);
            Check(combo.Width > 300 && combo.Parent!.ClientRectangle.Contains(combo.Bounds), "location selector layout " + locale);
            using (var image = new Bitmap(form.Width, form.Height))
            { form.DrawToBitmap(image, new Rectangle(0, 0, image.Width, image.Height)); image.Save(Path.Combine(AppContext.BaseDirectory, "game-location-" + locale + ".png")); }
            combo.SelectedIndex = 0;
            Check(form.SelectedGameRoot == (string)combo.SelectedItem!, "selected location drives original preflight " + locale);
            combo.SelectedIndex = 1;
            Check(form.SelectedGameRoot == (string)combo.SelectedItem!, "switching detected location " + locale);
            form.Close();
        }
        InstallerText.Language = oldLanguage;
        Check(File.ReadAllText(Path.Combine(haoplay, "GF2 Game", "GF2_Exilium.exe")) == SelfTest.Inert &&
            !Directory.Exists(Path.Combine(haoplay, "vrmod", "installer")) && !Directory.Exists(Path.Combine(steam, "vrmod")), "detection and preview perform no install writes");
        File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory, "game-location-validation.json"), Contract.Encode(new
        { passed = true, checks, locales = InstallerText.Languages, registryFixture = false, gameLaunches = 0, realInstallationWrites = 0, fixture }));
        return 0;
    }
}
