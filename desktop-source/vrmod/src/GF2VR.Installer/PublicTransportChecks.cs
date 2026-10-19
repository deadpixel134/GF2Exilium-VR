using System.Net;
using System.Net.Http;
using System.Text;
using System.Text.Json.Nodes;

namespace GF2VR.Installer;
internal static class PublicTransportChecks
{
    private sealed class Handler(Func<HttpRequestMessage,HttpResponseMessage> respond) : HttpMessageHandler
    {
        internal int Calls;
        protected override Task<HttpResponseMessage> SendAsync(HttpRequestMessage request,CancellationToken token)
        {
            token.ThrowIfCancellationRequested(); ++Calls;
            if(request.Headers.Authorization!=null || request.RequestUri!.Scheme!="https" || request.Method!=HttpMethod.Get)
                throw new InvalidDataException("Anonymous HTTPS GET required");
            return Task.FromResult(respond(request));
        }
    }
    internal static async Task<int> Run()
    {
        int checks=0;void Check(bool ok,string why){++checks;if(!ok)throw new InvalidDataException(why);}
        async Task Reject(Func<Task> action,string why){++checks;try{await action();}catch(Exception e)when(e is InstallError or HttpRequestException or OperationCanceledException){return;}throw new InvalidDataException("Expected rejection "+why);}
        byte[] data=Encoding.UTF8.GetBytes("verified descriptor");
        var asset=new ReleaseAsset(123,"GF2VR-Clean-update.json",data.Length,Paths.Hash(data));
        string api="https://api.github.com/"+UpdatePolicy.Api+"releases/assets/123";
        string download="https://github.com/"+UpdatePolicy.Repository+"/releases/download/v1.1.7/"+asset.Name;
        string mode="normal";
        HttpResponseMessage Response(HttpRequestMessage request)
        {
            string url=request.RequestUri!.AbsoluteUri;
            if(mode=="http-error")return new(HttpStatusCode.Forbidden);
            if(url.EndsWith("releases?per_page=20",StringComparison.Ordinal))return new(HttpStatusCode.OK){Content=new StringContent("[]")};
            if(url==api){var json=new JsonObject{["id"]=123,["name"]=asset.Name,["size"]=asset.Bytes,["state"]="uploaded",["url"]=api,["digest"]="sha256:"+(mode=="digest"?new string('0',64):asset.Hash),["browser_download_url"]=mode=="foreign-url"?"https://example.invalid/file":download};return new(HttpStatusCode.OK){Content=new StringContent(json.ToJsonString())};}
            if(url==download){var response=new HttpResponseMessage(HttpStatusCode.Found);response.Headers.Location=new Uri(mode=="foreign-redirect"?"https://example.invalid/secret":"https://release-assets.githubusercontent.com/fixture");return response;}
            if(url=="https://release-assets.githubusercontent.com/fixture")return new(HttpStatusCode.OK){Content=new ByteArrayContent(mode=="payload"?Encoding.UTF8.GetBytes("wrong"):mode=="large"?new byte[4096]:data)};
            throw new InvalidDataException("Unexpected URL "+url);
        }
        var handler=new Handler(Response);using var http=new HttpClient(handler);var client=new GithubOwnerClient(http);
        Check(client.Available,"no CLI/account prerequisite");
        Check(Encoding.UTF8.GetString(await client.Releases(default))=="[]","public catalog");
        Check((await client.Metadata(asset,default)).SequenceEqual(data),"metadata redirect and SHA256");
        Check(handler.Calls==4,"only pinned API/download/storage routes");
        foreach(string invalid in new[]{"digest","foreign-url","foreign-redirect","payload","large","http-error"}){
            mode=invalid;await Reject(async()=>{await client.Metadata(asset,default);},invalid);
        }
        mode="normal";using var canceled=new CancellationTokenSource();canceled.Cancel();await Reject(async()=>{await client.Releases(canceled.Token);},"cancellation");
        File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory,"public-transport-validation.json"),Contract.Encode(new{passed=true,checks,anonymousHttps=true,noCliRequired=true,realNetworkRequests=0}));return 0;
    }
    internal static async Task<int> CheckPublic()
    {
        var client=new GithubOwnerClient();var catalog=await client.Releases(default);
        var release=UpdatePolicy.Select(catalog,"1.1.4","Dirty")??throw new InstallError("updateInvalid");
        var bytes=await client.Metadata(release.Descriptor,default);var descriptor=UpdatePolicy.Descriptor(bytes,release);
        File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory,"public-source-validation.json"),Contract.Encode(new{passed=true,anonymousHttps=true,noCliOrLogin=true,repository=UpdatePolicy.Repository,release.Version,release.Id,descriptorHash=Paths.Hash(bytes),descriptor.BuildFlavor,credentialsRead=false,installationWrites=0}));return 0;
    }
}
