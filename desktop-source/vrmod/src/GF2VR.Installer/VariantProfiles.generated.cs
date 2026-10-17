// Generated only by build_variants.ps1 after BOTH variants pass.
#nullable enable
namespace GF2VR.Installer;
internal static class VariantProfiles
{
    internal const string CleanBuild="GF2VR-CLEAN-111-002";
    internal const string DirtyBuild="GF2VR-DIRTY-111-002";
    internal const string CleanHash="4F927C131C5E7288219C4805672D7B71C37E9024D33279AA9B5F7987E01A6C0D";
    internal const string DirtyHash="F4066EB15F2E8574B7696FBE89F697B91866ED0E7F07C103BA3072CF6C0757ED";
    internal const string CommonCoreSha256="AFBB28D1C82EC4209A2B59BAFC02EB336A2BFF9E62E36ADDDC1CBBC0BDAF456C";
    internal static bool IsVariant(string? id) => id is CleanBuild or DirtyBuild;
    internal static string? Flavor(string? id) => id == CleanBuild ? "Clean" : id == DirtyBuild ? "Dirty" : null;
}