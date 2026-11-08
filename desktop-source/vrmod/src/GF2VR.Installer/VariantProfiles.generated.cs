// Generated paired metadata for local live-settings candidate; no release publication.
#nullable enable
namespace GF2VR.Installer;
internal static class VariantProfiles {
 internal const string CleanBuild="GF2VR-CLEAN-1310-WORLDGRIP001";
 internal const string DirtyBuild="GF2VR-DIRTY-1310-WORLDGRIP001";
 internal const string CleanHash="C87A275E42EC900C1297A0E3F8A1442394742E0A586F8365D6B5FE6D08AD6746";
 internal const string DirtyHash="0C25D729ED9B6513E9041239B551384CF4F2D8A3E875E8C9D8274B4CDD8B8531";
 internal const string CommonCoreSha256="8E7A3F014A5B1E7B13FCDBDB3A0ACE2051D408EAA8983DD6E7A23AC6E7306109";
 internal static bool IsVariant(string? id) => id is CleanBuild or DirtyBuild;
 internal static string? Flavor(string? id) => id == CleanBuild ? "Clean" : id == DirtyBuild ? "Dirty" : null;
}