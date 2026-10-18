// Generated from a paired native source build; do not substitute frozen release hashes.
#nullable enable
namespace GF2VR.Installer;
internal static class VariantProfiles
{
    internal const string CleanBuild="GF2VR-CLEAN-113-001";
    internal const string DirtyBuild="GF2VR-DIRTY-113-001";
    internal const string CleanHash="31D2CC0F8098AE939C9B06A12B06104564E8AF337B1D58F4A9CD04D4151D93E2";
    internal const string DirtyHash="89520AD83F72903CF79E9185C4B4DEF3BA034D3A6CFDF0C12647BDFD3EE66D60";
    internal const string CommonCoreSha256="063E384231813F07776307F9DEB10C11E954BE081556249CF3271630212E0E22";
    internal static bool IsVariant(string? id) => id is CleanBuild or DirtyBuild;
    internal static string? Flavor(string? id) => id == CleanBuild ? "Clean" : id == DirtyBuild ? "Dirty" : null;
}
