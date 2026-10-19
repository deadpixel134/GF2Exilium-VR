// Generated from a paired native source build; do not substitute frozen release hashes.
#nullable enable
namespace GF2VR.Installer;
internal static class VariantProfiles
{
    internal const string CleanBuild="GF2VR-CLEAN-114-001";
    internal const string DirtyBuild="GF2VR-DIRTY-114-001";
    internal const string CleanHash="4C43DE2B433000EB4FD2D41F7240D4ECAB3B2F135A97CFAC90B555D775E3D135";
    internal const string DirtyHash="5711393862935F49096054296385AD5DA6FFD5BF09987D7C67608D93C8488970";
    internal const string CommonCoreSha256="4476A0A0597FD82197AB3E588E0E6DA04291C1F6F18DEE8EC4429646C6222005";
    internal static bool IsVariant(string? id) => id is CleanBuild or DirtyBuild;
    internal static string? Flavor(string? id) => id == CleanBuild ? "Clean" : id == DirtyBuild ? "Dirty" : null;
}
