// Generated from a paired native source build; do not substitute frozen release hashes.
#nullable enable
namespace GF2VR.Installer;
internal static class VariantProfiles
{
    internal const string CleanBuild="GF2VR-CLEAN-135-008";
    internal const string DirtyBuild="GF2VR-DIRTY-135-008";
    internal const string CleanHash="1A3D744F880879EA73675FBF726F997BCE99B0BBDD993C2A1A73E5561C13C579";
    internal const string DirtyHash="86B3B2AE3847FCFCE401016AF0BAB5F3F2FE0C4BFFEB22616D686E8114C95832";
    internal const string CommonCoreSha256="3F7BA42C7A956ED617AAF18AD2E69FF9F0D081FEC962723652E02426B71EFC4F";
    internal static bool IsVariant(string? id) => id is CleanBuild or DirtyBuild;
    internal static string? Flavor(string? id) => id == CleanBuild ? "Clean" : id == DirtyBuild ? "Dirty" : null;
}
