namespace GF2VR.Installer;

internal sealed class InstallerForm : Form
{
    private readonly InstallationEngine engine = new();
    private readonly TextBox root = new() { Anchor = AnchorStyles.Left | AnchorStyles.Right, ReadOnly = true };
    private readonly string packageRoot;
    private readonly Label bundleNotice = new() { Dock = DockStyle.Fill, AutoSize = false, TextAlign = ContentAlignment.MiddleLeft };
    private readonly FlowLayoutPanel languages = new() { Dock = DockStyle.Fill, FlowDirection = FlowDirection.LeftToRight, WrapContents = false };
    private readonly Label summary = new() { Dock = DockStyle.Fill, AutoSize = false };
    private readonly Label status = new() { Dock = DockStyle.Fill, AutoEllipsis = true, TextAlign = ContentAlignment.MiddleLeft };
    private readonly ListView files = new() { Dock = DockStyle.Fill, View = View.Details, FullRowSelect = true, HideSelection = false };
    private readonly TextBox log = new() { Dock = DockStyle.Fill, Multiline = true, ReadOnly = true, ScrollBars = ScrollBars.Vertical, BackColor = SystemColors.Window };
    private readonly ProgressBar progress = new() { Dock = DockStyle.Fill, Minimum = 0, Maximum = 100 };
    private readonly Button install = new() { AutoSize = true, MinimumSize = new Size(115, 36), Tag = "install" };
    private readonly Button remove = new() { AutoSize = true, MinimumSize = new Size(115, 36), Tag = "remove" };
    private readonly Button recover = new() { AutoSize = true, MinimumSize = new Size(100, 36), Tag = "recover" };
    private readonly Button openSettings = new() { AutoSize = true, MinimumSize = new Size(115, 36), Tag = "openSettings" };
    private readonly List<Control> texts = [];
    private readonly List<Control> operations = [];
    private readonly string? smoke;
    private readonly bool smokeWorkflow;
    private Plan? plan;
    private Receipt? receipt;
    private SettingsTarget? settings;
    private string? receiptHash;
    private bool pending, busy, hasPrevious, packageIncomplete;
    private int progressTotal, progressCurrent, operationSerial;
    private string statusKey = "ready";

    internal InstallerForm(string gameRoot, string? packageRoot, string? smokeFolder = null, bool verifyWorkflow = false)
    {
        smoke = smokeFolder; smokeWorkflow = verifyWorkflow;
        if (verifyWorkflow && smokeFolder == null) throw new InvalidDataException("Workflow validation requires an inert GUI fixture");
        Font = new Font("Segoe UI", 10); ClientSize = new Size(1080, 860); MinimumSize = new Size(1000, 780);
        AutoScaleMode = AutoScaleMode.Dpi; StartPosition = FormStartPosition.CenterScreen; BackColor = Color.FromArgb(244, 247, 251); Icon = SystemIcons.Application;
        if (smoke != null) { StartPosition = FormStartPosition.Manual; Location = new Point(-20000, -20000); ShowInTaskbar = false; }
        root.Text = gameRoot; this.packageRoot = PackageLocation.Resolve(packageRoot);
        packageIncomplete = !PackageLocation.IsAvailable(this.packageRoot);
        InstallerText.Language = InstallerText.LoadPreference(gameRoot);
        var layout = new TableLayoutPanel { Dock = DockStyle.Fill, Padding = new Padding(16), ColumnCount = 1, RowCount = 9 };
        layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        foreach (int height in new[] { 50, 44, 64, 88, 26 }) layout.RowStyles.Add(new RowStyle(SizeType.Absolute, height));
        layout.RowStyles[1].SizeType = SizeType.AutoSize;
        layout.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        foreach (int height in new[] { 96, 52, 26 }) layout.RowStyles.Add(new RowStyle(SizeType.Absolute, height));
        Controls.Add(layout);
        var heading = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 2 };
        heading.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100)); heading.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 246));
        var title = Label("title"); title.Font = new Font(Font.FontFamily, 16, FontStyle.Bold); heading.Controls.Add(title, 0, 0);
        foreach (var item in new[] { ("한국어", "ko"), ("English", "en"), ("日本語", "ja") })
        {
            var button = new Button { Text = item.Item1, AutoSize = true, MinimumSize = new Size(75, 34) };
            string code = item.Item2; button.Click += (_, _) => ChangeLanguage(code); languages.Controls.Add(button);
        }
        heading.Controls.Add(languages, 1, 0); layout.Controls.Add(heading, 0, 0);
        layout.Controls.Add(GameFolderRow(), 0, 1);
        bundleNotice.Font = new Font(Font.FontFamily, 9); bundleNotice.Margin = new Padding(4, 3, 4, 3); layout.Controls.Add(bundleNotice, 0, 2);
        summary.Font = new Font(Font.FontFamily, 9); summary.Padding = new Padding(4, 8, 4, 4); layout.Controls.Add(summary, 0, 3);
        layout.Controls.Add(progress, 0, 4);
        var details = new TabControl { Dock = DockStyle.Fill };
        var filesPage = new TabPage { Tag = "filesTab" }; var logPage = new TabPage { Tag = "logTab" };
        texts.Add(filesPage); texts.Add(logPage); details.TabPages.AddRange([filesPage, logPage]);
        files.Columns.Add("", 680); files.Columns.Add("", 180); filesPage.Controls.Add(files); logPage.Controls.Add(log); layout.Controls.Add(details, 0, 5);
        var advice = Label("policy"); advice.AutoSize = false; advice.Dock = DockStyle.Fill; advice.ForeColor = Color.FromArgb(71, 87, 110);
        advice.Font = new Font(Font.FontFamily, 9); advice.Margin = new Padding(4, 10, 4, 5); layout.Controls.Add(advice, 0, 6);
        var footer = new FlowLayoutPanel { Dock = DockStyle.Fill, FlowDirection = FlowDirection.RightToLeft, WrapContents = false, Padding = new Padding(0, 8, 0, 0) };
        texts.AddRange([install, remove, recover, openSettings]); operations.AddRange([install, remove, recover, openSettings]);
        install.Click += async (_, _) =>
        {
            var chosen = plan;
            if (chosen == null || !Confirm(InstallerText.Format("confirmInstall", chosen.Package.Version, chosen.Package.Files.Length, chosen.Root))) return;
            await Run(() => engine.Install(chosen, Progress), "installed", chosen.Package.Files.Length + 1);
        };
        remove.Click += async (_, _) =>
        {
            string selectedRoot = root.Text; string? hash = receiptHash; int total = (receipt?.Files.Length ?? 0) + 1;
            if (hash == null || !Confirm(InstallerText.Get("confirmRemove"))) return;
            await Run(() => engine.Remove(selectedRoot, hash, Progress), "removed", total);
        };
        recover.Click += async (_, _) =>
        {
            string selectedRoot = root.Text; if (!Confirm(InstallerText.Get("confirmRecover"))) return;
            await Run(() => { engine.Recover(selectedRoot); return new OperationResult("", false, ""); }, "recovered", 0);
        };
        openSettings.Click += (_, _) =>
        {
            try { if (settings == null) throw new InstallError("settingsUnavailable"); SettingsLaunch.Open(settings, engine); SetStatus("settingsOpened"); }
            catch (Exception ex) { DisplayError(ex, true); Inspect(false); }
        };
        footer.Controls.AddRange([install, remove, recover, openSettings, Button("inspect", (_, _) => Inspect())]);
        layout.Controls.Add(footer, 0, 7); layout.Controls.Add(status, 0, 8);
        FormClosing += (_, e) => { if (busy) e.Cancel = true; };
        Translate(); Inspect(); if (smoke != null) Shown += (_, _) => Smoke(details);
    }

    private Label Label(string key)
    {
        var label = new Label { Tag = key, AutoSize = true, Margin = new Padding(4, 8, 8, 4) }; texts.Add(label); return label;
    }
    private Button Button(string key, EventHandler action)
    {
        var button = new Button { AutoSize = true, MinimumSize = new Size(80, 36), Tag = key };
        texts.Add(button); operations.Add(button); button.Click += action; return button;
    }
    private TableLayoutPanel GameFolderRow()
    {
        var row = new TableLayoutPanel { Dock = DockStyle.Fill, AutoSize = true, ColumnCount = 3, Padding = new Padding(0, 0, 0, 6), Margin = Padding.Empty };
        row.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 154)); row.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100)); row.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        var label = Label("folder"); label.Anchor = AnchorStyles.Left; label.Margin = new Padding(4, 3, 8, 3);
        var browse = Button("browse", (_, _) => Browse()); browse.Anchor = AnchorStyles.Right;
        row.Controls.Add(label, 0, 0); row.Controls.Add(root, 1, 0); row.Controls.Add(browse, 2, 0); return row;
    }
    private void ChangeLanguage(string locale)
    {
        if (busy) return; InstallerText.Language = locale; Translate();
        try { InstallerText.SavePreference(root.Text, locale); SetStatus("languageSaved"); } catch (Exception ex) { DisplayError(ex, true); }
    }
    private void Translate()
    {
        Text = InstallerText.Get("title"); foreach (Control control in texts) control.Text = InstallerText.Get((string)control.Tag!);
        files.Columns[0].Text = InstallerText.Get("fileColumn"); files.Columns[1].Text = InstallerText.Get("actionColumn");
        SetStatus(statusKey, false); RefreshDisplay();
    }
    private void AppendLog(string message)
    {
        if (log.TextLength > 100000) log.Clear();
        log.AppendText($"[{DateTime.Now:HH:mm:ss}] {message}{Environment.NewLine}");
    }
    private void SetStatus(string key, bool record = true)
    {
        statusKey = key; status.Text = InstallerText.Get(key); if (record) AppendLog(status.Text);
    }
    private void RefreshDisplay()
    {
        bundleNotice.Text = InstallerText.Get(packageIncomplete ? "bundledPackageMissing" : "bundledPackageHelp");
        summary.Text = InstallerText.Format("summary", plan?.Package.Version ?? InstallerText.Get("unavailable"), receipt?.Version ?? InstallerText.Get("noReceipt"),
            (plan?.Package.Scope ?? receipt?.Scope) == "tools" ? InstallerText.Get("toolsScope") : plan?.Package.Scope == "vrmod" ? InstallerText.Get(NativeLayout.IsPortable(plan.Package) ? "portableScope" : "nativeScope") : InstallerText.Get("noRuntimeRelease"));
        files.Items.Clear(); foreach (var file in plan?.Package.Files ?? receipt?.Files ?? [])
            files.Items.Add(new ListViewItem([file.Path, InstallerText.Get(plan == null ? "ownedAction" : receipt == null ? "newAction" : "updateAction")]));
        install.Tag = receipt == null ? "install" : plan?.Package.Version == receipt.Version ? "reinstall" : "updateInstall";
        remove.Tag = hasPrevious ? "rollback" : "remove";
        install.Text = InstallerText.Get((string)install.Tag); remove.Text = InstallerText.Get((string)remove.Tag);
        install.Enabled = !busy && !pending && plan != null; remove.Enabled = !busy && !pending && receiptHash != null;
        recover.Enabled = !busy && pending; openSettings.Enabled = !busy && !pending && settings != null;
    }
    private void Inspect(bool setReady = true)
    {
        plan = null; receipt = null; receiptHash = null; settings = null; pending = false; hasPrevious = false;
        packageIncomplete = !PackageLocation.IsAvailable(packageRoot);
        try
        {
            root.Text = Paths.Root(root.Text);
            pending = engine.HasPending(root.Text); receipt = engine.Inspect(root.Text);
            if (receipt != null && !pending) { hasPrevious = engine.HasPreviousVersion(root.Text); receiptHash = engine.ReceiptHash(root.Text); }
            try { settings = SettingsLaunch.Resolve(root.Text, engine); } catch (Exception ex) { AppendLog(InstallerText.Error(ex)); }
            bool available = !packageIncomplete;
            if (!pending && available) plan = engine.Preview(root.Text, packageRoot);
            if (setReady) SetStatus(pending ? "pendingStatus" : available ? "ready" : "bundledPackageMissing");
        }
        catch (Exception ex)
        {
            if (ex is InstallError { Key: "manifestMissing" or "payloadMissing" or "incompletePackage" }) packageIncomplete = true;
            DisplayError(ex, false);
        }
        finally { RefreshDisplay(); }
    }
    private void Browse()
    {
        if (busy) return;
        using var dialog = new FolderBrowserDialog { Description = InstallerText.Get("folderHelp"), UseDescriptionForTitle = true };
        if (dialog.ShowDialog(this) != DialogResult.OK) return;
        root.Text = dialog.SelectedPath;
        InstallerText.Language = InstallerText.LoadPreference(root.Text); Translate();
        Inspect();
    }
    private bool Confirm(string message) => MessageBox.Show(this, message, InstallerText.Get("title"), MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes;
    private void DisplayError(Exception ex, bool modal)
    {
        statusKey = "errorTitle"; status.Text = InstallerText.Error(ex); AppendLog(status.Text);
        if (modal && smoke == null) MessageBox.Show(this, status.Text, InstallerText.Get("errorTitle"), MessageBoxButtons.OK, MessageBoxIcon.Error);
        else if (modal) throw ex;
    }
    private void Progress(string path)
    {
        int serial = operationSerial; int current = Interlocked.Increment(ref progressCurrent);
        if (IsHandleCreated) BeginInvoke(() =>
        {
            if (!busy || operationSerial != serial) return;
            progress.Value = Math.Clamp((current - 1) * 100 / Math.Max(1, progressTotal), 0, 99);
            status.Text = InstallerText.Format("workingCount", current, progressTotal, path); AppendLog(status.Text);
        });
    }
    private async Task Run(Func<OperationResult> work, string completed, int total)
    {
        busy = true; ++operationSerial; progressCurrent = 0; progressTotal = total; progress.Value = 0;
        progress.Style = total == 0 ? ProgressBarStyle.Marquee : ProgressBarStyle.Blocks;
        foreach (Control control in operations) control.Enabled = false; languages.Enabled = false;
        SetStatus("operationStarted");
        try { var result = await Task.Run(work); progress.Style = ProgressBarStyle.Blocks; progress.Value = 100; Inspect(false); SetStatus(result.RestoredPrevious ? "restoredPrevious" : completed); }
        catch (Exception ex) { progress.Style = ProgressBarStyle.Blocks; DisplayError(ex, true); Inspect(false); }
        finally { busy = false; languages.Enabled = true; foreach (Control control in operations) control.Enabled = true; RefreshDisplay(); }
    }
    private void VerifyPathRows()
    {
        PerformLayout();
        if (texts.Count(control => control is Button && control.Tag as string == "browse") != 1 ||
            texts.Any(control => control.Tag as string == "packageFolder"))
            throw new InvalidDataException("Installer must have exactly one game Browse and no package selection");
        var noticeSize = TextRenderer.MeasureText(bundleNotice.Text, bundleNotice.Font,
            new Size(bundleNotice.ClientSize.Width, int.MaxValue), TextFormatFlags.WordBreak | TextFormatFlags.TextBoxControl);
        if (noticeSize.Height > bundleNotice.ClientSize.Height)
            throw new InvalidDataException("Bundled-package guidance is clipped");
        foreach (TextBox value in new[] { root })
        {
            var browse = value.Parent!.Controls.OfType<Button>().Single();
            var pathBounds = value.Parent.RectangleToScreen(value.Bounds);
            var buttonBounds = browse.Parent!.RectangleToScreen(browse.Bounds);
            if (Math.Abs((pathBounds.Top + pathBounds.Bottom) - (buttonBounds.Top + buttonBounds.Bottom)) > 2)
                throw new InvalidDataException("Installer path and Browse are not vertically centered");
            foreach (Control child in new Control[] { value, browse })
                for (Control? parent = child.Parent; parent != null; parent = parent.Parent)
                    if (!parent.ClientRectangle.Contains(parent.RectangleToClient(child.Parent!.RectangleToScreen(child.Bounds))))
                        throw new InvalidDataException("Installer path or Browse border is clipped");
        }
    }

    private async void Smoke(TabControl details)
    {
        try
        {
            bool bundleAvailable = !packageIncomplete;
            if ((plan == null && bundleAvailable) || (smokeWorkflow && !bundleAvailable))
                throw new InvalidDataException("Fixture package preview missing");
            if (File.ReadAllText(Paths.GameExecutable(root.Text)) != SelfTest.Inert) throw new InvalidDataException("Only inert GUI fixtures are supported");
            Directory.CreateDirectory(smoke!);
            for (int i = 0; i < InstallerText.Languages.Length; i++)
            {
                ((Button)languages.Controls[i]).PerformClick(); PerformLayout();
                VerifyPathRows();
                if (InstallerText.LoadPreference(root.Text) != InstallerText.Languages[i] || install.Enabled != (plan != null && !pending) ||
                    remove.Enabled != (receiptHash != null && !pending) || (plan?.Package.Files.Length ?? receipt?.Files.Length ?? 0) != files.Items.Count ||
                    bundleNotice.Text != InstallerText.Get(bundleAvailable ? "bundledPackageHelp" : "bundledPackageMissing"))
                    throw new InvalidDataException("GUI state/locale mismatch");
                details.SelectedIndex = 0; using (var image = new Bitmap(Width, Height)) { DrawToBitmap(image, new Rectangle(0, 0, Width, Height)); image.Save(Path.Combine(smoke!, InstallerText.Language + ".png")); }
                details.SelectedIndex = 1; using var logImage = new Bitmap(Width, Height); DrawToBitmap(logImage, new Rectangle(0, 0, Width, Height)); logImage.Save(Path.Combine(smoke!, "log-" + InstallerText.Language + ".png"));
            }
            if (smokeWorkflow)
            {
                var chosen = plan!; await Run(() => engine.Install(chosen, Progress), "installed", chosen.Package.Files.Length + 1);
                if (progress.Value != 100 || receipt == null || busy) throw new InvalidDataException("Install progress/operation state mismatch");
                string hash = engine.ReceiptHash(root.Text); await Run(() => engine.Remove(root.Text, hash, Progress), "removed", receipt.Files.Length + 1);
                if (progress.Value != 100 || busy || !log.Text.Contains(InstallerText.Get("operationStarted"))) throw new InvalidDataException("Remove progress/log mismatch");
                details.SelectedIndex = 1; using var completed = new Bitmap(Width, Height); DrawToBitmap(completed, new Rectangle(0, 0, Width, Height)); completed.Save(Path.Combine(smoke!, "completed-workflow.png"));
            }
            Size fullSize = Size; Size = MinimumSize;
            foreach (string locale in InstallerText.Languages)
            {
                ChangeLanguage(locale); VerifyPathRows();
                using var compact = new Bitmap(Width, Height); DrawToBitmap(compact, new Rectangle(0, 0, Width, Height)); compact.Save(Path.Combine(smoke!, "minimum-" + locale + ".png"));
            }
            Size = fullSize;
            // Exercise scaled geometry without claiming physical monitor DPI acceptance.
            Scale(new SizeF(1.5f, 1.5f)); Size = MinimumSize;
            foreach (string locale in InstallerText.Languages)
            {
                ChangeLanguage(locale); VerifyPathRows();
                using var scaled = new Bitmap(Width, Height); DrawToBitmap(scaled, new Rectangle(0, 0, Width, Height)); scaled.Save(Path.Combine(smoke!, "scaled150-" + locale + ".png"));
            }
            File.WriteAllBytes(Path.Combine(smoke!, "gui-validation.json"), Contract.Encode(new { passed = true, locales = InstallerText.Languages, previewFiles = files.Items.Count, pathRowsCenteredAndUnclipped = 1, gameBrowseButtons = 1, packageSelectorVisible = false, bundledPackageFound = bundleAvailable, incompleteExtractionGuidance = !bundleAvailable, layoutScaleStress = 1.5, referenceActions = true, settingsAvailable = openSettings.Enabled, removeAvailable = remove.Enabled, recoveryAvailable = recover.Enabled, progressBar = true, retainedLog = true, installAction = install.Tag as string, removeAction = remove.Tag as string, fixtureOperations = smokeWorkflow ? 2 : 0, realInstallWrites = 0, gameLaunches = 0 }));
        }
        catch (Exception ex) { File.WriteAllText(Path.Combine(smoke!, "gui-error.txt"), ex.ToString()); Environment.ExitCode = 1; }
        finally { Close(); }
    }
}
