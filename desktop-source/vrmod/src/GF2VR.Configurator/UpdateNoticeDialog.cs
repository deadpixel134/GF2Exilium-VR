namespace GF2VR.Configurator;

internal sealed class UpdateNoticeDialog : Form
{
    private readonly CheckBox skip = new() { AutoSize = true, Margin = new Padding(12) };
    internal bool SkipVersion => skip.Checked;
    internal UpdateNoticeDialog(string version, bool applicable, string? detail = null)
    {
        Text = Texts.Get("title"); Font = new Font("Segoe UI", 10); ClientSize = new Size(660, 260);
        AutoScaleMode = AutoScaleMode.Dpi; StartPosition = FormStartPosition.CenterParent;
        MinimizeBox = MaximizeBox = false; FormBorderStyle = FormBorderStyle.FixedDialog;
        var layout = new TableLayoutPanel { Dock = DockStyle.Fill, Padding = new Padding(16), RowCount = 3, ColumnCount = 1 };
        layout.RowStyles.Add(new RowStyle(SizeType.Percent, 100)); layout.RowStyles.Add(new RowStyle(SizeType.AutoSize)); layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.Controls.Add(new Label { Dock = DockStyle.Fill, Text = Texts.Format(applicable ? "updateNoticeAvailable" : "updateManualVersion", version) + "\n\n" + (detail ?? Texts.Get(applicable ? "updateNoticeHint" : "updateManualHint")), AutoEllipsis = true }, 0, 0);
        skip.Text = Texts.Get("updateSkipVersion"); layout.Controls.Add(skip, 0, 1);
        var close = new Button { Text = Texts.Get("close"), AutoSize = true, MinimumSize = new Size(100, 34), DialogResult = DialogResult.OK, Anchor = AnchorStyles.Right };
        layout.Controls.Add(close, 0, 2); AcceptButton = CancelButton = close; Controls.Add(layout);
    }
}
