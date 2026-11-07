// Nonshipping C5 driver: only the disposable Steam target and explicitly hashed helper.
using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Security.Cryptography;
using System.Text.Json;
using System.Threading;
using System.Windows.Forms;
internal static class StartupHook
{
    private const BindingFlags Flags=BindingFlags.Public|BindingFlags.NonPublic|BindingFlags.Instance|BindingFlags.Static;
    private const string Steam=@"D:\Games\SteamLibrary\steamapps\common\GIRLS' FRONTLINE 2 EXILIUM";
    private static string output="";
    public static void Initialize()
    {
        string? config=Environment.GetEnvironmentVariable("GF2VR_RELEASE122_REPAIR_FIXTURE"); if(config==null)return;
        try
        {
            using var d=JsonDocument.Parse(File.ReadAllBytes(config));var c=d.RootElement;
            Require(c.GetProperty("root").GetString()==Steam,"Disposable target only");
            output=c.GetProperty("evidence").GetString()!;
            Require(new[]{"T3","T4"}.Any(s=>Path.GetFullPath(output).StartsWith(Steam+@"\vrmod\build\RELEASE122\"+s+@"\",StringComparison.OrdinalIgnoreCase)),"Evidence boundary");
            Require(File.ReadAllText(c.GetProperty("markerFile").GetString()!)==c.GetProperty("marker").GetString(),"Explicit marker");
            string dll=Path.ChangeExtension(Environment.ProcessPath!,".dll");
            Require(Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(dll)))==c.GetProperty("installerSha256").GetString(),"Exact helper");
            Require(!Process.GetProcessesByName("GF2_Exilium").Any(),"Game stopped");
            string package=c.GetProperty("package").GetString()!;Directory.CreateDirectory(output);
            bool inspectOnly=c.TryGetProperty("inspectOnly",out var inspect)&&inspect.GetBoolean();
            Exception? failure=null;var thread=new Thread(()=>{try{Run(Assembly.LoadFrom(dll),package,inspectOnly);}catch(Exception e){failure=e;}});
            thread.SetApartmentState(ApartmentState.STA);thread.Start();thread.Join();if(failure!=null)throw failure;Environment.Exit(0);
        }
        catch(Exception e){if(output.Length>0)File.WriteAllText(Path.Combine(output,"error.txt"),e.ToString());Environment.Exit(91);}
    }
    private static void Require(bool ok,string why){if(!ok)throw new InvalidDataException(why);}
    private static void Capture(Form f,string file){using var b=new Bitmap(f.Width,f.Height);f.DrawToBitmap(b,new Rectangle(0,0,b.Width,b.Height));b.Save(Path.Combine(output,file));}
    private static void Run(Assembly a,string package,bool inspectOnly)
    {
        Application.SetHighDpiMode(HighDpiMode.SystemAware);Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);
        var t=a.GetType("GF2VR.Installer.InstallerForm",true)!;var text=a.GetType("GF2VR.Installer.InstallerText",true)!;
        string inferred=(string)a.GetType("GF2VR.Installer.Program",true)!.GetMethod("FindRoot",Flags)!.Invoke(null,null)!;
        Require(inferred==Steam,"Executable ancestor auto-root");
        if(inspectOnly)Require(Path.TrimEndingDirectorySeparator(AppContext.BaseDirectory)==Path.TrimEndingDirectorySeparator(package),"Adjacent package");
        using var f=(Form)Activator.CreateInstance(t,Flags,null,new object?[]{inferred,inspectOnly?null:package,null,false,false},null)!;
        f.StartPosition=FormStartPosition.Manual;f.Location=new Point(-20000,-20000);f.ShowInTaskbar=false;
        var engine=t.GetField("engine",Flags)!.GetValue(f)!;var et=engine.GetType();
        object? Call(string name,params object?[] args)=>et.GetMethod(name,Flags)!.Invoke(engine,args);
        var before=Call("Inspect",Steam)!;string oldId=(string)before.GetType().GetProperty("TransactionId")!.GetValue(before)!;
        int confirms=0;bool clicked=false,done=false;Exception? failure=null;
        using var timer=new System.Windows.Forms.Timer{Interval=120};var watch=Stopwatch.StartNew();
        timer.Tick+=(_,_)=>
        {
            try
            {
                if(watch.ElapsedMilliseconds>60000)throw new TimeoutException("Repair GUI");
                var dialog=Application.OpenForms.Cast<Form>().FirstOrDefault(w=>w.GetType().Name=="RepairDialog");
                if(dialog!=null)
                {
                    Require(confirms==0,"Single confirmation");confirms++;Capture(dialog,"repair-plan.png");
                    var confirm=Children(dialog).OfType<Button>().Single(b=>b.DialogResult==DialogResult.Yes);
                    Require(confirm.Enabled,"Owned file needs no foreign inclusion");confirm.PerformClick();return;
                }
                if(!clicked)
                {
                    foreach(string lang in new[]{"ko","en","ja","zh-CN"})
                    {text.GetField("Language",Flags)!.SetValue(null,lang);t.GetMethod("Translate",Flags)!.Invoke(f,null);f.PerformLayout();Capture(f,"before-"+lang+".png");}
                    if(inspectOnly)
                    {
                        Require(t.GetField("plan",Flags)!.GetValue(f)!=null,"Automatic adjacent preview");
                        t.GetMethod("VerifyPathRows",Flags)!.Invoke(f,null);
                        File.WriteAllText(Path.Combine(output,"result.json"),JsonSerializer.Serialize(new{passed=true,actualSteam=true,inferredRoot=inferred,adjacentPackage=true,gameLaunches=0,installations=0}));
                        done=true;timer.Stop();f.Close();return;
                    }
                    text.GetField("Language",Flags)!.SetValue(null,"ko");t.GetMethod("Translate",Flags)!.Invoke(f,null);
                    var button=(Button)t.GetField("repair",Flags)!.GetValue(f)!;Require(button.Visible&&button.Enabled,"Visible repair action");clicked=true;button.PerformClick();return;
                }
                if((bool)t.GetField("busy",Flags)!.GetValue(f)!)return;
                string status=(string)t.GetField("statusKey",Flags)!.GetValue(f)!;
                Require(status=="repaired","Completed repair status: "+status);Capture(f,"completed.png");
                var after=Call("Inspect",Steam)!;string id=(string)after.GetType().GetProperty("TransactionId")!.GetValue(after)!;
                Require(id!=oldId&&confirms==1,"Genuine new transaction and one consent");
                File.WriteAllText(Path.Combine(output,"result.json"),JsonSerializer.Serialize(new{passed=true,confirms,priorTransaction=oldId,transaction=id,actualSteam=true,gameLaunches=0,receiptFabrications=0},new JsonSerializerOptions{WriteIndented=true}));
                done=true;timer.Stop();f.Close();
            }
            catch(Exception e){failure=e;timer.Stop();f.Close();}
        };
        timer.Start();Application.Run(f);if(failure!=null)throw failure;Require(done,"Completed desktop path");
    }
    private static System.Collections.Generic.IEnumerable<Control> Children(Control p)
    {foreach(Control c in p.Controls){yield return c;foreach(var x in Children(c))yield return x;}}
}
