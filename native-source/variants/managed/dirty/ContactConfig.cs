#if GF2VR_TOUCH_PRIVATE
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Text;

namespace GF2.Private
{
    // Cold bounded parser. Native also validates the complete JSON document.
    internal sealed class ContactSettings
    {
        internal bool Enabled = true;
        internal float HandRadius = .035f, BodyRadius = .06f, MaxDisplacement = .02f;
        internal float Frequency = 2.25f, Damping = .4f;
        internal float HandHoldSeconds = .1f, ReleaseImpulse = .3f;
        internal string[] Targets = { "Chest_L", "Chest_R", "Butt_L_Back01", "Butt_R_Back01" };

        internal static bool Read(byte[] utf8, out ContactSettings result)
        {
            result = new ContactSettings();
            if (utf8 == null) return true;
            try
            {
                if (utf8.Length == 0 || utf8.Length > 4096) throw new FormatException();
                Reader reader = new Reader(new UTF8Encoding(false, true).GetString(utf8));
                reader.Space(); reader.Take('{'); reader.Space();
                HashSet<string> seen = new HashSet<string>(StringComparer.Ordinal);
                bool schema = false;
                if (!reader.At('}')) do
                {
                    string key = reader.String();
                    if (!seen.Add(key)) throw new FormatException();
                    reader.Space(); reader.Take(':'); reader.Space();
                    if (key == "schemaVersion") { if (reader.Number() != 1) throw new FormatException(); schema = true; }
                    else if (key == "enabled") result.Enabled = reader.Boolean();
                    else if (key == "handRadiusMeters") result.HandRadius = reader.Bounded(.005, .15);
                    else if (key == "bodyRadiusMeters") result.BodyRadius = reader.Bounded(.005, .20);
                    else if (key == "maxDisplacementMeters") result.MaxDisplacement = reader.Bounded(.0001, .05);
                    else if (key == "springFrequencyHz") result.Frequency = reader.Bounded(.1, 15);
                    else if (key == "dampingRatio") result.Damping = reader.Bounded(.1, 2);
                    else if (key == "handHoldSeconds") result.HandHoldSeconds = reader.Bounded(.016, .25);
                    else if (key == "releaseImpulse") result.ReleaseImpulse = reader.Bounded(0, 1);
                    else if (key == "targetBones") result.Targets = reader.Targets();
                    else reader.Skip(0);
                    reader.Space();
                    if (!reader.At(',')) break;
                    reader.Take(','); reader.Space();
                    if (reader.At('}')) throw new FormatException();
                } while (true);
                reader.Take('}'); reader.Space();
                if (!schema || !reader.End) throw new FormatException();
                return true;
            }
            catch (Exception) { result.Enabled = false; return false; }
        }

        private sealed class Reader
        {
            private readonly string text; private int index;
            // The existing native loader accepts one UTF-8 BOM at byte zero.
            internal Reader(string value) { text = value; index = text.Length != 0 && text[0] == '\ufeff' ? 1 : 0; }
            internal bool End { get { return index == text.Length; } }
            internal void Space() { while (index < text.Length && (text[index] == ' ' || text[index] == '\n' || text[index] == '\r' || text[index] == '\t')) ++index; }
            internal bool At(char value) { return index < text.Length && text[index] == value; }
            internal void Take(char value) { if (!At(value)) throw new FormatException(); ++index; }
            internal string String()
            {
                Space(); Take('"'); StringBuilder value = new StringBuilder();
                while (index < text.Length)
                {
                    char c = text[index++];
                    if (c == '"') return value.ToString();
                    if (c < 32) throw new FormatException();
                    if (c == '\\')
                    {
                        if (index == text.Length) throw new FormatException(); c = text[index++];
                        if (c == 'u')
                        {
                            int number = 0;
                            for (int i = 0; i < 4; ++i)
                            {
                                if (index == text.Length) throw new FormatException(); char h = text[index++];
                                int n = h >= '0' && h <= '9' ? h - '0' : h >= 'a' && h <= 'f' ? h - 'a' + 10 : h >= 'A' && h <= 'F' ? h - 'A' + 10 : -1;
                                if (n < 0) throw new FormatException(); number = number * 16 + n;
                            }
                            c = (char)number;
                        }
                        else if (c == 'b') c = '\b'; else if (c == 'f') c = '\f';
                        else if (c == 'n') c = '\n'; else if (c == 'r') c = '\r'; else if (c == 't') c = '\t';
                        else if (c != '"' && c != '\\' && c != '/') throw new FormatException();
                    }
                    value.Append(c); if (value.Length > 4096) throw new FormatException();
                }
                throw new FormatException();
            }
            private void Literal(string value) { for (int i = 0; i < value.Length; ++i) { if (index == text.Length || text[index++] != value[i]) throw new FormatException(); } }
            internal bool Boolean() { if (At('t')) { Literal("true"); return true; } Literal("false"); return false; }
            internal double Number()
            {
                int first = index;
                if (At('-')) ++index;
                if (At('0')) ++index;
                else { if (index == text.Length || text[index] < '1' || text[index] > '9') throw new FormatException(); while (index < text.Length && text[index] >= '0' && text[index] <= '9') ++index; }
                if (At('.')) { ++index; int start = index; while (index < text.Length && text[index] >= '0' && text[index] <= '9') ++index; if (start == index) throw new FormatException(); }
                if (At('e') || At('E')) { ++index; if (At('+') || At('-')) ++index; int start = index; while (index < text.Length && text[index] >= '0' && text[index] <= '9') ++index; if (start == index) throw new FormatException(); }
                double number = Double.Parse(text.Substring(first, index - first), NumberStyles.Float, CultureInfo.InvariantCulture);
                if (Double.IsNaN(number) || Double.IsInfinity(number)) throw new FormatException(); return number;
            }
            internal float Bounded(double minimum, double maximum) { double value = Number(); if (value < minimum || value > maximum) throw new FormatException(); return (float)value; }
            internal string[] Targets()
            {
                Take('['); Space(); List<string> values = new List<string>(); HashSet<string> seen = new HashSet<string>(StringComparer.Ordinal);
                if (!At(']')) do
                {
                    string name = String(); if (name.Length == 0 || name.Length > 64 || values.Count == 32 || !seen.Add(name)) throw new FormatException();
                    for (int i = 0; i < name.Length; ++i) if (name[i] < 32 || Char.IsSurrogate(name[i])) throw new FormatException();
                    values.Add(name); Space(); if (!At(',')) break; Take(','); Space(); if (At(']')) throw new FormatException();
                } while (true);
                Take(']'); return values.ToArray();
            }
            internal void Skip(int depth)
            {
                if (depth > 16) throw new FormatException(); Space();
                if (At('"')) { String(); return; }
                if (At('{'))
                {
                    Take('{'); Space(); HashSet<string> seen = new HashSet<string>(StringComparer.Ordinal);
                    if (!At('}')) do { string key = String(); if (!seen.Add(key)) throw new FormatException(); Space(); Take(':'); Skip(depth + 1); Space(); if (!At(',')) break; Take(','); Space(); if (At('}')) throw new FormatException(); } while (true);
                    Take('}'); return;
                }
                if (At('[')) { Take('['); Space(); if (!At(']')) do { Skip(depth + 1); Space(); if (!At(',')) break; Take(','); Space(); if (At(']')) throw new FormatException(); } while (true); Take(']'); return; }
                if (At('t')) { Literal("true"); return; } if (At('f')) { Literal("false"); return; } if (At('n')) { Literal("null"); return; } Number();
            }
        }
    }
}
#endif
