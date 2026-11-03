// Generated paired metadata; tested Dirty bytes preserved.
#nullable enable
namespace GF2VR.Installer;
internal static class VariantProfiles {
 internal const string CleanBuild="GF2VR-CLEAN-137-010";
 internal const string DirtyBuild="GF2VR-DIRTY-HOME-TIMELINE-FIX001";
 internal const string CleanHash="6725EE78794BE06D76A3AD21CC91A3332E5DDF536A5F4905D347DA428C523FEB";
 internal const string DirtyHash="5C0B8A19691C67640EDE605410DDAF604C6B89304F170FC0F0C1D88F99401FCF";
 internal const string CommonCoreSha256="31C661A77C7BA58A1025BAA2C5177F16D7EF35D96AEAD2B342FC137617794186";
 internal static bool IsVariant(string? id) => id is CleanBuild or DirtyBuild;
 internal static string? Flavor(string? id) => id == CleanBuild ? "Clean" : id == DirtyBuild ? "Dirty" : null;
}