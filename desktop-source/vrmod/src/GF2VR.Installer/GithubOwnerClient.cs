using System.Net;
using System.Net.Http;
using System.Text.Json;

namespace GF2VR.Installer;

// Anonymous public HTTPS: no CLI, login, token or credential-store access.
// Keep the shared type name for existing Settings/Installer consumers.
internal sealed class GithubOwnerClient
{
    private static readonly HttpClient Shared = new(new HttpClientHandler
    { AllowAutoRedirect = false, UseCookies = false }) { Timeout = Timeout.InfiniteTimeSpan };
    private readonly HttpClient client;
    internal GithubOwnerClient(HttpClient? transport = null) { client = transport ?? Shared; }
    internal bool Available => true;
    internal async Task<byte[]> Releases(CancellationToken token)
    {
        using var output = new MemoryStream();
        await Request(new Uri("https://api.github.com/" + UpdatePolicy.Api + "releases?per_page=20"), false,
            output, 262144, TimeSpan.FromSeconds(12), token);
        return output.ToArray();
    }
    internal async Task<byte[]> Metadata(ReleaseAsset asset, CancellationToken token)
    {
        var uri = await DownloadUri(asset, token); using var output = new MemoryStream();
        await Request(uri, true, output, Math.Min(asset.Bytes, 262144), TimeSpan.FromSeconds(20), token);
        byte[] bytes = output.ToArray();
        if (bytes.LongLength != asset.Bytes || !Paths.Equal(Paths.Hash(bytes), asset.Hash)) throw new InstallError("updateInvalid");
        return bytes;
    }
    internal async Task Download(ReleaseAsset asset, string file, CancellationToken token)
    {
        var uri = await DownloadUri(asset, token);
        using var output = new FileStream(file, FileMode.CreateNew, FileAccess.Write, FileShare.None, 65536, true);
        await Request(uri, true, output, asset.Bytes, TimeSpan.FromMinutes(3), token);
        await output.FlushAsync(token); output.Flush(true);
        if (output.Length != asset.Bytes) throw new InstallError("updateInvalid");
        output.Close(); if (!Paths.Equal(Paths.FileHash(file), asset.Hash)) throw new InstallError("updateInvalid");
    }
    private async Task<Uri> DownloadUri(ReleaseAsset asset, CancellationToken token)
    {
        if (asset.Id < 1 || asset.Bytes is < 1 or > UpdatePolicy.MaxArchive) throw new InstallError("updateInvalid");
        Paths.HashText(asset.Hash);
        string endpoint = "https://api.github.com/" + UpdatePolicy.Api + "releases/assets/" + asset.Id;
        using var output = new MemoryStream();
        await Request(new Uri(endpoint), false, output, 65536, TimeSpan.FromSeconds(12), token);
        using var json = JsonDocument.Parse(output.ToArray()); Contract.Duplicates(json.RootElement); var a = json.RootElement;
        if (a.GetProperty("id").GetInt64() != asset.Id || a.GetProperty("name").GetString() != asset.Name ||
            a.GetProperty("size").GetInt64() != asset.Bytes || a.GetProperty("state").GetString() != "uploaded" ||
            a.GetProperty("url").GetString() != endpoint ||
            !Paths.Equal(a.GetProperty("digest").GetString(), "sha256:" + asset.Hash)) throw new InstallError("updateInvalid");
        string url = a.GetProperty("browser_download_url").GetString() ?? "";
        string prefix = "https://github.com/" + UpdatePolicy.Repository + "/releases/download/v";
        if (!url.StartsWith(prefix, StringComparison.Ordinal) || !url.EndsWith("/" + asset.Name, StringComparison.Ordinal)) throw new InstallError("updateInvalid");
        string version = url[prefix.Length..^(asset.Name.Length + 1)]; _ = UpdatePolicy.VersionOf(version);
        if (asset.Name != UpdatePolicy.DescriptorFor("Clean") && asset.Name != UpdatePolicy.DescriptorFor("Dirty") &&
            asset.Name != UpdatePolicy.DescriptorName && asset.Name != UpdatePolicy.ArchiveName(version, "Clean") &&
            asset.Name != UpdatePolicy.ArchiveName(version, "Dirty") && asset.Name != UpdatePolicy.ArchiveName(version)) throw new InstallError("updateInvalid");
        return new Uri(url);
    }
    private static bool Allowed(Uri uri) => uri.Scheme == "https" && uri.IsDefaultPort && uri.UserInfo == "" && uri.Fragment == "" &&
        (uri.Host is "api.github.com" or "github.com" or "release-assets.githubusercontent.com" or "objects.githubusercontent.com");
    private async Task Request(Uri uri, bool binary, Stream output, long limit, TimeSpan timeout, CancellationToken token)
    {
        using var cancel = CancellationTokenSource.CreateLinkedTokenSource(token); cancel.CancelAfter(timeout);
        for (int redirects = 0; redirects <= 5; ++redirects)
        {
            if (!Allowed(uri)) throw new InstallError("updateAccess");
            using var request = new HttpRequestMessage(HttpMethod.Get, uri);
            request.Headers.UserAgent.ParseAdd("GF2VR-Updater/1.1.6");
            request.Headers.Accept.ParseAdd(binary ? "application/octet-stream" : "application/vnd.github+json");
            using var response = await client.SendAsync(request, HttpCompletionOption.ResponseHeadersRead, cancel.Token);
            if (response.StatusCode is HttpStatusCode.MovedPermanently or HttpStatusCode.Found or HttpStatusCode.SeeOther or
                HttpStatusCode.TemporaryRedirect or HttpStatusCode.PermanentRedirect)
            {
                if (redirects == 5 || response.Headers.Location == null) throw new InstallError("updateAccess");
                uri = response.Headers.Location.IsAbsoluteUri ? response.Headers.Location : new Uri(uri, response.Headers.Location); continue;
            }
            response.EnsureSuccessStatusCode();
            if (response.Content.Headers.ContentLength > limit) throw new InstallError("updateInvalid");
            using var input = await response.Content.ReadAsStreamAsync(cancel.Token);
            byte[] buffer = new byte[65536]; long total = 0;
            while (true)
            {
                int read = await input.ReadAsync(buffer, cancel.Token); if (read == 0) return;
                if ((total += read) > limit) throw new InstallError("updateInvalid");
                await output.WriteAsync(buffer.AsMemory(0, read), cancel.Token);
            }
        }
        throw new InstallError("updateAccess");
    }
}
