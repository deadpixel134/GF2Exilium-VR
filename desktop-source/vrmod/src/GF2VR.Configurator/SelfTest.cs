using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;

namespace GF2VR.Configurator;

internal static class SelfTest
{
    private static int checks;
    private static void Assert(bool ok, string message)
    {
        ++checks; if (!ok) throw new InvalidDataException(message);
    }
    private static void Reject(Action act, string message)
    {
        ++checks;
        try { act(); } catch (SettingsError) { return; }
        throw new InvalidDataException("Expected rejection: " + message);
    }
    internal static int Run()
    {
        Texts.Verify();
        string root = Path.Combine(AppContext.BaseDirectory, "validation-fixture");
        // Persistent bounded fixture makes verification reviewable; never touches the real installation.
        if (Directory.Exists(root)) throw new InvalidOperationException("Use a fresh validation-fixture directory.");
        Directory.CreateDirectory(Path.Combine(root, "GF2 Game"));
        File.WriteAllText(Path.Combine(root, "GF2 Game", "GF2_Exilium.exe"), "INERT SETTINGS TEST FIXTURE; NEVER EXECUTE");
        var store = new SettingsStore(root, testFixture: true); store.Load();
        Reject(() => new SettingsStore(root), "unknown deployed runtime cannot expose active configuration");
        Assert(!Directory.Exists(Path.Combine(root, "vrmod", "config")), "load created configuration");
        var requested = Fields.Clone(store.Documents);
        requested["render_quality.json"]["eyeRenderScale"] = 2.5;
        requested["navigation.json"]["navigation"]!["locomotionHand"] = "right";
        requested["navigation.json"]["navigation"]!["futureSetting"] = new JsonObject { ["enabled"] = true };
        requested["stereo_depth.json"]["stereoDepthScale"] = 2.5;
        store.Save(requested);
        Assert(store.Documents["render_quality.json"]["eyeRenderScale"]!.GetValue<double>() == 2.5, "soft range clamp");
        Assert(store.Documents["stereo_depth.json"]["stereoDepthScale"]!.GetValue<double>() == 2.5, "depth guidance clamp");
        Assert(store.Documents["navigation.json"]["navigation"]!["locomotionHand"]!.GetValue<string>() == "right", "hand swap persistence");
        Assert(store.Documents["navigation.json"]["navigation"]!["futureSetting"]!["enabled"]!.GetValue<bool>(), "future field lost");
        string outside = Path.Combine(root, "native-outside"); Directory.CreateDirectory(outside);
        foreach (string f in Fields.Files) File.WriteAllBytes(Path.Combine(outside, f), SettingsStore.Encode(store.Documents[f]));
        string config = Path.Combine(root, "vrmod", "config");
        var original = Fields.Files.ToDictionary(f => f, f => File.ReadAllBytes(Path.Combine(config, f)));
        var changed = Fields.Clone(store.Documents); changed["render_quality.json"]["eyeRenderScale"] = .6;
        var beforeGuard = SettingsStore.GameRunning; SettingsStore.GameRunning = () => true;
        Reject(() => store.Save(changed), "game running"); SettingsStore.GameRunning = beforeGuard;
        foreach (string file in Fields.Files) Assert(original[file].SequenceEqual(File.ReadAllBytes(Path.Combine(config, file))), "guard mutated files");
        changed["navigation.json"]["navigation"]!["stickDeadzone"] = 1;
        Reject(() => store.Save(changed), "deadzone semantics");
        foreach (string file in Fields.Files) Assert(original[file].SequenceEqual(File.ReadAllBytes(Path.Combine(config, file))), "invalid save mutated files");
        changed = Fields.Clone(store.Documents); changed["world_drag.json"]["worldDrag"]!["enabled"] = true;
        changed["world_drag.json"]["worldDrag"]!["trackRightHand"] = false;
        Reject(() => store.Save(changed), "drag enabled with no tracked hand");
        changed["world_drag.json"]["worldDrag"]!["trackRightHand"] = true;
        changed["world_drag.json"]["worldDrag"]!["rightGripActivation"] = false;
        Reject(() => store.Save(changed), "drag enabled with no activation");
        Reject(() => SettingsStore.Parse(Encoding.UTF8.GetBytes("{\"schemaVersion\":1,\"schemaVersion\":1,\"eyeRenderScale\":1}"), "render_quality.json"), "duplicate");
        Reject(() => SettingsStore.Parse(Encoding.UTF8.GetBytes("{\"schemaVersion\":2,\"eyeRenderScale\":1}"), "render_quality.json"), "future schema");
        Reject(() => SettingsStore.Parse(Encoding.UTF8.GetBytes("{\"schemaVersion\":1,\"eyeRenderScale\":1,\"unknown\":1}"), "render_quality.json"), "strict native render schema");
        Reject(() => SettingsStore.Parse(new byte[4097], "navigation.json"), "runtime size bound");
        foreach (string value in new[] { "NaN", "Infinity", "-1", "-0", "1e999", "1e-999" })
            Reject(() => SettingsStore.Number(value, "movementSpeed"), "invalid numeric " + value);
        Assert(SettingsStore.Number("0", "movementSpeed") == 0, "explicit zero");
        Assert(SettingsStore.Number("6e-1", "eyeRenderScale") == .6, "exponent input");
        string nav = Path.Combine(config, "navigation.json");
        File.AppendAllText(nav, " ");
        Reject(() => store.Save(store.Documents), "external changes");
        Assert(File.ReadAllText(nav).EndsWith(' '), "external file overwritten");
        store.Load();
        var transactional = Fields.Clone(store.Documents);
        transactional["render_quality.json"]["eyeRenderScale"] = .8;
        transactional["stereo_depth.json"]["stereoDepthScale"] = .8;
        transactional["navigation.json"]["navigation"]!["movementSpeed"] = 2;
        // Permit reads but deny replacement on the third file. Earlier replacements must be restored.
        var transactionBefore = Fields.Files.ToDictionary(f => f, f => File.ReadAllBytes(Path.Combine(config, f)));
        using (var locked = new FileStream(nav, FileMode.Open, FileAccess.Read, FileShare.Read))
        {
            bool failed = false;
            try { store.Save(transactional); } catch (IOException) { failed = true; }
            Assert(failed, "partial transaction did not fail at locked navigation file");
        }
        foreach (string file in Fields.Files)
            Assert(transactionBefore[file].SequenceEqual(File.ReadAllBytes(Path.Combine(config, file))), "partial save rollback " + file);
        var fixtureNew = Fields.Clone(store.Documents); fixtureNew["render_quality.json"]["eyeRenderScale"] = .6;
        store.Save(fixtureNew);
        Assert(store.Documents["render_quality.json"]["eyeRenderScale"]!.GetValue<double>() == .6, "restart-persisted render choice");
        string profile = Path.Combine(root, "profile.json"); SettingsStore.Export(profile, store.Documents);
        var imported = SettingsStore.Import(profile);
        Assert(JsonNode.DeepEquals(imported["navigation.json"], store.Documents["navigation.json"]), "profile roundtrip");
        Reject(() => SettingsStore.Export(Path.Combine(config, "navigation.json"), store.Documents), "export into runtime file");
        Reject(() => SettingsStore.Export(Path.Combine(config, "activity_ui.json"), store.Documents, root), "export into other runtime settings");
        foreach (string lang in Texts.Languages)
        {
            store.SaveLanguage(lang); Assert(store.LoadLanguage() == lang, "locale persistence " + lang);
            Texts.Language = lang; Assert(Texts.Get("title").Length > 0 && Texts.Get("saved").Length > 0, "packaged strings " + lang);
        }
        string pref = Path.Combine(config, "configurator.json");
        File.WriteAllText(pref, "{\"schemaVersion\":1,\"language\":\"future\",\"retain\":42}");
        Assert(store.LoadLanguage() == "ko", "unknown language fallback");
        store.SaveLanguage("ja");
        Assert(JsonNode.Parse(File.ReadAllText(pref))!["retain"]!.GetValue<int>() == 42, "language update lost preference");
        Texts.Language = "future"; Assert(Texts.Get("title") == "GF2 VR 설정", "unknown locale lookup");
        Assert(Texts.Get("missing.translation") == Texts.Get("unavailable"), "missing key fallback");
        // An invalid existing file blocks load and is never rewritten to defaults.
        var future = Fields.Clone(store.Documents); future["render_quality.json"]["schemaVersion"] = 7;
        byte[] newer = SettingsStore.Encode(future["render_quality.json"]);
        File.WriteAllBytes(Path.Combine(config, "render_quality.json"), newer);
        Reject(store.Load, "invalid existing schema");
        Assert(newer.SequenceEqual(File.ReadAllBytes(Path.Combine(config, "render_quality.json"))), "invalid file overwritten");
        File.WriteAllBytes(Path.Combine(config, "render_quality.json"), SettingsStore.Encode(fixtureNew["render_quality.json"]));
        store.Load();
        // Leave real GUI smoke and native-parser fixtures in a valid, representative current state.
        fixtureNew = Fields.Clone(store.Documents);
        fixtureNew["navigation.json"]["navigation"]!["locomotionHand"] = "left";
        fixtureNew["stereo_depth.json"]["stereoDepthScale"] = 1;
        fixtureNew["world_drag.json"]["worldDrag"]!["enabled"] = true;
        store.Save(fixtureNew); store.SaveLanguage("ko");
        File.WriteAllText(Path.Combine(AppContext.BaseDirectory, "settings-validation.json"),
            JsonSerializer.Serialize(new { passed = true, checks, languages = Texts.Languages, fixture = root, liveWrites = 0, gameLaunches = 0 },
                new JsonSerializerOptions { WriteIndented = true }));
        return 0;
    }
}
