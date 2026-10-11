using System.Globalization;
using System.Text.Json.Nodes;

namespace GF2VR.Configurator;

internal sealed class MainForm : Form
{
    private SettingsStore? store;
    private Dictionary<string, JsonObject> editing = Fields.Defaults();
    private readonly TextBox rootBox = new() { Dock = DockStyle.Fill, ReadOnly = true };
    private readonly ComboBox language = new() { DropDownStyle = ComboBoxStyle.DropDownList, Width = 140 };
    private readonly TabControl tabs = new() { Dock = DockStyle.Fill };
    private readonly Label status = new() { Dock = DockStyle.Fill, AutoSize = false, AutoEllipsis = true, TextAlign = ContentAlignment.MiddleLeft, Tag = "loaded" };
    private readonly Label restart = new() { Dock = DockStyle.Fill, AutoSize = false, ForeColor = Color.FromArgb(71, 87, 110) };
    private readonly Label effectiveSpeed = new() { AutoSize = true, ForeColor = Color.FromArgb(33, 78, 110), MaximumSize = new Size(860, 0), Margin = new Padding(4, 12, 4, 8) };
    private readonly Button save = new() { AutoSize = true, MinimumSize = new Size(115, 34), Tag = "save" };
    private readonly Dictionary<string, Control> inputs = [];
    private readonly List<Control> localized = [];
    private readonly string? smokeFolder;
    private bool loading;
    private bool dirty;

    internal MainForm(string root, string? smoke = null)
    {
        smokeFolder = smoke;
        Font = new Font("Segoe UI", 10); MinimumSize = new Size(860, 700); Size = new Size(1080, 840);
        StartPosition = FormStartPosition.CenterScreen; BackColor = Color.FromArgb(244, 247, 251);
        Icon = SystemIcons.Application;
        if (smoke != null) { StartPosition = FormStartPosition.Manual; Location = new Point(-20000, -20000); ShowInTaskbar = false; }
        rootBox.Text = root;
        var layout = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 1, RowCount = 5, Padding = new Padding(18) };
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 72));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 48));
        layout.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 62));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 44));
        Controls.Add(layout);
        var banner = new Panel { Dock = DockStyle.Fill, BackColor = Color.FromArgb(30, 58, 92), Padding = new Padding(15, 5, 15, 5) };
        var title = Label("title"); title.Font = new Font(Font.FontFamily, 19, FontStyle.Bold); title.ForeColor = Color.White; title.Dock = DockStyle.Fill; title.AutoSize = false; title.TextAlign = ContentAlignment.MiddleLeft;
        banner.Controls.Add(title); layout.Controls.Add(banner, 0, 0);
        var row = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 4, Padding = new Padding(0, 8, 0, 4) };
        row.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize)); row.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        row.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize)); row.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        row.Controls.Add(Label("folder"), 0, 0); row.Controls.Add(rootBox, 1, 0);
        row.Controls.Add(Button("browse", Browse), 2, 0); row.Controls.Add(language, 3, 0);
        layout.Controls.Add(row, 0, 1);
        language.Items.AddRange(["한국어", "English", "日本語"]);
        language.SelectedIndexChanged += (_, _) =>
        {
            if (loading || language.SelectedIndex < 0) return;
            Texts.Language = Texts.Languages[language.SelectedIndex]; Translate();
            Run(() => { store?.SaveLanguage(Texts.Language); SetStatus("languageSaved"); });
        };
        layout.Controls.Add(tabs, 0, 2);
        BuildPage("renderTab", Fields.All.Where(f => f.File is "render_quality.json" or "stereo_depth.json"), "renderSummary");
        BuildPage("navigationTab", Fields.All.Where(f => f.File == "navigation.json"), "navigationSummary");
        BuildPage("dragTab", Fields.All.Where(f => f.File == "world_drag.json"), "conflictHelp");
        BuildPage("availabilityTab", [], "availabilityHelp");
        localized.Add(restart); restart.Tag = "restart"; layout.Controls.Add(restart, 0, 3);
        var footer = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 6 };
        footer.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        for (int i = 0; i < 5; ++i) footer.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        footer.Controls.Add(status, 0, 0);
        footer.Controls.Add(Button("reload", (_, _) => { if (Discard()) LoadRoot(rootBox.Text); }), 1, 0);
        footer.Controls.Add(Button("reset", (_, _) => { if (Confirm("resetQuestion")) ApplyDefaults(); }), 2, 0);
        footer.Controls.Add(Button("import", Import), 3, 0);
        footer.Controls.Add(Button("export", Export), 4, 0);
        localized.Add(save); save.Click += (_, _) => Run(() =>
        {
            if (store == null) throw new SettingsError("rootError");
            var requested = ReadControls(); store.Save(requested); editing = Fields.Clone(store.Documents);
            dirty = false; SetStatus("saved"); UpdateSave();
        });
        footer.Controls.Add(save, 5, 0); layout.Controls.Add(footer, 0, 4);
        FormClosing += (_, e) => { if (smokeFolder == null && !Discard()) e.Cancel = true; };
        LoadRoot(root); Translate();
        if (smoke != null) Shown += (_, _) => Smoke();
    }

    private Label Label(string key)
    {
        var label = new Label { Tag = key, AutoSize = true, Margin = new Padding(4, 5, 8, 5) };
        localized.Add(label); return label;
    }
    private Button Button(string key, EventHandler handler)
    {
        var b = new Button { Tag = key, AutoSize = true, MinimumSize = new Size(75, 34), Margin = new Padding(4, 0, 0, 0) };
        b.Click += handler; localized.Add(b); return b;
    }

    private void BuildPage(string key, IEnumerable<Field> fields, string summary)
    {
        var page = new TabPage { Tag = key, BackColor = Color.White, AutoScroll = true, Padding = new Padding(12) };
        localized.Add(page); tabs.TabPages.Add(page);
        var table = new TableLayoutPanel { Dock = DockStyle.Top, AutoSize = true, ColumnCount = 3, Padding = new Padding(6) };
        table.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 238));
        table.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 177));
        table.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        page.Controls.Add(table);
        var description = Label(summary); description.MaximumSize = new Size(870, 0); description.Margin = new Padding(2, 8, 2, 20);
        table.Controls.Add(description, 0, 0); table.SetColumnSpan(description, 3);
        int row = 1;
        foreach (Field f in fields)
        {
            table.Controls.Add(Label(f.Key), 0, row);
            Control input;
            if (f.Kind == "bool")
            {
                var check = new CheckBox { AutoSize = true, Margin = new Padding(8, 8, 8, 9) };
                check.CheckedChanged += (_, _) => Changed(); input = check;
            }
            else if (f.Kind == "choice")
            {
                var combo = new ComboBox { DropDownStyle = ComboBoxStyle.DropDownList, Width = 165 };
                combo.SelectedIndexChanged += (_, _) => Changed(); input = combo;
            }
            else
            {
                var text = new TextBox { Width = 160, Margin = new Padding(4, 5, 10, 9) };
                text.TextChanged += (_, _) => Changed(); input = text;
            }
            input.AccessibleName = f.Key; inputs.Add(f.Key, input); table.Controls.Add(input, 1, row);
            if (f.Help.Length > 0)
            {
                var help = Label(f.Help); help.ForeColor = Color.FromArgb(86, 101, 123); help.MaximumSize = new Size(495, 0);
                table.Controls.Add(help, 2, row);
            }
            row++;
        }
        if (key == "navigationTab") { table.Controls.Add(effectiveSpeed, 0, row); table.SetColumnSpan(effectiveSpeed, 3); }
    }

    private void Translate()
    {
        Text = Texts.Get("title");
        if (status.Tag is string state) status.Text = Texts.Get(state);
        foreach (Control c in localized) if (c.Tag is string key) c.Text = Texts.Get(key);
        foreach (Field f in Fields.All.Where(f => f.Kind == "choice"))
        {
            var combo = (ComboBox)inputs[f.Key]; int before = combo.SelectedIndex;
            bool wasLoading = loading; loading = true;
            combo.Items.Clear(); combo.Items.AddRange(f.Choices!.Select(c => (object)Texts.Get(c)).ToArray());
            combo.SelectedIndex = before; loading = wasLoading;
        }
        UpdateSave();
        UpdateEffectiveSpeed();
    }

    private void LoadRoot(string root)
    {
        Run(() =>
        {
            var candidate = new SettingsStore(root, smokeFolder != null); candidate.Load();
            // A damaged preference cannot replace valid runtime settings with defaults.
            string locale;
            try { locale = candidate.LoadLanguage(); } catch { locale = "ko"; }
            store = candidate; rootBox.Text = candidate.Root; editing = Fields.Clone(candidate.Documents);
            loading = true; Texts.Language = locale; language.SelectedIndex = Array.IndexOf(Texts.Languages, locale); loading = false;
            Translate(); ApplyControls(); dirty = false; SetStatus("loaded"); UpdateSave();
        }, failedLoad: true);
    }

    private void ApplyControls()
    {
        loading = true;
        try
        {
            foreach (Field f in Fields.All)
            {
                var value = Fields.Get(editing, f) ?? Fields.JsonSerializerNode(f.Default);
                Control c = inputs[f.Key];
                if (c is CheckBox cb) cb.Checked = value.GetValue<bool>();
                else if (c is ComboBox combo) combo.SelectedIndex = Array.IndexOf(f.Choices!, value.GetValue<string>());
                else c.Text = value.ToJsonString();
                c.Enabled = store != null;
            }
        }
        finally { loading = false; UpdateEffectiveSpeed(); }
    }

    internal Dictionary<string, JsonObject> ReadControls()
    {
        var result = Fields.Clone(editing);
        foreach (Field f in Fields.All)
        {
            Control c = inputs[f.Key]; JsonNode val;
            if (c is CheckBox cb) val = JsonValue.Create(cb.Checked)!;
            else if (c is ComboBox combo) val = JsonValue.Create(f.Choices![combo.SelectedIndex])!;
            else val = JsonValue.Create(SettingsStore.Number(c.Text, f.Key))!;
            Fields.Set(result, f, val);
        }
        SettingsStore.Validate(result); return result;
    }

    private void Changed()
    {
        if (loading) return;
        dirty = true; SetStatus("unsaved"); UpdateSave(); UpdateEffectiveSpeed();
    }
    private void UpdateEffectiveSpeed()
    {
        if (!inputs.TryGetValue("movementSpeed", out var speed) || !inputs.TryGetValue("locomotionScale", out var multiplier)) return;
        bool valid = double.TryParse(speed.Text, NumberStyles.Float, CultureInfo.InvariantCulture, out double s) && s >= 0 &&
            double.TryParse(multiplier.Text, NumberStyles.Float, CultureInfo.InvariantCulture, out double m) && m >= 0 && double.IsFinite(s * m);
        effectiveSpeed.Text = valid ? Texts.Format("effectiveSpeed", (s * double.Parse(multiplier.Text, CultureInfo.InvariantCulture)).ToString("G6", CultureInfo.InvariantCulture)) : Texts.Get("invalidSpeed");
    }
    private void SetStatus(string key) { status.Tag = key; status.Text = Texts.Get(key); }
    private void UpdateSave() => save.Enabled = store != null;
    private bool Confirm(string key) => MessageBox.Show(this, Texts.Get(key), Texts.Get("title"),
        MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes;
    private bool Discard() => !dirty || Confirm("discardQuestion");

    private void Browse(object? sender, EventArgs e)
    {
        if (!Discard()) return;
        using var dialog = new FolderBrowserDialog { Description = Texts.Get("folderHelp"), UseDescriptionForTitle = true };
        if (dialog.ShowDialog(this) == DialogResult.OK) LoadRoot(dialog.SelectedPath);
    }
    private void ApplyDefaults()
    {
        // Reset known fields only. Compatible unknown fields remain intact.
        foreach (var f in Fields.All) Fields.Set(editing, f, Fields.JsonSerializerNode(f.Default));
        ApplyControls(); dirty = true; SetStatus("defaultsPending");
    }
    private void Import(object? sender, EventArgs e)
    {
        if (!Discard()) return;
        using var dialog = new OpenFileDialog { Filter = Texts.Get("profileFilter"), CheckFileExists = true };
        if (dialog.ShowDialog(this) != DialogResult.OK) return;
        Run(() =>
        {
            var imported = SettingsStore.Import(dialog.FileName);
            // Apply known settings; preserve installed unknown fields independently of imported unknown metadata.
            foreach (var f in Fields.All) Fields.Set(editing, f, (Fields.Get(imported, f) ?? Fields.JsonSerializerNode(f.Default)).DeepClone());
            ApplyControls(); dirty = true; SetStatus("importPending");
        });
    }
    private void Export(object? sender, EventArgs e)
    {
        using var dialog = new SaveFileDialog { Filter = Texts.Get("profileFilter"), DefaultExt = "json", FileName = "GF2VR-profile.json" };
        if (dialog.ShowDialog(this) == DialogResult.OK) Run(() =>
        {
            SettingsStore.Export(dialog.FileName, ReadControls(), store?.Root); SetStatus("exported");
        });
    }
    private void Run(Action action, bool failedLoad = false)
    {
        try { action(); status.ForeColor = Color.FromArgb(33, 78, 110); }
        catch (Exception ex)
        {
            if (failedLoad)
            {
                store = null;
                foreach (Control c in inputs.Values) c.Enabled = false;
                save.Enabled = false;
            }
            SetStatus("error"); status.ForeColor = Color.Firebrick;
            if (smokeFolder == null) MessageBox.Show(this, Texts.Describe(ex), Texts.Get("error"), MessageBoxButtons.OK, MessageBoxIcon.Error);
            else throw;
        }
    }

    private void Smoke()
    {
        try
        {
            Texts.Verify(); Directory.CreateDirectory(smokeFolder!);
            // Real forms/resources, no runtime writes. Exercise language, all field creation, initial config and persistence mapping.
            var initial = ReadControls(); SettingsStore.Validate(initial);
            for (int i = 0; i < Texts.Languages.Length; i++)
            {
                Texts.Language = Texts.Languages[i]; loading = true; language.SelectedIndex = i; loading = false; Translate();
                tabs.SelectedIndex = i; PerformLayout(); tabs.SelectedTab!.PerformLayout();
                using var bitmap = new Bitmap(Width, Height); DrawToBitmap(bitmap, new Rectangle(0, 0, Width, Height));
                bitmap.Save(Path.Combine(smokeFolder!, Texts.Language + ".png"));
            }
            ((TextBox)inputs["eyeRenderScale"]).Text = "2.5";
            ((ComboBox)inputs["locomotionHand"]).SelectedIndex = 1;
            var edited = ReadControls();
            if (Fields.Get(edited, Fields.All[0])!.GetValue<double>() != 2.5 ||
                edited["navigation.json"]["navigation"]!["locomotionHand"]!.GetValue<string>() != "right")
                throw new InvalidDataException("GUI controls did not map to persistence");
            File.WriteAllText(Path.Combine(smokeFolder!, "gui-validation.json"), "{\"passed\":true,\"locales\":[\"ko\",\"en\",\"ja\"],\"fields\":18,\"liveSaves\":0,\"gameLaunches\":0}");
        }
        catch (Exception ex) { File.WriteAllText(Path.Combine(smokeFolder!, "gui-error.txt"), ex.ToString()); Environment.ExitCode = 1; }
        finally { Close(); }
    }
}
