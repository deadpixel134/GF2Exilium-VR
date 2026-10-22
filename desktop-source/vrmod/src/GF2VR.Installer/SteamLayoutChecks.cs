using System.Diagnostics;
using System.Text;

namespace GF2VR.Installer;

internal static class SteamLayoutChecks
{
    private static int checks;
    private static void Check(bool value, string label)
    { ++checks; if (!value) throw new InvalidDataException(label); }
    internal static int Run(string packageRoot)
    {
        string fixture = Path.Combine(AppContext.BaseDirectory, "steam-layout-validation-fixture");
        if (Directory.Exists(fixture)) throw new InvalidDataException("Preserve fixture; use a fresh output.");
        var source = Contract.Read<Package>(File.ReadAllBytes(Path.Combine(packageRoot, "package-manifest.json")));
        Check(source.Native != null && NativeLayout.IsFlexible(source), "supported paired native required");
        foreach (string version in new[] { "2.0.3936", "2.7.4330" })
        foreach (bool standard in new[] { true, false })
        {
            string root = Path.Combine(fixture, version, standard ? "launcher" : "Steam 한글 client");
            string game = standard ? Path.Combine(root, "GF2 Game") : root;
            Directory.CreateDirectory(game);
            string exe = Path.Combine(game, "GF2_Exilium.exe"); File.WriteAllText(exe, SelfTest.Inert);
            Check(Paths.Root(exe) == root && Paths.Root(game) == root, "both executable and folder entry");
            var references = NativeLayout.ReferenceGameFiles(version).Select(p =>
                new PayloadFile(p, Paths.Hash(Encoding.UTF8.GetBytes(p)), 1)).ToArray();
            Check(NativeLayout.ValidGameReferences(references), "complete known reference set");
            var bad = (PayloadFile[])references.Clone();
            int index = Array.FindIndex(bad, f => f.Path.Contains("/Codes/", StringComparison.Ordinal));
            bad[index] = bad[index] with { Path = bad[index].Path.Replace(version, "9.9.9999", StringComparison.Ordinal) };
            Check(!NativeLayout.ValidGameReferences(bad), "unknown/mixed Codes reference rejected");
            bad[index] = bad[index] with { Path = "GF2 Game/../version.dll" };
            Check(!NativeLayout.ValidGameReferences(bad), "reference traversal rejected");
            bad[index] = references[0]; Check(!NativeLayout.ValidGameReferences(bad), "duplicate reference rejected");
            string package = Path.Combine(root, "fixture-package");
            foreach (var file in source.Files)
                Paths.WriteNew(Paths.Contained(package, "payload/" + file.Path), File.ReadAllBytes(Paths.Contained(packageRoot, "payload/" + file.Path)));
            var manifest = source with { Native = source.Native! with { GameFiles = references } };
            Paths.WriteNew(Paths.Contained(package, "package-manifest.json"), Contract.Encode(manifest));
            // Actual unrelated game files must remain unowned and unchanged.
            var preserved = new Dictionary<string, byte[]>();
            foreach (string name in new[] { "GameAssembly.dll", "UnityPlayer.dll", "NEP2.dll" })
            {
                string path = Path.Combine(game, name); byte[] bytes = Encoding.UTF8.GetBytes("INERT ORIGINAL " + name);
                File.WriteAllBytes(path, bytes); preserved.Add(path, bytes);
            }
            string config = Paths.Contained(root, "vrmod/config/configurator.json");
            byte[] settings = Encoding.UTF8.GetBytes("{\"language\":\"ja\",\"future\":42}");
            Paths.WriteNew(config, settings);
            var engine = new InstallationEngine();
            var plan = engine.Preview(exe, package);
            Check(!File.Exists(Path.Combine(game, "version.dll")), "preview has no install mutation");
            engine.Install(plan);
            Check(Paths.Equal(Paths.FileHash(Path.Combine(game, "version.dll")), manifest.Native!.DllSha256), "native beside actual executable");
            Check(engine.Inspect(root)!.GameDirectory == (standard ? "GF2 Game" : ""), "receipt records actual layout");
            var target = SettingsLaunch.Resolve(root, engine)!;
            var info = new ProcessStartInfo(target.Executable) { UseShellExecute = false, CreateNoWindow = true, WindowStyle = ProcessWindowStyle.Hidden };
            foreach (string arg in new[] { "--check-installation", "--root", root }) info.ArgumentList.Add(arg);
            using (var process = Process.Start(info)!)
                Check(process.WaitForExit(20000) && process.ExitCode == 0, "actual Configurator contract for layout/Codes");
            Check(UpdateStaging.ManagedVersion(root) == source.Version, "updater recognizes current profile");
            engine.Remove(root, engine.ReceiptHash(root));
            Check(!File.Exists(Path.Combine(game, "version.dll")) && engine.Inspect(root) == null, "exact absent native/receipt restored");
            Check(File.ReadAllBytes(config).SequenceEqual(settings), "locale/future user settings preserved");
            foreach (var original in preserved)
                Check(File.ReadAllBytes(original.Key).SequenceEqual(original.Value), "original game file preserved " + original.Key);
        }
        File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory, "steam-layout-validation.json"), Contract.Encode(new { passed = true, checks, layouts = 2, codesBuilds = 2, actualConfiguratorProcesses = 4, gameLaunches = 0, gameBinariesLoaded = false }));
        return 0;
    }
}
