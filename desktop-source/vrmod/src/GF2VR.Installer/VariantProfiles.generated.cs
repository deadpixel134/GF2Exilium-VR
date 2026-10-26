// Generated from a paired native source build; do not substitute frozen release hashes.
#nullable enable
namespace GF2VR.Installer;
internal static class VariantProfiles
{
    internal const string CleanBuild="GF2VR-CLEAN-130-004";
    internal const string DirtyBuild="GF2VR-DIRTY-130-004";
    internal const string CleanHash="D0C0653D5EE41EA15C6654D9258DD754CA805AA3AC7D9032EED85FA455AD0492";
    internal const string DirtyHash="E3D3E92CE0D7493F088896D5D377D6FB4F92A1FAD9662A951E4F7B5E9A49872E";
    internal const string CommonCoreSha256="C97C8C72A82920CF35A17D4521E4F9218FB8AD09EB8D23B8D236951EC21430F9";
    internal static bool IsVariant(string? id) => id is CleanBuild or DirtyBuild;
    internal static string? Flavor(string? id) => id == CleanBuild ? "Clean" : id == DirtyBuild ? "Dirty" : null;
}
