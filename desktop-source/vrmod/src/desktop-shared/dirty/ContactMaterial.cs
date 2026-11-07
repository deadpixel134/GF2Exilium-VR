#if GF2VR_TOUCH_PRIVATE && GF2VR_DIRTY_CONTACT
using System;
namespace GF2.Private
{
    internal struct ContactMaterial
    {
        internal float Frequency, Damping, MaxDisplacement, ReleaseImpulse;
        internal float MaxTilt, MaxStretch, MaxSquash, PressGain, MaxPressSquash;
        internal float Stiffening, Vertical, Lateral, Depth;
        internal float TiltFrequency, TiltDamping, TiltGain, StretchGain, Detune;

        internal static ContactMaterial Defaults(bool butt)
        {
            return new ContactMaterial {
                Frequency=butt?3f:2f, Damping=butt?.22f:.12f, MaxDisplacement=butt?.025f:.04f,
                ReleaseImpulse=butt?.4f:.6f, MaxTilt=butt?6f:12f,
                MaxStretch=butt?.08f:.15f, MaxSquash=butt?.06f:.10f,
                PressGain=butt?.6f:1f, MaxPressSquash=butt?.08f:.15f,
                Stiffening=butt?3f:2f, Vertical=butt?1.1f:1.3f, Lateral=butt?.8f:.9f, Depth=butt?.6f:.7f,
                TiltGain=1f, StretchGain=1f, Detune=.03f
            };
        }
        internal void Set(int k,float v)
        {
            switch(k) {
                case 0:Frequency=v;break;case 1:Damping=v;break;case 2:MaxDisplacement=v;break;case 3:ReleaseImpulse=v;break;
                case 4:MaxTilt=v;break;case 5:MaxStretch=v;break;case 6:MaxSquash=v;break;case 7:PressGain=v;break;case 8:MaxPressSquash=v;break;
                case 9:Stiffening=v;break;case 10:Vertical=v;break;case 11:Lateral=v;break;case 12:Depth=v;break;
                case 13:TiltFrequency=v;break;case 14:TiltDamping=v;break;case 15:TiltGain=v;break;case 16:StretchGain=v;break;case 17:Detune=v;break;
            }
        }
        internal float Get(int k)
        {
            switch(k) {
                case 0:return Frequency;case 1:return Damping;case 2:return MaxDisplacement;case 3:return ReleaseImpulse;
                case 4:return MaxTilt;case 5:return MaxStretch;case 6:return MaxSquash;case 7:return PressGain;case 8:return MaxPressSquash;
                case 9:return Stiffening;case 10:return Vertical;case 11:return Lateral;case 12:return Depth;
                case 13:return TiltFrequency;case 14:return TiltDamping;case 15:return TiltGain;case 16:return StretchGain;default:return Detune;
            }
        }
        internal void Merge(ContactMaterial other,uint mask)
        { for(int k=0;k<18;++k)if((mask&(1u<<k))!=0)Set(k,other.Get(k)); }
    }
}
#endif
