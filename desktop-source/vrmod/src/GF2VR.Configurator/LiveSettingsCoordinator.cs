using System.Text.Json;
using System.Text.Json.Nodes;

namespace GF2VR.Configurator;

// A lease is issued only after this connection acknowledged the complete live snapshot.
internal sealed class LiveSaveLease
{
    private readonly ILiveSettingsConnection client;
    private readonly ulong revision;
    private readonly string root;
    private bool active = true;
    internal LiveSaveLease(ILiveSettingsConnection connection, LiveState applied)
    {
        if (applied.Phase != 2 || applied.Owners != 15 || applied.Transaction != connection.RequestId || applied.Revision != connection.State.Revision + 1)
            throw new SettingsError("liveApplyError");
        client = connection; revision = applied.Revision; root = applied.Root;
    }
    internal void Validate(string expectedRoot)
    {
        if (!active || revision == 0 || client.State.ProcessId <= 0 || !root.Equals(expectedRoot, StringComparison.OrdinalIgnoreCase))
            throw new SettingsError("liveConnectionError");
    }
    internal void End() => active = false;
}
internal sealed class LiveJournal
{
    public string Root { get; set; } = "";
    public int ProcessId { get; set; }
    public ulong Instance { get; set; }
    public ulong RequestId { get; set; }
    public ulong Revision { get; set; }
    public string Phase { get; set; } = "prepared";
    public Dictionary<string, byte[]?> Before { get; set; } = [];
    public Dictionary<string, byte[]> After { get; set; } = [];
}

internal static class LiveSettingsCoordinator
{
    internal static string JournalPath(string root) => Path.Combine(root, "vrmod", "runtime", "live-settings", "transaction.json");
    private static void WriteJournal(SettingsStore store, string path, LiveJournal journal)
    {
        store.CheckRuntimePath(path);
        Directory.CreateDirectory(Path.GetDirectoryName(path)!);
        SettingsStore.AtomicStandalone(path, JsonSerializer.SerializeToUtf8Bytes(journal));
    }

    internal static void Apply(SettingsStore store, Dictionary<string, JsonObject> requested)
    {
        SettingsStore.Validate(requested);
        using var held = store.AcquireWriteLock();
        using var stage = store.Stage(requested);
        using var client = LiveSettingsClient.Connect(store.Root);
        if (client.State.Phase != 0) throw new SettingsError("conflictError");
        if (store.RuntimeRevision.HasValue ? store.RuntimeInstance != client.State.Instance || store.RuntimeRevision.Value != client.State.Revision :
            store.Documents.Any(p => !client.State.Documents.TryGetValue(p.Key, out var doc) || !JsonNode.DeepEquals(doc, p.Value)))
            throw new SettingsError("conflictError");
        string path = JournalPath(store.Root); store.CheckRuntimePath(path);
        if (File.Exists(path)) throw new SettingsError("liveRecoveryError");
        var journal = new LiveJournal { Root = store.Root, ProcessId = client.State.ProcessId, Instance = client.State.Instance,
            RequestId = client.RequestId, Revision = client.State.Revision, Before = store.OriginalBytes(),
            After = requested.ToDictionary(p => p.Key, p => SettingsStore.Encode(p.Value)) };
        byte[] payload = LiveSettingsClient.EncodeDocuments(requested);
        // Persist recovery identity before the native owner can change anything.
        WriteJournal(store, path, journal);
        LiveSaveLease? lease = null;
        try
        {
            client.Require(LiveCommand.Prepare, journal.Revision, payload);
            var reply = client.Require(LiveCommand.Apply, journal.Revision, []);
            var actual = LiveSettingsClient.DecodeState(reply);
            if (!actual.Root.Equals(store.Root, StringComparison.OrdinalIgnoreCase) || actual.Instance != journal.Instance ||
                actual.ProcessId != journal.ProcessId || actual.Documents.Count != requested.Count ||
                requested.Any(p => !actual.Documents.TryGetValue(p.Key, out var doc) || !JsonNode.DeepEquals(doc, p.Value)))
                throw new SettingsError("liveApplyError");
            lease = new LiveSaveLease(client, actual);
            journal.Phase = "applied"; WriteJournal(store, path, journal);
            stage.Commit(lease);
            journal.After = store.OriginalBytes().ToDictionary(p => p.Key, p => p.Value ?? throw new SettingsError("liveRecoveryError"));
            journal.Phase = "saved"; WriteJournal(store, path, journal);
            client.Require(LiveCommand.Finalize, actual.Revision, []);
            store.BindRuntimeBaseline(actual);
            lease.End();
            // A lost final reply is resolved by the durable runtime transaction ID, never by guessing.
            journal.Phase = "committed"; WriteJournal(store, path, journal);
            File.Delete(path);
        }
        catch
        {
            lease?.End();
            client.Dispose(); // A new connection is allowed to resume this exact transaction ID.
            try { Recover(store); }
            catch { throw new SettingsError("liveRecoveryError"); }
            // Recovery may have resolved a committed Finalize. Only return success from its proof.
            if (journal.Phase is "saved" or "committed" && MatchesFiles(store, journal.After))
            {
                using var proof = LiveSettingsClient.Connect(store.Root, journal.RequestId);
                if (Committed(proof.State, journal)) { store.Load(); store.BindRuntimeBaseline(proof.State); return; }
            }
            throw;
        }
    }

    private static bool Committed(LiveState s, LiveJournal j) => s.ProcessId == j.ProcessId && s.Instance == j.Instance &&
        s.Revision == j.Revision + 1 && s.Phase == 0 && s.Transaction == j.RequestId;
    private static bool MatchesFiles(SettingsStore store, Dictionary<string, byte[]> documents) => documents.All(p =>
    {
        string path = Path.Combine(store.Root, "vrmod", "config", p.Key); store.CheckRuntimePath(path);
        return File.Exists(path) && File.ReadAllBytes(path).AsSpan().SequenceEqual(p.Value);
    });

    // Recover only whitelisted documents and conditional before/after bytes; never erase another writer.
    internal static void Recover(SettingsStore store)
    {
        using var held = store.AcquireWriteLock();
        string path = JournalPath(store.Root); store.CheckRuntimePath(path);
        if (!File.Exists(path)) return;
        LiveJournal j;
        try
        {
            if (new FileInfo(path).Length > 524288) throw new InvalidDataException();
            j = JsonSerializer.Deserialize<LiveJournal>(File.ReadAllBytes(path))!;
            if (j == null || !Path.GetFullPath(j.Root).Equals(store.Root, StringComparison.OrdinalIgnoreCase) || j.RequestId == 0 ||
                j.Instance == 0 || j.ProcessId <= 0 || j.Revision == 0 || j.Revision == ulong.MaxValue ||
                j.Phase is not ("prepared" or "applied" or "saved" or "committed") || j.Before == null || j.After == null ||
                j.Before.Keys.Any(k => !Fields.Files.Contains(k)) || j.After.Keys.Any(k => !Fields.Files.Contains(k)) ||
                j.Before.Count != Fields.Files.Length || j.After.Count != Fields.Files.Length) throw new InvalidDataException();
            foreach (var p in j.Before) if (p.Value != null) _ = SettingsStore.Parse(p.Value, p.Key);
            SettingsStore.Validate(j.After.ToDictionary(p => p.Key, p => SettingsStore.Parse(p.Value, p.Key)));
        }
        catch { throw new SettingsError("liveRecoveryError"); }
        if (store.IsRunning())
        {
            using var client = LiveSettingsClient.Connect(store.Root, j.RequestId);
            // A rejected Prepare never owned the native transaction. Do not roll back
            // another editor, or strand our harmless journal after a native validation error.
            if (j.Phase == "prepared" && client.State.Transaction != j.RequestId && j.Before.All(p =>
            {
                string file = Path.Combine(store.Root, "vrmod", "config", p.Key); store.CheckRuntimePath(file);
                return p.Value == null ? !File.Exists(file) : File.Exists(file) && File.ReadAllBytes(file).AsSpan().SequenceEqual(p.Value);
            })) { File.Delete(path); return; }
            if (client.State.ProcessId != j.ProcessId || client.State.Instance != j.Instance)
                throw new SettingsError("liveRecoveryError");
            if (Committed(client.State, j))
            {
                if (!MatchesFiles(store, j.After)) throw new SettingsError("liveRecoveryError");
                File.Delete(path); store.Load(); store.BindRuntimeBaseline(client.State); return;
            }
            // Even when all files reached disk, an unfinalized transaction is compensated.
            client.Require(LiveCommand.Rollback, j.Revision, []);
        }
        else if (j.Phase is "saved" or "committed")
        {
            if (!MatchesFiles(store, j.After)) throw new SettingsError("liveRecoveryError");
            File.Delete(path); store.Load(); return;
        }
        store.RestoreLive(j.Before, j.After); File.Delete(path);
    }
}
