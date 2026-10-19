using System.Text.Json.Nodes;
namespace GF2VR.Configurator;
internal static partial class SelfTest
{
    private static void LoggingChecks(SettingsStore store, string root)
    {
        var original=Fields.Clone(store.Documents);
        var f=Fields.All.Single(f=>f.Key=="loggingEnabled");
        Assert(!(bool)f.Default,"logging factory OFF");
        Assert(!Fields.Get(Fields.Defaults(),f)!.GetValue<bool>(),"logging default JSON OFF");
        foreach(bool enabled in new[]{true,false})
        {
            var requested=Fields.Clone(original);Fields.Set(requested,f,JsonValue.Create(enabled)!);
            requested[f.File]["futureLoggingMetadata"]=new JsonObject{["retain"]=42};
            store.Save(requested);var reloaded=new SettingsStore(root,true);reloaded.Load();
            Assert(Fields.Get(reloaded.Documents,f)!.GetValue<bool>()==enabled,"logging persisted and reloaded");
            Assert(reloaded.Documents[f.File]["futureLoggingMetadata"]!["retain"]!.GetValue<int>()==42,"unrelated metadata preserved");
            var profile=Path.Combine(root,"logging-profile.json");SettingsStore.Export(profile,requested);
            Assert(Fields.Get(SettingsStore.Import(profile),f)!.GetValue<bool>()==enabled,"logging profile round trip");
            var legacy=Fields.Clone(requested);legacy[f.File].Remove("diagnostics");SettingsStore.Export(profile,legacy);
            Assert(Fields.Get(SettingsStore.Import(profile,requested),f)!.GetValue<bool>()==enabled,"legacy profile preserves current logging");
            Assert(!Fields.Get(SettingsStore.Import(profile),f)!.GetValue<bool>(),"legacy without current defaults OFF");
        }
        foreach(JsonNode? value in new JsonNode?[]{null,JsonValue.Create(1),JsonValue.Create("true"),new JsonObject()})
        {
            var bad=Fields.Clone(original);bad[f.File]["diagnostics"]=new JsonObject{[f.Key]=value};
            Reject(()=>SettingsStore.Validate(bad),"invalid logging boolean");
            if(value is not JsonObject)
            {
                bad[f.File]["diagnostics"]=value?.DeepClone();
                Reject(()=>SettingsStore.Validate(bad),"invalid diagnostics object");
            }
        }
        var missing=Fields.Clone(original);missing[f.File].Remove("diagnostics");SettingsStore.Validate(missing);
        store.Save(original);
    }
}
