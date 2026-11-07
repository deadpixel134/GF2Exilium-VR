using System.Drawing.Drawing2D;
using System.Globalization;
using System.Text.Json.Nodes;

namespace GF2VR.Configurator;

// This surface consumes the same field registry, validators and locale resources as the PC form.
// It has no game input injection and never calls Unity or graphics APIs in the game process.
internal sealed class VrSettingsEditor : IDisposable
{
    internal const int Width = 1280, Height = 960;
    internal sealed record Hit(string Id, Rectangle Bounds, Action Activate, bool Enabled);
    private SettingsStore store;
    private readonly Font text = new("Segoe UI", 24, FontStyle.Regular, GraphicsUnit.Pixel);
    private readonly Font heading = new("Segoe UI", 32, FontStyle.Bold, GraphicsUnit.Pixel);
    private readonly Font small = new("Segoe UI", 20, FontStyle.Regular, GraphicsUnit.Pixel);
    private readonly List<Hit> hits = [];
    private Dictionary<string, JsonObject> baseline;
    internal Dictionary<string, JsonObject> Editing { get; private set; }
    internal bool Open { get; private set; } = true;
    internal bool Busy => pending != null;
    internal bool Dirty => baseline.Any(p => !JsonNode.DeepEquals(p.Value, Editing[p.Key]));
    private bool RenderScaleChanged => (baseline["render_quality.json"]["eyeRenderScale"]?.GetValue<double>() ?? 1d) !=
        Value(Find("eyeRenderScale")).GetValue<double>();
    internal bool Redraw { get; private set; } = true;
    internal IReadOnlyList<Hit> Hits => hits;
    private Task<SettingsStore>? pending;
    private bool reloading, discardReload;
    private int group, page;
    private string message = "", pressed = "";
    private Field? number;
    private Field? choice;
    private Field? help;
    private string numericText = "";
    private bool discard, groups;
    private static readonly string[] Groups = ["renderTab", "worldScaleTab", "navigationTab", "dragTab", "activityUiTab", "controllerTab", "handsTab", "panelTab",
#if GF2VR_DIRTY_CONTACT
        "jiggleTab",
#endif
        "diagnosticsTab"];

    internal VrSettingsEditor(SettingsStore settings)
    { store = settings; baseline = Fields.Clone(settings.Documents); Editing = Fields.Clone(baseline); }

    internal static string Group(Field f) => f.Group == "diagnostics" ? "diagnosticsTab" : f.Group == "auxiliaryPanel" ? "panelTab" :
        f.Group == "controllerInput" ? "controllerTab" : f.File switch
        {
            "world_scale.json" or "stereo_depth.json" => "worldScaleTab", "navigation.json" => "navigationTab", "world_drag.json" => "dragTab",
            "activity_ui.json" => "activityUiTab", "controller_hands.json" => "handsTab", "touch_interaction.json" => "jiggleTab", _ => "renderTab"
        };
    private Field Find(string key) => Fields.All.Single(f => f.Key == key);
    private JsonNode Value(Field f) => Fields.Get(Editing, f) ?? Fields.JsonSerializerNode(f.Default);
    private bool Flag(string key) => Value(Find(key)).GetValue<bool>();
    internal bool Available(Field f) => (!Fields.IsContactHaptic(f.Key) || store.ContactHapticsAvailable) &&
        (f.File != "performance.json" || store.PcOutputAvailable) &&
        (f.Key != "stereoDepthScale" || !Flag("linkStereoDepthToWorldScale")) &&
        (!(f.Key is "panelRotationPitch" or "panelRotationYaw" or "panelRotationRoll" or "panelScreenTilt") ||
            !Flag("panelViewerFacing") || Flag("panelApplyAnglesWhenFacing"));

    internal void Set(Field f, JsonNode value)
    {
        if (Busy || !Available(f)) return;
        if (f.Key == "linkStereoDepthToWorldScale" && Flag(f.Key) && !value.GetValue<bool>())
            Fields.Set(Editing, Find("stereoDepthScale"), JsonValue.Create(1 / Value(Find("worldScale")).GetValue<double>())!);
        Fields.Set(Editing, f, value); message = ""; Redraw = true;
    }
    private string Display(Field f)
    {
        if (f.Key == "stereoDepthScale" && Flag("linkStereoDepthToWorldScale"))
            return (1 / Value(Find("worldScale")).GetValue<double>()).ToString("G8", CultureInfo.InvariantCulture) + " · " + Texts.Get("liveLinked");
        var value = Value(f);
        return f.Kind == "bool" ? Texts.Get(value.GetValue<bool>() ? "liveOn" : "liveOff") : f.Kind == "choice" ? Fields.ChoiceText(f, value.GetValue<string>()) : value.ToJsonString();
    }
    private void Edit(Field f)
    {
        if (f.Kind == "bool") Set(f, JsonValue.Create(!Value(f).GetValue<bool>())!);
        else if (f.Kind == "choice") choice = f;
        else { number = f; numericText = Value(f).ToJsonString(); }
        Redraw = true;
    }
    private void Adjust(Field f, int direction)
    {
        if (Busy || !Available(f)) return;
        if (f.Kind == "bool") { Set(f, JsonValue.Create(!Value(f).GetValue<bool>())!); return; }
        if (f.Kind == "choice")
        {
            var options = Fields.ChoicesFor(Editing, f);
            int current = Array.IndexOf(options, Value(f).GetValue<string>());
            Set(f, JsonValue.Create(options[(current + direction + options.Length) % options.Length])!); return;
        }
        double step = f.Key.Contains("Percent", StringComparison.Ordinal) ? 5 : f.Key.Contains("Degrees", StringComparison.Ordinal) ? 5 :
            f.Key.Contains("DurationMs", StringComparison.Ordinal) ? 5 : f.Group == "auxiliaryPanel" ? (f.Key.Contains("Rotation", StringComparison.Ordinal) || f.Key == "panelScreenTilt" ? 5 : .01) : .05;
        double value = Value(f).GetValue<double>() + direction * step;
        try
        {
            var proposed = Fields.Clone(Editing); Fields.Set(proposed, f, JsonValue.Create(value)!); SettingsStore.Validate(proposed);
            Set(f, JsonValue.Create(value)!);
        }
        catch (Exception ex) { message = Texts.Describe(ex); Redraw = true; }
    }
    internal void ToggleOpen()
    {
        if (Busy) return;
        if (Open) Close();
        else { Open = true; message = ""; Redraw = true; }
    }
    internal void EnsureOpen() { if (!Busy) { Open = true; Redraw = true; } }
    internal void Invalidate() => Redraw = true;
    internal void Close()
    {
        if (Busy) return;
        CancelPointer();
        groups = false;
        discardReload = false;
        bool numberDraft = number != null && numericText != Value(number).ToJsonString();
        if (Dirty || numberDraft) discard = true;
        else { Open = false; number = choice = help = null; groups = false; }
        Redraw = true;
    }
    internal void Reload()
    {
        if (Busy) return;
        CancelPointer();
        if (Dirty || (number != null && numericText != Value(number).ToJsonString()))
        { discard = true; discardReload = true; Redraw = true; }
        else BeginReload();
    }
    private void BeginReload()
    {
        number = choice = help = null; groups = false; reloading = true; message = Texts.Get("reload");
        pending = Task.Run(store.ReloadSnapshot); Redraw = true;
    }
    internal void CancelPointer() { pressed = ""; }
    internal void Pointer(int type, int x, int y)
    {
        if (type == 3) { CancelPointer(); return; }
        var hit = hits.LastOrDefault(h => h.Enabled && h.Bounds.Contains(x, y));
        if (type == 1) pressed = hit?.Id ?? "";
        else if (type == 2)
        {
            string held = pressed; pressed = "";
            if (hit != null && hit.Id == held) { hit.Activate(); Redraw = true; }
        }
    }
    internal void Apply()
    {
        if (Busy) return;
        try
        {
            var snapshot = Fields.Clone(Editing); SettingsStore.Validate(snapshot);
            message = Texts.Get(RenderScaleChanged ? "liveRenderScaleApplying" : "liveApplying"); CancelPointer();
            pending = Task.Run(() => { store.Save(snapshot); return store; }); Redraw = true;
        }
        catch (Exception ex) { message = Texts.Describe(ex); Redraw = true; }
    }
    internal void Tick()
    {
        if (pending?.IsCompleted != true) return;
        try
        {
            store = pending.GetAwaiter().GetResult();
            baseline = Fields.Clone(store.Documents); Editing = Fields.Clone(baseline); message = Texts.Get(reloading ? "loaded" : "liveApplied");
        }
        catch (Exception ex) { message = Texts.Describe(ex); }
        finally { pending = null; reloading = false; Redraw = true; }
    }

    internal Bitmap Render()
    {
        var image = new Bitmap(Width, Height, System.Drawing.Imaging.PixelFormat.Format32bppArgb);
        using var g = Graphics.FromImage(image); g.Clear(Color.FromArgb(16, 24, 39));
        g.SmoothingMode = SmoothingMode.AntiAlias; g.TextRenderingHint = System.Drawing.Text.TextRenderingHint.AntiAliasGridFit;
        hits.Clear();
        void Label(string value, Rectangle bounds, Font? font = null, Color? color = null)
        { using var brush = new SolidBrush(color ?? Color.White); g.DrawString(value, font ?? text, brush, bounds); }
        void Button(string id, string label, Rectangle bounds, Action action, bool enabled = true, bool selected = false)
        {
            bool field = id.StartsWith("field:", StringComparison.Ordinal);
            using var background = new SolidBrush(!enabled || field ? Color.FromArgb(26, 39, 60) : selected ? Color.FromArgb(22, 109, 99) : Color.FromArgb(48, 66, 93));
            g.FillRectangle(background, bounds);
            Label(label, Rectangle.Inflate(bounds, -16, -12), text, !enabled ? Color.FromArgb(126, 139, 159) : field ? Color.FromArgb(125, 226, 206) : Color.FromArgb(231, 237, 247));
            hits.Add(new(id, bounds, action, enabled));
        }
        using (var accent = new SolidBrush(Color.FromArgb(66, 185, 164))) g.FillRectangle(accent, 0, 0, Width, 5);
        Label(Texts.Get("title") + (Dirty ? " *" : ""), new(37, 32, 850, 52), heading);
        int locale = Array.IndexOf(Texts.Languages, Texts.Normalize(Texts.Language));
        bool modal = number != null || choice != null || help != null || discard || groups;
        Button("language", new[] { "한국어", "English", "日本語", "简体中文" }[locale] + "  >", new(933, 24, 315, 60),
            () => { string language = Texts.Languages[(locale + 1) % Texts.Languages.Length]; Texts.Language = language; store.SaveLanguage(language); }, !Busy && !modal);
        var fields = Fields.All.Where(f => Group(f) == Groups[group]).ToArray();
        const int perPage = 6;
        page = Math.Clamp(page, 0, Math.Max(0, (fields.Length - 1) / perPage));
        Label(Texts.Format("livePageHint", page + 1, Math.Max(1, (fields.Length + perPage - 1) / perPage)), new(37, 85, 1190, 42), small, Color.LightSteelBlue);
        if (!modal)
        {
            Button("groups", Texts.Get(Groups[group]) + "  >", new(32, 122, 1216, 64), () => groups = true, !Busy, true);
            foreach (var (field, index) in fields.Skip(page * perPage).Take(perPage).Select((f, i) => (f, i)))
            {
                int y = 198 + index * 94;
                using (var row = new SolidBrush(Color.FromArgb(26, 39, 60))) g.FillRectangle(row, 32, y, 1216, 80);
                Label(Texts.Get(field.Key) + (field.Help.Length != 0 ? " ⓘ" : ""), new(48, y + 6, 616, 40));
                string detail = Texts.Get(Groups[group]) + (!Available(field) ? " · " + Texts.Get("unavailable") : "");
                Label(detail, new(48, y + 48, 616, 30), small, Color.LightSteelBlue);
                if (field.Help.Length != 0) hits.Add(new("help:" + field.Key, new(32, y, 640, 80), () => help = field, !Busy));
                Button("decrease:" + field.Key, "−", new(680, y, 86, 80), () => Adjust(field, -1), !Busy && Available(field));
                Button("field:" + field.Key, Display(field), new(772, y, 376, 80), () => Edit(field), !Busy && Available(field));
                Button("increase:" + field.Key, "+", new(1154, y, 86, 80), () => Adjust(field, 1), !Busy && Available(field));
            }
            Button("previous", Texts.Get("livePrevious"), new(32, 853, 235, 75), () => --page, !Busy && page > 0);
            Button("next", Texts.Get("liveNext"), new(277, 853, 235, 75), () => ++page, !Busy && (page + 1) * perPage < fields.Length);
            Button("apply", Texts.Get("liveApply"), new(522, 853, 235, 75), Apply, !Busy, true);
            Button("reload", Texts.Get("reload"), new(767, 853, 235, 75), Reload, !Busy);
            Button("close", Texts.Get("liveClose"), new(1012, 853, 235, 75), Close, !Busy);
        }
        else
        {
            // Modal controls replace the hit set; a release can never activate a covered game-setting control.
            hits.Clear();
            if (groups)
            {
                for (int i = 0; i < Groups.Length; ++i)
                {
                    int selected = i;
                    Button("group:" + Groups[i], Texts.Get(Groups[i]), new(32 + i % 2 * 620, 160 + i / 2 * 110, 596, 94),
                        () => { group = selected; page = 0; groups = false; }, !Busy, group == i);
                }
                Button("cancel", Texts.Get("liveCancel"), new(800, 770, 400, 75), () => groups = false);
            }
            else if (discard)
            {
                Label(Texts.Get("discardQuestion"), new(320, 220, 880, 180), heading);
                Button("discard", Texts.Get(discardReload ? "reload" : "liveDiscard"), new(320, 470, 450, 100), () =>
                {
                    bool reload = discardReload; discard = discardReload = false;
                    if (reload) BeginReload();
                    else { Editing = Fields.Clone(baseline); Open = false; number = choice = help = null; }
                });
                Button("cancel", Texts.Get("liveCancel"), new(800, 470, 400, 100), () => { discard = discardReload = false; });
            }
            else if (help != null)
            {
                Label(Texts.Get(help.Key), new(310, 110, 920, 110), heading);
                Label(Texts.Get(help.Help), new(310, 240, 920, 440));
                Button("cancel", Texts.Get("liveConfirm"), new(800, 720, 400, 90), () => help = null);
            }
            else if (choice != null)
            {
                Field selected = choice; Label(Texts.Get(selected.Key), new(310, 110, 920, 76), heading);
                var options = Fields.ChoicesFor(Editing, selected);
                for (int i = 0; i < options.Length; ++i)
                {
                    string option = options[i];
                    Button("choice:" + option, Fields.ChoiceText(selected, option), new(310 + (i % 2) * 470, 214 + (i / 2) * 112, 450, 96),
                        () => { Set(selected, JsonValue.Create(option)!); choice = null; }, !Busy, Value(selected).GetValue<string>() == option);
                }
                Button("cancel", Texts.Get("liveCancel"), new(800, 720, 400, 90), () => choice = null);
            }
            else if (number != null)
            {
                Label(Texts.Get(number.Key), new(310, 102, 920, 70), heading);
                Label(numericText, new(310, 184, 920, 80), heading, Color.Aquamarine);
                string[] keys = ["7", "8", "9", "-", "4", "5", "6", "+", "1", "2", "3", "e", "0", ".", "⌫", "C"];
                for (int i = 0; i < keys.Length; ++i)
                {
                    string key = keys[i];
                    Button("number:" + key, key == "C" ? Texts.Get("liveClear") : key, new(310 + (i % 4) * 232, 282 + (i / 4) * 102, 214, 86), () =>
                    {
                        if (key == "C") numericText = "";
                        else if (key == "⌫") { if (numericText.Length != 0) numericText = numericText[..^1]; }
                        else if (numericText.Length < 4096) numericText += key;
                    });
                }
                Button("number:confirm", Texts.Get("liveConfirm"), new(310, 720, 450, 90), () =>
                {
                    try { Set(number, JsonValue.Create(SettingsStore.Number(numericText, number.Key))!); number = null; message = ""; }
                    catch (Exception ex) { message = Texts.Describe(ex); }
                });
                Button("cancel", Texts.Get("liveCancel"), new(790, 720, 450, 90), () => number = null);
            }
        }
        string footer = message;
        if (footer.Length == 0 && !modal && RenderScaleChanged) footer = Texts.Get("liveRenderScaleWait");
        if (footer.Length == 0 && !modal)
        {
            double world = Value(Find("worldScale")).GetValue<double>();
            double movement = Flag("linkMovementSpeedToWorldScale") ? world : 1;
            double speed = Value(Find("movementSpeed")).GetValue<double>() * Value(Find("locomotionScale")).GetValue<double>() * movement;
            footer = Texts.Format("effectiveSpatial", world.ToString("G6", CultureInfo.InvariantCulture), (1 / world).ToString("G6", CultureInfo.InvariantCulture), movement.ToString("G6", CultureInfo.InvariantCulture)) + "\n" +
                Texts.Format("effectiveSpeed", speed.ToString("G6", CultureInfo.InvariantCulture));
        }
        Label(footer, new(37, modal ? 852 : 770, 1200, modal ? 100 : 78), small, Color.FromArgb(125, 226, 206));
        Redraw = false; return image;
    }
    public void Dispose() { text.Dispose(); heading.Dispose(); small.Dispose(); }
}
