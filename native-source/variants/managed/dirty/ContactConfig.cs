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
        internal float HandRadius=.035f, BodyRadius=.06f, HandHoldSeconds=.1f;
        internal float StrengthPercent=40f, DurationMultiplier=.4f, RangePercent=75f, TiltPercent=50f, StretchPercent=45f;
        internal ContactKnobs BreastKnobs,ButtKnobs;
        internal struct ContactKnobs
        {
            internal float Strength,Duration,Range,Tilt,Stretch;
            internal void Set(int k,float v){switch(k){case 0:Strength=v;break;case 1:Duration=v;break;case 2:Range=v;break;case 3:Tilt=v;break;case 4:Stretch=v;break;}}
            internal void Merge(ContactKnobs other,uint mask){for(int k=0;k<5;++k)if((mask&(1u<<k))!=0)Set(k,other.Get(k));}
            internal float Get(int k){return k==0?Strength:k==1?Duration:k==2?Range:k==3?Tilt:Stretch;}
        }
        private ContactKnobs CommonKnobs(){return new ContactKnobs{Strength=StrengthPercent,Duration=DurationMultiplier,Range=RangePercent,Tilt=TiltPercent,Stretch=StretchPercent};}
        private static ContactKnobs DefaultKnobs(bool butt){return butt?
            new ContactKnobs{Strength=120,Duration=.5f,Range=120,Tilt=120,Stretch=140}:
            new ContactKnobs{Strength=40,Duration=.4f,Range=75,Tilt=50,Stretch=45};}
        private static readonly string[] KnobNames={"jiggleStrengthPercent","jiggleDurationMultiplier","jiggleRangePercent","jiggleTiltPercent","jiggleStretchPercent"};
        internal ContactMaterial Breast, Butt;
        // Legacy diagnostics and old fixture adapters read the breast profile.
        internal float Frequency { get { return Breast.Frequency; } }
        internal float Damping { get { return Breast.Damping; } }
        internal float MaxDisplacement { get { return Breast.MaxDisplacement; } }
        internal float ReleaseImpulse { get { return Breast.ReleaseImpulse; } }
        internal string[] Targets = { "Chest_L", "Chest_R", "Butt_L_Back01", "Butt_R_Back01" };
        private static readonly string[] Names={"springFrequencyHz","dampingRatio","maxDisplacementMeters","releaseImpulse","maxTiltDegrees","maxStretch","maxSquash","pressSquashGain","maxPressSquash","stiffeningGain","verticalCompliance","lateralCompliance","depthCompliance","tiltFrequencyHz","tiltDampingRatio","tiltGain","stretchGain","lrDetune"};

        internal ContactSettings()
        {
            Breast=ContactMaterial.Defaults(false);Butt=ContactMaterial.Defaults(true);
            BreastKnobs=DefaultKnobs(false);ButtKnobs=DefaultKnobs(true);
            Finish(ref Breast,0,BreastKnobs);Finish(ref Butt,0,ButtKnobs);
        }
        internal static int Region(string name)
        {
            return name!=null && name.StartsWith("Chest_",StringComparison.Ordinal)?1:
                name!=null && name.StartsWith("Butt_",StringComparison.Ordinal)?2:0;
        }
        internal ContactMaterial ForBone(string name)
        {
            ContactMaterial m=Region(name)==2?Butt:Breast;
            if(name!=null && (name.EndsWith("_R",StringComparison.Ordinal)||name.IndexOf("_R_",StringComparison.Ordinal)>=0))
            {m.Frequency*=1+m.Detune;m.TiltFrequency*=1+m.Detune;}
            return m;
        }
        // Only representation and physical domains are constraints. Presets
        // remain recommendations; no frequency/amplitude/duration taste cap.
        private static float Representable(double value,bool positive)
        {
            float f=(float)value;
            if(Double.IsNaN(value)||Double.IsInfinity(value)||Single.IsNaN(f)||Single.IsInfinity(f)||
                value<0||(positive&&value<=0)||(value!=0&&f==0))throw new FormatException("contact numeric domain");
            return f;
        }
        private static float Compression(double value)
        {
            // A zero/negative scale makes volume preservation singular. This
            // is the largest positive float below100%, not a product cap.
            return Representable(Math.Min(.999999940395355224609375,value),false);
        }
        private static void CheckMaterial(ContactMaterial m)
        {
            for(int k=0;k<18;++k)Representable(m.Get(k),false);
            Representable((double)m.Frequency*(1+(double)m.Detune),false);
            Representable((double)m.TiltFrequency*(1+(double)m.Detune),false);
            if(m.MaxTilt>0)Representable(m.MaxTilt*Math.PI/180,true);
        }
        private void Finish(ref ContactMaterial m,uint explicitMask,ContactKnobs knobs)
        {
            if((explicitMask&(1u<<13))==0)m.TiltFrequency=Representable((double)m.Frequency*1.3,false);
            if((explicitMask&(1u<<14))==0)m.TiltDamping=Representable((double)m.Damping*.85,false);
            m.ReleaseImpulse=Representable((double)m.ReleaseImpulse*knobs.Strength/100,false);
            m.TiltGain=Representable((double)m.TiltGain*knobs.Strength/100,false);
            m.Damping=Representable((double)m.Damping/knobs.Duration,false);
            m.TiltDamping=Representable((double)m.TiltDamping/knobs.Duration,false);
            m.MaxDisplacement=Representable((double)m.MaxDisplacement*knobs.Range/100,false);
            m.MaxTilt=Representable((double)m.MaxTilt*knobs.Tilt/100,false);
            m.MaxStretch=Representable((double)m.MaxStretch*knobs.Stretch/100,false);
            m.MaxSquash=Compression((double)m.MaxSquash*knobs.Stretch/100);
            m.MaxPressSquash=Compression((double)m.MaxPressSquash*knobs.Stretch/100);
            CheckMaterial(m);
        }
        internal static bool Read(byte[] utf8, out ContactSettings result)
        {
            result=new ContactSettings();
            if(utf8==null)return true;
            try {
                if(utf8.Length==0 || utf8.Length>4096)throw new FormatException();
                Reader reader=new Reader(new UTF8Encoding(false,true).GetString(utf8));
                reader.Space();reader.Take('{');reader.Space();
                HashSet<string> seen=new HashSet<string>(StringComparer.Ordinal);bool schema=false;
                ContactMaterial common=new ContactMaterial(),breast=new ContactMaterial(),butt=new ContactMaterial();uint commonMask=0,breastMask=0,buttMask=0;
                ContactKnobs breastKnobs=new ContactKnobs(),buttKnobs=new ContactKnobs();uint commonKnobMask=0,breastKnobMask=0,buttKnobMask=0;
                if(!reader.At('}'))do {
                    string key=reader.String();if(!seen.Add(key))throw new FormatException();reader.Space();reader.Take(':');reader.Space();
                    if(key=="schemaVersion"){if(reader.Number()!=1)throw new FormatException();schema=true;}
                    else if(key=="enabled")result.Enabled=reader.Boolean();
                    else if(key=="handRadiusMeters")result.HandRadius=reader.Physical(false);
                    else if(key=="bodyRadiusMeters")result.BodyRadius=reader.Physical(false);
                    else if(key=="handHoldSeconds"){result.HandHoldSeconds=reader.Physical(false);if(result.HandHoldSeconds>.25f)throw new FormatException("contact tracking age");}
                    else if(key=="targetBones")result.Targets=reader.Targets();
                    else if(key=="jiggleStrengthPercent"){result.StrengthPercent=reader.Physical(false);commonKnobMask|=1;}
                    else if(key=="jiggleDurationMultiplier"){result.DurationMultiplier=reader.Physical(true);commonKnobMask|=2;}
                    else if(key=="jiggleRangePercent"){result.RangePercent=reader.Physical(false);commonKnobMask|=4;}
                    else if(key=="jiggleTiltPercent"){result.TiltPercent=reader.Physical(false);commonKnobMask|=8;}
                    else if(key=="jiggleStretchPercent"){result.StretchPercent=reader.Physical(false);commonKnobMask|=16;}
                    else {
                        int region=key.StartsWith("breast",StringComparison.Ordinal)?1:key.StartsWith("butt",StringComparison.Ordinal)?2:0;
                        string name=region==0?key:key.Substring(region==1?6:4);
                        if(region!=0 && name.Length!=0)name=Char.ToLowerInvariant(name[0])+name.Substring(1);
                        int knob=region==0?-1:Array.IndexOf(KnobNames,name);
                        if(knob>=0){
                            float value=reader.Physical(knob==1);
                            if(region==1){breastKnobs.Set(knob,value);breastKnobMask|=1u<<knob;}
                            else{buttKnobs.Set(knob,value);buttKnobMask|=1u<<knob;}
                        }else{
                        int k=Array.IndexOf(Names,name);
                        if(k<0)reader.Skip(0);
                        else {
                            float value=reader.Physical(false);
                            if((k==6||k==8)&&value>=1)throw new FormatException("contact compression singularity");
                            if(region==1){breast.Set(k,value);breastMask|=1u<<k;}
                            else if(region==2){butt.Set(k,value);buttMask|=1u<<k;}
                            else {common.Set(k,value);commonMask|=1u<<k;}
                        }
                    }
                    }
                    reader.Space();if(!reader.At(','))break;reader.Take(',');reader.Space();if(reader.At('}'))throw new FormatException();
                }while(true);
                reader.Take('}');reader.Space();if(!schema||!reader.End)throw new FormatException();
                result.Breast=ContactMaterial.Defaults(false);result.Butt=ContactMaterial.Defaults(true);
                result.Breast.Merge(common,commonMask);result.Breast.Merge(breast,breastMask);
                result.Butt.Merge(common,commonMask);result.Butt.Merge(butt,buttMask);
                result.BreastKnobs=DefaultKnobs(false);result.ButtKnobs=DefaultKnobs(true);
                result.BreastKnobs.Merge(result.CommonKnobs(),commonKnobMask);result.ButtKnobs.Merge(result.CommonKnobs(),commonKnobMask);
                result.BreastKnobs.Merge(breastKnobs,breastKnobMask);result.ButtKnobs.Merge(buttKnobs,buttKnobMask);
                result.Finish(ref result.Breast,commonMask|breastMask,result.BreastKnobs);result.Finish(ref result.Butt,commonMask|buttMask,result.ButtKnobs);
                return true;
            }catch(Exception){result.Enabled=false;return false;}
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
            internal float Physical(bool positive) { return Representable(Number(),positive); }
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
