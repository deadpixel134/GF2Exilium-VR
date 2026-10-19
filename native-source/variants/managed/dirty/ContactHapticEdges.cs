#if GF2VR_TOUCH_PRIVATE && GF2VR_DIRTY_CONTACT
namespace GF2.Private
{
    // Engine-free per-physical-hand edges. Missing observations cannot prove
    // separation. Preserve held bits through source/tracking suspension.
    internal sealed class ContactHapticEdges
    {
        private int held, pending;
        private long leftStamp, rightStamp;

        internal void Suspend() { pending = 0; }

        internal void Observe(int valid, int inside, long now)
        {
            valid &= 3; inside &= valid;
            int entered = inside & ~held;
            held = (held & ~valid) | inside;
            pending &= inside;
            pending |= entered;
            if ((entered & 1) != 0) leftStamp = now;
            if ((entered & 2) != 0) rightStamp = now;
        }

        internal int Drain(long now, long maximumAge)
        {
            int result = pending; pending = 0;
            if (now < leftStamp || now - leftStamp > maximumAge) result &= ~1;
            if (now < rightStamp || now - rightStamp > maximumAge) result &= ~2;
            return result;
        }
    }
}
#endif
