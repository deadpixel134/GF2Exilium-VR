// Generated from a paired native source build; do not substitute frozen release hashes.
#nullable enable
namespace GF2VR.Installer;
internal static class VariantProfiles
{
    internal const string CleanBuild="GF2VR-CLEAN-1311-RELEASE001";
    internal const string DirtyBuild="GF2VR-DIRTY-1311-RELEASE001";
    internal const string CleanHash="8764E8305B351130D102A2E81F85A80AC2787EE534ACE25DF7C465F16F9F5A95";
    internal const string DirtyHash="59BB43A485A8EFE6277B924D72F5FDB81B27B0D551901C1DF4F37CC728E37D3C";
    internal const string CommonCoreSha256="5C2380950B35C3FCD96D3DAC178BFD8679BCFBD917D9752695A47550FEC71843";
    internal static bool IsVariant(string? id) => id is CleanBuild or DirtyBuild;
    internal static string? Flavor(string? id) => id == CleanBuild ? "Clean" : id == DirtyBuild ? "Dirty" : null;
}
