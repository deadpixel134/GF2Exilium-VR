// Generated paired metadata for local live-settings candidate; no release publication.
#nullable enable
namespace GF2VR.Installer;
internal static class VariantProfiles {
 internal const string CleanBuild="GF2VR-CLEAN-139-LIVESET004";
 internal const string DirtyBuild="GF2VR-DIRTY-139-LIVESET004";
 internal const string CleanHash="76668E014AC5159207CAD1DA510B477C68DF438AA5B5EC87E2947439C6B7BF9F";
 internal const string DirtyHash="8877C5E59366B760860350A76D23AD41A090231AD3B73D0545C3C887D4A8115A";
 internal const string CommonCoreSha256="79668A4069A6CD6357478E5AEBA9667C5EA54A0EFAC43AC31F89C04F330B1A01";
 internal static bool IsVariant(string? id) => id is CleanBuild or DirtyBuild;
 internal static string? Flavor(string? id) => id == CleanBuild ? "Clean" : id == DirtyBuild ? "Dirty" : null;
}