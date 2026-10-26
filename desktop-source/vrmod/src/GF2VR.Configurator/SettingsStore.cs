using System.Diagnostics;
using System.Globalization;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;

namespace GF2VR.Configurator;

internal sealed class SettingsError(string key, params object[] arguments) : Exception(key)
{
    internal string Key { get; } = key;
    internal object[] Arguments { get; } = arguments;
}

internal sealed class SettingsStore
{
    internal string Root { get; }
    internal bool ContactHapticsAvailable { get; private set; }
    private string Config => Path.Combine(Root, "vrmod", "config");
    private readonly Dictionary<string, byte[]?> originals = [];
    private readonly bool fixture;
    internal Dictionary<string, JsonObject> Documents { get; private set; } = Fields.Defaults();
    internal bool PcOutputAvailable { get; private set; }
    private static readonly JsonSerializerOptions Json = new() { WriteIndented = true };
    internal static Func<bool> GameRunning = () =>
    {
        var processes = Process.GetProcessesByName("GF2_Exilium");
        try { return processes.Length != 0; }
        finally { foreach (var p in processes) p.Dispose(); }
    };

    internal SettingsStore(string root, bool testFixture = false)
    {
        Root = NormalizeRoot(root);
        fixture = testFixture;
        EnsureRoot();
    }

    private static string NormalizeRoot(string input)
    {
        string full = Path.TrimEndingDirectorySeparator(Path.GetFullPath(input));
        if (File.Exists(full))
        {
            if (!Path.GetFileName(full).Equals("GF2_Exilium.exe", StringComparison.OrdinalIgnoreCase))
                throw new SettingsError("rootError");
            full = Path.GetDirectoryName(full)!;
        }
        // Preserve the established parent/vrmod layout when given its executable directory.
        if (File.Exists(Path.Combine(full, "GF2_Exilium.exe")) &&
            Path.GetFileName(full).Equals("GF2 Game", StringComparison.OrdinalIgnoreCase) &&
            !full.Equals(Path.TrimEndingDirectorySeparator(Path.GetPathRoot(full)!), StringComparison.OrdinalIgnoreCase))
            return Directory.GetParent(full)?.FullName ?? full;
        return full;
    }

    private static string GameExecutable(string root)
    {
        string direct = Path.Combine(root, "GF2_Exilium.exe");
        string standard = Path.Combine(root, "GF2 Game", "GF2_Exilium.exe");
        bool hasDirect = File.Exists(direct), hasStandard = File.Exists(standard);
        if (hasDirect == hasStandard) throw new SettingsError("rootError");
        return hasDirect ? direct : standard;
    }

    internal static string FindRoot(string[]? starts = null)
    {
        foreach (string start in starts ?? new[] { AppContext.BaseDirectory, Environment.CurrentDirectory })
        {
            DirectoryInfo? dir = new(start);
            for (int i = 0; i < 8 && dir != null; i++, dir = dir.Parent)
                if (File.Exists(Path.Combine(dir.FullName, "GF2_Exilium.exe")) ||
                    File.Exists(Path.Combine(dir.FullName, "GF2 Game", "GF2_Exilium.exe")))
                {
                    string root = NormalizeRoot(dir.FullName);
                    _ = GameExecutable(root); return root;
                }
        }
        return Environment.CurrentDirectory;
    }

    private void EnsureRoot()
    {
        string executable = GameExecutable(Root);
        if (!File.Exists(executable))
            throw new SettingsError("rootError");
        CheckPath(Config);
        if (fixture)
        {
            string metadata = Path.Combine(AppContext.BaseDirectory, "runtime-contract.json");
            if (File.Exists(metadata))
            {
                using var known = JsonDocument.Parse(File.ReadAllBytes(metadata));
                ContactHapticsAvailable = known.RootElement.TryGetProperty("buildFlavor", out var flavor) && flavor.GetString() == "Dirty";
                PcOutputAvailable = SupportsPcOutput(known.RootElement);
            }
        }
        if (!fixture)
        {
            // Existing mod embeds absolute paths. A moved/unknown build must never appear configured successfully.
            string manifest = Path.Combine(AppContext.BaseDirectory, "runtime-contract.json");
            using var j = JsonDocument.Parse(File.ReadAllBytes(manifest));
            var c = j.RootElement;
            ContactHapticsAvailable = c.TryGetProperty("buildFlavor", out var flavor) && flavor.GetString() == "Dirty";
            PcOutputAvailable = SupportsPcOutput(c);
            string dll = Path.Combine(Path.GetDirectoryName(executable)!, "version.dll");
            if (c.GetProperty("schemaVersion").GetInt32() != 1 ||
                !Path.GetFullPath(c.GetProperty("configurationRoot").GetString()!).Equals(Config, StringComparison.OrdinalIgnoreCase) ||
                !File.Exists(dll) ||
                !Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(dll))).Equals(c.GetProperty("dllSha256").GetString(), StringComparison.OrdinalIgnoreCase) ||
                !MatchesRuntimeFiles(c))
                throw new SettingsError("runtimeError");
        }
    }

    // Keep the legacy eight-key portable contract usable by deployed updaters.
    // Actual read/write ownership remains Fields.Files, including performance.
    internal static bool MatchesRuntimeFiles(JsonElement contract)
    {
        string[] actual = contract.GetProperty("files").EnumerateArray().Select(e => e.GetString()!).ToArray();
        return new[] { Fields.Files, Fields.Files.Where(f => f != "performance.json").ToArray() }
            .Any(expected => actual.Length == expected.Length && actual.ToHashSet(StringComparer.Ordinal).SetEquals(expected));
    }

    internal static bool SupportsPcOutput(JsonElement contract)
    {
        if (contract.TryGetProperty("pcOutputSupported", out var explicitSupport)) return explicitSupport.GetBoolean();
        if (!contract.TryGetProperty("buildId", out var id) || !contract.TryGetProperty("dllSha256", out var hash)) return false;
        string? build = id.GetString(), digest = hash.GetString();
        // Release profiles are generated from the reviewed paired native build.
        // Do not infer support from a version number or an arbitrary contract.
        return (build == GF2VR.Installer.VariantProfiles.CleanBuild &&
                string.Equals(digest, GF2VR.Installer.VariantProfiles.CleanHash, StringComparison.OrdinalIgnoreCase)) ||
               (build == GF2VR.Installer.VariantProfiles.DirtyBuild &&
                string.Equals(digest, GF2VR.Installer.VariantProfiles.DirtyHash, StringComparison.OrdinalIgnoreCase));
    }

    // Reject reparse points on the exact write path; never enumerate the installation.
    private void CheckPath(string path)
    {
        var full = Path.GetFullPath(path);
        string prefix = Path.EndsInDirectorySeparator(Root) ? Root : Root + Path.DirectorySeparatorChar;
        if (!full.StartsWith(prefix, StringComparison.OrdinalIgnoreCase))
            throw new SettingsError("pathError");
        for (string? part = full; part != null; part = Path.GetDirectoryName(part))
        {
            if ((File.Exists(part) || Directory.Exists(part)) &&
                (File.GetAttributes(part) & FileAttributes.ReparsePoint) != 0)
                throw new SettingsError("pathError");
        }
    }

    internal void Load()
    {
        var next = Fields.Defaults();
        var bytes = new Dictionary<string, byte[]?>();
        foreach (string file in Fields.Files)
        {
            string path = Path.Combine(Config, file); CheckPath(path);
            byte[]? raw = ReadBounded(path);
            bytes[file] = raw;
            if (raw != null) next[file] = Parse(raw, file);
        }
        // No new panel mode is activated merely by saving an unrelated tab.
        if (bytes["controller_input.json"] == null) next["controller_input.json"].Remove("auxiliaryPanel");
        Validate(next);
        Documents = next; originals.Clear();
        foreach (var p in bytes) originals.Add(p.Key, p.Value);
    }

    private static byte[]? ReadBounded(string path)
    {
        if (!File.Exists(path)) return null;
        using var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read);
        if (stream.Length < 1 || stream.Length > FileLimit(Path.GetFileName(path))) throw new SettingsError("sizeError", Path.GetFileName(path));
        byte[] result = new byte[stream.Length]; stream.ReadExactly(result); return result;
    }

    private static int FileLimit(string name) => name == "performance.json" ? 16384 : 4096;

    internal static JsonObject Parse(byte[] bytes, string name)
    {
        if (bytes.Length < 1 || bytes.Length > FileLimit(name)) throw new SettingsError("sizeError", name);
        var span = bytes.AsSpan();
        if (span.StartsWith(new byte[] { 239, 187, 191 })) span = span[3..];
        try
        {
            using var json = JsonDocument.Parse(span.ToArray(), new JsonDocumentOptions { MaxDepth = 16 });
            CheckDuplicates(json.RootElement);
            var doc = JsonNode.Parse(json.RootElement.GetRawText()) as JsonObject
                ?? throw new SettingsError("formatError", name);
            // Deployed Activity UI files predate the GUI and have no schema member.
            // Accept that exact legacy contract without changing the file during load.
            bool legacyOptional = (name is "activity_ui.json" or "controller_input.json" or "performance.json") && !doc.ContainsKey("schemaVersion");
            if (!legacyOptional && (doc["schemaVersion"] is not JsonValue v || !v.TryGetValue<int>(out int version) || version != 1))
                throw new SettingsError("schemaError", name);
            if (name == "render_quality.json" && (doc.Count != 2 || doc["eyeRenderScale"] == null))
                throw new SettingsError("formatError", name); // Native reader rejects unknown keys.
            if (name == "performance.json") ValidatePerformance(doc);
            return doc;
        }
        catch (JsonException) { throw new SettingsError("formatError", name); }
    }

    private static void ValidatePerformance(JsonObject doc)
    {
        // pose.lateRelocate: optional runtime bool (missing = native default ON).
        // A present pose member must be an object and an explicit value must be a bool.
        if (doc.ContainsKey("pose"))
        {
            if (doc["pose"] is not JsonObject pose) throw new SettingsError("formatError", "performance.json");
            if (pose.ContainsKey("lateRelocate") && (pose["lateRelocate"] is not JsonValue late || !late.TryGetValue<bool>(out _)))
                throw new SettingsError("formatError", "performance.json");
        }
        if (!doc.ContainsKey("pcOutput")) return;
        if (doc["pcOutput"] is not JsonObject pc) throw new SettingsError("formatError", "performance.json");
        foreach (string key in new[] { "mode", "scope" })
            if (pc.ContainsKey(key) && (pc[key] is not JsonValue v || !v.TryGetValue<string>(out var s) ||
                !(key == "mode" ? s is "NATIVE" or "FIXED" : s is "IMMERSIVE_ONLY" or "ALWAYS")))
                throw new SettingsError("formatError", "performance.json");
        foreach (string key in new[] { "width", "height" })
            if (pc.ContainsKey(key) && (pc[key] is not JsonValue v || v.GetValueKind() != JsonValueKind.Number ||
                !double.TryParse(v.ToJsonString(), NumberStyles.Float, CultureInfo.InvariantCulture, out var n) ||
                !double.IsFinite(n) || n < 1 || n > 16384 || n != Math.Truncate(n)))
                throw new SettingsError("formatError", "performance.json");
        // renderScale, dynamicEyeScale and future members belong to runtime or
        // deferred features: retain their entire JSON values without editing.
    }

    private static void CheckDuplicates(JsonElement e)
    {
        if (e.ValueKind == JsonValueKind.Object)
        {
            var keys = new HashSet<string>(StringComparer.Ordinal);
            foreach (var p in e.EnumerateObject())
            {
                if (!keys.Add(p.Name)) throw new SettingsError("duplicateError", p.Name);
                CheckDuplicates(p.Value);
            }
        }
        else if (e.ValueKind == JsonValueKind.Array) foreach (var v in e.EnumerateArray()) CheckDuplicates(v);
    }

    internal static double Number(string raw, string key)
    {
        if (Fields.IsPanelNumber(key))
        {
            // Signed positions/angles use native float storage. Reject actual
            // representation loss, never clamp to the reference UI's advice.
            if (!double.TryParse(raw, NumberStyles.Float, CultureInfo.InvariantCulture, out double p) ||
                !double.IsFinite(p) || !float.IsFinite((float)p) || (p != 0 && (float)p == 0))
                throw new SettingsError("panelNumberError", Texts.Get(key));
            if (p == 0 && raw.Split('e', 'E')[0].Any(c => c is >= '1' and <= '9'))
                throw new SettingsError("panelNumberError", Texts.Get(key));
            return p;
        }
        // Native numbers are nonnegative, locale-independent JSON numbers, not NaN/Infinity/-0.
        if (raw.TrimStart().StartsWith('-') || !double.TryParse(raw, NumberStyles.Float,
            CultureInfo.InvariantCulture, out double number) || !double.IsFinite(number) || number < 0)
            throw new SettingsError("numberError", Texts.Get(key));
        if (number == 0 && raw.Any(c => c is >= '1' and <= '9') &&
            raw.Split('e', 'E')[0].Any(c => c is >= '1' and <= '9'))
            throw new SettingsError("numberError", Texts.Get(key)); // Underflow isn't a requested zero.
        if (key == "contactHapticStrengthPercent" && (number > 100 || (number > 0 && (float)(number / 100d) == 0)))
            throw new SettingsError("contactHapticStrengthError");
        if (key == "contactHapticDurationMs")
        {
            double ns = number * 1000000d;
            if (!double.IsFinite(ns) || ns >= 9223372036854775808d || (number > 0 && ns < 1))
                throw new SettingsError("contactHapticDurationError");
        }
        return number;
    }

    internal static void Validate(Dictionary<string, JsonObject> docs)
    {
        var defaults = Fields.Defaults();
        foreach (string name in Fields.Files)
        {
            if (!docs.TryGetValue(name, out var doc)) throw new SettingsError("formatError", name);
            _ = Parse(Encode(doc), name);
            if ((name == "navigation.json" && doc["navigation"] is not JsonObject) ||
                (doc.ContainsKey("worldDrag") && doc["worldDrag"] is not JsonObject) ||
                (name == "controller_input.json" && doc.ContainsKey("controllerInput") && doc["controllerInput"] is not JsonObject) ||
                (name == "controller_input.json" && doc.ContainsKey("auxiliaryPanel") && doc["auxiliaryPanel"] is not JsonObject) ||
                (name == "controller_input.json" && doc.ContainsKey("diagnostics") && doc["diagnostics"] is not JsonObject))
                throw new SettingsError("formatError", name);
        }
        foreach (Field f in Fields.All)
        {
            JsonNode? value = Fields.Get(docs, f);
            if (value == null)
            {
                if ((f.Group is "auxiliaryPanel" or "diagnostics") && (docs[f.File][f.Group] as JsonObject)?.ContainsKey(Fields.JsonKey(f)) == true)
                    throw new SettingsError("formatError", f.File);
                // Runtime optional members use their actual policy defaults, without rewriting the file on load.
                if (f.Key is "eyeRenderScale" or "stereoDepthScale" or "worldScale") throw new SettingsError("formatError", f.File);
                if (f.File == "world_scale.json" && docs[f.File].ContainsKey(f.Key)) throw new SettingsError("formatError", f.File);
                if (Fields.IsContactHaptic(f.Key) && docs[f.File].ContainsKey(Fields.JsonKey(f)))
                    throw new SettingsError("formatError", f.File);
                if (f.Key == "controllerHandsEnabled" && docs[f.File].ContainsKey(Fields.JsonKey(f)))
                    throw new SettingsError("formatError", f.File);
                if (f.Key == "activityPrimaryUiEnabled" && docs[f.File].ContainsKey(f.Key))
                    throw new SettingsError("formatError", f.File); // Explicit null is not the native missing-key default.
                if ((f.Key is "swapFaceButtons" or "swapPanelPointerHands") && (docs[f.File][f.Group!] as JsonObject)?.ContainsKey(f.Key) == true)
                    throw new SettingsError("formatError", f.File);
                if (f.Key == "cameraFollowEnabled" && (docs[f.File][f.Group!] as JsonObject)?.ContainsKey(f.Key) == true)
                    throw new SettingsError("formatError", f.File);
                if (f.Key == "lateRelocate" && (docs[f.File][f.Group!] as JsonObject)?.ContainsKey(f.Key) == true)
                    throw new SettingsError("formatError", f.File); // Explicit null is not the native missing-key default.
                continue;
            }
            if (value is not JsonValue val) throw new SettingsError("formatError", f.File);
            if (f.Kind == "bool" && !val.TryGetValue<bool>(out _)) throw new SettingsError("formatError", f.File);
            if (f.Kind == "choice" && (!val.TryGetValue<string>(out var c) || !Fields.ChoicesFor(docs, f).Contains(c)))
                throw new SettingsError("formatError", f.File);
            if (f.Kind == "number")
            {
                double d = Number(val.ToJsonString(), f.Key);
                if ((f.Key is "panelMaximumWidth" or "panelMaximumHeight") && d <= 0)
                    throw new SettingsError("panelNumberError", Texts.Get(f.Key));
                if ((f.Key is "eyeRenderScale" or "worldScale" && d <= 0) || (f.Key == "stickDeadzone" && d >= 1))
                    throw new SettingsError("numberError", Texts.Get(f.Key)); // Semantic/backend constraints, not advice.
            }
        }
#if GF2VR_DIRTY_CONTACT
        if (!GF2.Private.ContactSettings.Read(Encode(docs["touch_interaction.json"]), out _)) throw new SettingsError("jiggleConfigError");
#endif
        bool Flag(string key) { var f = Fields.All.Single(f => f.Key == key); return (Fields.Get(docs, f) ?? Fields.Get(defaults, f))!.GetValue<bool>(); }
        if (Flag("enabled") && (!(Flag("leftGripActivation") || Flag("leftTriggerActivation") ||
            Flag("rightGripActivation") || Flag("rightTriggerActivation")) || !(Flag("trackLeftHand") || Flag("trackRightHand"))))
            throw new SettingsError("dragError");
        double Num(string key) { var f = Fields.All.Single(f => f.Key == key); return Number((Fields.Get(docs, f) ?? Fields.Get(defaults, f))!.ToJsonString(), key); }
        double world = Num("worldScale"), inverse = 1 / world;
        if (!double.IsFinite(inverse) || inverse <= 0) throw new SettingsError("numberError", Texts.Get("worldScale"));
        double speed = Num("movementSpeed") * Num("locomotionScale") * (Flag("linkMovementSpeedToWorldScale") ? world : 1);
        if (!double.IsFinite(speed) || (Num("movementSpeed") > 0 && Num("locomotionScale") > 0 && speed == 0)) throw new SettingsError("speedError");
    }

    internal static byte[] Encode(JsonObject doc) => Encoding.UTF8.GetBytes(doc.ToJsonString(Json) + "\n");
    private static bool Same(byte[]? a, byte[]? b) => a == null ? b == null : b != null && a.AsSpan().SequenceEqual(b);

    internal void Save(Dictionary<string, JsonObject> requested)
    {
        Validate(requested); EnsureRoot();
        if (GameRunning()) throw new SettingsError("runningError");
        if (originals.Count != Fields.Files.Length) throw new SettingsError("reloadError");
        string mutexName = "Local\\GF2VR.Settings." + Convert.ToHexString(SHA256.HashData(Encoding.UTF8.GetBytes(Root.ToUpperInvariant())));
        using var mutex = new Mutex(false, mutexName);
        bool held; try { held = mutex.WaitOne(0); } catch (AbandonedMutexException) { held = true; }
        if (!held) throw new SettingsError("conflictError");
        var committed = new List<string>();
        var output = requested.ToDictionary(p => p.Key, p => Encode(p.Value));
        var staged = new Dictionary<string, string>();
        string transaction = Guid.NewGuid().ToString("N");
        try
        {
            Directory.CreateDirectory(Config);
            // Preflight ALL files before any replacement. No overwrite after external changes or future schemas.
            foreach (string file in Fields.Files)
            {
                string path = Path.Combine(Config, file); CheckPath(path);
                if (!Same(ReadBounded(path), originals[file])) throw new SettingsError("conflictError");
                if (JsonNode.DeepEquals(requested[file], Documents[file]) && originals[file] != null) continue;
                string temp = Path.Combine(Config, "." + file + "." + transaction + ".tmp");
                WriteNew(temp, output[file]); staged.Add(file, temp);
            }
            if (GameRunning()) throw new SettingsError("runningError");
            foreach (var p in staged)
            {
                string dest = Path.Combine(Config, p.Key);
                if (!Same(ReadBounded(dest), originals[p.Key])) throw new SettingsError("conflictError");
                if (originals[p.Key] != null) File.Replace(p.Value, dest, dest + "." + transaction + ".bak");
                else File.Move(p.Value, dest);
                committed.Add(p.Key);
            }
            Load(); // Publish GUI state only after the entire transaction completes.
        }
        catch
        {
            // Restore only files still equal to OUR transaction. Never erase an external writer's change.
            var unresolved = new List<string>();
            foreach (string file in committed.AsEnumerable().Reverse())
            {
                string dest = Path.Combine(Config, file), backup = dest + "." + transaction + ".bak";
                try
                {
                    if (!Same(ReadBounded(dest), output[file])) { unresolved.Add(backup); continue; }
                    if (originals[file] == null) File.Delete(dest);
                    else if (File.Exists(backup)) File.Replace(backup, dest, null);
                }
                catch { unresolved.Add(backup); }
            }
            if (unresolved.Count != 0) throw new SettingsError("rollbackError", string.Join("\n", unresolved));
            throw;
        }
        finally
        {
            try { foreach (string temp in staged.Values) { try { if (File.Exists(temp)) File.Delete(temp); } catch (IOException) { } } }
            finally { mutex.ReleaseMutex(); }
        }
    }

    internal static void WriteNew(string path, byte[] bytes)
    {
        using var stream = new FileStream(path, FileMode.CreateNew, FileAccess.Write, FileShare.None);
        stream.Write(bytes); stream.Flush(true);
    }

    internal static void Export(string path, Dictionary<string, JsonObject> docs, string? installedRoot = null)
    {
        Validate(docs);
        // Export is a separate profile, never a disguised overwrite of a live runtime settings file.
        if ((installedRoot != null && Path.GetFullPath(path).StartsWith(Path.Combine(Path.GetFullPath(installedRoot), "vrmod", "config") + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase)) ||
            Fields.Files.Contains(Path.GetFileName(path), StringComparer.OrdinalIgnoreCase) ||
            Path.GetFileName(path).Equals("configurator.json", StringComparison.OrdinalIgnoreCase))
            throw new SettingsError("exportError");
        var profile = new JsonObject { ["schemaVersion"] = 1, ["product"] = "GF2VR.Configurator" };
        var config = new JsonObject(); profile["configurations"] = config;
        foreach (var p in docs) config[p.Key] = p.Value.DeepClone();
        AtomicStandalone(path, Encode(profile));
    }

    internal static Dictionary<string, JsonObject> Import(string path, Dictionary<string, JsonObject>? current = null)
    {
        using var stream = File.OpenRead(path);
        if (stream.Length > (Fields.Files.Length - 1) * 4096 + 16384 + 4096) throw new SettingsError("profileError");
        using var doc = JsonDocument.Parse(stream, new JsonDocumentOptions { MaxDepth = 20 }); CheckDuplicates(doc.RootElement);
        var profile = JsonNode.Parse(doc.RootElement.GetRawText())!.AsObject();
        if (profile["schemaVersion"]?.ToJsonString() != "1" || profile["product"]?.GetValue<string>() != "GF2VR.Configurator" ||
            profile["configurations"] is not JsonObject config)
            throw new SettingsError("profileError");
        // Older four/five-document profiles cannot reset a newer UI/face choice.
        var missing = new[] { "activity_ui.json", "controller_input.json", "world_scale.json", "controller_hands.json", "performance.json"
#if GF2VR_DIRTY_CONTACT
            , "touch_interaction.json"
#endif
        }.Where(f => !config.ContainsKey(f)).ToHashSet(StringComparer.Ordinal);
        var expected = Fields.Files.Where(f => !missing.Contains(f)).ToHashSet(StringComparer.Ordinal);
        if (!expected.SetEquals(config.Select(p => p.Key))) throw new SettingsError("profileError");
        var result = new Dictionary<string, JsonObject>();
        foreach (string file in Fields.Files)
            result[file] = missing.Contains(file)
                ? (JsonObject)(current ?? Fields.Defaults())[file].DeepClone()
                : config[file] as JsonObject ?? throw new SettingsError("profileError");
        // Profiles from the face-only GUI share this document but lack the
        // newer role field. Import must not silently reset the current hands.
        Validate(result);
        // Capture presence before filling fields: result shares the imported
        // JSON nodes, so checking afterwards would observe our own defaults.
        bool importedHasPanel = (config["controller_input.json"] as JsonObject)?.ContainsKey("auxiliaryPanel") == true;
        // Pre-panel profiles cannot silently reset a newer panel pose/mode.
        foreach (var field in Fields.All.Where(f => f.Group == "auxiliaryPanel"))
            if (Fields.Get(result, field) == null)
                Fields.Set(result, field, (Fields.Get(current ?? Fields.Defaults(), field) ?? Fields.JsonSerializerNode(field.Default)).DeepClone());
        if (!importedHasPanel)
            if (current != null && !current["controller_input.json"].ContainsKey("auxiliaryPanel"))
                result["controller_input.json"].Remove("auxiliaryPanel");
        foreach (var field in Fields.All.Where(f => Fields.IsContactHaptic(f.Key)))
            if (Fields.Get(result, field) == null)
                Fields.Set(result, field, (Fields.Get(current ?? Fields.Defaults(), field) ?? Fields.JsonSerializerNode(field.Default)).DeepClone());
        var roleField = Fields.All.Single(f => f.Key == "swapPanelPointerHands");
        if (Fields.Get(result, roleField) == null)
            Fields.Set(result, roleField, (Fields.Get(current ?? Fields.Defaults(), roleField) ?? Fields.JsonSerializerNode(false)).DeepClone());
        var followField = Fields.All.Single(f => f.Key == "cameraFollowEnabled");
        var logField = Fields.All.Single(f => f.Key == "loggingEnabled");
        if (Fields.Get(result, logField) == null)
            Fields.Set(result, logField, (Fields.Get(current ?? Fields.Defaults(), logField) ?? Fields.JsonSerializerNode(false)).DeepClone());
        if (Fields.Get(result, followField) == null)
            Fields.Set(result, followField, (Fields.Get(current ?? Fields.Defaults(), followField) ?? Fields.JsonSerializerNode(true)).DeepClone());
        Validate(result); return Fields.Clone(result);
    }

    internal static void AtomicStandalone(string path, byte[] bytes)
    {
        string full = Path.GetFullPath(path), temp = full + "." + Guid.NewGuid().ToString("N") + ".tmp";
        try
        {
            WriteNew(temp, bytes);
            if (File.Exists(full)) File.Replace(temp, full, null); else File.Move(temp, full);
        }
        finally { if (File.Exists(temp)) File.Delete(temp); }
    }

    internal string LoadLanguage()
    {
        string file = Path.Combine(Config, "configurator.json"); CheckPath(file);
        if (!File.Exists(file)) return Texts.Normalize(CultureInfo.CurrentUICulture.TwoLetterISOLanguageName);
        using var doc = JsonDocument.Parse(ReadBounded(file)!); CheckDuplicates(doc.RootElement);
        return doc.RootElement.TryGetProperty("language", out var l) && l.ValueKind == JsonValueKind.String
            ? Texts.Normalize(l.GetString()) : "ko";
    }

    internal void SaveLanguage(string language)
        => SavePreference(doc => doc["language"] = Texts.Normalize(language));

    private JsonObject ReadPreference()
    {
        string file = Path.Combine(Config, "configurator.json"); CheckPath(file);
        byte[]? bytes = ReadBounded(file);
        if (bytes == null) return new JsonObject { ["schemaVersion"] = 1 };
        using var j = JsonDocument.Parse(bytes); CheckDuplicates(j.RootElement);
        var doc = JsonNode.Parse(bytes)?.AsObject() ?? throw new SettingsError("schemaError", "configurator.json");
        if (doc["schemaVersion"]?.ToJsonString() != "1") throw new SettingsError("schemaError", "configurator.json");
        return doc;
    }
    internal bool ShouldNotifyUpdate(string version)
    {
        _ = GF2VR.Installer.UpdatePolicy.VersionOf(version);
        var doc = ReadPreference();
        return doc["lastNotifiedUpdate"]?.GetValue<string>() != version && doc["skippedUpdate"]?.GetValue<string>() != version;
    }
    internal void MarkUpdateNotice(string version, bool skip)
    {
        _ = GF2VR.Installer.UpdatePolicy.VersionOf(version);
        SavePreference(doc => { doc["lastNotifiedUpdate"] = version; if (skip) doc["skippedUpdate"] = version; });
    }
    private void SavePreference(Action<JsonObject> change)
    {
        string file = Path.Combine(Config, "configurator.json"); CheckPath(file);
        Directory.CreateDirectory(Config);
        byte[]? raw = ReadBounded(file);
        JsonObject doc = raw == null ? new() : JsonNode.Parse(raw)!.AsObject();
        if (raw != null) { using var j = JsonDocument.Parse(raw); CheckDuplicates(j.RootElement); }
        if (doc.ContainsKey("schemaVersion") && doc["schemaVersion"]?.ToJsonString() != "1")
            throw new SettingsError("schemaError", "configurator.json");
        doc["schemaVersion"] = 1; change(doc);
        byte[] bytes = Encode(doc);
        if (bytes.Length > 4096) throw new SettingsError("sizeError", "configurator.json");
        if (!Same(raw, ReadBounded(file))) throw new SettingsError("conflictError");
        AtomicStandalone(file, bytes);
    }
}
