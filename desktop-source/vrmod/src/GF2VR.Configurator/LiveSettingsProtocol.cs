using System.Diagnostics;
using System.IO.Pipes;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.Json.Nodes;

namespace GF2VR.Configurator;

internal enum LiveCommand : uint { State = 1, Prepare = 2, Apply = 3, Finalize = 4, Rollback = 5 }
internal enum LiveStatus : uint { Ok, Rejected, Busy, Unavailable, Pending }
internal sealed record LiveState(int ProcessId, bool Dirty, ulong Revision, uint Phase, uint Owners,
    ulong Instance, ulong Transaction, string Root, string Build, Dictionary<string, JsonObject> Documents);
internal sealed record LiveReply(LiveStatus Status, ulong Revision, byte[] Payload);

internal interface ILiveSettingsConnection : IDisposable
{
    ulong RequestId { get; }
    LiveState State { get; }
    LiveReply Require(LiveCommand command, ulong revision, byte[] payload);
}

internal sealed class LiveSettingsClient : ILiveSettingsConnection
{
    internal const uint Magic = 0x31535647;
    internal const int MaximumPayload = 262144;
    private readonly NamedPipeClientStream pipe;
    private readonly ulong request;
    public LiveState State { get; private set; }
    public ulong RequestId => request;
    private static long sequence = DateTime.UtcNow.Ticks;

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool GetNamedPipeServerProcessId(Microsoft.Win32.SafeHandles.SafePipeHandle pipe, out uint pid);

    // Inert test servers implement the same acknowledgement contract.
    internal static Func<string, ulong, ILiveSettingsConnection>? TestConnect { get; set; }
    internal static Action<Exception>? TestFailure { get; set; }
    internal static ILiveSettingsConnection Connect(string root, ulong requestId = 0)
    {
        if (TestConnect != null) return TestConnect(root, requestId);
        string normalized = Path.TrimEndingDirectorySeparator(Path.GetFullPath(root));
        foreach (var process in Process.GetProcessesByName("GF2_Exilium"))
        {
            using (process)
            {
                try
                {
                    string? exe = process.MainModule?.FileName;
                    if (exe == null) continue;
                    string direct = Path.Combine(normalized, "GF2_Exilium.exe"), nested = Path.Combine(normalized, "GF2 Game", "GF2_Exilium.exe");
                    if (!exe.Equals(direct, StringComparison.OrdinalIgnoreCase) && !exe.Equals(nested, StringComparison.OrdinalIgnoreCase)) continue;
                    return new LiveSettingsClient(process.Id, normalized, requestId: requestId);
                }
                catch (SettingsError) { throw; }
                catch (Exception ex) when (ex is System.ComponentModel.Win32Exception or InvalidOperationException or IOException) { }
            }
        }
        throw new SettingsError("runningError");
    }

    internal LiveSettingsClient(int pid, string root, bool fixture = false, ulong requestId = 0)
    {
        request = requestId != 0 ? requestId : unchecked((ulong)Interlocked.Increment(ref sequence));
        pipe = new NamedPipeClientStream(".", "GF2VR.LiveSettings." + pid, PipeDirection.InOut, PipeOptions.Asynchronous);
        try
        {
            using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(3));
            pipe.ConnectAsync(timeout.Token).GetAwaiter().GetResult();
            if (!GetNamedPipeServerProcessId(pipe.SafePipeHandle, out uint server) || server != (uint)pid) throw new SettingsError("liveConnectionError");
            var reply = Send(LiveCommand.State, 0, []);
            if (reply.Status != LiveStatus.Ok) throw new SettingsError("liveConnectionError");
            State = DecodeState(reply);
            if (State.ProcessId != pid || !Path.TrimEndingDirectorySeparator(Path.GetFullPath(State.Root)).Equals(Path.TrimEndingDirectorySeparator(Path.GetFullPath(root)), StringComparison.OrdinalIgnoreCase))
                throw new SettingsError("liveConnectionError");
            if (!fixture)
            {
                var contract = JsonNode.Parse(File.ReadAllBytes(Path.Combine(AppContext.BaseDirectory, "runtime-contract.json")))!.AsObject();
                if (contract["liveSettingsProtocol"]?.GetValue<int>() != 1 || State.Build != contract["buildId"]!.GetValue<string>() || State.Dirty != (contract["buildFlavor"]!.GetValue<string>() == "Dirty"))
                    throw new SettingsError("liveConnectionError");
            }
        }
        catch (SettingsError) { pipe.Dispose(); throw; }
        catch { pipe.Dispose(); throw new SettingsError("liveConnectionError"); }
    }

    internal LiveReply Send(LiveCommand command, ulong revision, byte[] payload)
    {
        if (payload.Length > MaximumPayload) throw new SettingsError("livePayloadError");
        try
        {
            using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(30));
            byte[] header = new byte[32];
            using (var writer = new BinaryWriter(new MemoryStream(header), Encoding.UTF8, false))
            { writer.Write(Magic); writer.Write(1u); writer.Write((uint)command); writer.Write((uint)payload.Length); writer.Write(request); writer.Write(revision); }
            pipe.WriteAsync(header, timeout.Token).AsTask().GetAwaiter().GetResult();
            if (payload.Length != 0) pipe.WriteAsync(payload, timeout.Token).AsTask().GetAwaiter().GetResult();
            pipe.FlushAsync(timeout.Token).GetAwaiter().GetResult();
            pipe.ReadExactlyAsync(header, timeout.Token).AsTask().GetAwaiter().GetResult();
            using var reader = new BinaryReader(new MemoryStream(header), Encoding.UTF8, false);
            if (reader.ReadUInt32() != Magic || reader.ReadUInt32() != 1) throw new SettingsError("livePayloadError");
            var status = (LiveStatus)reader.ReadUInt32(); uint length = reader.ReadUInt32();
            if (!Enum.IsDefined(status) || length > MaximumPayload || reader.ReadUInt64() != request) throw new SettingsError("livePayloadError");
            ulong actual = reader.ReadUInt64(); byte[] body = new byte[length];
            if (length != 0) pipe.ReadExactlyAsync(body, timeout.Token).AsTask().GetAwaiter().GetResult();
            return new(status, actual, body);
        }
        catch (SettingsError) { throw; }
        catch (Exception ex) { TestFailure?.Invoke(ex); throw new SettingsError("liveConnectionError"); }
    }

    public LiveReply Require(LiveCommand command, ulong revision, byte[] payload)
    {
        var reply = Send(command, revision, payload);
        if (reply.Status != LiveStatus.Ok)
            throw new SettingsError(reply.Status == LiveStatus.Busy ? "conflictError" : "liveApplyError");
        return reply;
    }

    internal static byte[] EncodeDocuments(Dictionary<string, JsonObject> documents)
    {
        using var stream = new MemoryStream(); using var writer = new BinaryWriter(stream, Encoding.UTF8, true);
        writer.Write((uint)documents.Count);
        foreach (var pair in documents.OrderBy(p => p.Key, StringComparer.Ordinal))
        { WriteText(writer, pair.Key); WriteBytes(writer, SettingsStore.Encode(pair.Value)); }
        if (stream.Length > MaximumPayload) throw new SettingsError("livePayloadError");
        return stream.ToArray();
    }
    private static void WriteBytes(BinaryWriter writer, byte[] bytes) { writer.Write((uint)bytes.Length); writer.Write(bytes); }
    private static void WriteText(BinaryWriter writer, string text) => WriteBytes(writer, Encoding.UTF8.GetBytes(text));
    private static byte[] ReadBytes(BinaryReader reader, int maximum)
    {
        uint length = reader.ReadUInt32(); if (length > maximum || length > reader.BaseStream.Length - reader.BaseStream.Position) throw new SettingsError("livePayloadError");
        return reader.ReadBytes((int)length);
    }
    private static string ReadText(BinaryReader reader, int maximum) => new UTF8Encoding(false, true).GetString(ReadBytes(reader, maximum));
    internal static LiveState DecodeState(LiveReply reply)
    {
        using var stream = new MemoryStream(reply.Payload); using var reader = new BinaryReader(stream, Encoding.UTF8, true);
        int pid = checked((int)reader.ReadUInt32()); bool dirty = reader.ReadUInt32() != 0;
        uint phase = reader.ReadUInt32(), owners = reader.ReadUInt32();
        ulong instance = reader.ReadUInt64(), transaction = reader.ReadUInt64();
        string root = ReadText(reader, 4096), build = ReadText(reader, 128);
        uint count = reader.ReadUInt32(); if (count > 10) throw new SettingsError("livePayloadError");
        var docs = new Dictionary<string, JsonObject>(StringComparer.Ordinal);
        for (uint i = 0; i < count; ++i)
        {
            string file = ReadText(reader, 64); byte[] raw = ReadBytes(reader, 16384);
            if (!Fields.Files.Contains(file) || !docs.TryAdd(file, SettingsStore.Parse(raw, file))) throw new SettingsError("livePayloadError");
        }
        if (stream.Position != stream.Length) throw new SettingsError("livePayloadError");
        if (count != Fields.Files.Length || reply.Revision == 0 || instance == 0 || phase > 4) throw new SettingsError("livePayloadError");
        SettingsStore.Validate(docs);
        return new(pid, dirty, reply.Revision, phase, owners, instance, transaction, root, build, docs);
    }
    public void Dispose() => pipe.Dispose();
}
