// Generated from a paired native source build; do not substitute frozen release hashes.
#nullable enable
namespace GF2VR.Installer;
internal static class VariantProfiles
{
    internal const string CleanBuild="GF2VR-CLEAN-133-006";
    internal const string DirtyBuild="GF2VR-DIRTY-133-006";
    internal const string CleanHash="0FCFDDED88A5F73E52F9E91C7D49BB4D6044ACAD24FC2303542E59979FA2D0B8";
    internal const string DirtyHash="CE0AB541E864F93545BA8F65E6710B989249109F2F5B18026F1C7D0C95031647";
    internal const string CommonCoreSha256="18182F3346F5AA6C60CDB468BA66C18F08837E63656A493AA96E485A6C44FE3B";
    internal static bool IsVariant(string? id) => id is CleanBuild or DirtyBuild;
    internal static string? Flavor(string? id) => id == CleanBuild ? "Clean" : id == DirtyBuild ? "Dirty" : null;
}
