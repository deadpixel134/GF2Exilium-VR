namespace GF2VR.Installer;

internal sealed class InstallerForm : Form
{
    private readonly InstallationEngine engine = new();
    private readonly TextBox root = new() { Dock = DockStyle.Fill, ReadOnly = true };
    private readonly TextBox package = new() { Dock = DockStyle.Fill, ReadOnly = true };
    private readonly ComboBox language = new() { DropDownStyle = ComboBoxStyle.DropDownList, Width = 128 };
    private readonly Label summary = new() { Dock = DockStyle.Fill, AutoSize = false };
    private readonly Label status = new() { Dock = DockStyle.Fill, AutoEllipsis = true, TextAlign = ContentAlignment.MiddleLeft };
    private readonly ListView files = new() { Dock = DockStyle.Fill, View = View.Details, FullRowSelect = true, HideSelection = false };
    private readonly Button install = new() { AutoSize = true, MinimumSize = new Size(115, 36), Tag = "install" };
    private readonly Button remove = new() { AutoSize = true, MinimumSize = new Size(115, 36), Tag = "remove" };
    private readonly Button recover = new() { AutoSize = true, MinimumSize = new Size(100, 36), Tag = "recover" };
    private readonly List<Control> texts = [];
    private readonly List<Control> operations = [];
    private readonly string? smoke;
    private Plan? plan;
    private Receipt? receipt;
    private string? receiptHash;
    private bool pending, busy, loading;
    private string statusKey = "ready";

    internal InstallerForm(string gameRoot, string? packageRoot, string? smokeFolder = null)
    {
        smoke = smokeFolder; Font = new Font("Segoe UI", 10); Size = new Size(1050, 850); MinimumSize = new Size(960, 800);
        StartPosition = FormStartPosition.CenterScreen; BackColor = Color.FromArgb(244, 247, 251); Icon = SystemIcons.Application;
        if (smoke != null) { StartPosition = FormStartPosition.Manual; Location = new Point(-20000, -20000); ShowInTaskbar = false; }
        root.Text = gameRoot; package.Text = packageRoot ?? "";
        var layout = new TableLayoutPanel { Dock = DockStyle.Fill, Padding = new Padding(18), ColumnCount = 1, RowCount = 7 };
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 70));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 42)); layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 42));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 84)); layout.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 158)); layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 44));
        Controls.Add(layout);
        var banner = new Panel { Dock = DockStyle.Fill, BackColor = Color.FromArgb(30, 58, 92), Padding = new Padding(15, 5, 15, 5) };
        var heading = Label("title"); heading.Font = new Font(Font.FontFamily, 19, FontStyle.Bold); heading.ForeColor = Color.White;
        heading.AutoSize = false; heading.Dock = DockStyle.Fill; heading.TextAlign = ContentAlignment.MiddleLeft;
        banner.Controls.Add(heading); layout.Controls.Add(banner, 0, 0);
        var rootRow = Row("folder", root, (_, _) => Browse(root));
        rootRow.ColumnCount = 4; rootRow.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize)); rootRow.Controls.Add(language, 3, 0);
        layout.Controls.Add(rootRow, 0, 1); layout.Controls.Add(Row("packageFolder", package, (_, _) => Browse(package)), 0, 2);
        language.Items.AddRange(["한국어", "English", "日本語"]);
        InstallerText.Language = InstallerText.LoadPreference(gameRoot); loading = true;
        language.SelectedIndex = Array.IndexOf(InstallerText.Languages, InstallerText.Language); loading = false;
        language.SelectedIndexChanged += (_, _) =>
        {
            if (loading) return; InstallerText.Language = InstallerText.Languages[language.SelectedIndex]; Translate();
            try { InstallerText.SavePreference(root.Text, InstallerText.Language); SetStatus("languageSaved"); }
            catch (Exception ex) { DisplayError(ex); }
        };
        summary.Font = new Font(Font.FontFamily, 9); summary.Padding = new Padding(4, 8, 4, 4); layout.Controls.Add(summary, 0, 3);
        files.Columns.Add("", 680); files.Columns.Add("", 150); layout.Controls.Add(files, 0, 4);
        var advice = Label("policy"); advice.AutoSize = false; advice.Dock = DockStyle.Fill; advice.ForeColor = Color.FromArgb(71, 87, 110);
        advice.Font = new Font(Font.FontFamily, 9);
        advice.Margin = new Padding(4, 10, 4, 5); layout.Controls.Add(advice, 0, 5);
        var footer = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 5 };
        footer.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        for (int i = 0; i < 4; ++i) footer.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        footer.Controls.Add(status, 0, 0); footer.Controls.Add(Button("inspect", (_, _) => Inspect()), 1, 0);
        texts.Add(install); texts.Add(remove); texts.Add(recover); operations.AddRange([install, remove, recover]);
        install.Click += async (_, _) =>
        {
            var chosen = plan; if (chosen == null) return;
            if (!Confirm(InstallerText.Format("confirmInstall", chosen.Package.Version, chosen.Package.Files.Length, chosen.Root))) return;
            await Run(() => engine.Install(chosen, Progress), "installed");
        };
        remove.Click += async (_, _) =>
        {
            string selectedRoot = root.Text; string? hash = receiptHash;
            if (hash == null || !Confirm(InstallerText.Get("confirmRemove"))) return;
            await Run(() => engine.Remove(selectedRoot, hash, Progress), "removed");
        };
        recover.Click += async (_, _) =>
        {
            string selectedRoot = root.Text; if (!Confirm(InstallerText.Get("confirmRecover"))) return;
            await Run(() => { engine.Recover(selectedRoot); return new OperationResult("", false, ""); }, "recovered");
        };
        footer.Controls.Add(recover, 2, 0); footer.Controls.Add(remove, 3, 0); footer.Controls.Add(install, 4, 0); layout.Controls.Add(footer, 0, 6);
        FormClosing += (_, e) => { if (busy) e.Cancel = true; };
        Translate(); Inspect(); if (smoke != null) Shown += (_, _) => Smoke();
    }
    private Label Label(string key)
    {
        var l = new Label { Tag = key, AutoSize = true, Margin = new Padding(4, 8, 8, 4) }; texts.Add(l); return l;
    }
    private Button Button(string key, EventHandler action)
    {
        var b = new Button { AutoSize = true, MinimumSize = new Size(80, 32), Tag = key, Margin = new Padding(4, 0, 0, 0) };
        texts.Add(b); operations.Add(b); b.Click += action; return b;
    }
    private TableLayoutPanel Row(string key, TextBox value, EventHandler action)
    {
        var row = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 3, Padding = new Padding(0, 6, 0, 0) };
        row.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 154)); row.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        row.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        row.Controls.Add(Label(key), 0, 0); row.Controls.Add(value, 1, 0); row.Controls.Add(Button("browse", action), 2, 0); return row;
    }
    private void Translate()
    {
        Text = InstallerText.Get("title"); foreach (Control c in texts) c.Text = InstallerText.Get((string)c.Tag!);
        files.Columns[0].Text = InstallerText.Get("fileColumn"); files.Columns[1].Text = InstallerText.Get("actionColumn");
        SetStatus(statusKey); RefreshDisplay();
    }
    private void SetStatus(string key) { statusKey = key; status.Text = InstallerText.Get(key); }
    private void RefreshDisplay()
    {
        summary.Text = InstallerText.Format("summary", plan?.Package.Version ?? InstallerText.Get("unavailable"),
            receipt?.Version ?? InstallerText.Get("noReceipt"), (plan?.Package.Scope ?? receipt?.Scope) == "tools" ? InstallerText.Get("toolsScope") :
            plan?.Package.Scope == "vrmod" ? InstallerText.Get(NativeLayout.IsPortable(plan.Package) ? "portableScope" : "nativeScope") : InstallerText.Get("noRuntimeRelease"));
        files.Items.Clear();
        foreach (var f in plan?.Package.Files ?? receipt?.Files ?? [])
            files.Items.Add(new ListViewItem([f.Path, InstallerText.Get(plan == null ? "ownedAction" : receipt == null ? "newAction" : "updateAction")]));
        install.Enabled = !busy && !pending && plan != null;
        remove.Enabled = !busy && !pending && receipt != null;
        recover.Enabled = !busy && pending;
    }
    private void Inspect()
    {
        plan = null; receipt = null; receiptHash = null; pending = false;
        try
        {
            pending = engine.HasPending(root.Text);
            receipt = engine.Inspect(root.Text);
            if (receipt != null) receiptHash = engine.ReceiptHash(root.Text);
            if (!pending && package.Text.Length > 0) plan = engine.Preview(root.Text, package.Text);
            SetStatus(pending ? "pendingStatus" : "ready");
        }
        catch (Exception ex) { DisplayError(ex); }
        finally { RefreshDisplay(); }
    }
    private void Browse(TextBox target)
    {
        using var dialog = new FolderBrowserDialog { Description = InstallerText.Get(target == root ? "folderHelp" : "packageHelp"), UseDescriptionForTitle = true };
        if (dialog.ShowDialog(this) == DialogResult.OK)
        {
            target.Text = dialog.SelectedPath;
            if (target == root)
            {
                InstallerText.Language = InstallerText.LoadPreference(root.Text); loading = true;
                language.SelectedIndex = Array.IndexOf(InstallerText.Languages, InstallerText.Language); loading = false; Translate();
            }
            Inspect();
        }
    }
    private bool Confirm(string message) => MessageBox.Show(this, message, InstallerText.Get("title"),
        MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes;
    private void DisplayError(Exception ex)
    {
        SetStatus("errorTitle");
        if (smoke == null) MessageBox.Show(this, InstallerText.Error(ex), InstallerText.Get("errorTitle"), MessageBoxButtons.OK, MessageBoxIcon.Error);
        else throw ex;
    }
    private void Progress(string path)
    {
        if (IsHandleCreated) BeginInvoke(() => status.Text = InstallerText.Format("working", path));
    }
    private async Task Run(Func<OperationResult> work, string completed)
    {
        busy = true; foreach (Control c in operations) c.Enabled = false; language.Enabled = false;
        try
        {
            var result = await Task.Run(work); Inspect(); SetStatus(result.RestoredPrevious ? "restoredPrevious" : completed);
        }
        catch (Exception ex) { DisplayError(ex); Inspect(); }
        finally { busy = false; language.Enabled = true; foreach (Control c in operations) c.Enabled = true; RefreshDisplay(); }
    }
    private void Smoke()
    {
        try
        {
            if (plan == null) throw new InvalidDataException("Fixture package preview missing");
            Directory.CreateDirectory(smoke!);
            for (int i = 0; i < InstallerText.Languages.Length; i++)
            {
                InstallerText.Language = InstallerText.Languages[i]; loading = true; language.SelectedIndex = i; loading = false;
                Translate(); PerformLayout();
                if (!install.Enabled || remove.Enabled || plan.Package.Files.Length != files.Items.Count) throw new InvalidDataException("GUI preflight state mismatch");
                using var image = new Bitmap(Width, Height); DrawToBitmap(image, new Rectangle(0, 0, Width, Height));
                image.Save(Path.Combine(smoke!, InstallerText.Language + ".png"));
            }
            File.WriteAllBytes(Path.Combine(smoke!, "gui-validation.json"), Contract.Encode(new { passed = true, locales = InstallerText.Languages, previewFiles = files.Items.Count, realInstallWrites = 0, gameLaunches = 0 }));
        }
        catch (Exception ex) { File.WriteAllText(Path.Combine(smoke!, "gui-error.txt"), ex.ToString()); }
        finally { Close(); }
    }
}
