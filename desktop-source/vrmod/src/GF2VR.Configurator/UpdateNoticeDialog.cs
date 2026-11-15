namespace GF2VR.Configurator;

internal sealed class UpdateNoticeDialog : Form
{
    private readonly CheckBox skip = new() { AutoSize = true, Margin = new Padding(12) };
    internal bool SkipVersion => skip.Checked;
    internal UpdateNoticeDialog(string version, bool applicable, string? detail = null, string? releaseNotes = null)
    {
        Text = Texts.Get("title"); Font = new Font("Segoe UI", 10); ClientSize = new Size(680, 410);
        AutoScaleMode = AutoScaleMode.Dpi; StartPosition = FormStartPosition.CenterParent;
        MinimizeBox = MaximizeBox = false; FormBorderStyle = FormBorderStyle.FixedDialog;
        var layout = new TableLayoutPanel { Dock = DockStyle.Fill, Padding = new Padding(16), RowCount = 6, ColumnCount = 1 };
        layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        for (int row = 0; row < 6; ++row) layout.RowStyles.Add(new RowStyle(row == 2 ? SizeType.Percent : SizeType.AutoSize, row == 2 ? 100 : 0));
        layout.Controls.Add(new Label { AutoSize = true, Dock = DockStyle.Fill, Text = Texts.Format(applicable ? "updateNoticeAvailable" : "updateManualVersion", version) }, 0, 0);
        layout.Controls.Add(new Label { AutoSize = true, Dock = DockStyle.Fill, Text = Texts.Get("updateSummaryTitle"), Margin = new Padding(3, 12, 3, 6) }, 0, 1);
        string summary = UpdateSummary.Create(releaseNotes, Texts.Language);
        layout.Controls.Add(new TextBox { Name = "updateSummary", Dock = DockStyle.Fill, Multiline = true, ReadOnly = true,
            ScrollBars = ScrollBars.Vertical, WordWrap = true, Text = summary.Length == 0 ? Texts.Get("updateSummaryMissing") : summary,
            AccessibleName = Texts.Get("updateSummaryTitle") }, 0, 2);
        layout.Controls.Add(new Label { AutoSize = true, Dock = DockStyle.Fill, MaximumSize = new Size(640, 0),
            Text = detail ?? Texts.Get(applicable ? "updateNoticeHint" : "updateManualHint"), Margin = new Padding(3, 10, 3, 0) }, 0, 3);
        skip.Text = Texts.Get("updateSkipVersion"); layout.Controls.Add(skip, 0, 4);
        var close = new Button { Text = Texts.Get("close"), AutoSize = true, MinimumSize = new Size(100, 34), DialogResult = DialogResult.OK, Anchor = AnchorStyles.Right };
        layout.Controls.Add(close, 0, 5); AcceptButton = CancelButton = close; Controls.Add(layout);
    }
}
