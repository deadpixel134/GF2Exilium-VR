#requires -Version 7.0
[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildOutput)
$ErrorActionPreference='Stop'
$BuildOutput=[IO.Path]::GetFullPath($BuildOutput)
$metadata=Join-Path $BuildOutput 'desktop-metadata'
if(Test-Path -LiteralPath $metadata){throw 'Preserve existing desktop metadata; use a fresh paired native build.'}

# Source identity, not a reproduction of the legacy release's undocumented
# commonCoreSha256 algorithm. The exact byte-level rule is documented alongside
# the build instructions. Compare both materialized common trees before hashing.
$common=Join-Path $PSScriptRoot 'common'
$entries=[Collections.Generic.Dictionary[string,string]]::new([StringComparer]::Ordinal)
foreach($file in Get-ChildItem -LiteralPath $common -File -Recurse){
 $relative=[IO.Path]::GetRelativePath($common,$file.FullName).Replace('\','/')
 $entries.Add($relative,(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash)
}
if(!$entries.Count){throw 'Common source tree is empty.'}
$paths=[string[]]@($entries.Keys)
[Array]::Sort($paths,[StringComparer]::Ordinal)
$manifest="GF2VR-COMMON-SOURCE-MANIFEST-V1`n"
$variants=[ordered]@{}
$releaseRecipes=[ordered]@{}
foreach($flavor in @('Clean','Dirty')){
 $resultPath=Join-Path $BuildOutput ($flavor+'/native/build-result.json')
 $result=Get-Content -LiteralPath $resultPath -Raw | ConvertFrom-Json
 $recipe=Get-Content -LiteralPath (Join-Path $BuildOutput ($flavor+'/materialized/native-build.json')) -Raw | ConvertFrom-Json
 $dll=Join-Path $BuildOutput ($flavor+'/native/version.dll')
 $hash=(Get-FileHash -LiteralPath $dll -Algorithm SHA256).Hash
 if($result.build -cne $recipe.build -or $result.build -notmatch '^GF2VR-[A-Z0-9-]+$' -or
    $result.dllSha256 -cne $hash){throw ('Native output/build report does not match: '+$flavor)}
 if(($flavor -eq 'Clean' -and $result.build -notmatch '^GF2VR-CLEAN-') -or
    ($flavor -eq 'Dirty' -and $result.build -notmatch '^GF2VR-DIRTY-')){throw ('Unexpected native build identity: '+$flavor)}
 foreach($relative in $paths){
  $materialized=Join-Path $BuildOutput ($flavor+'/materialized/'+$relative)
  if((Get-FileHash -LiteralPath $materialized -Algorithm SHA256).Hash -cne $entries[$relative]){
   throw ('Materialized common source differs from source tree: '+$flavor+'/'+$relative)
  }
 }
 $variants[$flavor]=[ordered]@{buildId=$result.build;dllSha256=$hash}
 $releaseRecipes[$flavor]=@{exact=($hash -ceq $recipe.expectedDllSha256);core=$recipe.commonCoreSha256}
}
foreach($relative in $paths){$manifest+=$relative+"`t"+$entries[$relative]+"`n"}
$utf8=[Text.UTF8Encoding]::new($false)
$commonHash=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($utf8.GetBytes($manifest)))
# Release commonCore is the reviewed DLL-bound installation contract. The
# public dependency-closure source manifest has a separate reproducible hash.
$sourceManifestHash=$commonHash
$commonAlgorithm='GF2VR-COMMON-SOURCE-MANIFEST-V1'
if($releaseRecipes.Clean.exact -and $releaseRecipes.Dirty.exact -and
   $releaseRecipes.Clean.core -cmatch '^[A-F0-9]{64}$' -and
   $releaseRecipes.Clean.core -ceq $releaseRecipes.Dirty.core){
 $commonHash=$releaseRecipes.Clean.core
 $commonAlgorithm='RELEASE-NATIVE-003-DLL-BOUND'
}
$files=@('render_quality.json','stereo_depth.json','navigation.json','world_drag.json',
 'activity_ui.json','controller_input.json','world_scale.json','controller_hands.json')
[IO.Directory]::CreateDirectory($metadata)|Out-Null
foreach($flavor in @('Clean','Dirty')){
 $folder=Join-Path $metadata $flavor
 [IO.Directory]::CreateDirectory($folder)|Out-Null
 $contract=[ordered]@{schemaVersion=1;buildId=$variants[$flavor].buildId;dllSha256=$variants[$flavor].dllSha256;
  configurationRoot='INSTALL_ROOT/vrmod/config';files=$(if($flavor -eq 'Dirty'){@($files)+@('touch_interaction.json')}else{$files});apply='GAME_RESTART';buildFlavor=$flavor;commonCoreSha256=$commonHash}
 [IO.File]::WriteAllText((Join-Path $folder 'runtime-contract.json'),((($contract|ConvertTo-Json -Depth 4).Replace("`r`n","`n"))+"`n"),$utf8)
}
$profiles=@"
// Generated from a paired native source build; do not substitute frozen release hashes.
#nullable enable
namespace GF2VR.Installer;
internal static class VariantProfiles
{
    internal const string CleanBuild="$($variants.Clean.buildId)";
    internal const string DirtyBuild="$($variants.Dirty.buildId)";
    internal const string CleanHash="$($variants.Clean.dllSha256)";
    internal const string DirtyHash="$($variants.Dirty.dllSha256)";
    internal const string CommonCoreSha256="$commonHash";
    internal static bool IsVariant(string? id) => id is CleanBuild or DirtyBuild;
    internal static string? Flavor(string? id) => id == CleanBuild ? "Clean" : id == DirtyBuild ? "Dirty" : null;
}
"@
[IO.File]::WriteAllText((Join-Path $metadata 'VariantProfiles.generated.cs'),(($profiles.Replace("`r`n","`n")+"`n").Replace("`n","`r`n")),$utf8)
[IO.File]::WriteAllText((Join-Path $metadata 'common-source-manifest.txt'),$manifest,$utf8)
$report=[ordered]@{schemaVersion=1;commonCoreAlgorithm=$commonAlgorithm;commonCoreSha256=$commonHash;publicSourceManifestSha256=$sourceManifestHash;variants=$variants}
[IO.File]::WriteAllText((Join-Path $metadata 'metadata.json'),(($report|ConvertTo-Json -Depth 5)+"`n"),$utf8)
Write-Output ('Paired native desktop metadata: '+$metadata)
