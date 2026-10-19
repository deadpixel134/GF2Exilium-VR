#if GF2VR_DIRTY_CONTACT
using System.Text.Json.Nodes;
namespace GF2VR.Configurator;
internal static partial class SelfTest
{
    private static void JiggleChecks(SettingsStore store,string root)
    {
        var fields=Fields.All.Where(f=>f.File=="touch_interaction.json").ToArray();
        Assert(fields.Length==10,"Dirty ten independent controls");
        File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory,"touch-defaults-native.json"),SettingsStore.Encode(Fields.Defaults()["touch_interaction.json"]));
        foreach(var language in Texts.Languages){Texts.Language=language;foreach(var f in fields)Assert(Texts.Get(f.Key)!=Texts.Get("unavailable")&&Texts.Get(f.Help)!=Texts.Get("unavailable"),"Dirty packaged locale");}
        Texts.Language="ko";
        var original=Fields.Clone(store.Documents);
        var expected = new Dictionary<string,double> { ["jiggleStrengthPercent"]=50,["jiggleDurationMultiplier"]=.4,["jiggleRangePercent"]=80,["jiggleTiltPercent"]=50,["jiggleStretchPercent"]=50 };
                var selectedBreast=new double[]{40,.4,75,50,45};var selectedButt=new double[]{120,.5,120,120,140};
        var suffixes=new[]{"StrengthPercent","DurationMultiplier","RangePercent","TiltPercent","StretchPercent"};
        for(int i=0;i<5;i++)foreach(string region in new[]{"breast","butt"}){
            var f=fields.Single(x=>x.Key==region+"Jiggle"+suffixes[i]);
            Assert((double)f.Default==(region=="breast"?selectedBreast[i]:selectedButt[i]),"captured perregion default");
            foreach(string language in Texts.Languages){Texts.Language=language;Assert(Texts.Get(f.Help).Contains(((double)f.Default).ToString(System.Globalization.CultureInfo.InvariantCulture)),"localized perregion default help");}
        }
        Texts.Language="ko";
        var legacy=Fields.Clone(original);
        legacy["touch_interaction.json"]=new JsonObject { ["schemaVersion"]=1,["enabled"]=true,["futureRegionSetting"]=42 };
        foreach(var p in expected)legacy["touch_interaction.json"][p.Key]=p.Value;
        SettingsStore.Validate(legacy);
        foreach(var f in fields)Assert(Fields.Get(legacy,f)!.GetValue<double>()==expected[Fields.CommonJiggleKey(f.Key)],"legacy common displayed in both regions");
        var breast=fields.Single(f=>f.Key=="breastJiggleStrengthPercent");var butt=fields.Single(f=>f.Key=="buttJiggleStrengthPercent");
        Fields.Set(legacy,breast,JsonValue.Create(175d)!);
        Assert(Fields.Get(legacy,breast)!.GetValue<double>()==175&&Fields.Get(legacy,butt)!.GetValue<double>()==50,"breast edit leaves butt independent");
        store.Save(legacy);store.Load();
        Assert(store.Documents[breast.File]["jiggleStrengthPercent"]!.GetValue<double>()==50&&store.Documents[breast.File]["futureRegionSetting"]!.GetValue<int>()==42,"regional save retains common and unknown values");
        string regionProfile=Path.Combine(root,"region-jiggle-profile.json");SettingsStore.Export(regionProfile,store.Documents);
        var roundTrip=SettingsStore.Import(regionProfile);
        Assert(Fields.Get(roundTrip,breast)!.GetValue<double>()==175&&Fields.Get(roundTrip,butt)!.GetValue<double>()==50,"regional profile independence");
        foreach(var f in fields){
            foreach(double value in Fields.CommonJiggleKey(f.Key)=="jiggleDurationMultiplier"?new[]{.01,10d}:new[]{0d,1000d}){
                var outside=Fields.Clone(original);Fields.Set(outside,f,JsonValue.Create(value)!);store.Save(outside);store.Load();
                Assert(Fields.Get(store.Documents,f)!.GetValue<double>()==value,"outside old guidance saved without clamp");
                string outsidePath=Path.Combine(root,"unrestricted-contact-profile.json");SettingsStore.Export(outsidePath,store.Documents);
                Assert(Fields.Get(SettingsStore.Import(outsidePath),f)!.GetValue<double>()==value,"outside old guidance profile preserves value");
            }
        }
        var invalidDuration=Fields.Clone(original);invalidDuration["touch_interaction.json"]["breastJiggleDurationMultiplier"]=0;
        Reject(()=>SettingsStore.Validate(invalidDuration),"duration zero division domain");
        store.Save(original);store.Load();
        foreach(var f in fields){
            var docs=Fields.Clone(original);Fields.Set(docs,f,Fields.JsonSerializerNode(Fields.CommonJiggleKey(f.Key)=="jiggleDurationMultiplier"?2.5d:200d));store.Save(docs);store.Load();
            Assert(Fields.Get(store.Documents,f)!.ToJsonString()==Fields.Get(docs,f)!.ToJsonString(),"jiggle atomic save/reload");
            string profile=Path.Combine(root,"jiggle-profile.json");SettingsStore.Export(profile,store.Documents);
            Assert(JsonNode.DeepEquals(SettingsStore.Import(profile)[f.File],store.Documents[f.File]),"jiggle profile roundtrip");
            foreach(var value in new JsonNode?[]{null,JsonValue.Create(-1),JsonValue.Create("bad"),new JsonObject()}){
                var bad=Fields.Clone(original);bad[f.File][f.Key]=value?.DeepClone();Reject(()=>SettingsStore.Validate(bad),"jiggle invalid value rejected");
            }
        }
        store.Save(original);store.Load();
        var old=new JsonObject{["schemaVersion"]=1,["product"]="GF2VR.Configurator",["configurations"]=new JsonObject()};
        foreach(var p in original.Where(p=>p.Key!="touch_interaction.json"))old["configurations"]![p.Key]=p.Value.DeepClone();
        string oldPath=Path.Combine(root,"pre-jiggle-profile.json");File.WriteAllBytes(oldPath,SettingsStore.Encode(old));
        Assert(JsonNode.DeepEquals(SettingsStore.Import(oldPath,store.Documents)["touch_interaction.json"],original["touch_interaction.json"]),"older profile preserves contact settings");
        Assert(SettingsStore.Import(oldPath)["touch_interaction.json"]["breastJiggleStrengthPercent"]!.GetValue<double>()==40,"old profile defaults");
        var full=Fields.Clone(original);
        foreach(var f in Fields.All)
            if(Fields.Get(full,f)==null)Fields.Set(full,f,Fields.JsonSerializerNode(f.Default));
        foreach(var p in full.Where(p=>p.Key!="render_quality.json")){
            p.Value["profileCapacityPadding"]="";
            p.Value["profileCapacityPadding"]=new string('x',4090-SettingsStore.Encode(p.Value).Length);
        }
        string fullPath=Path.Combine(root,"nine-document-full-profile.json");SettingsStore.Export(fullPath,full);
        Assert(new FileInfo(fullPath).Length>32768&&new FileInfo(fullPath).Length<=40960,"nine document profile capacity fixture");
        var imported=SettingsStore.Import(fullPath);
        Assert(full.All(p=>JsonNode.DeepEquals(imported[p.Key],p.Value)),"full nine-document profile imports without dropping extensions");
    }
}
#endif
