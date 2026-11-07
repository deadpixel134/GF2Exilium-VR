// Disposable Steam desktop validation only. Never compiled into either product.
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
internal static class StartupHook
{
    private const string Steam=@"D:\Games\SteamLibrary\steamapps\common\GIRLS' FRONTLINE 2 EXILIUM";
    private const BindingFlags Flags=BindingFlags.Public|BindingFlags.NonPublic|BindingFlags.Instance|BindingFlags.Static;
    private static string output="";private static int requests;
    [DllImport("kernel32.dll")]private static extern uint GetCurrentThreadId();
    private delegate bool EnumProc(IntPtr hwnd,IntPtr value);
    [DllImport("user32.dll")]private static extern bool EnumThreadWindows(uint id,EnumProc callback,IntPtr value);
    [DllImport("user32.dll",CharSet=CharSet.Unicode)]private static extern int GetClassName(IntPtr h,System.Text.StringBuilder b,int max);
    [DllImport("user32.dll")]private static extern IntPtr GetDlgItem(IntPtr h,int id);
    [DllImport("user32.dll")]private static extern bool EnumChildWindows(IntPtr h,EnumProc callback,IntPtr value);
    [DllImport("user32.dll")]private static extern int GetDlgCtrlID(IntPtr h);
    [DllImport("user32.dll",CharSet=CharSet.Unicode)]private static extern int GetWindowText(IntPtr h,System.Text.StringBuilder text,int max);
    [DllImport("user32.dll")]private static extern IntPtr SendMessage(IntPtr h,uint msg,IntPtr w,IntPtr l);
    [DllImport("user32.dll")]private static extern bool PrintWindow(IntPtr h,IntPtr dc,uint flags);
    [StructLayout(LayoutKind.Sequential)]private struct Rect{public int Left,Top,Right,Bottom;}
    [DllImport("user32.dll")]private static extern bool GetWindowRect(IntPtr h,out Rect r);
    public static void Initialize()
    {
        string? file=Environment.GetEnvironmentVariable("GF2VR_RELEASE122_SAVE_FIXTURE");if(file==null)return;
        try
        {
            using var doc=JsonDocument.Parse(File.ReadAllBytes(file));var c=doc.RootElement;
            Require(c.GetProperty("root").GetString()==Steam,"Disposable Steam only");output=c.GetProperty("evidence").GetString()!;
            Require(Path.GetFullPath(output).StartsWith(Steam+@"\vrmod\build\RELEASE122\T5\",StringComparison.OrdinalIgnoreCase),"Evidence boundary");Directory.CreateDirectory(output);
            Require(File.ReadAllText(c.GetProperty("markerFile").GetString()!)==c.GetProperty("marker").GetString(),"Explicit marker");
            string app=Path.Combine(Steam,@"vrmod\bin\GF2VR.Configurator\GF2VR.Configurator.exe");
            Require(Path.GetFullPath(Environment.ProcessPath!)==app&&!Process.GetProcessesByName("GF2_Exilium").Any(),"Actual installed GUI, stopped game");
            string dll=Path.ChangeExtension(app,".dll");Require(Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(dll)))==c.GetProperty("guiSha256").GetString(),"Exact installed GUI");
            var a=Assembly.LoadFrom(dll);var http=(HttpClient)a.GetType("GF2VR.Installer.GithubOwnerClient",true)!.GetField("Shared",Flags)!.GetValue(null)!;
            typeof(HttpMessageInvoker).GetField("_handler",Flags)!.SetValue(http,new EmptyCatalog());
            Exception? failure=null;var thread=new Thread(()=>{try{Run(a);}catch(Exception e){failure=e;}});thread.SetApartmentState(ApartmentState.STA);thread.Start();thread.Join();if(failure!=null)throw failure;Environment.Exit(0);
        }
        catch(Exception e){if(output.Length>0)File.WriteAllText(Path.Combine(output,"error.txt"),e.ToString());Environment.Exit(91);}
    }
    private static void Require(bool ok,string why){if(!ok)throw new InvalidDataException(why);}
    private static IntPtr Dialog()
    {
        IntPtr result=IntPtr.Zero;EnumThreadWindows(GetCurrentThreadId(),(h,_)=>{var b=new System.Text.StringBuilder(64);GetClassName(h,b,64);if(b.ToString()=="#32770")result=h;return true;},IntPtr.Zero);return result;
    }
    private static void NativeCapture(IntPtr h,string file)
    {
        Require(GetWindowRect(h,out var r),"Dialog rectangle");using var b=new Bitmap(r.Right-r.Left,r.Bottom-r.Top);using(var g=Graphics.FromImage(b)){IntPtr dc=g.GetHdc();try{Require(PrintWindow(h,dc,0),"Dialog screenshot");}finally{g.ReleaseHdc(dc);}}b.Save(Path.Combine(output,file));
    }
    private static void Capture(Form f,string name){using var b=new Bitmap(f.Width,f.Height);f.DrawToBitmap(b,new Rectangle(0,0,b.Width,b.Height));b.Save(Path.Combine(output,name));}
    private static void Run(Assembly a)
    {
        Application.SetHighDpiMode(HighDpiMode.SystemAware);Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);
        var t=a.GetType("GF2VR.Configurator.MainForm",true)!;var texts=a.GetType("GF2VR.Configurator.Texts",true)!;
        using var f=(Form)Activator.CreateInstance(t,Flags,null,new object?[]{Steam,null},null)!;
        f.StartPosition=FormStartPosition.Manual;f.Location=new Point(-20000,-20000);f.ShowInTaskbar=false;
        var inputs=(System.Collections.IDictionary)t.GetField("inputs",Flags)!.GetValue(f)!;
        var combo=(ComboBox)inputs["pcOutputPreset"]!;var save=(Button)t.GetField("save",Flags)!.GetValue(f)!;
        var status=(Label)t.GetField("status",Flags)!.GetValue(f)!;
        bool Dirty()=>(bool)t.GetField("dirty",Flags)!.GetValue(f)!;
        int Choice(string part)=>combo.Items.Cast<object>().Select((x,i)=>(x,i)).Single(x=>x.x.ToString()!.Contains(part,StringComparison.Ordinal)).i;
        int original=combo.SelectedIndex,phase=0,errorDialogs=0,cancelledCloses=0,confirmedCloses=0;FileStream? lease=null;Exception? failure=null;bool done=false;
        string performance=Path.Combine(Steam,@"vrmod\config\performance.json");
        byte[] before=File.ReadAllBytes(performance);using var originalJson=JsonDocument.Parse(before);string? dynamicBefore=originalJson.RootElement.TryGetProperty("dynamicEyeScale",out var dynamicValue)?dynamicValue.GetRawText():null;
        void CaptureLanguages(string state)
        {
            foreach(string lang in new[]{"ko","en","ja","zh-CN"}){texts.GetField("Language",Flags)!.SetValue(null,lang);t.GetMethod("Translate",Flags)!.Invoke(f,null);Require(f.Text.EndsWith(" *")==Dirty(),"Translated title marker");Capture(f,state+"-"+lang+".png");}
            texts.GetField("Language",Flags)!.SetValue(null,"ko");t.GetMethod("Translate",Flags)!.Invoke(f,null);
        }
        using var timer=new System.Windows.Forms.Timer{Interval=100};var watch=Stopwatch.StartNew();
        timer.Tick+=(_,_)=>
        {
            try
            {
                if(watch.ElapsedMilliseconds>60000)throw new TimeoutException("Save state GUI phase "+phase);
                if(phase is 2 or 6 or 9)
                {
                    var dialog=Dialog();if(dialog==IntPtr.Zero)return;
                    timer.Stop();int button=phase==2?1:phase==6?7:6;
                    NativeCapture(dialog,phase==2?"save-error-dialog.png":phase==6?"close-cancel-dialog.png":"close-confirm-dialog.png");
                    if(phase==2){errorDialogs++;phase=3;}else if(phase==6){cancelledCloses++;phase=7;}else{confirmedCloses++;done=true;phase=10;}
                    var children=new List<(IntPtr Handle,int Id,string Text)>();
                    EnumChildWindows(dialog,(h,_)=>{var cl=new System.Text.StringBuilder(64);GetClassName(h,cl,64);if(cl.ToString()=="Button"){var label=new System.Text.StringBuilder(256);GetWindowText(h,label,256);children.Add((h,GetDlgCtrlID(h),label.ToString().Replace("&","")));}return true;},IntPtr.Zero);
                    File.WriteAllText(Path.Combine(output,"dialog-actions-"+button+".json"),JsonSerializer.Serialize(children.Select(x=>new{x.Id,x.Text})));
                    var target=GetDlgItem(dialog,button);
                    if(target==IntPtr.Zero)target=children.Single(x=>button==1?x.Text is "확인" or "OK":button==6?x.Text is "예" or "Yes":x.Text is "아니요" or "No").Handle;
                    Require(target!=IntPtr.Zero,"Expected localized dialog action");SendMessage(target,0xF5,IntPtr.Zero,IntPtr.Zero);if(!done)timer.Start();return;
                }
                if(phase==0)
                {
                    combo.SelectedIndex=Choice("1280");Require(Dirty()&&f.Text.EndsWith(" *")&&status.Tag as string=="unsaved","Unsaved state");CaptureLanguages("unsaved");phase=1;return;
                }
                if(phase==1)
                {lease=new FileStream(performance,FileMode.Open,FileAccess.Read,FileShare.None);phase=2;save.PerformClick();return;}
                if(phase==3)
                {
                    lease?.Dispose();lease=null;Require(errorDialogs==1&&Dirty()&&status.Tag as string=="saveFailed"&&f.Text.EndsWith(" *"),"Explicit failed save");Require(File.ReadAllBytes(performance).SequenceEqual(before),"Failure preserves actual settings");CaptureLanguages("failed");phase=4;return;
                }
                if(phase==4)
                {
                    save.PerformClick();Require(!Dirty()&&!f.Text.EndsWith(" *")&&status.Tag as string=="saved","Saved state");
                    using var saved=JsonDocument.Parse(File.ReadAllBytes(performance));var pc=saved.RootElement.GetProperty("pcOutput");Require(pc.GetProperty("mode").GetString()=="FIXED"&&pc.GetProperty("width").GetInt32()==1280&&pc.GetProperty("height").GetInt32()==720,"Persisted 720p");
                    bool dynamicAfter=saved.RootElement.TryGetProperty("dynamicEyeScale",out var dynamicSaved);
                    Require(dynamicBefore==null?!dynamicAfter:dynamicAfter&&System.Text.Json.Nodes.JsonNode.DeepEquals(System.Text.Json.Nodes.JsonNode.Parse(dynamicSaved.GetRawText()),System.Text.Json.Nodes.JsonNode.Parse(dynamicBefore)),"Optional opaque dynamic object/absence preserved");CaptureLanguages("saved");phase=5;return;
                }
                if(phase==5){combo.SelectedIndex=Choice("1920");phase=6;f.Close();return;}
                if(phase==7)
                {Require(!f.IsDisposed&&Dirty()&&cancelledCloses==1,"Unsaved close cancelled");combo.SelectedIndex=original;save.PerformClick();Require(!Dirty()&&status.Tag as string=="saved","Original preset restored through GUI");Capture(f,"restored.png");phase=8;return;}
                if(phase==8){combo.SelectedIndex=Choice("1920");phase=9;f.Close();}
            }
            catch(Exception e){failure=e;lease?.Dispose();lease=null;timer.Stop();t.GetField("dirty",Flags)!.SetValue(f,false);f.Close();}
        };
        timer.Start();try{Application.Run(f);}finally{lease?.Dispose();}
        if(failure!=null)throw failure;Require(done&&errorDialogs==1&&cancelledCloses==1&&confirmedCloses==1,"Complete save/close path");
        File.WriteAllText(Path.Combine(output,"result.json"),JsonSerializer.Serialize(new{passed=true,actualInstalledGui=true,errorDialogs,cancelledCloses,confirmedCloses,locales=new[]{"ko","en","ja","zh-CN"},requests,archiveDownloads=0,gameLaunches=0,originalPresetRestored=true},new JsonSerializerOptions{WriteIndented=true}));
    }
    private sealed class EmptyCatalog:HttpMessageHandler
    {
        protected override Task<HttpResponseMessage> SendAsync(HttpRequestMessage r,CancellationToken token)
        {requests++;Require(r.RequestUri!.AbsoluteUri=="https://api.github.com/repos/deadpixel134/GF2Exilium-VR/releases?per_page=20","Catalog only");return Task.FromResult(new HttpResponseMessage(HttpStatusCode.OK){Content=new StringContent("[]"),RequestMessage=r});}
    }
}
