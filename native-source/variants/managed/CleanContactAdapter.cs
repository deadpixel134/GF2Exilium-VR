#if GF2VR_TOUCH_PRIVATE && !GF2VR_DIRTY_CONTACT
using UnityEngine;
namespace GF2.Private
{
    // ABI compatibility only. Clean has no contact settings, actor lookup,
    // deformation, bone writes or before-render subscription implementation.
    public static partial class ControllerDisplay
    {
        private static void RetryContact(Camera source) { }
        private static void ClearContact() { }
        private static void CacheContactHands(Vector3 left, Vector3 right, float radius, float unit,
            int mask, long epoch, long generation, long ticket) { }
        public static void ConfigureContact(byte[] configuration, ref int status) { status = 1; }
        public static void ServiceContact(ref int status) { status = 1; }
        public static string ContactDiagnostics()
        { return "{\"event\":\"controller_hands_backend\",\"flavor\":\"Clean\",\"contact_available\":false}"; }
    }
}
#endif
