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
        new("stereo_depth.json", null, "stereoDepthScale", "number", 1d, "depthHelp"),
        new("navigation.json", "navigation", "locomotionEnabled", "bool", true, ""),
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

    internal static Dictionary<string, JsonObject> Defaults()
    {
        var docs = Files.ToDictionary(n => n, _ => new JsonObject { ["schemaVersion"] = 1 });
        foreach (var f in All) Set(docs, f, JsonSerializerNode(f.Default));
        return docs;
    }

    internal static JsonNode JsonSerializerNode(object value) => System.Text.Json.JsonSerializer.SerializeToNode(value)!;

    internal static JsonNode? Get(Dictionary<string, JsonObject> docs, Field f)
        => (f.Group == null ? docs[f.File] : docs[f.File][f.Group] as JsonObject)?[f.Key];

    internal static void Set(Dictionary<string, JsonObject> docs, Field f, JsonNode value)
    {
        var doc = docs[f.File];
        if (f.Group != null)
        {
            if (doc[f.Group] == null) doc[f.Group] = new JsonObject();
            doc = doc[f.Group]!.AsObject();
        }
        doc[f.Key] = value;
    }

    internal static Dictionary<string, JsonObject> Clone(Dictionary<string, JsonObject> docs)
        => docs.ToDictionary(k => k.Key, k => (JsonObject)k.Value.DeepClone());
}
