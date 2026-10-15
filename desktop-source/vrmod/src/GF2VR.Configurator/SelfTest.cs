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
        var uiField = Fields.All.Single(f => f.Key == "activityPrimaryUiEnabled");
        var faceField = Fields.All.Single(f => f.Key == "swapFaceButtons");
        var roleField = Fields.All.Single(f => f.Key == "swapPanelPointerHands");
        Assert(!Fields.Get(store.Documents, roleField)!.GetValue<bool>() && !(bool)roleField.Default, "role pair default must be left panel/right pointer");
        Assert(!Fields.Get(store.Documents, faceField)!.GetValue<bool>() && !(bool)faceField.Default, "face default must be original click/back");
        Assert(Fields.Get(store.Documents, uiField)!.GetValue<bool>(), "missing Activity file must match native enabled default");
        string config = Path.Combine(root, "vrmod", "config"); Directory.CreateDirectory(config);
        string activity = Path.Combine(config, "activity_ui.json");
        byte[] legacyUi = Encoding.UTF8.GetBytes("{\"activityPrimaryUiEnabled\":false,\"futureUiMetadata\":{\"retain\":42}}");
        File.WriteAllBytes(activity, legacyUi); store.Load();
        Assert(!Fields.Get(store.Documents, uiField)!.GetValue<bool>(), "legacy Activity OFF failed to load");
        Assert(legacyUi.SequenceEqual(File.ReadAllBytes(activity)), "loading legacy UI file rewrote schema or setting");
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
        Assert(!Fields.Get(store.Documents, uiField)!.GetValue<bool>(), "saving unrelated settings changed Activity OFF");
        var uiBefore = Fields.Files.Where(f => f != "activity_ui.json").ToDictionary(f => f, f => File.ReadAllBytes(Path.Combine(config, f)));
        foreach (bool value in new[] { true, false })
        {
            var uiChoice = Fields.Clone(store.Documents); Fields.Set(uiChoice, uiField, JsonValue.Create(value)!);
            store.Save(uiChoice); var reloaded = new SettingsStore(root, testFixture: true); reloaded.Load();
            Assert(Fields.Get(reloaded.Documents, uiField)!.GetValue<bool>() == value, "Activity choice did not persist/reload");
            Assert(reloaded.Documents["activity_ui.json"]["futureUiMetadata"]!["retain"]!.GetValue<int>() == 42, "Activity unknown metadata lost");
            foreach (var p in uiBefore) Assert(p.Value.SequenceEqual(File.ReadAllBytes(Path.Combine(config, p.Key))), "Activity toggle changed " + p.Key);
        }
        var missingUi = Fields.Clone(store.Documents); missingUi["activity_ui.json"] = new JsonObject();
        SettingsStore.Validate(missingUi);
        Assert(Fields.Get(missingUi, uiField) == null && (bool)uiField.Default, "missing Activity key must preserve native ON fallback");
        foreach (var bad in new[] { "null", "0", "\"false\"", "[]", "{}" })
        {
            var invalidUi = Fields.Clone(store.Documents); invalidUi["activity_ui.json"]["activityPrimaryUiEnabled"] = JsonNode.Parse(bad);
            Reject(() => store.Save(invalidUi), "nonboolean Activity " + bad);
        }
        Reject(() => SettingsStore.Parse(Encoding.UTF8.GetBytes("{\"schemaVersion\":2,\"activityPrimaryUiEnabled\":true}"), "activity_ui.json"), "future Activity schema");
        Reject(() => SettingsStore.Parse(Encoding.UTF8.GetBytes("{\"activityPrimaryUiEnabled\":true,\"activityPrimaryUiEnabled\":false}"), "activity_ui.json"), "duplicate Activity choice");
        var faceDocs = Fields.Clone(store.Documents);
        faceDocs["controller_input.json"]["futureFace"] = new JsonObject { ["retain"] = 12 };
        faceDocs["controller_input.json"]["controllerInput"]!["futureBinding"] = "keep";
        store.Save(faceDocs);
        var faceBefore = Fields.Files.Where(f => f != "controller_input.json").ToDictionary(f => f, f => File.ReadAllBytes(Path.Combine(config, f)));
        foreach (bool choice in new[] { true, false })
        {
            var faceChoice = Fields.Clone(store.Documents); Fields.Set(faceChoice, faceField, JsonValue.Create(choice)!);
            store.Save(faceChoice); var reopened = new SettingsStore(root, testFixture: true); reopened.Load();
            Assert(Fields.Get(reopened.Documents, faceField)!.GetValue<bool>() == choice, "face save/reopen mismatch");
            Assert(reopened.Documents["controller_input.json"]["futureFace"]!["retain"]!.GetValue<int>() == 12 &&
                reopened.Documents["controller_input.json"]["controllerInput"]!["futureBinding"]!.GetValue<string>() == "keep", "face unknown metadata lost");
            foreach (var p in faceBefore) Assert(p.Value.SequenceEqual(File.ReadAllBytes(Path.Combine(config, p.Key))), "face toggle changed " + p.Key);
            File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory, "face-native-" + choice + ".json"), SettingsStore.Encode(reopened.Documents["controller_input.json"]));
        }
        foreach (string bad in new[] { "null", "1", "\"true\"", "[]", "{}" })
        {
            var invalid = Fields.Clone(store.Documents); invalid["controller_input.json"]["controllerInput"]!["swapFaceButtons"] = JsonNode.Parse(bad);
            Reject(() => store.Save(invalid), "invalid face value " + bad);
        }
        foreach (string group in new[] { "null", "false", "[]" })
        {
            var invalid = Fields.Clone(store.Documents); invalid["controller_input.json"]["controllerInput"] = JsonNode.Parse(group);
            Reject(() => store.Save(invalid), "invalid face group " + group);
        }
        Reject(() => SettingsStore.Parse(Encoding.UTF8.GetBytes("{\"controllerInput\":{\"swapFaceButtons\":true,\"swapFaceButtons\":false}}"), "controller_input.json"), "duplicate face option");
        var optionalFace = Fields.Clone(store.Documents); optionalFace["controller_input.json"] = SettingsStore.Parse(Encoding.UTF8.GetBytes("{}"), "controller_input.json");
        SettingsStore.Validate(optionalFace); Assert(Fields.Get(optionalFace, faceField) == null, "native-compatible empty face document rejected");
        string faceProfile = Path.Combine(root, "face-profile.json"); SettingsStore.Export(faceProfile, store.Documents, root);
        Assert(!Fields.Get(SettingsStore.Import(faceProfile), faceField)!.GetValue<bool>(), "face profile roundtrip changed false");
        var currentFace = Fields.Clone(store.Documents); Fields.Set(currentFace, faceField, JsonValue.Create(true)!);
        foreach (bool omitActivity in new[] { false, true })
        {
            var faceOldProfile = JsonNode.Parse(File.ReadAllText(faceProfile))!.AsObject();
            faceOldProfile["configurations"]!.AsObject().Remove("controller_input.json");
            if (omitActivity) faceOldProfile["configurations"]!.AsObject().Remove("activity_ui.json");
            File.WriteAllText(faceProfile, faceOldProfile.ToJsonString());
            var faceMigrated = SettingsStore.Import(faceProfile, currentFace);
            Assert(Fields.Get(faceMigrated, faceField)!.GetValue<bool>() && faceMigrated["controller_input.json"]["futureFace"]!["retain"]!.GetValue<int>() == 12, "old profile reset newer face choice");
        }
        string facePath = Path.Combine(config, "controller_input.json"); byte[] faceOriginal = File.ReadAllBytes(facePath);
        File.WriteAllText(facePath, "{\"schemaVersion\":1,\"controllerInput\":{\"swapFaceButtons\":true}}");
        Reject(() => store.Save(Fields.Clone(store.Documents)), "external face change");
        Assert(File.ReadAllText(facePath).Contains("true"), "external face preference overwritten");
        File.WriteAllBytes(facePath, faceOriginal); store.Load();
        var roleBefore = Fields.Files.Where(f => f != "controller_input.json").ToDictionary(f => f, f => File.ReadAllBytes(Path.Combine(config, f)));
        foreach (bool role in new[] { true, false }) foreach (bool face in new[] { true, false })
        {
            var choice = Fields.Clone(store.Documents); Fields.Set(choice, roleField, JsonValue.Create(role)!); Fields.Set(choice, faceField, JsonValue.Create(face)!);
            store.Save(choice); var reopened = new SettingsStore(root, testFixture: true); reopened.Load();
            Assert(Fields.Get(reopened.Documents, roleField)!.GetValue<bool>() == role && Fields.Get(reopened.Documents, faceField)!.GetValue<bool>() == face, "role/face combination did not persist");
            Assert(reopened.Documents["controller_input.json"]["futureFace"]!["retain"]!.GetValue<int>() == 12, "role edit lost unknown fields");
            foreach (var p in roleBefore) Assert(p.Value.SequenceEqual(File.ReadAllBytes(Path.Combine(config, p.Key))), "role edit changed " + p.Key);
            File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory, "role-native-" + role + "-" + face + ".json"), SettingsStore.Encode(reopened.Documents["controller_input.json"]));
        }
        foreach (string bad in new[] { "null", "1", "\"true\"", "[]", "{}" })
        {
            var invalid = Fields.Clone(store.Documents); invalid["controller_input.json"]["controllerInput"]!["swapPanelPointerHands"] = JsonNode.Parse(bad);
            Reject(() => store.Save(invalid), "invalid hand role " + bad);
        }
        Reject(() => SettingsStore.Parse(Encoding.UTF8.GetBytes("{\"controllerInput\":{\"swapPanelPointerHands\":true,\"swapPanelPointerHands\":false}}"), "controller_input.json"), "duplicate role option");
        var roleCurrent = Fields.Clone(store.Documents); Fields.Set(roleCurrent, roleField, JsonValue.Create(true)!);
        string roleProfile = Path.Combine(root, "role-profile.json"); SettingsStore.Export(roleProfile, roleCurrent, root);
        Assert(Fields.Get(SettingsStore.Import(roleProfile), roleField)!.GetValue<bool>(), "role profile did not roundtrip");
        var faceOnlyProfile = JsonNode.Parse(File.ReadAllText(roleProfile))!.AsObject();
        faceOnlyProfile["configurations"]!["controller_input.json"]!["controllerInput"]!.AsObject().Remove("swapPanelPointerHands");
        File.WriteAllText(roleProfile, faceOnlyProfile.ToJsonString());
        Assert(Fields.Get(SettingsStore.Import(roleProfile, roleCurrent), roleField)!.GetValue<bool>(), "face-only old profile reset role preference");
        var explicitRolesOff = JsonNode.Parse(File.ReadAllText(roleProfile))!.AsObject();
        explicitRolesOff["configurations"]!["controller_input.json"]!["controllerInput"]!["swapPanelPointerHands"] = false;
        File.WriteAllText(roleProfile, explicitRolesOff.ToJsonString());
        Assert(!Fields.Get(SettingsStore.Import(roleProfile, roleCurrent), roleField)!.GetValue<bool>(), "explicit imported role off ignored");
        foreach (string lang in Texts.Languages)
        {
            Texts.Language = lang;
            Assert(Texts.Get("swapPanelPointerHands") != Texts.Get("unavailable") && Texts.Get("handRolesHelp") != Texts.Get("unavailable"), "missing packaged role localization");
        }
        string outside = Path.Combine(root, "native-outside"); Directory.CreateDirectory(outside);
        foreach (string f in Fields.Files) File.WriteAllBytes(Path.Combine(outside, f), SettingsStore.Encode(store.Documents[f]));
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
        Assert(!Fields.Get(imported, uiField)!.GetValue<bool>(), "profile roundtrip lost Activity OFF");
        var oldProfile = JsonNode.Parse(File.ReadAllText(profile))!.AsObject();
        oldProfile["configurations"]!.AsObject().Remove("activity_ui.json");
        string legacyProfile = Path.Combine(root, "legacy-profile.json"); File.WriteAllBytes(legacyProfile, SettingsStore.Encode(oldProfile));
        var migrated = SettingsStore.Import(legacyProfile, store.Documents);
        Assert(!Fields.Get(migrated, uiField)!.GetValue<bool>(), "old profile reset current Activity OFF");
        Assert(migrated["activity_ui.json"]["futureUiMetadata"]!["retain"]!.GetValue<int>() == 42, "old profile lost current UI metadata");
        Assert(Fields.Get(SettingsStore.Import(legacyProfile), uiField)!.GetValue<bool>(), "old profile without current state lost default ON");
        oldProfile["configurations"]!["unexpected.json"] = new JsonObject(); File.WriteAllBytes(legacyProfile, SettingsStore.Encode(oldProfile));
        Reject(() => SettingsStore.Import(legacyProfile, store.Documents), "unknown legacy profile document");
        Reject(() => SettingsStore.Export(Path.Combine(config, "navigation.json"), store.Documents), "export into runtime file");
        Reject(() => SettingsStore.Export(Path.Combine(config, "activity_ui.json"), store.Documents, root), "export into other runtime settings");
        foreach (string lang in Texts.Languages)
        {
            store.SaveLanguage(lang); Assert(store.LoadLanguage() == lang, "locale persistence " + lang);
            Texts.Language = lang; Assert(Texts.Get("title").Length > 0 && Texts.Get("saved").Length > 0, "packaged strings " + lang);
            foreach (string key in new[] { "activityUiTab", "activityPrimaryUiEnabled", "activityUiSummary", "activityUiHelp" })
                Assert(Texts.Get(key).Length > 0 && Texts.Get(key) != Texts.Get("unavailable"), "Activity locale text " + lang + "/" + key);
        }
        string pref = Path.Combine(config, "configurator.json");
        // Seven-file persistence and native-facing spatial samples. Previous
        // six-file profiles preserve the current newer spatial preferences.
        var worldField = Fields.All.Single(f => f.Key == "worldScale");
        var depthLink = Fields.All.Single(f => f.Key == "linkStereoDepthToWorldScale");
        var speedLink = Fields.All.Single(f => f.Key == "linkMovementSpeedToWorldScale");
        Assert((double)worldField.Default == 1 && (bool)depthLink.Default && (bool)speedLink.Default, "world neutral/default links");
        int sampleIndex = 0;
        foreach (double world in new[] { .01, .25, .5, 1d, 2, 4, 100 }) foreach (bool dl in new[] { true, false }) foreach (bool ml in new[] { true, false })
        {
            var choice = Fields.Clone(store.Documents); choice["world_scale.json"]["futureSpatial"] = 42;
            Fields.Set(choice, worldField, JsonValue.Create(world)!); Fields.Set(choice, depthLink, JsonValue.Create(dl)!); Fields.Set(choice, speedLink, JsonValue.Create(ml)!);
            var beforeFiles = Fields.Files.Where(f => f != "world_scale.json").ToDictionary(f => f, f => File.ReadAllBytes(Path.Combine(config, f)));
            store.Save(choice); var spatialReload = new SettingsStore(root, testFixture: true); spatialReload.Load();
            Assert(spatialReload.Documents["world_scale.json"]["worldScale"]!.GetValue<double>() == world, "world range clamped or reload lost scale");
            Assert(spatialReload.Documents["world_scale.json"]["linkStereoDepthToWorldScale"]!.GetValue<bool>() == dl && spatialReload.Documents["world_scale.json"]["linkMovementSpeedToWorldScale"]!.GetValue<bool>() == ml, "spatial links not persisted");
            Assert(spatialReload.Documents["world_scale.json"]["futureSpatial"]!.GetValue<int>() == 42, "spatial unknown metadata lost");
            foreach (var p in beforeFiles) Assert(p.Value.SequenceEqual(File.ReadAllBytes(Path.Combine(config, p.Key))), "world change altered " + p.Key);
            File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory, $"world-native-{sampleIndex++}.json"), File.ReadAllBytes(Path.Combine(config, "world_scale.json")));
        }
        var worldProfile = Path.Combine(root, "world-profile.json"); SettingsStore.Export(worldProfile, store.Documents);
        Assert(JsonNode.DeepEquals(SettingsStore.Import(worldProfile)["world_scale.json"], store.Documents["world_scale.json"]), "world profile roundtrip");
        var sixFile = JsonNode.Parse(File.ReadAllText(worldProfile))!.AsObject(); sixFile["configurations"]!.AsObject().Remove("world_scale.json"); File.WriteAllBytes(worldProfile, SettingsStore.Encode(sixFile));
        Assert(JsonNode.DeepEquals(SettingsStore.Import(worldProfile, store.Documents)["world_scale.json"], store.Documents["world_scale.json"]), "old six-file profile reset current world");
        Assert(SettingsStore.Import(worldProfile)["world_scale.json"]["worldScale"]!.GetValue<double>() == 1, "old profile lost neutral default");
        foreach (var badWorld in new JsonNode?[] { null, JsonValue.Create(0), JsonValue.Create(-1), JsonValue.Create("2"), JsonValue.Create(1e-320) })
        {
            var badChoice = Fields.Clone(store.Documents); badChoice["world_scale.json"]["worldScale"] = badWorld?.DeepClone(); Reject(() => SettingsStore.Validate(badChoice), "malformed/unrepresentable world");
        }
        foreach (string key in new[] { "linkStereoDepthToWorldScale", "linkMovementSpeedToWorldScale" })
        {
            var badChoice = Fields.Clone(store.Documents); badChoice["world_scale.json"][key] = null; Reject(() => SettingsStore.Validate(badChoice), "null spatial link");
        }
        var overflowSpeed = Fields.Clone(store.Documents); overflowSpeed["world_scale.json"]["worldScale"] = 100; overflowSpeed["world_scale.json"]["linkMovementSpeedToWorldScale"] = true; overflowSpeed["navigation.json"]["navigation"]!["movementSpeed"] = 1e307;
        Reject(() => SettingsStore.Validate(overflowSpeed), "effective spatial speed overflow");
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
        var followField = Fields.All.Single(f => f.Key == "cameraFollowEnabled");
        Assert((bool)followField.Default, "camera following factory default must remain ON");
        var legacyFollow = Fields.Clone(store.Documents);
        legacyFollow["navigation.json"]["navigation"]!.AsObject().Remove("cameraFollowEnabled");
        SettingsStore.Validate(legacyFollow);
        File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory, "follow-native-2.json"), SettingsStore.Encode(legacyFollow["navigation.json"]));
        int followIndex = 0;
        foreach (bool enabled in new[] { true, false })
        {
            var change = Fields.Clone(store.Documents); Fields.Set(change, followField, JsonValue.Create(enabled)!);
            var preserved = Fields.Files.Where(n => n != "navigation.json").ToDictionary(n => n, n => File.ReadAllBytes(Path.Combine(config, n)));
            store.Save(change); var reload = new SettingsStore(root, true); reload.Load();
            Assert(Fields.Get(reload.Documents, followField)!.GetValue<bool>() == enabled, "follow startup choice lost on reload");
            foreach (var p in preserved) Assert(p.Value.SequenceEqual(File.ReadAllBytes(Path.Combine(config, p.Key))), "follow choice changed " + p.Key);
            File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory, $"follow-native-{followIndex++}.json"), File.ReadAllBytes(Path.Combine(config, "navigation.json")));
            var followProfile = Path.Combine(root, "follow-profile.json"); SettingsStore.Export(followProfile, store.Documents);
            Assert(Fields.Get(SettingsStore.Import(followProfile), followField)!.GetValue<bool>() == enabled, "follow profile roundtrip");
            SettingsStore.Export(followProfile, legacyFollow);
            Assert(Fields.Get(SettingsStore.Import(followProfile, store.Documents), followField)!.GetValue<bool>() == enabled, "old seven-document profile reset current follow choice");
            Assert(Fields.Get(SettingsStore.Import(followProfile), followField)!.GetValue<bool>(), "old profile without saved state lost default ON");
        }
        foreach (JsonNode? invalid in new JsonNode?[] { null, JsonValue.Create(0), JsonValue.Create("false"), new JsonObject(), new JsonArray() })
        {
            var badFollow = Fields.Clone(store.Documents); badFollow["navigation.json"]["navigation"]!["cameraFollowEnabled"] = invalid?.DeepClone();
            Reject(() => SettingsStore.Validate(badFollow), "follow explicit invalid/null must not mean missing default");
        }
        Assert(Fields.Get(Fields.Defaults(), followField)!.GetValue<bool>(), "reset lost camera follow ON default");
        // Leave real GUI smoke and native-parser fixtures in a valid, representative current state.
        fixtureNew = Fields.Clone(store.Documents);
        fixtureNew["navigation.json"]["navigation"]!["locomotionHand"] = "left";
        fixtureNew["navigation.json"]["navigation"]!["cameraFollowEnabled"] = true;
        fixtureNew["stereo_depth.json"]["stereoDepthScale"] = 1;
        fixtureNew["world_drag.json"]["worldDrag"]!["enabled"] = true;
        fixtureNew["activity_ui.json"]["activityPrimaryUiEnabled"] = true;
        store.Save(fixtureNew); store.SaveLanguage("ko");
        File.WriteAllText(Path.Combine(AppContext.BaseDirectory, "settings-validation.json"),
            JsonSerializer.Serialize(new { passed = true, checks, languages = Texts.Languages, fixture = root, liveWrites = 0, gameLaunches = 0 },
                new JsonSerializerOptions { WriteIndented = true }));
        return 0;
    }
}
