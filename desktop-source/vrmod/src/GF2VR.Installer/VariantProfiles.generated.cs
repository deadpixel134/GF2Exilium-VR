// Generated from a paired native source build; do not substitute frozen release hashes.
#nullable enable
namespace GF2VR.Installer;
internal static class VariantProfiles
{
    internal const string CleanBuild="GF2VR-CLEAN-136-009";
    internal const string DirtyBuild="GF2VR-DIRTY-136-009";
    internal const string CleanHash="762E27B49A03D909C5CC91D6D2B25574D0AAFFC8617C0D805FD5C5AAC38E8E48";
    internal const string DirtyHash="1C77D227501786288E8E47DE8B793B64163375DC59460FCF8BE4E67A7C00E1A4";
    internal const string CommonCoreSha256="E9BE54B53339847DA9EBB324BE2A1582EFF60F70FA5EB74EC80FE74322D2EAC1";
    internal static bool IsVariant(string? id) => id is CleanBuild or DirtyBuild;
    internal static string? Flavor(string? id) => id == CleanBuild ? "Clean" : id == DirtyBuild ? "Dirty" : null;
}
