// Generated from a paired native source build; do not substitute frozen release hashes.
#nullable enable
namespace GF2VR.Installer;
internal static class VariantProfiles
{
    internal const string CleanBuild="GF2VR-CLEAN-115-001";
    internal const string DirtyBuild="GF2VR-DIRTY-115-001";
    internal const string CleanHash="7806AE352C49B9326BD51C21374C0F2A92CD4E46C8B9812E2459B178088C4663";
    internal const string DirtyHash="DEB4A0DD6462E661455002BF369ECE5C11A566EA66995C9B84F41EA4C6390EDC";
    internal const string CommonCoreSha256="6D8D9340A97D4A11F32C7620F784D4C35442906D4D60D933B9F891D055EB5C84";
    internal static bool IsVariant(string? id) => id is CleanBuild or DirtyBuild;
    internal static string? Flavor(string? id) => id == CleanBuild ? "Clean" : id == DirtyBuild ? "Dirty" : null;
}
