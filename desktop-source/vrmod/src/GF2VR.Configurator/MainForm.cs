using System.Globalization;
using System.Text.Json.Nodes;
using GF2VR.Installer;

namespace GF2VR.Configurator;

internal sealed partial class MainForm : Form
{
    private SettingsStore? store;
    private Dictionary<string, JsonObject> editing = Fields.Defaults();
    private readonly TextBox rootBox = new() { Anchor = AnchorStyles.Left | AnchorStyles.Right, ReadOnly = true };
    private readonly FlowLayoutPanel languages = new() { Dock = DockStyle.Fill, Margin = Padding.Empty, FlowDirection = FlowDirection.LeftToRight, WrapContents = false };
    private readonly FlowLayoutPanel actions = new() { Dock = DockStyle.Fill, Margin = Padding.Empty, FlowDirection = FlowDirection.RightToLeft, WrapContents = true };
    private readonly TabControl tabs = new() { Dock = DockStyle.Fill };
    private readonly Label status = new() { Dock = DockStyle.Fill, AutoSize = false, AutoEllipsis = true, TextAlign = ContentAlignment.MiddleLeft, Tag = "loaded" };
    private readonly Label restart = new() { Dock = DockStyle.Fill, AutoSize = false, ForeColor = Color.FromArgb(71, 87, 110) };
    private readonly Label effectiveSpeed = new() { AutoSize = true, ForeColor = Color.FromArgb(33, 78, 110), MaximumSize = new Size(860, 0), Margin = new Padding(4, 12, 4, 8) };
    private readonly Label effectiveSpatial = new() { AutoSize = true, ForeColor = Color.FromArgb(33, 78, 110), MaximumSize = new Size(860, 0), Margin = new Padding(4, 12, 4, 8) };
    private string manualDepthText = "1";
    private bool depthWasLinked;
    private readonly Button save = new() { AutoSize = true, MinimumSize = new Size(115, 34), Margin = new Padding(4), Tag = "save" };
    private readonly Dictionary<string, Control> inputs = [];
    private readonly List<Control> localized = [];
    private readonly string? smokeFolder;
    private bool loading;
    private bool dirty;
    private bool panelConfigurationTouched;
    private bool pcOutputConfigurationTouched;
    private Label? pcOutputAvailability;
    private readonly Label updateStatus = new() { Dock = DockStyle.Fill, AutoEllipsis = true, TextAlign = ContentAlignment.MiddleLeft, ForeColor = Color.FromArgb(71, 87, 110) };
    private readonly CancellationTokenSource updateCancel = new();
    private string updateKey = "updateIdle";
    private InstallError? updateRefusal;
    private readonly ToolTip updateHint = new();
    private readonly Button updateButton = new() { AutoSize = true, MinimumSize = new Size(110, 34), Margin = new Padding(4), Text = "" };
    private (ReleaseCandidate Release, UpdateDescriptor Descriptor, byte[] Bytes, string Root)? readyUpdate;
    private string? updateVersion;
    private readonly Button releasePage = new() { AutoSize = true, MinimumSize = new Size(110, 34), Margin = new Padding(4), Visible = false };
    internal static Action<string> OpenReleasePage = url => { using var p = System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo(url) { UseShellExecute = true }); };
    internal static Action<string> OpenLogFolder = path => { using var p = System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo(path) { UseShellExecute = true }); };
    private Button? logFolderButton;
    private Label? logFolderLocation;
    private string logFolderClick = "notRun";
    // Native runtime_logging.h creates these only while logging is ON; the GUI never creates them.
    internal static string LogFolder(string root) => Path.Combine(root, "vrmod", "logs");
    internal static string? ExistingLogFolder(string root)
    {
        string logs = LogFolder(root), bootstrap = Path.Combine(logs, "bootstrap");
        return Directory.Exists(bootstrap) ? bootstrap : Directory.Exists(logs) ? logs : null;
    }

    internal MainForm(string root, string? smoke = null)
    {
        smokeFolder = smoke;
        Font = new Font("Segoe UI", 10); MinimumSize = new Size(900, 760); ClientSize = new Size(1080, 900); AutoScaleMode = AutoScaleMode.Dpi;
        StartPosition = FormStartPosition.CenterScreen; BackColor = Color.FromArgb(244, 247, 251);
        Icon = SystemIcons.Application;
        if (smoke != null) { StartPosition = FormStartPosition.Manual; Location = new Point(-20000, -20000); ShowInTaskbar = false; }
        rootBox.Text = root;
        var layout = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 1, RowCount = 5, Padding = new Padding(12) };
        layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 62));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 50));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 26));
        Controls.Add(layout);
        updateButton.Visible = false; updateButton.Click += async (_, _) => await ApplyUpdate();
        var row = new TableLayoutPanel { Dock = DockStyle.Fill, AutoSize = true, ColumnCount = 3, Padding = new Padding(0, 0, 0, 8), Margin = Padding.Empty };
        row.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize)); row.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        row.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        var folderLabel = Label("folder"); folderLabel.Anchor = AnchorStyles.Left;
        var browse = Button("browse", Browse); browse.Anchor = AnchorStyles.Right;
        row.Controls.Add(folderLabel, 0, 0); row.Controls.Add(rootBox, 1, 0);
        row.Controls.Add(browse, 2, 0);
        layout.Controls.Add(row, 0, 0);
        foreach (var item in new[] { ("한국어", "ko"), ("English", "en"), ("日本語", "ja"), ("简体中文", "zh-CN") })
        {
            var button = new Button { Text = item.Item1, AutoSize = true, MinimumSize = new Size(75, 34), Margin = new Padding(4), AccessibleName = "language-" + item.Item2 };
            string code = item.Item2; button.Click += (_, _) => SwitchLanguage(code); languages.Controls.Add(button);
        }
        layout.Controls.Add(tabs, 0, 1);
        BuildPage("renderTab", Fields.All.Where(f => f.File is "render_quality.json" or "performance.json"), "renderSummary");
        BuildPage("navigationTab", Fields.All.Where(f => f.File == "navigation.json"), "navigationSummary");
        BuildPage("dragTab", Fields.All.Where(f => f.File == "world_drag.json"), "conflictHelp");
        BuildPage("activityUiTab", Fields.All.Where(f => f.File == "activity_ui.json"), "activityUiSummary");
        BuildPage("controllerTab", Fields.All.Where(f => f.Group == "controllerInput"), "controllerSummary");
        BuildPage("worldScaleTab", new[] { "worldScale", "linkStereoDepthToWorldScale", "stereoDepthScale", "linkMovementSpeedToWorldScale" }.Select(k => Fields.All.Single(f => f.Key == k)), "worldScaleSummary");
        BuildPage("handsTab", Fields.All.Where(f => f.File == "controller_hands.json"), "handsSummary");
        BuildPage("panelTab", Fields.All.Where(f => f.Group == "auxiliaryPanel"), "panelSummary");
#if GF2VR_DIRTY_CONTACT
        BuildJigglePage();
#endif
        BuildPage("diagnosticsTab", Fields.All.Where(f => f.Group == "diagnostics"), "diagnosticsSummary");
        localized.Add(restart); restart.Tag = "restart"; restart.Margin = new Padding(3, 8, 3, 4); layout.Controls.Add(restart, 0, 2);
        var footer = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 2, Padding = new Padding(0, 4, 0, 4), Margin = Padding.Empty };
        footer.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize)); footer.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        footer.Controls.Add(languages, 0, 0); footer.Controls.Add(actions, 1, 0);
        localized.Add(save); save.Click += (_, _) => SaveSettings();
        actions.Controls.Add(save);
        actions.Controls.Add(Button("reload", (_, _) => { if (Discard()) LoadRoot(rootBox.Text); }));
        actions.Controls.Add(Button("reset", (_, _) => { if (Confirm("resetQuestion")) ApplyDefaults(); }));
        actions.Controls.Add(Button("export", Export)); actions.Controls.Add(Button("import", Import)); actions.Controls.Add(updateButton);
        releasePage.Click += (_, _) => Run(() => { if (updateVersion != null) OpenReleasePage(UpdatePolicy.ReleasePage(updateVersion)); });
        actions.Controls.Add(releasePage);
        // Measure against the available width, including margins and wrapped rows.
        // Unconstrained AutoSize on a wrapping FlowLayoutPanel reserves excess height.
        languages.MinimumSize = languages.GetPreferredSize(Size.Empty);
        void FitFooter()
        {
            if (actions.ClientSize.Width <= 0) return;
            int languageWidth = languages.Padding.Horizontal + languages.Controls.Cast<Control>()
                .Sum(c => Math.Max(c.Width, c.GetPreferredSize(Size.Empty).Width) + c.Margin.Horizontal);
            if (languages.MinimumSize.Width != languageWidth)
                languages.MinimumSize = new Size(languageWidth, languages.MinimumSize.Height);
            int height = footer.Padding.Vertical + Math.Max(
                languages.GetPreferredSize(new Size(languages.ClientSize.Width, 0)).Height,
                actions.GetPreferredSize(new Size(actions.ClientSize.Width, 0)).Height);
            if (layout.RowStyles[3].Height != height) layout.RowStyles[3].Height = height;
        }
        footer.Layout += (_, _) => FitFooter();
        actions.Layout += (_, _) => FitFooter();
        layout.Controls.Add(footer, 0, 3);
        var statusRow = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 2 };
        statusRow.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 45)); statusRow.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 55));
        status.Height = updateStatus.Height = 24; statusRow.Controls.Add(status, 0, 0); statusRow.Controls.Add(updateStatus, 1, 0);
        layout.Controls.Add(statusRow, 0, 4);
        updateStatus.Click += (_, _) =>
        {
            if (updateRefusal != null)
                MessageBox.Show(this, Texts.UpdateError(updateRefusal), Texts.Get("updateDetails"), MessageBoxButtons.OK, MessageBoxIcon.Information);
        };
        FormClosing += (_, e) => { if (smokeFolder == null && !Discard()) e.Cancel = true; };
        LoadRoot(root); Translate();
        if (smoke != null) Shown += (_, _) => Smoke();
        else Shown += async (_, _) => await CheckUpdates();
        FormClosed += (_, _) => { updateCancel.Cancel(); updateHint.Dispose(); };
    }

    private void SwitchLanguage(string locale)
    {
        if (loading) return;
        Texts.Language = Texts.Normalize(locale); Translate();
        Run(() => { store?.SaveLanguage(Texts.Language); SetStatus("languageSaved"); });
    }

    private void UpdateState(string key, InstallError? refusal = null)
    {
        updateKey = key; updateRefusal = refusal;
        updateStatus.Text = updateVersion != null && key == "updateManual" ? Texts.Format("updateManualVersion", updateVersion) :
            key == "updateAvailable" && updateVersion != null ? Texts.Format("updateAvailableVersion", updateVersion) : refusal != null ? Texts.Get("updateDetails") : Texts.Get(key);
        updateStatus.Cursor = refusal == null ? Cursors.Default : Cursors.Hand;
        updateStatus.AccessibleName = updateStatus.Text + (refusal == null ? "" : "\n" + Texts.UpdateError(refusal));
        updateHint.SetToolTip(updateStatus, updateStatus.AccessibleName);
        updateButton.Text = Texts.Get("updateAction");
        releasePage.Text = Texts.Get("updateOpenRelease");
        releasePage.Visible = updateVersion != null;
    }
    private async Task CheckUpdates()
    {
        string root = store?.Root ?? rootBox.Text;
        try
        {
            UpdateState("updateChecking"); var service = new UpdateService();
            var check = await service.Check(root, updateCancel.Token);
            if (IsDisposed || !string.Equals(rootBox.Text, root, StringComparison.OrdinalIgnoreCase) || updateCancel.IsCancellationRequested) return;
            if (store == null && check.DiscoveredVersion != null)
                check = check with { Release = null, Descriptor = null, Bytes = null, State = "updateManual", Refusal = new InstallError("settingsUnavailable") };
            updateVersion = check.DiscoveredVersion; readyUpdate = null; updateButton.Visible = false;
            if (check.Release != null && check.Descriptor != null && check.Bytes != null)
            { readyUpdate = (check.Release, check.Descriptor, check.Bytes, root); updateButton.Visible = true; }
            UpdateState(check.State, check.Refusal);
            try
            {
                if (store != null && updateVersion != null && store.ShouldNotifyUpdate(updateVersion))
                {
                    store.MarkUpdateNotice(updateVersion, false);
                    using var notice = new UpdateNoticeDialog(updateVersion, readyUpdate != null);
                    notice.ShowDialog(this);
                    if (notice.SkipVersion) store.MarkUpdateNotice(updateVersion, true);
                }
            }
            catch (Exception) { updateHint.SetToolTip(updateStatus, updateStatus.AccessibleName + "\n" + Texts.Get("updateNoticeSaveFailed")); }
        }
        catch (OperationCanceledException) { }
        catch (Exception ex) { if (!IsDisposed) UpdateState("updateFailed", ex as InstallError ?? new InstallError("updateInvalid")); }
    }
    private async Task ApplyUpdate()
    {
        if (readyUpdate is not { } ready || store?.Root != ready.Root) return;
        try
        {
            if (MessageBox.Show(this, Texts.Format("updateQuestion", ready.Release.Version), Texts.Get("title"), MessageBoxButtons.YesNo, MessageBoxIcon.Question) != DialogResult.Yes) return;
            if (!Discard()) return;
            Enabled = false; UpdateState("updateDownloading");
            var staged = await new UpdateService().Stage(ready.Root, ready.Release, ready.Descriptor, ready.Bytes, updateCancel.Token);
            if (IsDisposed || updateCancel.IsCancellationRequested) return;
            UpdateStaging.Handoff(staged); dirty = false; Close();
        }
        catch (OperationCanceledException) { }
        catch (Exception ex) { if (!IsDisposed) UpdateState("updateFailed", ex as InstallError ?? new InstallError("updateInvalid")); } // No raw auth/transport stderr in UI/log.
        finally { if (!IsDisposed) Enabled = true; }
    }

    private Label Label(string key)
    {
        var label = new Label { Tag = key, AutoSize = true, Margin = new Padding(4, 5, 8, 5) };
        localized.Add(label); return label;
    }
    private Button Button(string key, EventHandler handler)
    {
        var b = new Button { Tag = key, AutoSize = true, MinimumSize = new Size(75, 34), Margin = new Padding(4) };
        b.Click += handler; localized.Add(b); return b;
    }

    private void BuildPage(string key, IEnumerable<Field> fields, string summary, TabControl? owner = null)
    {
        var page = new TabPage { Tag = key, BackColor = Color.White, AutoScroll = true, Padding = new Padding(12) };
        localized.Add(page); (owner ?? tabs).TabPages.Add(page);
        var table = new TableLayoutPanel { Dock = DockStyle.Top, AutoSize = true, AutoSizeMode = AutoSizeMode.GrowAndShrink, GrowStyle = TableLayoutPanelGrowStyle.AddRows, ColumnCount = 2, Padding = new Padding(16) };
        table.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 42));
        table.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 58));
        page.Controls.Add(table);
        var description = Label(summary); description.MaximumSize = new Size(870, 0); description.Margin = new Padding(2, 8, 2, 20);
        table.SizeChanged += (_, _) => description.MaximumSize = new Size(Math.Max(120, table.ClientSize.Width - table.Padding.Horizontal - 8), 0);
        table.Controls.Add(description, 0, 0); table.SetColumnSpan(description, 2);
        int row = 1;
        foreach (Field f in fields)
        {
            void Edited() { if (!loading && f.Group == "auxiliaryPanel") panelConfigurationTouched = true; if (!loading && f.File == "performance.json") pcOutputConfigurationTouched = true; Changed(); }
            table.Controls.Add(Label(f.Key), 0, row);
            Control input;
            if (f.Kind == "bool")
            {
                var check = new CheckBox { AutoSize = true, Margin = new Padding(8, 8, 8, 9) };
                check.CheckedChanged += (_, _) => Edited(); input = check;
            }
            else if (f.Kind == "choice")
            {
                var combo = new ComboBox { DropDownStyle = ComboBoxStyle.DropDownList, Width = f.Key == "pcOutputPreset" ? 230 : 165 };
                combo.SelectedIndexChanged += (_, _) => Edited(); input = combo;
            }
            else
            {
                var text = new TextBox { Width = 160, Margin = new Padding(4, 5, 10, 9) };
                text.TextChanged += (_, _) => Edited(); input = text;
            }
            input.AccessibleName = f.Key; inputs.Add(f.Key, input);
            var editor = new FlowLayoutPanel { Dock = DockStyle.Fill, AutoSize = true, AutoSizeMode = AutoSizeMode.GrowAndShrink, FlowDirection = FlowDirection.TopDown, WrapContents = false, Margin = new Padding(3, 4, 3, 4) };
            editor.Controls.Add(input); table.Controls.Add(editor, 1, row);
            if (f.Help.Length > 0)
            {
                var help = Label(f.Help); help.ForeColor = Color.FromArgb(86, 101, 123); help.MaximumSize = new Size(495, 0);
                editor.SizeChanged += (_, _) => help.MaximumSize = new Size(Math.Max(120, editor.ClientSize.Width - 8), 0);
                help.Margin = new Padding(4, 0, 4, 8); editor.Controls.Add(help);
            }
            row++;
        }
        if (key == "navigationTab") { table.Controls.Add(effectiveSpeed, 0, row); table.SetColumnSpan(effectiveSpeed, 2); }
        if (key == "worldScaleTab") { table.Controls.Add(effectiveSpatial, 0, row); table.SetColumnSpan(effectiveSpatial, 2); }
        if (key == "renderTab") { pcOutputAvailability = Label("pcOutputUnavailable"); table.Controls.Add(pcOutputAvailability, 0, row); table.SetColumnSpan(pcOutputAvailability, 2); }
        if (key == "diagnosticsTab")
        {
            table.Controls.Add(Label("logFolder"), 0, row);
            logFolderButton = Button("openLogFolder", OpenLogs); logFolderButton.AccessibleName = "openLogFolder"; logFolderButton.Margin = new Padding(8, 4, 8, 6);
            logFolderLocation = new Label { AutoSize = true, ForeColor = Color.FromArgb(86, 101, 123), MaximumSize = new Size(495, 0), Margin = new Padding(4, 0, 4, 8) };
            var logs = new FlowLayoutPanel { Dock = DockStyle.Fill, AutoSize = true, AutoSizeMode = AutoSizeMode.GrowAndShrink, FlowDirection = FlowDirection.TopDown, WrapContents = false, Margin = new Padding(3, 4, 3, 4) };
            logs.Controls.Add(logFolderButton); logs.Controls.Add(logFolderLocation);
            logs.SizeChanged += (_, _) => logFolderLocation.MaximumSize = new Size(Math.Max(120, logs.ClientSize.Width - 8), 0);
            table.Controls.Add(logs, 1, row);
        }
    }

    private void OpenLogs(object? sender, EventArgs e) => Run(() =>
    {
        if (store == null) throw new SettingsError("rootError");
        string? folder = ExistingLogFolder(store.Root);
        if (folder == null)
        {
            logFolderClick = "missing"; SetStatus("logFolderMissing");
            if (smokeFolder == null)
                MessageBox.Show(this, Texts.Format("logFolderMissingDetail", Path.Combine(LogFolder(store.Root), "bootstrap")), Texts.Get("title"), MessageBoxButtons.OK, MessageBoxIcon.Information);
            return;
        }
        logFolderClick = "opened"; OpenLogFolder(folder);
    });

#if GF2VR_DIRTY_CONTACT
    private TabControl? jiggleRegions;
    private void BuildJigglePage()
    {
        var page = new TabPage { Tag = "jiggleTab", BackColor = Color.White, Padding = new Padding(6) };
        localized.Add(page); tabs.TabPages.Add(page);
        jiggleRegions = new TabControl { Dock = DockStyle.Fill };
        page.Controls.Add(jiggleRegions);
        BuildPage("breastJiggleTab", Fields.All.Where(f => f.File == "touch_interaction.json" && f.Key.StartsWith("breast", StringComparison.Ordinal)), "breastJiggleSummary", jiggleRegions);
        BuildPage("buttJiggleTab", Fields.All.Where(f => f.File == "touch_interaction.json" && f.Key.StartsWith("butt", StringComparison.Ordinal)), "buttJiggleSummary", jiggleRegions);
    }
#endif

    private void Translate()
    {
        Text = Texts.Get("title");
        UpdateState(updateKey, updateRefusal);
        if (status.Tag is string state) status.Text = Texts.Get(state);
        foreach (Control c in localized) if (c.Tag is string key) c.Text = Texts.Get(key);
        if (logFolderLocation != null) logFolderLocation.Text = store == null ? "" : Texts.Format("logFolderLocation", Path.Combine(LogFolder(store.Root), "bootstrap"));
        foreach (Field f in Fields.All.Where(f => f.Kind == "choice"))
        {
            var combo = (ComboBox)inputs[f.Key]; int before = combo.SelectedIndex;
            bool wasLoading = loading; loading = true;
            combo.Items.Clear(); combo.Items.AddRange(Fields.ChoicesFor(editing, f).Select(c => (object)Fields.ChoiceText(f, c)).ToArray());
            combo.SelectedIndex = before; loading = wasLoading;
        }
        UpdateSave();
        UpdateEffectiveSpeed();
    }

    private void LoadRoot(string root)
    {
        readyUpdate = null; updateVersion = null; updateButton.Visible = false;
        UpdateState("updateIdle");
        Run(() =>
        {
            var candidate = new SettingsStore(root, smokeFolder != null); candidate.Load();
            // A damaged preference cannot replace valid runtime settings with defaults.
            string locale;
            try { locale = candidate.LoadLanguage(); } catch { locale = "ko"; }
            store = candidate; rootBox.Text = candidate.Root; editing = Fields.Clone(candidate.Documents); panelConfigurationTouched = false; pcOutputConfigurationTouched = false;
            Texts.Language = locale;
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
                else if (c is ComboBox combo)
                {
                    if (f.File == "performance.json")
                    {
                        combo.Items.Clear(); combo.Items.AddRange(Fields.ChoicesFor(editing, f).Select(v => (object)Fields.ChoiceText(f, v)).ToArray());
                    }
                    combo.SelectedIndex = Array.IndexOf(Fields.ChoicesFor(editing, f), value.GetValue<string>());
                }
                else c.Text = value.ToJsonString();
                c.Enabled = store != null && (!Fields.IsContactHaptic(f.Key) || store.ContactHapticsAvailable);
                if (f.File == "performance.json") c.Enabled = store?.PcOutputAvailable == true;
            }
            if (pcOutputAvailability != null) pcOutputAvailability.Visible = store != null && !store.PcOutputAvailable;
            if (logFolderButton != null) logFolderButton.Enabled = store != null;
            manualDepthText = inputs["stereoDepthScale"].Text;
            depthWasLinked = ((CheckBox)inputs["linkStereoDepthToWorldScale"]).Checked;
        }
        finally { loading = false; UpdateEffectiveSpeed(); UpdatePanelAngleAvailability(); }
    }

    internal Dictionary<string, JsonObject> ReadControls()
    {
        var result = Fields.Clone(editing);
        foreach (Field f in Fields.All)
        {
            if (f.File == "performance.json" && (!pcOutputConfigurationTouched || store?.PcOutputAvailable != true)) continue;
            Control c = inputs[f.Key]; JsonNode val;
            if (c is CheckBox cb) val = JsonValue.Create(cb.Checked)!;
            else if (c is ComboBox combo) val = JsonValue.Create(Fields.ChoicesFor(editing, f)[combo.SelectedIndex])!;
            else val = JsonValue.Create(SettingsStore.Number(f.Key == "stereoDepthScale" && ((CheckBox)inputs["linkStereoDepthToWorldScale"]).Checked ? manualDepthText : c.Text, f.Key))!;
            Fields.Set(result, f, val);
        }
        if (!panelConfigurationTouched && !editing["controller_input.json"].ContainsKey("auxiliaryPanel"))
            result["controller_input.json"].Remove("auxiliaryPanel");
        SettingsStore.Validate(result); return result;
    }

    private void Changed()
    {
        if (loading) return;
        dirty = true; SetStatus("unsaved"); UpdateSave(); UpdateEffectiveSpeed(); UpdatePanelAngleAvailability();
    }
    private void UpdatePanelAngleAvailability()
    {
        if (!inputs.TryGetValue("panelViewerFacing", out var facing) ||
            !inputs.TryGetValue("panelApplyAnglesWhenFacing", out var policy)) return;
        bool apply = !((CheckBox)facing).Checked || ((CheckBox)policy).Checked;
        foreach (string key in new[] { "panelRotationPitch", "panelRotationYaw", "panelRotationRoll", "panelScreenTilt" })
            if (inputs.TryGetValue(key, out var control)) control.Enabled = store != null && apply;
    }
    private void UpdateEffectiveSpeed()
    {
        if (!inputs.TryGetValue("movementSpeed", out var speed) || !inputs.TryGetValue("locomotionScale", out var multiplier) || !inputs.ContainsKey("worldScale")) return;
        bool linked = ((CheckBox)inputs["linkStereoDepthToWorldScale"]).Checked;
        var depth = inputs["stereoDepthScale"];
        bool wasLoading = loading; loading = true;
        try
        {
            if (depthWasLinked && !linked) manualDepthText = depth.Text;
            else if (!depthWasLinked) manualDepthText = depth.Text;
            bool worldValid = double.TryParse(inputs["worldScale"].Text, NumberStyles.Float, CultureInfo.InvariantCulture, out double w) && w > 0 && double.IsFinite(w) && double.IsFinite(1 / w) && 1 / w > 0;
            depth.Text = linked ? (worldValid ? (1 / w).ToString("G17", CultureInfo.InvariantCulture) : "") : manualDepthText;
            depth.Enabled = store != null && !linked;
            effectiveSpatial.Text = worldValid ? Texts.Format("effectiveSpatial", w.ToString("G6", CultureInfo.InvariantCulture), (1 / w).ToString("G6", CultureInfo.InvariantCulture), ((CheckBox)inputs["linkMovementSpeedToWorldScale"]).Checked ? w.ToString("G6", CultureInfo.InvariantCulture) : "1") : Texts.Get("invalidSpeed");
            depthWasLinked = linked;
        }
        finally { loading = wasLoading; }
        double worldMultiplier = ((CheckBox)inputs["linkMovementSpeedToWorldScale"]).Checked ? (double.TryParse(inputs["worldScale"].Text, NumberStyles.Float, CultureInfo.InvariantCulture, out double world) ? world : double.NaN) : 1;
        bool valid = double.TryParse(speed.Text, NumberStyles.Float, CultureInfo.InvariantCulture, out double s) && s >= 0 &&
            double.TryParse(multiplier.Text, NumberStyles.Float, CultureInfo.InvariantCulture, out double m) && m >= 0 && worldMultiplier > 0 && double.IsFinite(s * m * worldMultiplier);
        effectiveSpeed.Text = valid ? Texts.Format("effectiveSpeed", (s * double.Parse(multiplier.Text, CultureInfo.InvariantCulture) * worldMultiplier).ToString("G6", CultureInfo.InvariantCulture)) : Texts.Get("invalidSpeed");
    }
    private void SetStatus(string key)
    {
        status.Tag = key; status.ForeColor = key is "error" or "saveFailed" ? Color.Firebrick : Color.FromArgb(33, 78, 110);
        updateHint.SetToolTip(status, null); UpdateSave();
    }
    private void UpdateSave()
    {
        save.Enabled = store != null;
        Text = Texts.Get("title") + (dirty ? " *" : "");
        if (status.Tag is string key)
            status.Text = Texts.Get(key) + (dirty && key is not ("unsaved" or "saveFailed") ? " · " + Texts.Get("unsaved") : "");
    }
    private void SaveSettings() => Run(() =>
    {
        if (store == null) throw new SettingsError("rootError");
        var requested = ReadControls(); store.Save(requested); editing = Fields.Clone(store.Documents);
        pcOutputConfigurationTouched = false; ApplyControls();
        dirty = false; SetStatus("saved");
    }, failedSave: true);
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
        foreach (var f in Fields.All)
            if (f.File != "performance.json" || store?.PcOutputAvailable == true) Fields.Set(editing, f, Fields.JsonSerializerNode(f.Default));
        ApplyControls(); dirty = true; SetStatus("defaultsPending");
    }
    private void Import(object? sender, EventArgs e)
    {
        if (!Discard()) return;
        using var dialog = new OpenFileDialog { Filter = Texts.Get("profileFilter"), CheckFileExists = true };
        if (dialog.ShowDialog(this) != DialogResult.OK) return;
        Run(() =>
        {
            var imported = SettingsStore.Import(dialog.FileName, editing);
            // Apply known settings; preserve installed unknown fields independently of imported unknown metadata.
            foreach (var f in Fields.All)
                if (f.File != "performance.json" || store?.PcOutputAvailable == true) Fields.Set(editing, f, (Fields.Get(imported, f) ?? Fields.JsonSerializerNode(f.Default)).DeepClone());
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
    private void Run(Action action, bool failedLoad = false, bool failedSave = false)
    {
        try { action(); status.ForeColor = Color.FromArgb(33, 78, 110); }
        catch (Exception ex)
        {
            if (failedLoad)
            {
                store = null;
                foreach (Control c in inputs.Values) c.Enabled = false;
                save.Enabled = false;
                if (logFolderButton != null) logFolderButton.Enabled = false;
                if (logFolderLocation != null) logFolderLocation.Text = "";
            }
            SetStatus(failedSave ? "saveFailed" : "error"); updateHint.SetToolTip(status, Texts.Describe(ex));
            if (smokeFolder == null) MessageBox.Show(this, Texts.Describe(ex), Texts.Get("error"), MessageBoxButtons.OK, MessageBoxIcon.Error);
            else throw;
        }
    }

    private void VerifyButtonGeometry()
    {
        PerformLayout();
        foreach (var button in languages.Controls.Cast<Control>().Concat(actions.Controls.Cast<Control>()))
        {
            if (!button.Visible) continue;
            for (Control? parent = button.Parent; parent != null; parent = parent.Parent)
            {
                var bounds = parent.RectangleToClient(button.Parent!.RectangleToScreen(button.Bounds));
                if (!parent.ClientRectangle.Contains(bounds))
                    throw new InvalidDataException($"Clipped button {button.Text}: {bounds} outside {parent.GetType().Name} {parent.ClientRectangle}");
            }
        }
        var browse = localized.OfType<Button>().Single(b => Equals(b.Tag, "browse"));
        var pathBounds = rootBox.RectangleToScreen(rootBox.ClientRectangle);
        var browseBounds = browse.RectangleToScreen(browse.ClientRectangle);
        if (Math.Abs((pathBounds.Top + pathBounds.Bottom) - (browseBounds.Top + browseBounds.Bottom)) > 2)
            throw new InvalidDataException("Folder path and Browse are not vertically centered");
        if (!browse.Parent!.ClientRectangle.Contains(browse.Bounds))
            throw new InvalidDataException("Browse button is clipped");
        var buttons = languages.Controls.Cast<Control>().Concat(actions.Controls.Cast<Control>()).Where(b => b.Visible).ToArray();
        if (buttons.Select(b => b.Height).Distinct().Count() != 1)
            throw new InvalidDataException("Footer buttons have inconsistent heights");
    }

    private void Smoke()
    {
        try
        {
            Texts.Verify(); Directory.CreateDirectory(smokeFolder!);
            // Simulated availability only for rendering the new notification;
            // no release, download, consent or installation is synthesized.
            updateVersion = "0.6.0"; UpdateState("updateAvailable"); updateButton.Visible = true;
            // Real forms/resources, no runtime writes. Exercise language, all field creation, initial config and persistence mapping.
            var initial = ReadControls(); SettingsStore.Validate(initial);
            initial["performance.json"]["pcOutput"]!["renderScale"] = .875;
            initial["performance.json"]["dynamicEyeScale"] = new JsonObject { ["enabled"] = false, ["futureHidden"] = 42 };
            editing = Fields.Clone(initial); ApplyControls();
            if (initial["controller_input.json"].ContainsKey("auxiliaryPanel") != editing["controller_input.json"].ContainsKey("auxiliaryPanel"))
                throw new InvalidDataException("Unrelated settings save activated a panel layout");
            for (int i = 0; i < Texts.Languages.Length; i++)
            {
                ((Button)languages.Controls[i]).PerformClick();
                if (Texts.Language != Texts.Languages[i] || store?.LoadLanguage() != Texts.Languages[i])
                    throw new InvalidDataException("Language button did not select and persist its locale");
                UpdateState("updateFailed", new InstallError("ownedMissing", "GF2 Game/version.dll"));
                Translate();
                if (updateStatus.Cursor != Cursors.Hand || updateStatus.Text != Texts.Get("updateDetails") ||
                    updateHint.GetToolTip(updateStatus)?.Contains("GF2 Game/version.dll") != true ||
                    updateStatus.AccessibleName?.Contains(Texts.Get("updateNextAction")) != true)
                    throw new InvalidDataException("Localized update refusal lost condition, file, action or details affordance");
                UpdateState("updateAvailable");
                VerifyButtonGeometry();
                var previousOpener = OpenReleasePage; string? opened = null;
                try
                {
                    OpenReleasePage = url => opened = url;
                    UpdateState("updateManual", new InstallError("layoutChanged")); updateButton.Visible = false;
                    if (!updateStatus.Text.Contains(updateVersion!, StringComparison.Ordinal) || !releasePage.Visible || opened != null)
                        throw new InvalidDataException("Manual update lost version or opened a page automatically");
                    releasePage.PerformClick();
                    if (opened != UpdatePolicy.ReleasePage(updateVersion!)) throw new InvalidDataException("Release Open button target");
                    using var manual = new Bitmap(Width, Height); DrawToBitmap(manual, new Rectangle(0, 0, Width, Height)); manual.Save(Path.Combine(smokeFolder!, "update-manual-" + Texts.Language + ".png"));
                    using var notice = new UpdateNoticeDialog(updateVersion!, false);
                    notice.Show(); notice.Refresh();
                    using var image = new Bitmap(notice.Width, notice.Height); notice.DrawToBitmap(image, new Rectangle(0, 0, image.Width, image.Height)); image.Save(Path.Combine(smokeFolder!, "update-notice-" + Texts.Language + ".png")); notice.Close();
                }
                finally { OpenReleasePage = previousOpener; UpdateState("updateAvailable"); updateButton.Visible = true; }
                tabs.SelectedIndex = 0;
                var pcPreset = (ComboBox)inputs["pcOutputPreset"];
                var pcScope = (CheckBox)inputs["pcOutputImmersiveOnly"];
                if (store?.PcOutputAvailable == true)
                {
                    for (int option = 0; option < 5; ++option)
                    {
                        pcPreset.SelectedIndex = option;
                        foreach (bool immersive in new[] { true, false })
                        {
                            pcScope.Checked = immersive;
                            var values = ReadControls(); var pc = values["performance.json"]["pcOutput"]!;
                            if (Fields.Get(values, Fields.All.Single(f => f.Key == "pcOutputPreset"))!.GetValue<string>() != new[] { "NATIVE", "3200x1800", "2560x1440", "1920x1080", "1280x720" }[option] ||
                                pc["scope"]!.GetValue<string>() != (immersive ? "IMMERSIVE_ONLY" : "ALWAYS") ||
                                pc["renderScale"]!.GetValue<double>() != .875 || values["performance.json"]["dynamicEyeScale"]!["futureHidden"]!.GetValue<int>() != 42)
                                throw new InvalidDataException("PC GUI mapping changed hidden runtime settings");
                        }
                    }
                    pcPreset.SelectedIndex = 4; pcScope.Checked = true;
                    var lateToggle = (CheckBox)inputs["lateRelocate"];
                    if (!lateToggle.Checked) throw new InvalidDataException("Late relocate default must display ON when the key is absent");
                    foreach (bool enabled in new[] { false, true })
                    {
                        lateToggle.Checked = enabled;
                        var values = ReadControls();
                        if (values["performance.json"]["pose"]!["lateRelocate"]!.GetValue<bool>() != enabled ||
                            values["performance.json"]["pcOutput"]!["renderScale"]!.GetValue<double>() != .875 || values["performance.json"]["dynamicEyeScale"]!["futureHidden"]!.GetValue<int>() != 42)
                            throw new InvalidDataException("Late relocate toggle did not map to pose.lateRelocate or changed hidden settings");
                    }
                }
                else if (pcPreset.Enabled || pcScope.Enabled || inputs["lateRelocate"].Enabled || !JsonNode.DeepEquals(ReadControls()["performance.json"], initial["performance.json"]))
                    throw new InvalidDataException("Unsupported runtime exposed or rewrote PC controls");
                PerformLayout(); tabs.SelectedTab!.PerformLayout();
                using (var pcBitmap = new Bitmap(Width, Height))
                {
                    DrawToBitmap(pcBitmap, new Rectangle(0, 0, Width, Height));
                    pcBitmap.Save(Path.Combine(smokeFolder!, "pc-output-" + Texts.Language + ".png"));
                }
                editing = Fields.Clone(initial); pcOutputConfigurationTouched = false; ApplyControls();
                tabs.SelectedIndex = 1; PerformLayout(); tabs.SelectedTab!.PerformLayout();
                var follow = (CheckBox)inputs["cameraFollowEnabled"]; bool oldFollow = follow.Checked;
                foreach (bool enabled in new[] { false, true })
                {
                    follow.Checked = enabled;
                    if (ReadControls()["navigation.json"]["navigation"]!["cameraFollowEnabled"]!.GetValue<bool>() != enabled)
                        throw new InvalidDataException("Camera follow startup selection did not map to persistence");
                    using var followBitmap = new Bitmap(Width, Height); DrawToBitmap(followBitmap, new Rectangle(0, 0, Width, Height));
                    followBitmap.Save(Path.Combine(smokeFolder!, "camera-follow-" + (enabled ? "on-" : "off-") + Texts.Language + ".png"));
                }
                follow.Checked = oldFollow;
                tabs.SelectedIndex = i; PerformLayout(); tabs.SelectedTab!.PerformLayout();
                if (!save.Enabled || !updateButton.Visible || !Enabled) throw new InvalidDataException("Notification blocked settings");
                using var bitmap = new Bitmap(Width, Height); DrawToBitmap(bitmap, new Rectangle(0, 0, Width, Height));
                bitmap.Save(Path.Combine(smokeFolder!, Texts.Language + ".png"));
                tabs.SelectedIndex = 3; PerformLayout(); tabs.SelectedTab!.PerformLayout();
                var primaryUi = (CheckBox)inputs["activityPrimaryUiEnabled"];
                bool before = primaryUi.Checked; primaryUi.Checked = false;
                if (ReadControls()["activity_ui.json"]["activityPrimaryUiEnabled"]!.GetValue<bool>())
                    throw new InvalidDataException("Activity UI OFF did not map to persistence");
                using var uiBitmap = new Bitmap(Width, Height); DrawToBitmap(uiBitmap, new Rectangle(0, 0, Width, Height));
                uiBitmap.Save(Path.Combine(smokeFolder!, "activity-ui-" + Texts.Language + ".png"));
                primaryUi.Checked = true;
                if (!ReadControls()["activity_ui.json"]["activityPrimaryUiEnabled"]!.GetValue<bool>())
                    throw new InvalidDataException("Activity UI ON did not map to persistence");
                primaryUi.Checked = before;
                tabs.SelectedIndex = 4; PerformLayout(); tabs.SelectedTab!.PerformLayout();
                var faceSwap = (CheckBox)inputs["swapFaceButtons"]; bool oldSwap = faceSwap.Checked;
                foreach (bool swap in new[] { true, false })
                {
                    faceSwap.Checked = swap;
                    if (ReadControls()["controller_input.json"]["controllerInput"]!["swapFaceButtons"]!.GetValue<bool>() != swap)
                        throw new InvalidDataException("Face checkbox did not map to persistence");
                }
                faceSwap.Checked = oldSwap;
                var handRoles = (CheckBox)inputs["swapPanelPointerHands"]; bool oldRoles = handRoles.Checked;
                foreach (bool value in new[] { true, false })
                {
                    handRoles.Checked = value;
                    if (ReadControls()["controller_input.json"]["controllerInput"]!["swapPanelPointerHands"]!.GetValue<bool>() != value)
                        throw new InvalidDataException("Hand-role checkbox did not map to persistence");
                }
                handRoles.Checked = oldRoles;
                tabs.SelectedIndex = 6; PerformLayout(); tabs.SelectedTab!.PerformLayout();
                var hands = (CheckBox)inputs["controllerHandsEnabled"]; bool oldHands = hands.Checked;
                foreach (bool value in new[] { false, true })
                {
                    hands.Checked = value;
                    if (ReadControls()["controller_hands.json"]["enabled"]!.GetValue<bool>() != value)
                        throw new InvalidDataException("Hand visibility checkbox did not map to native JSON");
                }
                foreach (string key in new[] { "contactHapticStrengthPercent", "contactHapticDurationMs" })
                    if (inputs[key].Enabled != store!.ContactHapticsAvailable)
                        throw new InvalidDataException("Dirty-only haptic availability mismatch");
                var hapticStrength = (TextBox)inputs["contactHapticStrengthPercent"];
                var hapticDuration = (TextBox)inputs["contactHapticDurationMs"];
                string savedStrength = hapticStrength.Text, savedDuration = hapticDuration.Text;
                hapticStrength.Text = "62.5"; hapticDuration.Text = "125.5";
                var hapticValues = ReadControls()["controller_hands.json"];
                if (hapticValues["contactHapticStrengthPercent"]!.GetValue<double>() != 62.5 ||
                    hapticValues["contactHapticDurationMs"]!.GetValue<double>() != 125.5)
                    throw new InvalidDataException("Haptic controls did not map to native settings");
                hapticStrength.Text = savedStrength; hapticDuration.Text = savedDuration;
                using var handsBitmap = new Bitmap(Width, Height); DrawToBitmap(handsBitmap, new Rectangle(0, 0, Width, Height));
                handsBitmap.Save(Path.Combine(smokeFolder!, "hands-" + Texts.Language + ".png"));
#if GF2VR_DIRTY_CONTACT
                foreach (var field in Fields.All.Where(f => f.File == "touch_interaction.json"))
                {
                    var input = (TextBox)inputs[field.Key]; string saved = input.Text;
                    if (!input.Enabled) throw new InvalidDataException("Jiggle input disabled");
                    input.Text = Fields.CommonJiggleKey(field.Key) == "jiggleDurationMultiplier" ? "1.5" : "150";
                    if (ReadControls()[field.File][field.Key]!.GetValue<double>() != (Fields.CommonJiggleKey(field.Key) == "jiggleDurationMultiplier" ? 1.5 : 150))
                        throw new InvalidDataException("Jiggle control persistence mapping");
                    input.Text = saved;
                    tabs.SelectedIndex = 8;
                    jiggleRegions!.SelectedIndex = field.Key.StartsWith("breast", StringComparison.Ordinal) ? 0 : 1;
                    tabs.PerformLayout(); jiggleRegions.PerformLayout(); jiggleRegions.SelectedTab!.PerformLayout();
                    jiggleRegions.SelectedTab.ScrollControlIntoView(input); PerformLayout();
                    var rectangle = jiggleRegions.SelectedTab.RectangleToClient(input.RectangleToScreen(input.ClientRectangle));
                    if (!jiggleRegions.SelectedTab.ClientRectangle.Contains(rectangle)) throw new InvalidDataException("Jiggle input cannot be scrolled into view");
                    using var jiggleBitmap = new Bitmap(Width, Height); DrawToBitmap(jiggleBitmap, new Rectangle(0, 0, Width, Height));
                    jiggleBitmap.Save(Path.Combine(smokeFolder!, field.Key + "-" + Texts.Language + ".png"));
                }
#endif
                hands.Checked = oldHands; tabs.SelectedIndex = 4;
                using var faceBitmap = new Bitmap(Width, Height); DrawToBitmap(faceBitmap, new Rectangle(0, 0, Width, Height));
                faceBitmap.Save(Path.Combine(smokeFolder!, "controller-" + Texts.Language + ".png"));
                tabs.SelectedIndex = 5;
                var world = (TextBox)inputs["worldScale"]; var linkDepth = (CheckBox)inputs["linkStereoDepthToWorldScale"]; var linkSpeed = (CheckBox)inputs["linkMovementSpeedToWorldScale"];
                linkSpeed.Checked = true; world.Text = "invalid";
                if (effectiveSpeed.Text != Texts.Get("invalidSpeed")) throw new InvalidDataException("Malformed world must not show a neutral valid speed");
                linkDepth.Checked = true; linkSpeed.Checked = true; world.Text = "2";
                if (inputs["stereoDepthScale"].Enabled || SettingsStore.Number(inputs["stereoDepthScale"].Text, "stereoDepthScale") != .5 || !effectiveSpeed.Text.Contains("2")) throw new InvalidDataException("Linked world preview mismatch");
                PerformLayout(); tabs.SelectedTab!.PerformLayout();
                using var worldBitmap = new Bitmap(Width, Height); DrawToBitmap(worldBitmap, new Rectangle(0, 0, Width, Height)); worldBitmap.Save(Path.Combine(smokeFolder!, "world-on-" + Texts.Language + ".png"));
                linkDepth.Checked = false;
                if (!inputs["stereoDepthScale"].Enabled || ReadControls()["stereo_depth.json"]["stereoDepthScale"]!.GetValue<double>() != .5) throw new InvalidDataException("Depth OFF did not retain effective value");
                world.Text = ".5";
                if (ReadControls()["stereo_depth.json"]["stereoDepthScale"]!.GetValue<double>() != .5 || !effectiveSpeed.Text.Contains("0.5")) throw new InvalidDataException("Unlinked depth changed or proportional speed wrong");
                linkDepth.Checked = true;
                if (inputs["stereoDepthScale"].Enabled || SettingsStore.Number(inputs["stereoDepthScale"].Text, "stereoDepthScale") != 2) throw new InvalidDataException("Depth relink did not recompute");
                linkDepth.Checked = false; linkSpeed.Checked = false; world.Text = "2";
                if (ReadControls()["stereo_depth.json"]["stereoDepthScale"]!.GetValue<double>() != 2 || !effectiveSpeed.Text.Contains("1")) throw new InvalidDataException("Unlinked settings changed with world");
                using var offBitmap = new Bitmap(Width, Height); DrawToBitmap(offBitmap, new Rectangle(0, 0, Width, Height)); offBitmap.Save(Path.Combine(smokeFolder!, "world-off-" + Texts.Language + ".png"));
                tabs.SelectedIndex = 7; PerformLayout(); tabs.SelectedTab!.PerformLayout();
                var panelFacing = (CheckBox)inputs["panelViewerFacing"];
                var anglePolicy = (CheckBox)inputs["panelApplyAnglesWhenFacing"];
                foreach (bool facing in new[] { false, true })
                foreach (bool applyAngles in new[] { false, true })
                {
                    panelFacing.Checked = !facing; panelFacing.Checked = facing;
                    anglePolicy.Checked = !applyAngles; anglePolicy.Checked = applyAngles;
                    inputs["panelOffsetX"].Text = "-0.75"; inputs["panelRotationPitch"].Text = "-270"; inputs["panelScreenTilt"].Text = "5";
                    var panel = ReadControls()["controller_input.json"]["auxiliaryPanel"]!;
                    if (panel["viewerFacing"]!.GetValue<bool>() != facing || panel["applyAnglesWhenFacing"]!.GetValue<bool>() != applyAngles || panel["offsetX"]!.GetValue<double>() != -.75 || panel["rotationPitch"]!.GetValue<double>() != -270 || panel["screenTilt"]!.GetValue<double>() != 5)
                        throw new InvalidDataException("Panel mode/signed position/angle mapping failed");
                    foreach (string key in new[] { "panelRotationPitch", "panelRotationYaw", "panelRotationRoll", "panelScreenTilt" })
                        if (inputs[key].Enabled != (!facing || applyAngles)) throw new InvalidDataException("Panel angle policy enablement " + key);
                    if (!inputs["panelOffsetY"].Enabled || !inputs["panelMaximumHeight"].Enabled) throw new InvalidDataException("Policy disabled position/size");
                    tabs.SelectedTab!.AutoScrollPosition = Point.Empty;
                    using var panelBitmap = new Bitmap(Width, Height); DrawToBitmap(panelBitmap, new Rectangle(0, 0, Width, Height));
                    panelBitmap.Save(Path.Combine(smokeFolder!, "panel-" + (facing ? "on-" : "off-") + (applyAngles ? "angles-on-" : "angles-off-") + Texts.Language + ".png"));
                    tabs.SelectedTab!.ScrollControlIntoView(inputs["panelRotationPitch"]);
                    using var angleBitmap = new Bitmap(Width, Height); DrawToBitmap(angleBitmap, new Rectangle(0, 0, Width, Height));
                    angleBitmap.Save(Path.Combine(smokeFolder!, "panel-angles-" + (facing ? "on-" : "off-") + (applyAngles ? "on-" : "off-") + Texts.Language + ".png"));
                }
                tabs.SelectedTab!.ScrollControlIntoView(inputs["panelMaximumHeight"]);
                using (var panelBottom = new Bitmap(Width, Height))
                { DrawToBitmap(panelBottom, new Rectangle(0, 0, Width, Height)); panelBottom.Save(Path.Combine(smokeFolder!, "panel-bottom-" + Texts.Language + ".png")); }
                ApplyDefaults();
                var defaultValues = ReadControls();
                foreach (var field in Fields.All)
                    if (Fields.Get(defaultValues, field)!.ToJsonString() != Fields.JsonSerializerNode(field.Default).ToJsonString())
                        throw new InvalidDataException("Reset lost selected default " + field.Key);
                tabs.SelectedTab!.AutoScrollPosition = Point.Empty;
                using (var defaultsPanel = new Bitmap(Width, Height))
                { DrawToBitmap(defaultsPanel, new Rectangle(0, 0, Width, Height)); defaultsPanel.Save(Path.Combine(smokeFolder!, "defaults-panel-" + Texts.Language + ".png")); }
                tabs.SelectedTab.ScrollControlIntoView(inputs["panelScreenTilt"]);
                using (var defaultsAngles = new Bitmap(Width, Height))
                { DrawToBitmap(defaultsAngles, new Rectangle(0, 0, Width, Height)); defaultsAngles.Save(Path.Combine(smokeFolder!, "defaults-angles-" + Texts.Language + ".png")); }
#if GF2VR_DIRTY_CONTACT
                tabs.SelectedIndex = 8;
                foreach (int region in new[] { 0, 1 }) {
                    jiggleRegions!.SelectedIndex = region; jiggleRegions.SelectedTab!.AutoScrollPosition = Point.Empty;
                    PerformLayout(); jiggleRegions.PerformLayout(); jiggleRegions.SelectedTab.PerformLayout();
                    using var defaultsJiggle = new Bitmap(Width, Height); DrawToBitmap(defaultsJiggle, new Rectangle(0, 0, Width, Height));
                    defaultsJiggle.Save(Path.Combine(smokeFolder!, "defaults-jiggle-" + region + "-" + Texts.Language + ".png"));
                }
#endif
                tabs.SelectedTab = tabs.TabPages.Cast<TabPage>().Single(p => (string?)p.Tag == "diagnosticsTab");
                var logging = (CheckBox)inputs["loggingEnabled"];
                if (logging.Checked) throw new InvalidDataException("Logging default must be OFF");
                foreach (bool enabled in new[] { false, true })
                {
                    logging.Checked = enabled; PerformLayout(); tabs.SelectedTab.PerformLayout();
                    if (ReadControls()["controller_input.json"]["diagnostics"]!["loggingEnabled"]!.GetValue<bool>() != enabled)
                        throw new InvalidDataException("Logging checkbox persistence mapping");
                    using var diagnostics = new Bitmap(Width, Height); DrawToBitmap(diagnostics, new Rectangle(0, 0, Width, Height));
                    diagnostics.Save(Path.Combine(smokeFolder!, "logging-" + (enabled ? "on-" : "off-") + Texts.Language + ".png"));
                }
                var openLogs = logFolderButton!;
                if (openLogs.Parent?.Parent?.Parent != tabs.SelectedTab || !openLogs.Visible || !openLogs.Enabled || openLogs.Text != Texts.Get("openLogFolder"))
                    throw new InvalidDataException("Log folder button missing from diagnostics tab");
                var logBounds = tabs.SelectedTab.RectangleToClient(openLogs.RectangleToScreen(openLogs.ClientRectangle));
                if (!tabs.SelectedTab.ClientRectangle.Contains(logBounds) || openLogs.Width < openLogs.GetPreferredSize(Size.Empty).Width)
                    throw new InvalidDataException("Log folder button clipped");
                if (!logFolderLocation!.Text.Contains(Path.Combine(store!.Root, "vrmod", "logs", "bootstrap"), StringComparison.Ordinal))
                    throw new InvalidDataException("Log folder location text");
                var beforeLogs = ReadControls(); bool dirtyBeforeLogs = dirty; var previousLogOpener = OpenLogFolder; string? openedLogs = null;
                try
                {
                    OpenLogFolder = path => openedLogs = path;
                    string? expectedLogs = ExistingLogFolder(store.Root);
                    openLogs.PerformClick();
                    if (openedLogs != expectedLogs || logFolderClick != (expectedLogs == null ? "missing" : "opened") ||
                        (expectedLogs == null && (string?)status.Tag != "logFolderMissing"))
                        throw new InvalidDataException("Log folder button target");
                    var afterLogs = ReadControls();
                    if (dirty != dirtyBeforeLogs || afterLogs.Keys.Any(k => !JsonNode.DeepEquals(afterLogs[k], beforeLogs[k])))
                        throw new InvalidDataException("Log folder button changed settings");
                    // Resolver contract on a scratch root; the real root is never created or written.
                    string scratch = Path.Combine(smokeFolder!, "log-root-" + Texts.Language), scratchLogs = Path.Combine(scratch, "vrmod", "logs");
                    if (Directory.Exists(scratch)) Directory.Delete(scratch, true);
                    if (ExistingLogFolder(scratch) != null) throw new InvalidDataException("Missing log folder resolved");
                    Directory.CreateDirectory(scratchLogs);
                    if (ExistingLogFolder(scratch) != scratchLogs) throw new InvalidDataException("Log root fallback");
                    Directory.CreateDirectory(Path.Combine(scratchLogs, "bootstrap"));
                    if (ExistingLogFolder(scratch) != Path.Combine(scratchLogs, "bootstrap")) throw new InvalidDataException("Bootstrap log folder preference");
                }
                finally { OpenLogFolder = previousLogOpener; }
                using (var logFolderBitmap = new Bitmap(Width, Height))
                { DrawToBitmap(logFolderBitmap, new Rectangle(0, 0, Width, Height)); logFolderBitmap.Save(Path.Combine(smokeFolder!, "log-folder-" + Texts.Language + ".png")); }
                editing = Fields.Clone(initial); panelConfigurationTouched = false; ApplyControls();
            }
            ((TextBox)inputs["eyeRenderScale"]).Text = "2.5";
            ((ComboBox)inputs["locomotionHand"]).SelectedIndex = 1;
            var edited = ReadControls();
            if (Fields.Get(edited, Fields.All[0])!.GetValue<double>() != 2.5 ||
                edited["navigation.json"]["navigation"]!["locomotionHand"]!.GetValue<string>() != "right")
                throw new InvalidDataException("GUI controls did not map to persistence");
            Size fullSize = Size; Size = MinimumSize;
            foreach (string locale in Texts.Languages)
            {
                SwitchLanguage(locale); VerifyButtonGeometry();
                using var compact = new Bitmap(Width, Height); DrawToBitmap(compact, new Rectangle(0, 0, Width, Height)); compact.Save(Path.Combine(smokeFolder!, "minimum-" + locale + ".png"));
            }
            Size = fullSize;
            // Layout stress, not a claim about a physical monitor's DPI configuration.
            Scale(new SizeF(1.5f, 1.5f)); Size = MinimumSize;
            foreach (string locale in Texts.Languages)
            {
                SwitchLanguage(locale); VerifyButtonGeometry();
                using var scaled = new Bitmap(Width, Height); DrawToBitmap(scaled, new Rectangle(0, 0, Width, Height)); scaled.Save(Path.Combine(smokeFolder!, "scaled150-" + locale + ".png"));
            }
            File.WriteAllText(Path.Combine(smokeFolder!, "gui-validation.json"), System.Text.Json.JsonSerializer.Serialize(new { passed=true, locales=Texts.Languages, fields=Fields.All.Length, languageButtons=Texts.Languages.Length, buttonBoundsAndPathAlignment=true, layoutScaleStress=1.5, cameraFollowStartupOnOff=true, activityUiOnOff=true, faceExchange=true, pairedHandRoles=true, worldScaleLinks=true, commonHandsOnOff=true, contactHapticStrengthDuration=true, dirtyOnlyHapticAvailability=true, logFolderButton=true, logFolderClick, liveSaves=0, gameLaunches=0 }));
        }
        catch (Exception ex) { File.WriteAllText(Path.Combine(smokeFolder!, "gui-error.txt"), ex.ToString()); Environment.ExitCode = 1; }
        finally { Close(); }
    }
}
