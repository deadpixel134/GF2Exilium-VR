using System.Net;
using System.Text.RegularExpressions;

namespace GF2VR.Configurator;

internal static class UpdateSummary
{
    internal const int MaxItems = 5, MaxItemLength = 180;
    internal static string Create(string? markdown, string locale)
    {
        if (string.IsNullOrWhiteSpace(markdown) || markdown.Length > 65536) return "";
        var sections = new Dictionary<string, List<string>>(StringComparer.Ordinal);
        var preamble = new List<string>(); List<string> current = preamble; bool fenced = false;
        foreach (string raw in markdown.Replace("\r", "").Split('\n'))
        {
            string line = raw.Trim();
            if (line.StartsWith("```") || line.StartsWith("~~~")) { fenced = !fenced; continue; }
            if (fenced) continue;
            if (line.StartsWith('#'))
            {
                string heading = line.Trim('#', ' ', '\t').ToLowerInvariant();
                string? language = heading switch
                {
                    "ko" or "한국어" or "korean" => "ko",
                    "en" or "english" => "en",
                    "ja" or "日本語" or "japanese" => "ja",
                    "zh-cn" or "简体中文" or "简体中文（zh-cn）" or "chinese" => "zh-CN",
                    _ => null
                };
                if (language != null)
                {
                    if (!sections.TryGetValue(language, out var section)) sections[language] = section = [];
                    current = section;
                }
                continue;
            }
            current.Add(line);
        }
        var languageOrder = new[] { Texts.Normalize(locale), "en", "ko" }.Concat(sections.Keys).Distinct();
        foreach (var lines in sections.Count == 0 ? new[] { preamble } : languageOrder.Where(sections.ContainsKey).Select(k => sections[k]))
        {
            var cleaned = lines.Where(l => l.Length > 0 && !l.StartsWith("![") && !l.StartsWith('|'))
                .Select(l => (Bullet: Regex.IsMatch(l, @"^(?:[-*+]\s+|\d+[.)]\s+)"), Text: Plain(l)))
                .Where(l => l.Text.Length > 0).ToArray();
            var chosen = cleaned.Any(l => l.Bullet) ? cleaned.Where(l => l.Bullet) : cleaned;
            string result = string.Join(Environment.NewLine, chosen.Take(MaxItems).Select(l => "• " + Shorten(l.Text)));
            if (result.Length > 0) return result;
        }
        return "";
    }
    private static string Plain(string text)
    {
        text = Regex.Replace(text, @"^(?:[-*+]\s+|\d+[.)]\s+)", "");
        text = Regex.Replace(text, @"!?\[([^\]]*)\]\([^)]*\)", "$1");
        text = Regex.Replace(text, @"<[^>]*>", "");
        text = WebUtility.HtmlDecode(text);
        text = Regex.Replace(text, @"https?://\S+", "");
        text = new string(text.Where(c => !char.IsControl(c) && char.GetUnicodeCategory(c) != System.Globalization.UnicodeCategory.Format).ToArray());
        return Regex.Replace(text.Replace("**", "").Replace("__", "").Replace("`", ""), @"\s+", " ").Trim();
    }
    private static string Shorten(string text)
    {
        if (text.Length <= MaxItemLength) return text;
        int end = MaxItemLength - 1;
        if (char.IsHighSurrogate(text[end - 1])) --end;
        return text[..end].TrimEnd() + "…";
    }
}
