// Generated from a paired native source build; do not substitute frozen release hashes.
#nullable enable
namespace GF2VR.Installer;
internal static class VariantProfiles
{
    internal const string CleanBuild="GF2VR-CLEAN-121-003";
    internal const string DirtyBuild="GF2VR-DIRTY-121-003";
    internal const string CleanHash="FE4A7B3F4D3F8524D92099D739CB056EF5B37D62315FFA01E9C2FE5E211D909D";
    internal const string DirtyHash="F83E2711C166E4405315072AF5EE32FE792E77772B672682C3427664E91ABE64";
    internal const string CommonCoreSha256="6EDCB5FD041CCEB5BD596345E81D5A339FE1FBEA4BDB43391961D31AF23032AB";
    internal static bool IsVariant(string? id) => id is CleanBuild or DirtyBuild;
    internal static string? Flavor(string? id) => id == CleanBuild ? "Clean" : id == DirtyBuild ? "Dirty" : null;
}
