// Generated from a paired native source build; do not substitute frozen release hashes.
#nullable enable
namespace GF2VR.Installer;
internal static class VariantProfiles
{
    internal const string CleanBuild="GF2VR-CLEAN-134-007";
    internal const string DirtyBuild="GF2VR-DIRTY-134-007";
    internal const string CleanHash="C8E71A58640E2F1017F4E4C56C69768244BD2E175FCDD9DED784337B16606E9C";
    internal const string DirtyHash="45506FC737EA3CA418D6804BF4D1A974F3FBB14B9468229C22D71221C1A0E4B7";
    internal const string CommonCoreSha256="F25F2877E742D43AB5A000839D34C715F31E65848B6D8A9CA80DE8E9E8587837";
    internal static bool IsVariant(string? id) => id is CleanBuild or DirtyBuild;
    internal static string? Flavor(string? id) => id == CleanBuild ? "Clean" : id == DirtyBuild ? "Dirty" : null;
}
