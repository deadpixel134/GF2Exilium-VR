using System.Text;
namespace GF2VR.Installer;
internal static class RuntimeLogCleanupChecks
{
    internal static int Run()
    {
        int checks=0;void Check(bool condition,string name){++checks;if(!condition)throw new InvalidDataException(name);}
        string fixture=Path.Combine(AppContext.BaseDirectory,"log-cleanup-fixture");
        if(Directory.Exists(fixture))throw new InvalidDataException("Fresh fixture required");
        string Root(string name,string? config)
        {
            string root=Path.Combine(fixture,name);Directory.CreateDirectory(root);
            if(config!=null)Paths.WriteNew(Paths.Contained(root,"vrmod/config/controller_input.json"),Encoding.UTF8.GetBytes(config));
            Paths.WriteNew(Paths.Contained(root,"vrmod/logs/bootstrap/GF2VR-DIRTY-114-001.jsonl"),Encoding.UTF8.GetBytes("old runtime log"));
            Paths.WriteNew(Paths.Contained(root,"vrmod/logs/bootstrap/user-notes.jsonl"),Encoding.UTF8.GetBytes("preserve foreign file"));
            Paths.WriteNew(Paths.Contained(root,"vrmod/logs/bootstrap/GF2VR-DIRTY-114-001.txt"),Encoding.UTF8.GetBytes("preserve other extension"));
            return root;
        }
        string Log(string root)=>Paths.Contained(root,"vrmod/logs/bootstrap/GF2VR-DIRTY-114-001.jsonl");
        foreach(string? config in new[]{null,"{}","{\"schemaVersion\":1}","{\"diagnostics\":{\"loggingEnabled\":false}}"})
        {
            string root=Root("off-"+checks,config);Check(RuntimeLogCleanup.Apply(root,true)=="cleanup-attempted","OFF migration");
            Check(!File.Exists(Log(root)),"OFF old log deleted");Check(File.Exists(Paths.Contained(root,"vrmod/logs/bootstrap/user-notes.jsonl")),"foreign log preserved");
            Check(File.Exists(Paths.Contained(root,"vrmod/logs/bootstrap/GF2VR-DIRTY-114-001.txt")),"other file preserved");
            Paths.WriteNew(Log(root),Encoding.UTF8.GetBytes("later log"));RuntimeLogCleanup.Apply(root,true);Check(File.Exists(Log(root)),"OFF cleanup never repeats");
        }
        string on=Root("on","{\"diagnostics\":{\"loggingEnabled\":true}}");
        Check(RuntimeLogCleanup.Apply(on,true)=="preserved-logging-on"&&File.Exists(Log(on)),"ON exempt");
        File.WriteAllText(Paths.Contained(on,"vrmod/config/controller_input.json"),"{\"diagnostics\":{\"loggingEnabled\":false}}");
        Check(RuntimeLogCleanup.Apply(on,true)=="already-decided"&&File.Exists(Log(on)),"ON exemption survives later OFF");
        string fresh=Root("fresh",null);Check(RuntimeLogCleanup.Apply(fresh,false)=="fresh-install"&&File.Exists(Log(fresh)),"fresh install preserves preexisting unknown ownership");
        RuntimeLogCleanup.Apply(fresh,true);Check(File.Exists(Log(fresh)),"fresh migration never deferred to next upgrade");
        foreach(string invalid in new[]{"", "[]","{\"schemaVersion\":2}","{\"diagnostics\":null}","{\"diagnostics\":{\"loggingEnabled\":null}}","{\"diagnostics\":{\"loggingEnabled\":true,\"loggingEnabled\":false}}"})
        {
            string root=Root("invalid-"+checks,invalid);RuntimeLogCleanup.Apply(root,true);
            Check(File.Exists(Log(root))&&!File.Exists(Paths.Contained(root,RuntimeLogCleanup.Marker)),"invalid settings preserve logs and decision");
        }
        string locked=Root("locked",null);
        using(var lease=new FileStream(Log(locked),FileMode.Open,FileAccess.Read,FileShare.Read))RuntimeLogCleanup.Apply(locked,true);
        Check(File.Exists(Log(locked)),"locked log preserved");RuntimeLogCleanup.Apply(locked,true);Check(File.Exists(Log(locked)),"failed removal not retried later");
        string running=Root("running",null);var previous=InstallationEngine.GameRunning;
        try{InstallationEngine.GameRunning=()=>true;RuntimeLogCleanup.Apply(running,true);}finally{InstallationEngine.GameRunning=previous;}
        Check(File.Exists(Log(running))&&!File.Exists(Paths.Contained(running,RuntimeLogCleanup.Marker)),"running game refuses migration");
        File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory,"log-cleanup-validation.json"),Contract.Encode(new{passed=true,checks,fixture,gameLaunches=0,realInstallationWrites=0}));return 0;
    }
}
