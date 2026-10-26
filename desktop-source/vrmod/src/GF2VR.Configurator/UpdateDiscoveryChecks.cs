using System.Net;
using System.Text;
using System.Text.Json.Nodes;
using GF2VR.Installer;

namespace GF2VR.Configurator;

internal static class UpdateDiscoveryChecks
{
    private sealed class Handler(Func<HttpRequestMessage, HttpResponseMessage> response) : HttpMessageHandler
    { protected override Task<HttpResponseMessage> SendAsync(HttpRequestMessage request, CancellationToken token) => Task.FromResult(response(request)); }
    internal static int Run(string candidate)
    {
        int checks = 0, archives = 0;
        void Check(bool value, string why) { ++checks; if (!value) throw new InvalidDataException(why); }
        string fixture = Path.Combine(AppContext.BaseDirectory, "update-discovery-fixture");
        if (Directory.Exists(fixture)) throw new InvalidDataException("Preserve prior discovery fixture");
        string root = Path.Combine(fixture, "game");
        SettingsStore.WriteNew(Create(Path.Combine(root, "GF2 Game/GF2_Exilium.exe")), Encoding.UTF8.GetBytes("INERT INSTALLER FIXTURE — NEVER EXECUTE AS A GAME"));
        var engine = new InstallationEngine(); engine.Install(engine.Preview(root, candidate));
        var prior = engine.Inspect(root)!; string before = engine.ReceiptHash(root);
        var installedVersion = Version.Parse(prior.Version);
        string offeredVersion = new Version(installedVersion.Major, installedVersion.Minor, installedVersion.Build + 1).ToString();
        var d = new UpdateDescriptor(1, "GF2VR", UpdatePolicy.Repository, offeredVersion, "stable", UpdatePolicy.ArchiveName(offeredVersion, prior.BuildFlavor), new string('A',64), new string('B',64),
            UpdatePolicy.HelperFiles.Select(n => new PayloadFile(n, new string('C',64), 10)).ToArray(), prior.BuildFlavor, prior.CommonCoreSha256);
        byte[] meta = Contract.Encode(d); string mode = "valid";
        JsonObject Asset(long id, string name, long size, string hash) => new() { ["id"] = id, ["name"] = name, ["size"] = size, ["state"] = "uploaded", ["digest"] = "sha256:" + hash,
            ["url"] = "https://api.github.com/" + UpdatePolicy.Api + "releases/assets/" + id, ["browser_download_url"] = "https://github.com/" + UpdatePolicy.Repository + "/releases/download/v" + offeredVersion + "/" + name };
        var descriptorAsset = Asset(102, UpdatePolicy.DescriptorFor(prior.BuildFlavor), meta.Length, Paths.Hash(meta));
        byte[] Catalog() => Contract.Encode(new JsonArray(new JsonObject { ["id"] = 101, ["draft"] = false, ["prerelease"] = false, ["published_at"] = "2026-10-25T00:00:00Z", ["tag_name"] = mode == "same" ? "v"+prior.Version : "v"+offeredVersion,
            ["assets"] = mode == "manual" ? new JsonArray() : new JsonArray(descriptorAsset.DeepClone(), Asset(103, d.ArchiveName, 100, d.ArchiveSha256)) }));
        HttpResponseMessage Response(HttpRequestMessage r)
        {
            if (mode == "offline") throw new HttpRequestException("INERT OFFLINE FIXTURE");
            string url = r.RequestUri!.AbsoluteUri; byte[] b;
            if (url.EndsWith("releases?per_page=20", StringComparison.Ordinal)) b = Catalog();
            else if (url.EndsWith("assets/102", StringComparison.Ordinal)) b = Contract.Encode(descriptorAsset);
            else if (url.EndsWith("/"+UpdatePolicy.DescriptorFor(prior.BuildFlavor), StringComparison.Ordinal)) b = meta;
            else { archives++; throw new InvalidDataException("Discovery attempted archive access"); }
            return new(HttpStatusCode.OK) { Content = new ByteArrayContent(b), RequestMessage = r };
        }
        using var http = new HttpClient(new Handler(Response)); var service = new UpdateService(new GithubOwnerClient(http));
        var good = service.Check(root, default).GetAwaiter().GetResult();
        Check(good.DiscoveredVersion == offeredVersion && good.State == "updateAvailable" && good.Release != null, "discovery and applicability");
        mode = "manual"; var manual = service.Check(root,default).GetAwaiter().GetResult();
        Check(manual.DiscoveredVersion == offeredVersion && manual.State == "updateManual" && manual.Release == null && manual.PageUrl == UpdatePolicy.ReleasePage(offeredVersion), "unsupported assets retain version/manual page");
        mode = "valid"; string dll = Path.Combine(root, "GF2 Game/version.dll"); byte[] bytes = File.ReadAllBytes(dll);
        File.WriteAllText(dll, "INERT MODIFIED");
        try { var changed = service.Check(root, default).GetAwaiter().GetResult(); Check(changed.DiscoveredVersion == offeredVersion && changed.Refusal?.Key == "modified", "modified install still discovers version"); }
        finally { File.WriteAllBytes(dll, bytes); }
        File.Move(dll, dll+".quarantine");
        try { var missing = service.Check(root,default).GetAwaiter().GetResult(); Check(missing.DiscoveredVersion == offeredVersion && missing.State == "updateAvailable" && missing.Refusal == null, "verified receipt with absent file remains eligible without writing"); }
        finally { File.Move(dll+".quarantine",dll); }
        string receiptPath=Path.Combine(root,"vrmod/installer/install-state.json");byte[] realReceipt=File.ReadAllBytes(receiptPath);File.WriteAllText(receiptPath,"{");
        try { var invalid=service.Check(root,default).GetAwaiter().GetResult(); Check(invalid.DiscoveredVersion==offeredVersion&&invalid.State=="updateManual"&&invalid.Refusal?.Key=="badState","invalid receipt still discovers version without application"); }
        finally { File.WriteAllBytes(receiptPath,realReceipt); }
        mode="same"; Check(service.Check(root,default).GetAwaiter().GetResult().DiscoveredVersion == null,"same version has no notification");
        mode="offline"; bool refused=false; try { service.Check(root,default).GetAwaiter().GetResult(); } catch(HttpRequestException) {refused=true;} Check(refused,"offline leaves installation usable");
        Check(archives == 0 && engine.ReceiptHash(root) == before, "discovery performs no archive download/install");
        var store = new SettingsStore(root,true); store.Load();
        string pref = Path.Combine(root,"vrmod/config/configurator.json");
        File.WriteAllText(pref,"{\"schemaVersion\":1,\"language\":\"en\",\"future\":{\"retain\":42}}");
        Check(store.ShouldNotifyUpdate("1.2.3"),"initial notice"); store.MarkUpdateNotice("1.2.3",false);
        Check(!new SettingsStore(root,true).ShouldNotifyUpdate("1.2.3"),"once survives restart");
        Check(store.ShouldNotifyUpdate("1.2.4"),"new version gets notice"); store.MarkUpdateNotice("1.2.4",true);
        Check(!new SettingsStore(root,true).ShouldNotifyUpdate("1.2.4"),"skip survives restart");
        store.SaveLanguage("zh-CN"); var saved=JsonNode.Parse(File.ReadAllBytes(pref))!;
        Check(saved["future"]!["retain"]!.GetValue<int>() == 42 && saved["skippedUpdate"]!.GetValue<string>()=="1.2.4" && store.LoadLanguage()=="zh-CN","unknown/preferences/locale roundtrip");
        byte[] valid=File.ReadAllBytes(pref);File.WriteAllText(pref,"{");
        bool bad=false;try{store.MarkUpdateNotice("1.2.5",false);}catch(System.Text.Json.JsonException){bad=true;}
        Check(bad&&File.ReadAllText(pref)=="{","broken preference is preserved");File.WriteAllBytes(pref,valid);
        string oldLanguage=Texts.Language;
        foreach(string lang in Texts.Languages) { Texts.Language=lang; Check(Texts.Format("updateManualVersion","1.2.3").Contains("1.2.3",StringComparison.Ordinal),"localized version "+lang); }
        Texts.Language=oldLanguage;
        Check(Texts.Normalize("zh-Hans-CN")=="zh-CN"&&Texts.Normalize("en-US")=="en"&&Texts.Normalize("unsupported")=="ko","culture/fallback");
        File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory,"update-discovery-validation.json"),Contract.Encode(new{passed=true,checks,archives,oncePerVersion=true,skipPersisted=true,unknownKeysPreserved=true,locales=Texts.Languages,gameLaunches=0,fixture}));
        return 0;
    }
    private static string Create(string p) { Directory.CreateDirectory(Path.GetDirectoryName(p)!); return p; }
}
