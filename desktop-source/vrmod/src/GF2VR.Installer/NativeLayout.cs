using System.Text.Json;
using System.Text.Json.Nodes;

namespace GF2VR.Installer;

// Exact provider profiles; Installer consumes these artifacts without recompiling the renderer.
internal static class NativeLayout
{
    internal const string BuildId = "ACTIVITY-OVERLAY-VIEWPORT-001";
    internal const string DllHash = "0DA26906544F341D262983DF5521760785C768C93A048BC24F35E8739AD48A42";
    internal const string LoaderHash = "66946EA6E9CC2649E299EC412521C5EAFE56C635D05055097FE4DB81E11932DF";
    internal const string PortableBuild = "BOOTSTRAP-PORTABLE-001";
    internal const string PortableDllHash = "B4EC288F021FEE970E2C2E6F7CB46918FF4D5DE3312F82AD438783799B36A2A7";
    internal const string StablePortableBuild = "BOOTSTRAP-STABLE-001";
    internal const string StablePortableDllHash = "F5281E05D0BEA0D83A0B895B2D02558B37C500FCE384739DDE643C182E1A695D";
    internal const string FlexibleBuild = "BOOTSTRAP-FLEX-001";
    internal const string FlexibleDllHash = "6215CEAFFFA46A3B89336DA73203122DD0095977206FDB395A1259F967B54771";
    internal const string PortableBinding = "MODULE_RELATIVE_V1";
    internal const string ContractTemplateRoot = "INSTALL_ROOT/vrmod/config";
    internal static string BoundRuntimeRoot = "G:/GF2Exilium"; // Only isolated fixture verification temporarily substitutes this.
    internal const string ToolRoot = "vrmod/bin/GF2VR.Configurator/";
    internal static readonly string[] ConfigFiles = ["render_quality.json", "stereo_depth.json", "navigation.json", "world_drag.json"];
    internal static readonly string[] StableConfigFiles =
        [.. ConfigFiles, "activity_ui.json", "controller_input.json", "world_scale.json"];
    internal static readonly string[] Files =
        ["GF2 Game/version.dll", "vrmod/runtime/openxr_loader.dll", "vrmod/runtime/openxr-LICENSE.txt", "vrmod/runtime/minhook-LICENSE.txt",
         .. Paths.ToolFiles.Select(n => ToolRoot + n)];
    private const string Codes = "GF2 Game/GF2_Exilium_Data/LocalCache/Data/ClientRes_Windows/2.0.3936/Codes/";
    internal static readonly string[] GameFiles =
        ["GF2 Game/GameAssembly.dll", "GF2 Game/UnityPlayer.dll", "GF2 Game/GF2_Exilium_Data/boot.config",
         "GF2 Game/GF2_Exilium_Data/globalgamemanagers", Codes + "Unity.RenderPipelines.Universal.Runtime.dll.bytes",
         Codes + "Cinemachine.dll.bytes", Codes + "Unity.RenderPipelines.Universal.Runtime.dll.bytes_HUDll.u",
         Codes + "Cinemachine.dll.bytes_HUDll.u", Codes + "Cinemachine.dll.bytes_AOT.u", Codes + "UnityEngine.CoreModule.dll.bytes"];

    internal static void Validate(string root, Package package, HashSet<string> files)
    {
        var native = package.Native;
        if (native == null || !files.SetEquals(Files)) throw new InstallError("incompletePackage");
        bool portable = IsPortable(package);
        if (portable)
        {
            string expectedHash = ExpectedHash(package);
            if (package.BoundGameRoot != null || native.ResourceBinding != PortableBinding ||
                !Paths.Equal(native.DllSha256, expectedHash)) throw new InstallError("nativeBuild");
            // Provider's bounded UTF-16 path buffers; do not claim support for truncation.
            if (Paths.Contained(root, "vrmod/logs/bootstrap/" + native.BuildId + ".jsonl").Length >= 1024)
                throw new InstallError("path", root);
        }
        else
        {
            if (package.BoundGameRoot == null) throw new InstallError("portablePending");
            if (!Path.GetFullPath(root).Equals(Path.GetFullPath(BoundRuntimeRoot), StringComparison.OrdinalIgnoreCase))
                throw new InstallError("boundRoot");
            if (native.BuildId != BuildId || native.ResourceBinding != null || !Paths.Equal(native.DllSha256, DllHash))
                throw new InstallError("nativeBuild");
        }
        if (native.BuildId != FlexibleBuild && File.Exists(Paths.Contained(root, "GF2_Exilium.exe")))
            throw new InstallError("nativeBuild");
        if (!Paths.Equal(native.LoaderSha256, LoaderHash) ||
            package.RequiresInstalledDllSha256 != null) throw new InstallError("nativeBuild");
        if (!Paths.Equal(package.Files.Single(f => f.Path == "GF2 Game/version.dll").Sha256, native.DllSha256) ||
            !Paths.Equal(package.Files.Single(f => f.Path == "vrmod/runtime/openxr_loader.dll").Sha256, native.LoaderSha256))
            throw new InstallError("nativeBuild");
        if (native.GameFiles == null || native.GameFiles.Length != GameFiles.Length ||
            !native.GameFiles.Select(f => f?.Path).ToHashSet(StringComparer.Ordinal).SetEquals(GameFiles))
            throw new InstallError("incompletePackage");
        // Known read-only prerequisites never grant ownership or installation permission.
        foreach (var f in native.GameFiles)
        {
            if (f == null || f.Bytes < 1) throw new InstallError("badPackage");
            Paths.HashText(f.Sha256);
            // Read-only reference metadata may describe a different game patch
            // or cache directory. It never grants installation ownership.
        }
        // Legacy PE export forwarders still name this System32 directory. Never infer portability from a ZIP.
        string expected = Path.GetFullPath("C:/Windows/System32");
        string actual = Path.TrimEndingDirectorySeparator(Environment.GetFolderPath(Environment.SpecialFolder.System));
        if (!File.Exists(Path.Combine(actual, "version.dll"))) throw new InstallError("systemVersionMissing");
        if (!portable && !actual.Equals(expected, StringComparison.OrdinalIgnoreCase)) throw new InstallError("systemBinding");
    }
    internal static bool IsPortable(Package package) => package.Native?.BuildId is PortableBuild or StablePortableBuild or FlexibleBuild;
    internal static string ExpectedHash(Package package) => package.Native?.BuildId switch
    {
        FlexibleBuild => FlexibleDllHash,
        StablePortableBuild => StablePortableDllHash,
        PortableBuild => PortableDllHash,
        _ => DllHash
    };
    internal static void ConfiguratorContract(string root, Package package, Dictionary<string, byte[]> payload)
    {
        try
        {
            byte[] bytes = payload[ToolRoot + "runtime-contract.json"];
            using var doc = JsonDocument.Parse(bytes); Contract.Duplicates(doc.RootElement);
            var c = doc.RootElement;
            bool portable = IsPortable(package);
            bool stable = package.Native?.BuildId is StablePortableBuild or FlexibleBuild;
            string[] configFiles = stable ? StableConfigFiles : ConfigFiles;
            string expectedHash = ExpectedHash(package);
            if (package.Native == null || !Paths.Equal(package.Native.DllSha256, expectedHash) ||
                (!portable && package.Native.BuildId != BuildId) ||
                (portable && (package.BoundGameRoot != null || package.Native.ResourceBinding != PortableBinding)))
                throw new InstallError("configContract");
            string expectedRoot = Path.Combine(root, "vrmod", "config");
            if (c.GetProperty("schemaVersion").GetInt32() != 1 || c.GetProperty("buildId").GetString() != package.Native!.BuildId ||
                !Paths.Equal(c.GetProperty("dllSha256").GetString(), package.Native!.DllSha256) ||
                !(portable ? c.GetProperty("configurationRoot").GetString() == ContractTemplateRoot :
                    Path.TrimEndingDirectorySeparator(Path.GetFullPath(c.GetProperty("configurationRoot").GetString()!))
                    .Equals(expectedRoot, StringComparison.OrdinalIgnoreCase)) ||
                c.GetProperty("apply").GetString() != "GAME_RESTART" ||
                c.GetProperty("files").GetArrayLength() != configFiles.Length ||
                !c.GetProperty("files").EnumerateArray().Select(v => v.GetString()).ToHashSet(StringComparer.Ordinal).SetEquals(configFiles))
                throw new InstallError("configContract");
            if (portable)
            {
                // A single explicit placeholder; never interpolate arbitrary manifest text.
                string[] keys = ["schemaVersion", "buildId", "dllSha256", "configurationRoot", "files", "apply"];
                if (c.EnumerateObject().Count() != keys.Length ||
                    !c.EnumerateObject().Select(p => p.Name).ToHashSet(StringComparer.Ordinal).SetEquals(keys))
                    throw new InstallError("configContract");
                var bound = JsonNode.Parse(bytes)!.AsObject(); bound["configurationRoot"] = expectedRoot;
                payload[ToolRoot + "runtime-contract.json"] = Contract.Encode(bound);
            }
        }
        catch (Exception ex) when (ex is JsonException or InvalidOperationException or KeyNotFoundException or ArgumentException)
        { throw new InstallError("configContract"); }
    }
}
