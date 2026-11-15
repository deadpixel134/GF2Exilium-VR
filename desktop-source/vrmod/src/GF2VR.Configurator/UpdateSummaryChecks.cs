using System.Text.Json.Nodes;
using GF2VR.Installer;

namespace GF2VR.Configurator;

internal static class UpdateSummaryChecks
{
    internal static int Run()
    {
        int checks = 0;
        void Check(bool value, string why) { ++checks; if (!value) throw new InvalidDataException(why); }
        const string notes = "## ko\n- 설정 개선\n## en\n- Settings improvements\n## ja\n- 設定改善\n## zh-CN\n- 设置改进";
        var expected = new Dictionary<string, string> { ["ko"] = "설정 개선", ["en"] = "Settings improvements", ["ja"] = "設定改善", ["zh-CN"] = "设置改进" };
        string originalLanguage = Texts.Language;
        Texts.Verify();
        foreach (var pair in expected)
        {
            Texts.Language = pair.Key;
            Check(UpdateSummary.Create(notes, pair.Key) == "• " + pair.Value, "locale isolation " + pair.Key);
            using var dialog = new UpdateNoticeDialog("1.3.12", true, releaseNotes: notes);
            dialog.StartPosition = FormStartPosition.Manual; dialog.Location = new Point(-20000, -20000);
            dialog.ShowInTaskbar = false; dialog.Show(); Application.DoEvents();
            var summary = dialog.Controls.Find("updateSummary", true).OfType<TextBox>().Single();
            Check(summary.Text == "• " + pair.Value && summary.ReadOnly && summary.Multiline, "packaged popup summary " + pair.Key);
            Check(summary.Width > 300 && summary.Height > 80, "summary area fits " + pair.Key);
            using var image = new Bitmap(dialog.Width, dialog.Height);
            dialog.DrawToBitmap(image, new Rectangle(0, 0, image.Width, image.Height));
            image.Save(Path.Combine(AppContext.BaseDirectory, "update-summary-" + pair.Key + ".png"));
            dialog.Close();
            using var missing = new UpdateNoticeDialog("1.3.12", false);
            Check(missing.Controls.Find("updateSummary", true).OfType<TextBox>().Single().Text == Texts.Get("updateSummaryMissing"), "localized missing summary " + pair.Key);
        }
        Texts.Language = originalLanguage;
        Check(UpdateSummary.Create(null, "ko") == "" && UpdateSummary.Create("", "en") == "", "absent notes");
        Check(UpdateSummary.Create(new string('x', 65537), "en") == "", "oversize notes");
        Check(UpdateSummary.Create("## en\n- English fallback\n## ja\n- Japanese", "ko") == "• English fallback", "missing locale falls back to English");
        Check(UpdateSummary.Create("## ko\n- 한국어", "ja") == "• 한국어", "Korean fallback");
        Check(UpdateSummary.Create("## ja\n![image](https://example.com/a.png)\n## en\n- Usable", "ja") == "• Usable", "empty localized section fallback");
        Check(UpdateSummary.Create("## English\n1. **Fixed** [controls](https://example.com)\n![image](a.png)\n```\n- code\n```\n|table|", "en") == "• Fixed controls", "plain text excludes code/images/tables and link destinations");
        Check(UpdateSummary.Create("## Release notes\nA short description.", "en") == "• A short description.", "unsectioned notes");
        Check(UpdateSummary.Create("## ko\n- 한글\n## en\n- English", "en-US") == "• English", "locale normalization");
        string bounded = UpdateSummary.Create(string.Join('\n', Enumerable.Range(1, 12).Select(i => "- " + new string('a', 300))), "en");
        Check(bounded.Split(Environment.NewLine).Length == 5 && bounded.Split(Environment.NewLine).All(s => s.Length <= 182), "item count and length limits");
        Check(UpdateSummary.Create("- <b>Fix</b> &amp; polish\u202e", "en") == "• Fix & polish", "HTML entities and format controls");
        var stable = new JsonObject { ["draft"] = false, ["prerelease"] = false, ["published_at"] = "2026-11-15T00:00:00Z", ["tag_name"] = "v1.3.12", ["body"] = notes };
        byte[] Catalog(params JsonNode?[] rows) => Contract.Encode(new JsonArray(rows));
        Check(UpdatePolicy.ReleaseBody(Catalog(stable.DeepClone()), "1.3.12") == notes, "exact discovered release body");
        Check(UpdatePolicy.ReleaseBody(Catalog(stable.DeepClone()), "1.3.13") == null, "different version never used");
        var draft = stable.DeepClone().AsObject(); draft["draft"] = true;
        Check(UpdatePolicy.ReleaseBody(Catalog(draft), "1.3.12") == null, "draft ignored");
        var preview = stable.DeepClone().AsObject(); preview["prerelease"] = true;
        Check(UpdatePolicy.ReleaseBody(Catalog(preview), "1.3.12") == null, "prerelease ignored");
        Check(UpdatePolicy.ReleaseBody(Catalog(stable.DeepClone(), stable.DeepClone()), "1.3.12") == null, "ambiguous release notes ignored");
        foreach (JsonNode? body in new JsonNode?[] { null, JsonValue.Create(42), JsonValue.Create(new string('x', 65537)) })
        {
            var bad = stable.DeepClone().AsObject(); bad["body"] = body;
            Check(UpdatePolicy.ReleaseBody(Catalog(bad), "1.3.12") == null, "invalid/missing/oversized body fallback");
        }
        File.WriteAllBytes(Path.Combine(AppContext.BaseDirectory, "update-summary-validation.json"), Contract.Encode(new { passed = true, checks, locales = Texts.Languages, maxItems = 5, maxItemLength = 180, gameLaunches = 0, installationWrites = 0 }));
        return 0;
    }
}
