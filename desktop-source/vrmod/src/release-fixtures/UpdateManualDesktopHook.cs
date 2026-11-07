// Separate desktop test driver, never included in either product project.
using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

internal static class StartupHook
{
    private const string Steam = @"D:\Games\SteamLibrary\steamapps\common\GIRLS' FRONTLINE 2 EXILIUM";
    private const BindingFlags Flags = BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static;
    private static string output = "";
    private static int requests;
    private static int notices;
    private static int opens;
    private static void Require(bool ok, string why) { if (!ok) throw new InvalidDataException(why); }
    private static void Write(string name, object value) => File.WriteAllText(Path.Combine(output,name),JsonSerializer.Serialize(value,new JsonSerializerOptions{WriteIndented=true}));
    public static void Initialize()
    {
        string? file=Environment.GetEnvironmentVariable("GF2VR_RELEASE122_MANUAL_FIXTURE");if(file==null)return;
        try
        {
            using var doc=JsonDocument.Parse(File.ReadAllBytes(file));var c=doc.RootElement;
            string root=Path.TrimEndingDirectorySeparator(Path.GetFullPath(c.GetProperty("root").GetString()!));
            Require(Path.GetFullPath(root).Equals(Steam,StringComparison.OrdinalIgnoreCase),"Disposable Steam target only");
            output=c.GetProperty("evidence").GetString()!;
            Require(Path.GetFullPath(output).StartsWith(Path.Combine(root,"vrmod","build","RELEASE122")+Path.DirectorySeparatorChar,StringComparison.OrdinalIgnoreCase),"Evidence boundary");
            Directory.CreateDirectory(output);
            Require(File.ReadAllText(c.GetProperty("markerFile").GetString()!)==c.GetProperty("marker").GetString(),"Explicit test marker");
            Require(!Process.GetProcessesByName("GF2_Exilium").Any(),"Game remains stopped");
            string app=Path.Combine(root,"vrmod","bin","GF2VR.Configurator","GF2VR.Configurator.exe");
            Require(Path.GetFullPath(Environment.ProcessPath!).Equals(app,StringComparison.OrdinalIgnoreCase),"Actual installed Configurator");
            string dll=Path.ChangeExtension(app,".dll");
            Require(Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(dll)))==c.GetProperty("guiSha256").GetString(),"Installed GUI bytes");
            bool expectNotice=c.GetProperty("expectNotice").GetBoolean();
            var assembly=Assembly.LoadFrom(dll);
            var http=(HttpClient)assembly.GetType("GF2VR.Installer.GithubOwnerClient",true)!.GetField("Shared",Flags)!.GetValue(null)!;
            typeof(HttpMessageInvoker).GetField("_handler",Flags)!.SetValue(http,new Transport());
            var type=assembly.GetType("GF2VR.Configurator.MainForm",true)!;
            type.GetField("OpenReleasePage",Flags)!.SetValue(null,(Action<string>)(url=>{Require(url=="https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.2.3","Fixed release page");opens++;}));
            Exception? failure=null;
            var thread=new Thread(()=>{try{Run(type,root,expectNotice);}catch(Exception e){failure=e;}});
            thread.SetApartmentState(ApartmentState.STA);thread.Start();thread.Join();
            if(failure!=null)throw failure;
            Environment.Exit(0);
        }
        catch(Exception e){if(output.Length!=0)Write("error.json",new{error=e.ToString()});Environment.Exit(91);}
    }
    private static void Capture(Form form,string name)
    {using var b=new Bitmap(form.Width,form.Height);form.DrawToBitmap(b,new Rectangle(0,0,b.Width,b.Height));b.Save(Path.Combine(output,name));}
    private static void Run(Type type,string root,bool expectNotice)
    {
        Application.SetHighDpiMode(HighDpiMode.SystemAware);Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);
        using var form=(Form)Activator.CreateInstance(type,Flags,null,new object?[]{root,null},null)!;
        form.StartPosition=FormStartPosition.Manual;form.Location=new Point(-20000,-20000);form.ShowInTaskbar=false;
        var watch=Stopwatch.StartNew();using var timer=new System.Windows.Forms.Timer{Interval=100};Exception? failure=null;bool done=false;
        timer.Tick+=(_,_)=>
        {
            try
            {
                if(watch.ElapsedMilliseconds>30000)throw new TimeoutException("Manual notice GUI");
                var notice=Application.OpenForms.Cast<Form>().FirstOrDefault(f=>f.GetType().Name=="UpdateNoticeDialog");
                if(notice!=null)
                {
                    Require(expectNotice&&notices==0,"One notice only for this version");notices++;
                    CheckBox skip=Children(notice).OfType<CheckBox>().Single();skip.Checked=true;
                    Capture(notice,"notice-skip.png");
                    Children(notice).OfType<Button>().Single().PerformClick();return;
                }
                if((string?)type.GetField("updateKey",Flags)!.GetValue(form)!="updateManual")return;
                if(watch.ElapsedMilliseconds<1200)return;
                var status=(Label)type.GetField("updateStatus",Flags)!.GetValue(form)!;
                var apply=(Button)type.GetField("updateButton",Flags)!.GetValue(form)!;
                var page=(Button)type.GetField("releasePage",Flags)!.GetValue(form)!;
                Require(status.Text.Contains("1.2.3",StringComparison.Ordinal)&&page.Visible&&!apply.Visible,"Version/manual/Open retained, no apply");
                Require(notices==(expectNotice?1:0)&&opens==0,"No unsolicited page or repeated notice");
                page.PerformClick();Require(opens==1,"Open only from button");Capture(form,"manual-page.png");
                using var pref=JsonDocument.Parse(File.ReadAllBytes(Path.Combine(root,"vrmod","config","configurator.json")));
                Require(pref.RootElement.GetProperty("lastNotifiedUpdate").GetString()=="1.2.3"&&pref.RootElement.GetProperty("skippedUpdate").GetString()=="1.2.3","Persisted notice/skip");
                Write("result.json",new{passed=true,notices,expectNotice,opens,requests,archiveDownloads=0,version="1.2.3",actualInstalledGui=true,transport="explicit local catalog, not public E2E",gameLaunches=0});
                done=true;timer.Stop();form.Close();
            }
            catch(Exception e){failure=e;timer.Stop();form.Close();}
        };
        timer.Start();Application.Run(form);
        if(failure!=null)throw failure;Require(done,"Desktop check completed");
    }
    private static System.Collections.Generic.IEnumerable<Control> Children(Control parent)
    {foreach(Control c in parent.Controls){yield return c;foreach(var child in Children(c))yield return child;}}
    private sealed class Transport:HttpMessageHandler
    {
        protected override Task<HttpResponseMessage> SendAsync(HttpRequestMessage r,CancellationToken token)
        {
            requests++;Require(r.RequestUri!.AbsoluteUri=="https://api.github.com/repos/deadpixel134/GF2Exilium-VR/releases?per_page=20","No archive/metadata download for unsupported release");
            return Task.FromResult(new HttpResponseMessage(HttpStatusCode.OK){Content=new StringContent("[{\"id\":101,\"tag_name\":\"v1.2.3\",\"draft\":false,\"prerelease\":false,\"published_at\":\"2026-10-25T00:00:00Z\",\"assets\":[]}]",Encoding.UTF8,"application/json"),RequestMessage=r});
        }
    }
}
