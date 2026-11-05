// Generated paired metadata; tested Dirty bytes preserved.
#nullable enable
namespace GF2VR.Installer;
internal static class VariantProfiles {
 internal const string CleanBuild="GF2VR-CLEAN-138-011";
 internal const string DirtyBuild="GF2VR-DIRTY-HOME-SEMANTIC-FIELDS001";
 internal const string CleanHash="69A63DC2E0C1901936FDFE2CC35217F5547029FB189A6E64AC30663FED260961";
 internal const string DirtyHash="C6396AB3506E0C2B8BD9173B553C2B2E16C898FA94FA5DF164B531451391CF33";
 internal const string CommonCoreSha256="1345204867BB72743C2407BE35FA4FD9514D1EDF5B0FA254D527210D78C4A9C5";
 internal static bool IsVariant(string? id) => id is CleanBuild or DirtyBuild;
 internal static string? Flavor(string? id) => id == CleanBuild ? "Clean" : id == DirtyBuild ? "Dirty" : null;
}