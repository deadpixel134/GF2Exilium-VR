using System.Diagnostics;

namespace GF2VR.Installer;

// Owner-only optional update transport. No token is read into the app, persisted
// or logged. The user's existing GitHub CLI credential store handles auth.
internal sealed class GithubOwnerClient
{
    private readonly string cli = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "GitHub CLI", "gh.exe");
    internal bool Available => File.Exists(cli);
    internal async Task<byte[]> Releases(CancellationToken token)
    {
        using var output = new MemoryStream();
        await Request(UpdatePolicy.Api + "releases?per_page=20", false, output, 262144, TimeSpan.FromSeconds(12), token);
        return output.ToArray();
    }
    internal async Task<byte[]> Metadata(ReleaseAsset asset, CancellationToken token)
    {
        using var output = new MemoryStream();
        await Request(UpdatePolicy.Api + "releases/assets/" + asset.Id, true, output, 262144, TimeSpan.FromSeconds(20), token);
        return output.ToArray();
    }
    internal async Task Download(ReleaseAsset asset, string file, CancellationToken token)
    {
        using var output = new FileStream(file, FileMode.CreateNew, FileAccess.Write, FileShare.None, 65536, true);
        await Request(UpdatePolicy.Api + "releases/assets/" + asset.Id, true, output, asset.Bytes, TimeSpan.FromMinutes(3), token);
        await output.FlushAsync(token); output.Flush(true);
        if (output.Length != asset.Bytes) throw new InstallError("updateInvalid");
    }
    private async Task Request(string endpoint, bool binary, Stream output, long limit, TimeSpan timeout, CancellationToken token)
    {
        if (!Available || !endpoint.StartsWith(UpdatePolicy.Api, StringComparison.Ordinal)) throw new InstallError("updateAccess");
        var start = new ProcessStartInfo(cli) { UseShellExecute = false, CreateNoWindow = true, WindowStyle = ProcessWindowStyle.Hidden, RedirectStandardOutput = true, RedirectStandardError = true };
        foreach (string a in new[] { "api", endpoint, "--hostname", "github.com", "--method", "GET", "-H", binary ? "Accept: application/octet-stream" : "Accept: application/vnd.github+json" }) start.ArgumentList.Add(a);
        start.Environment["GH_PROMPT_DISABLED"] = "1"; start.Environment["GH_NO_UPDATE_NOTIFIER"] = "1";
        start.Environment.Remove("GH_TOKEN"); start.Environment.Remove("GITHUB_TOKEN"); start.Environment.Remove("GH_HOST");
        using var cancel = CancellationTokenSource.CreateLinkedTokenSource(token); cancel.CancelAfter(timeout);
        using var process = Process.Start(start) ?? throw new InstallError("updateAccess");
        async Task DiscardErrors()
        {
            char[] buffer = new char[1024]; long n = 0;
            while (true) { int read = await process.StandardError.ReadAsync(buffer, cancel.Token); if (read == 0) return; if ((n += read) > 65536) throw new InstallError("updateAccess"); }
        }
        var errors = DiscardErrors();
        try
        {
            byte[] buffer = new byte[65536]; long total = 0;
            while (true) { int read = await process.StandardOutput.BaseStream.ReadAsync(buffer, cancel.Token); if (read == 0) break; if ((total += read) > limit) throw new InstallError("updateInvalid"); await output.WriteAsync(buffer.AsMemory(0, read), cancel.Token); }
            await process.WaitForExitAsync(cancel.Token); await errors;
            if (process.ExitCode != 0) throw new InstallError("updateAccess");
        }
        finally
        {
            if (!process.HasExited) process.Kill(entireProcessTree: true); // Our transport only, never game/SteamVR.
            cancel.Cancel(); try { await errors; } catch { }
        }
    }
}
