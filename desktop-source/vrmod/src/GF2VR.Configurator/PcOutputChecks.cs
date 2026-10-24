using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;

namespace GF2VR.Configurator;

internal static partial class SelfTest
{
    private static void PcOutputChecks(SettingsStore store, string root)
    {
        const string file = "performance.json";
        var preset = Fields.All.Single(f => f.Key == "pcOutputPreset");
        var scope = Fields.All.Single(f => f.Key == "pcOutputImmersiveOnly");
        var original = Fields.Clone(store.Documents);
        string path = Path.Combine(root, "vrmod", "config", file);
        Assert(Fields.Files.Contains(file) && Fields.StableConfigFiles.Contains(file), "Configurator stable performance registration");
        var contract = new JsonObject { ["buildId"] = GF2VR.Installer.VariantProfiles.CleanBuild,
            ["dllSha256"] = GF2VR.Installer.VariantProfiles.CleanHash,
            ["files"] = new JsonArray(Fields.Files.Select(f => (JsonNode?)JsonValue.Create(f)).ToArray()) };
        bool ContractFiles() { using var doc = JsonDocument.Parse(contract.ToJsonString()); return SettingsStore.MatchesRuntimeFiles(doc.RootElement); }
        bool PcSupported() { using var doc = JsonDocument.Parse(contract.ToJsonString()); return SettingsStore.SupportsPcOutput(doc.RootElement); }
        Assert(ContractFiles(), "full runtime file set remains accepted");
        contract["files"] = new JsonArray(Fields.Files.Where(f => f != file).Select(f => (JsonNode?)JsonValue.Create(f)).ToArray());
        Assert(ContractFiles(), "legacy runtime set accepted without dropping performance ownership");
        Assert(PcSupported(), "reviewed Clean003 legacy contract enables PC output without extra contract key");
        contract["buildId"] = GF2VR.Installer.VariantProfiles.DirtyBuild; contract["dllSha256"] = GF2VR.Installer.VariantProfiles.DirtyHash;
        Assert(PcSupported(), "reviewed Dirty003 legacy contract enables PC output");
        contract["dllSha256"] = new string('0', 64); Assert(!PcSupported(), "unknown native hash never enables PC controls");
        contract["buildId"] = "GF2VR-DIRTY-130-ACT001"; Assert(!PcSupported(), "old native not assumed to consume performance");
        contract["files"]!.AsArray().Add("unexpected.json"); Assert(!ContractFiles(), "extra runtime file rejected");
        contract["files"] = new JsonArray(Fields.Files.Where(f => f != file && f != "navigation.json").Select(f => (JsonNode?)JsonValue.Create(f)).ToArray());
        Assert(!ContractFiles(), "missing required legacy runtime file rejected");
        contract["files"]!.AsArray().Add("render_quality.json"); Assert(!ContractFiles(), "duplicate cannot substitute missing file");
        Assert(Fields.Files.Contains(file) && Fields.Defaults().ContainsKey(file), "legacy metadata does not change saved file ownership");
        Assert(Fields.All.Count(f => f.File == file) == 2 && !Fields.All.Any(f => f.Key is "renderScale" or "dynamicEyeScale"), "only supported PC controls exposed");
        Assert(Fields.Get(Fields.Defaults(), preset)!.GetValue<string>() == "NATIVE" && Fields.Get(Fields.Defaults(), scope)!.GetValue<bool>(), "NATIVE immersive-only defaults");
        File.Delete(path); store.Load();
        Assert(!File.Exists(path) && Fields.Get(store.Documents, preset)!.GetValue<string>() == "NATIVE", "missing file read is NATIVE and creates nothing");
        store.Save(Fields.Clone(store.Documents));
        Assert(File.Exists(path) && store.Documents[file]["pcOutput"]!["mode"]!.GetValue<string>() == "NATIVE", "save creates missing performance file");
        var hidden = Fields.Clone(store.Documents);
        hidden[file] = JsonNode.Parse("""
        {"schemaVersion":1,"pcOutput":{"mode":"FIXED","width":1280,"height":720,"scope":"IMMERSIVE_ONLY","renderScale":0.875,"futurePc":{"items":[1,"keep",null]}},"dynamicEyeScale":{"enabled":false,"floor":0.6,"lowFreshPerSecond":70,"highFreshPerSecond":80,"future":[true,{"keep":"all"}]},"futureRoot":{"keep":42}}
        """)!.AsObject();
        store.Save(hidden);
        string dynamicJson = store.Documents[file]["dynamicEyeScale"]!.ToJsonString();
        string unknownPcJson = store.Documents[file]["pcOutput"]!["futurePc"]!.ToJsonString();
        var unrelated = Fields.Files.Where(f => f != file).ToDictionary(f => f, f => File.ReadAllBytes(Path.Combine(root, "vrmod", "config", f)));
        var dimensions = new[] { ("NATIVE", 1280, 720), ("3200x1800", 3200, 1800), ("2560x1440", 2560, 1440), ("1920x1080", 1920, 1080), ("1280x720", 1280, 720) };
        foreach (var (token, width, height) in dimensions)
        foreach (bool immersive in new[] { true, false })
        {
            var request = Fields.Clone(store.Documents);
            Fields.Set(request, preset, JsonValue.Create(token)!); Fields.Set(request, scope, JsonValue.Create(immersive)!);
            store.Save(request); var reloaded = new SettingsStore(root, true); reloaded.Load();
            var doc = reloaded.Documents[file]; var pc = doc["pcOutput"]!;
            Assert(pc["mode"]!.GetValue<string>() == (token == "NATIVE" ? "NATIVE" : "FIXED"), "preset mode " + token);
            if (token != "NATIVE") Assert(pc["width"]!.GetValue<int>() == width && pc["height"]!.GetValue<int>() == height, "preset dimensions " + token);
            Assert(Fields.Get(reloaded.Documents, preset)!.GetValue<string>() == token && pc["scope"]!.GetValue<string>() == (immersive ? "IMMERSIVE_ONLY" : "ALWAYS"), "preset scope roundtrip");
            Assert(doc["dynamicEyeScale"]!.ToJsonString() == dynamicJson && pc["futurePc"]!.ToJsonString() == unknownPcJson && pc["renderScale"]!.GetValue<double>() == .875 && doc["futureRoot"]!["keep"]!.GetValue<int>() == 42, "hidden objects/unknown PC/root values preserved");
            Assert(!pc.AsObject().ContainsKey(preset.Key) && !pc.AsObject().ContainsKey(scope.Key), "virtual fields are not written as runtime keys");
            foreach (var p in unrelated) Assert(p.Value.SequenceEqual(File.ReadAllBytes(Path.Combine(root, "vrmod", "config", p.Key))), "PC edit changed unrelated " + p.Key);
        }
        var custom = Fields.Clone(store.Documents);
        custom[file]["pcOutput"]!["width"] = 1600; custom[file]["pcOutput"]!["height"] = 900;
        SettingsStore.Validate(custom);
        Assert(Fields.Get(custom, preset)!.GetValue<string>() == "1600x900" && Fields.ChoicesFor(custom, preset).Last() == "1600x900", "manual non-preset value displayed and preserved");
        var decimalSize = SettingsStore.Parse(Encoding.UTF8.GetBytes("{\"pcOutput\":{\"mode\":\"FIXED\",\"width\":1.28e3,\"height\":720.0}}"), file);
        custom[file] = decimalSize; SettingsStore.Validate(custom);
        Assert(Fields.Get(custom, preset)!.GetValue<string>() == "1280x720", "integer-valued exponent/decimal dimensions match runtime");
        string profile = Path.Combine(root, "performance-profile.json");
        SettingsStore.Export(profile, store.Documents);
        Assert(JsonNode.DeepEquals(SettingsStore.Import(profile)[file], store.Documents[file]), "whole performance profile roundtrip");
        var old = JsonNode.Parse(File.ReadAllBytes(profile))!.AsObject(); old["configurations"]!.AsObject().Remove(file);
        File.WriteAllBytes(profile, SettingsStore.Encode(old));
        Assert(JsonNode.DeepEquals(SettingsStore.Import(profile, store.Documents)[file], store.Documents[file]), "old profile preserves current entire performance object");
        Assert(Fields.Get(SettingsStore.Import(profile), preset)!.GetValue<string>() == "NATIVE", "old profile without current uses NATIVE");
        Reject(() => SettingsStore.Export(path, store.Documents), "performance export cannot overwrite live settings");
        byte[] good = File.ReadAllBytes(path);
        foreach (string bad in new[] { "{", "[]", "{\"schemaVersion\":2}", "{\"pcOutput\":null}", "{\"pcOutput\":[]}", "{\"pcOutput\":{\"mode\":\"bad\"}}", "{\"pcOutput\":{\"scope\":null}}", "{\"pcOutput\":{\"scope\":\"bad\"}}", "{\"pcOutput\":{\"width\":0}}", "{\"pcOutput\":{\"height\":16385}}", "{\"pcOutput\":{\"width\":12.5}}", "{\"pcOutput\":{\"width\":\"1280\"}}", "{\"pcOutput\":{\"mode\":\"NATIVE\",\"mode\":\"FIXED\"}}" })
        {
            byte[] bytes = Encoding.UTF8.GetBytes(bad); File.WriteAllBytes(path, bytes);
            Reject(() => new SettingsStore(root, true).Load(), "invalid performance rejected " + bad);
            Assert(bytes.SequenceEqual(File.ReadAllBytes(path)), "invalid performance bytes preserved");
        }
        File.WriteAllBytes(path, good); store.Load();
        var padded = Fields.Clone(store.Documents); padded[file]["futureLargeObject"] = new string('p', 5000);
        store.Save(padded); Assert(new FileInfo(path).Length > 4096 && new FileInfo(path).Length < 16384, "native-compatible performance file limit16KiB");
        SettingsStore.Export(profile, store.Documents);
        Assert(JsonNode.DeepEquals(SettingsStore.Import(profile)[file], store.Documents[file]), "large unknown payload profile preserved");
        Reject(() => SettingsStore.Parse(new byte[16385], file), "performance size beyond runtime bound");
        byte[] external = SettingsStore.Encode(new JsonObject { ["schemaVersion"] = 1, ["externalOwner"] = true });
        File.WriteAllBytes(path, external);
        Reject(() => store.Save(Fields.Clone(store.Documents)), "external performance edit blocks entire save");
        Assert(external.SequenceEqual(File.ReadAllBytes(path)), "external performance bytes preserved");
        File.WriteAllBytes(path, good); store.Load();
        string language = Texts.Language;
        foreach (string locale in Texts.Languages)
        {
            Texts.Language = locale;
            Assert(Texts.Get(preset.Key) != Texts.Get("unavailable") && Texts.Get(scope.Key) != Texts.Get("unavailable"), "PC labels packaged " + locale);
            Assert(Texts.Get("pcOutputHelp").Contains("720p") && Texts.Get("pcOutputScopeHelp").Contains("GAME_RESTART"), "localized720p/restart help " + locale);
            Assert(Fields.ChoiceText(preset, "NATIVE") == Texts.Get("pcOutputNative") && Fields.ChoiceText(preset, "1280x720") == "1280×720", "localized preset mapping " + locale);
        }
        Texts.Language = language;
        foreach (var f in Fields.All.Where(f => f.File == file)) Fields.Set(store.Documents, f, Fields.JsonSerializerNode(f.Default));
        Assert(store.Documents[file]["dynamicEyeScale"]!.ToJsonString() == dynamicJson && store.Documents[file]["pcOutput"]!["renderScale"]!.GetValue<double>() == .875, "reset known PC fields retains deferred/unsupported settings");
        store.Save(original);
    }
}
