using System.Text.Json.Nodes;

namespace GF2VR.Configurator;

internal record Field(string File, string? Group, string Key, string Kind, object Default,
    string Help, string[]? Choices = null);

internal static class Fields
{
    // Only currently deployed runtime contracts. Unimplemented features have no writable field.
    internal static readonly Field[] All =
    [
        new("render_quality.json", null, "eyeRenderScale", "number", 1d, "renderHelp"),
        new("performance.json", "pcOutput", "pcOutputPreset", "choice", "NATIVE", "pcOutputHelp", ["NATIVE", "3200x1800", "2560x1440", "1920x1080", "1280x720"]),
        new("performance.json", "pcOutput", "pcOutputImmersiveOnly", "bool", true, "pcOutputScopeHelp"),
        new("performance.json", "pose", "lateRelocate", "bool", true, "lateRelocateHelp"),
        new("stereo_depth.json", null, "stereoDepthScale", "number", 1d, "depthHelp"),
        new("world_scale.json", null, "worldScale", "number", 1d, "worldScaleHelp"),
        new("world_scale.json", null, "linkStereoDepthToWorldScale", "bool", true, "depthLinkHelp"),
        new("world_scale.json", null, "linkMovementSpeedToWorldScale", "bool", true, "speedLinkHelp"),
        new("activity_ui.json", null, "activityPrimaryUiEnabled", "bool", true, "activityUiHelp"),
        new("controller_input.json", "controllerInput", "swapFaceButtons", "bool", false, "faceSwapHelp"),
        new("controller_input.json", "controllerInput", "swapPanelPointerHands", "bool", false, "handRolesHelp"),
        new("controller_input.json", "diagnostics", "loggingEnabled", "bool", false, "loggingHelp"),
        new("controller_input.json", "auxiliaryPanel", "panelViewerFacing", "bool", true, "panelFacingHelp"),
        new("controller_input.json", "auxiliaryPanel", "panelApplyAnglesWhenFacing", "bool", false, "panelAnglePolicyHelp"),
        new("controller_input.json", "auxiliaryPanel", "panelOffsetX", "number", 0d, "panelOffsetHelp"),
        new("controller_input.json", "auxiliaryPanel", "panelOffsetY", "number", -.1d, ""),
        new("controller_input.json", "auxiliaryPanel", "panelOffsetZ", "number", .01d, ""),
        new("controller_input.json", "auxiliaryPanel", "panelScreenTilt", "number", 45d, "panelTiltHelp"),
        new("controller_input.json", "auxiliaryPanel", "panelRotationPitch", "number", -67d, "panelRotationHelp"),
        new("controller_input.json", "auxiliaryPanel", "panelRotationYaw", "number", 3d, ""),
        new("controller_input.json", "auxiliaryPanel", "panelRotationRoll", "number", 0d, ""),
        new("controller_input.json", "auxiliaryPanel", "panelMaximumWidth", "number", .66d, "panelSizeHelp"),
        new("controller_input.json", "auxiliaryPanel", "panelMaximumHeight", "number", .66d, ""),
        new("controller_hands.json", null, "controllerHandsEnabled", "bool", true, "controllerHandsHelp"),
        new("controller_hands.json", null, "contactHapticStrengthPercent", "number", 15d, "contactHapticStrengthHelp"),
        new("controller_hands.json", null, "contactHapticDurationMs", "number", 30d, "contactHapticDurationHelp"),
#if GF2VR_DIRTY_CONTACT
        new("touch_interaction.json", null, "breastJiggleStrengthPercent", "number", 40d, "breastJiggleStrengthHelp"),
        new("touch_interaction.json", null, "breastJiggleDurationMultiplier", "number", .4d, "breastJiggleDurationHelp"),
        new("touch_interaction.json", null, "breastJiggleRangePercent", "number", 75d, "breastJiggleRangeHelp"),
        new("touch_interaction.json", null, "breastJiggleTiltPercent", "number", 50d, "breastJiggleTiltHelp"),
        new("touch_interaction.json", null, "breastJiggleStretchPercent", "number", 45d, "breastJiggleStretchHelp"),
        new("touch_interaction.json", null, "buttJiggleStrengthPercent", "number", 120d, "buttJiggleStrengthHelp"),
        new("touch_interaction.json", null, "buttJiggleDurationMultiplier", "number", .5d, "buttJiggleDurationHelp"),
        new("touch_interaction.json", null, "buttJiggleRangePercent", "number", 120d, "buttJiggleRangeHelp"),
        new("touch_interaction.json", null, "buttJiggleTiltPercent", "number", 120d, "buttJiggleTiltHelp"),
        new("touch_interaction.json", null, "buttJiggleStretchPercent", "number", 140d, "buttJiggleStretchHelp"),
#endif
        new("navigation.json", "navigation", "locomotionEnabled", "bool", true, ""),
        new("navigation.json", "navigation", "cameraFollowEnabled", "bool", true, "cameraFollowHelp"),
        new("navigation.json", "navigation", "locomotionHand", "choice", "left", "handHelp", ["left", "right"]),
        new("navigation.json", "navigation", "turnEnabled", "bool", true, ""),
        new("navigation.json", "navigation", "turnMode", "choice", "snap", "modeHelp", ["snap", "smooth"]),
        new("navigation.json", "navigation", "movementSpeed", "number", 1d, "speedHelp"),
        new("navigation.json", "navigation", "locomotionScale", "number", 1d, "multiplierHelp"),
        new("navigation.json", "navigation", "snapAngleDegrees", "number", 30d, "snapHelp"),
        new("navigation.json", "navigation", "smoothTurnDegreesPerSecond", "number", 60d, "smoothHelp"),
        new("navigation.json", "navigation", "stickDeadzone", "number", .2d, "deadzoneHelp"),
        new("world_drag.json", "worldDrag", "enabled", "bool", false, "dragHelp"),
        new("world_drag.json", "worldDrag", "leftGripActivation", "bool", false, ""),
        new("world_drag.json", "worldDrag", "leftTriggerActivation", "bool", false, ""),
        new("world_drag.json", "worldDrag", "rightGripActivation", "bool", true, ""),
        new("world_drag.json", "worldDrag", "rightTriggerActivation", "bool", false, ""),
        new("world_drag.json", "worldDrag", "trackLeftHand", "bool", false, ""),
        new("world_drag.json", "worldDrag", "trackRightHand", "bool", true, "")
    ];

    internal static string[] Files => All.Select(f => f.File).Distinct().ToArray();
    // Configurator-owned registration; Installer.NativeLayout has a separate
    // protected list owned by MS-IN-540 and must not change in this task.
    internal static string[] StableConfigFiles => Files.Where(f => f != "touch_interaction.json").ToArray();

    internal static Dictionary<string, JsonObject> Defaults()
    {
        var docs = Files.ToDictionary(n => n, _ => new JsonObject { ["schemaVersion"] = 1 });
        foreach (var f in All) Set(docs, f, JsonSerializerNode(f.Default));
        return docs;
    }

    internal static JsonNode JsonSerializerNode(object value) => System.Text.Json.JsonSerializer.SerializeToNode(value)!;

    internal static JsonNode? Get(Dictionary<string, JsonObject> docs, Field f)
    {
        var owner = f.Group == null ? docs[f.File] : docs[f.File][f.Group] as JsonObject;
        // pose.lateRelocate is a plain runtime bool: a missing key is the native default (ON).
        if (f.File == "performance.json" && f.Group == "pcOutput")
        {
            if (f.Key == "pcOutputImmersiveOnly") return JsonValue.Create((owner?["scope"]?.GetValue<string>() ?? "IMMERSIVE_ONLY") == "IMMERSIVE_ONLY");
            string mode = owner?["mode"]?.GetValue<string>() ?? "NATIVE";
            return JsonValue.Create(mode == "NATIVE" ? "NATIVE" : $"{(int)SettingsStore.Number(owner?["width"]?.ToJsonString() ?? "2560", f.Key)}x{(int)SettingsStore.Number(owner?["height"]?.ToJsonString() ?? "1440", f.Key)}");
        }
#if GF2VR_DIRTY_CONTACT
        // A present regional null is invalid, never legacy fallback. Reading
        // an old common value must not rewrite settings merely by loading.
        if (f.File == "touch_interaction.json" && owner != null && !owner.ContainsKey(f.Key))
        {
            string common = CommonJiggleKey(f.Key);
            if (owner.ContainsKey(common)) return owner[common];
        }
#endif
        return owner?[JsonKey(f)];
    }

    internal static string CommonJiggleKey(string key)
    {
        string suffix = key.StartsWith("breastJiggle", StringComparison.Ordinal) ? key[6..] :
            key.StartsWith("buttJiggle", StringComparison.Ordinal) ? key[4..] : key;
        return char.ToLowerInvariant(suffix[0]) + suffix[1..];
    }

    internal static string JsonKey(Field f) => f.Key == "controllerHandsEnabled" ? "enabled" :
        f.Group == "auxiliaryPanel" ? char.ToLowerInvariant(f.Key[5]) + f.Key[6..] : f.Key;

    internal static bool IsContactHaptic(string key) => key is "contactHapticStrengthPercent" or "contactHapticDurationMs";

    internal static bool IsPanelNumber(string key) => All.Any(f => f.Group == "auxiliaryPanel" && f.Kind == "number" && f.Key == key);

    internal static string[] ChoicesFor(Dictionary<string, JsonObject> docs, Field f)
    {
        if (f.Key != "pcOutputPreset") return f.Choices!;
        string current = Get(docs, f)!.GetValue<string>();
        string[] choices = f.Choices!;
        // A valid manually edited size remains visible/preserved; it is not an
        // editable arbitrary-size input or an extra built-in preset.
        return choices.Contains(current) ? choices : [.. choices, current];
    }

    internal static string ChoiceText(Field f, string value) => f.Key == "pcOutputPreset"
        ? value == "NATIVE" ? Texts.Get("pcOutputNative") : f.Choices!.Contains(value) ? value.Replace("x", "×", StringComparison.Ordinal) : Texts.Format("pcOutputCustom", value.Replace("x", "×", StringComparison.Ordinal))
        : Texts.Get(value);

    internal static void Set(Dictionary<string, JsonObject> docs, Field f, JsonNode value)
    {
        var doc = docs[f.File];
        if (f.Group != null)
        {
            if (doc[f.Group] == null) doc[f.Group] = new JsonObject();
            doc = doc[f.Group]!.AsObject();
        }
        if (f.File == "performance.json" && f.Group == "pcOutput")
        {
            if (f.Key == "pcOutputImmersiveOnly") doc["scope"] = value.GetValue<bool>() ? "IMMERSIVE_ONLY" : "ALWAYS";
            else
            {
                string preset = value.GetValue<string>();
                if (preset == "NATIVE") doc["mode"] = "NATIVE";
                else
                {
                    var size = preset.Split('x');
                    if (size.Length != 2 || !int.TryParse(size[0], System.Globalization.NumberStyles.None, System.Globalization.CultureInfo.InvariantCulture, out int width) ||
                        !int.TryParse(size[1], System.Globalization.NumberStyles.None, System.Globalization.CultureInfo.InvariantCulture, out int height) ||
                        width is < 1 or > 16384 || height is < 1 or > 16384) throw new SettingsError("formatError", f.File);
                    doc["mode"] = "FIXED"; doc["width"] = width; doc["height"] = height;
                }
                if (!doc.ContainsKey("width")) doc["width"] = 2560;
                if (!doc.ContainsKey("height")) doc["height"] = 1440;
            }
            return;
        }
        doc[JsonKey(f)] = value;
    }

    internal static Dictionary<string, JsonObject> Clone(Dictionary<string, JsonObject> docs)
        => docs.ToDictionary(k => k.Key, k => (JsonObject)k.Value.DeepClone());
}
