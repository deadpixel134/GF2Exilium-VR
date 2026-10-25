using System.Text;
using System.Text.Json.Nodes;

namespace GF2VR.Configurator;

internal sealed partial class MainForm
{
    internal static int VerifySaveState()
    {
        int checks = 0;
        void Check(bool ok, string why) { ++checks; if (!ok) throw new InvalidDataException(why); }
        string fixture = Path.Combine(AppContext.BaseDirectory, "save-state-fixture");
        if (Directory.Exists(fixture)) throw new InvalidDataException("Preserve save-state fixture");
        foreach (string locale in Texts.Languages)
        {
            string root = Path.Combine(fixture, locale), config = Path.Combine(root,"vrmod","config");
            Directory.CreateDirectory(Path.Combine(root,"GF2 Game"));Directory.CreateDirectory(config);
            File.WriteAllText(Path.Combine(root,"GF2 Game","GF2_Exilium.exe"),"INERT SAVE STATE FIXTURE; NEVER EXECUTE");
            string pref=Path.Combine(config,"configurator.json");File.WriteAllText(pref,"{\"schemaVersion\":1,\"language\":\""+locale+"\"}");
            string performance=Path.Combine(config,"performance.json");
            File.WriteAllText(performance,"{\"pcOutput\":{\"mode\":\"NATIVE\",\"scope\":\"IMMERSIVE_ONLY\",\"renderScale\":1.0},\"dynamicEyeScale\":{\"enabled\":false,\"future\":[1,null]},\"unknown\":42}");
            using var form=new MainForm(root,Path.Combine(fixture,"screenshots-"+locale));
            Check(form.store!=null&&!form.dirty&&!form.Text.EndsWith(" *"),"initial loaded title "+locale);
            var preset=(ComboBox)form.inputs["pcOutputPreset"];int index=Array.IndexOf(Fields.ChoicesFor(form.editing,Fields.All.Single(f=>f.Key=="pcOutputPreset")),"1280x720");
            Check(index>=0,"720p preset contract "+locale);preset.SelectedIndex=index;
            Check(form.dirty&&form.Text.EndsWith(" *")&&form.status.Tag as string=="unsaved","edited state "+locale);
            byte[] original=File.ReadAllBytes(performance);
            using(var lease=new FileStream(performance,FileMode.Open,FileAccess.Read,FileShare.None))
            {
                bool refused=false;try{form.SaveSettings();}catch(IOException){refused=true;}
                Check(refused&&form.dirty&&form.Text.EndsWith(" *")&&form.status.Tag as string=="saveFailed"&&form.status.Text==Texts.Get("saveFailed"),"real lock failure preserves dirty state "+locale);
            }
            Check(File.ReadAllBytes(performance).SequenceEqual(original),"locked file preserved "+locale);
            File.AppendAllText(performance,"\n");byte[] external=File.ReadAllBytes(performance);
            bool conflict=false;try{form.SaveSettings();}catch(SettingsError ex)when(ex.Key=="conflictError"){conflict=true;}
            Check(conflict&&form.dirty&&form.status.Tag as string=="saveFailed"&&File.ReadAllBytes(performance).SequenceEqual(external),"external change preserved "+locale);
            File.WriteAllBytes(performance,original);form.SaveSettings();
            Check(!form.dirty&&!form.Text.EndsWith(" *")&&form.status.Tag as string=="saved","verified save clears marker "+locale);
            var after=JsonNode.Parse(File.ReadAllBytes(performance))!;
            Check(after["pcOutput"]!["mode"]!.GetValue<string>()=="FIXED"&&after["pcOutput"]!["width"]!.GetValue<int>()==1280&&after["pcOutput"]!["height"]!.GetValue<int>()==720,"saved preset values "+locale);
            Check(after["dynamicEyeScale"]!["future"]!.AsArray().Count==2&&after["unknown"]!.GetValue<int>()==42&&after["pcOutput"]!["renderScale"]!.GetValue<double>()==1,"opaque values preserved "+locale);
            using(var reopened=new MainForm(root,Path.Combine(fixture,"reopened-"+locale)))
                Check(!reopened.dirty&&!reopened.Text.EndsWith(" *")&&reopened.status.Tag as string=="loaded","reopen persisted state "+locale);
            form.ApplyDefaults();Check(form.dirty&&form.Text.EndsWith(" *")&&form.status.Text.Contains(Texts.Get("unsaved")),"defaults pending title/status "+locale);
            form.SetStatus("exported");Check(form.status.Text.Contains(Texts.Get("unsaved")),"export does not hide unsaved state "+locale);
            form.Translate();Check(form.Text.EndsWith(" *")&&form.status.Text.Contains(Texts.Get("unsaved")),"translation keeps state "+locale);
            ((TextBox)form.inputs["eyeRenderScale"]).Text="invalid";
            bool invalid=false;try{form.SaveSettings();}catch(SettingsError){invalid=true;}
            Check(invalid&&form.dirty&&form.Text.EndsWith(" *")&&form.status.Tag as string=="saveFailed","validation failure explicit "+locale);
        }
        File.WriteAllText(Path.Combine(AppContext.BaseDirectory,"save-state-validation.json"),System.Text.Json.JsonSerializer.Serialize(new{passed=true,checks,locales=Texts.Languages,realIoFailure=true,externalConflict=true,gameLaunches=0}));
        return 0;
    }
}
