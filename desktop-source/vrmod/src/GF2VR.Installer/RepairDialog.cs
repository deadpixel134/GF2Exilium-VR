namespace GF2VR.Installer;

internal sealed class RepairDialog : Form
{
    private readonly CheckBox foreign = new() { AutoSize = true, Margin = new Padding(8) };
    internal bool IncludeUnowned => foreign.Checked;
    internal RepairDialog(RepairPlan plan)
    {
        Text = InstallerText.Get("repair"); Font = new Font("Segoe UI", 10); ClientSize = new Size(900, 560);
        MinimumSize = new Size(740, 460); AutoScaleMode = AutoScaleMode.Dpi; StartPosition = FormStartPosition.CenterParent;
        var layout = new TableLayoutPanel { Dock = DockStyle.Fill, Padding = new Padding(16), RowCount = 4, ColumnCount = 1 };
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize)); layout.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize)); layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.Controls.Add(new Label { AutoSize = true, MaximumSize = new Size(840, 0), Text = InstallerText.Get("repairReview") }, 0, 0);
        var listing = new TextBox { Dock = DockStyle.Fill, Multiline = true, ReadOnly = true, ScrollBars = ScrollBars.Both, WordWrap = false, Font = new Font("Consolas", 9), AccessibleName = "repair-hash-plan" };
        listing.Text = string.Join(Environment.NewLine + Environment.NewLine, plan.Files.Select(f => f.Path + (f.Unowned ? " [" + InstallerText.Get("repairUnowned") + "]" : "") + Environment.NewLine +
            InstallerText.Get("repairBefore") + ": " + (f.BeforeHash ?? InstallerText.Get("unavailable")) + Environment.NewLine + InstallerText.Get("repairAfter") + ": " + f.AfterHash));
        layout.Controls.Add(listing, 0, 1);
        bool hasForeign = plan.Files.Any(f => f.Unowned);
        foreign.Text = InstallerText.Get("repairIncludeUnowned"); foreign.Visible = hasForeign; layout.Controls.Add(foreign, 0, 2);
        var buttons = new FlowLayoutPanel { AutoSize = true, Dock = DockStyle.Fill, FlowDirection = FlowDirection.RightToLeft };
        var yes = new Button { Text = InstallerText.Get("repairConfirm"), AutoSize = true, MinimumSize = new Size(120, 36), DialogResult = DialogResult.Yes, Enabled = !hasForeign };
        var cancel = new Button { Text = InstallerText.Get("repairCancel"), AutoSize = true, MinimumSize = new Size(100, 36), DialogResult = DialogResult.Cancel };
        foreign.CheckedChanged += (_, _) => yes.Enabled = !hasForeign || foreign.Checked;
        buttons.Controls.Add(yes); buttons.Controls.Add(cancel); layout.Controls.Add(buttons, 0, 3);
        AcceptButton = yes; CancelButton = cancel; Controls.Add(layout);
    }
}
