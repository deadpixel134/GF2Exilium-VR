#requires -Version 7.0
[CmdletBinding()]
param([string]$Zig='zig',[string]$Kernel32Library=([IO.Path]::Combine(${env:ProgramFiles(x86)},'Windows Kits/10/Lib/10.0.26100.0/um/x64/kernel32.lib')),[string]$Output,[switch]$AllowSourceChanges)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath($PSScriptRoot)
if(!$Output){$Output=Join-Path $root 'build'}
$Output=[IO.Path]::GetFullPath($Output)
New-Item -ItemType Directory -Path $Output -Force|Out-Null
$previousLocalCache=$env:ZIG_LOCAL_CACHE_DIR
$previousGlobalCache=$env:ZIG_GLOBAL_CACHE_DIR
try {
$env:ZIG_LOCAL_CACHE_DIR=Join-Path $Output 'zig-local-cache'
$env:ZIG_GLOBAL_CACHE_DIR=Join-Path $Output 'zig-global-cache'
if(!(Test-Path -LiteralPath $Kernel32Library -PathType Leaf)){throw 'Supply Windows SDK x64 kernel32.lib using -Kernel32Library'}
$recipe=Get-Content -LiteralPath (Join-Path $root 'native-build.json') -Raw|ConvertFrom-Json
$unitDefines=@($recipe.unitDefines|ForEach-Object {'-DGF2VR_'+$_})
$objects=[ordered]@{}
function Compile([string[]]$Arguments,[string]$Name){
 & $Zig @Arguments 2>&1|Out-File -LiteralPath (Join-Path $Output ($Name+'.log')) -Encoding UTF8
 if($LASTEXITCODE){throw ('Native build failed: '+$Name+'; see output log')}
}
$zigExe=(Get-Command $Zig -ErrorAction Stop).Source
if($recipe.compilerSha256 -and (Get-FileHash -LiteralPath $zigExe).Hash -cne $recipe.compilerSha256){throw 'Release compiler SHA256 differs'}
$zigHeaders=Join-Path ([IO.Path]::GetDirectoryName($zigExe)) 'lib/libc/include/any-windows-any'
foreach($p in $recipe.units.PSObject.Properties){
 $src=Join-Path $root ('src/stereo/'+$p.Value+'.c');$obj=Join-Path $Output ($p.Name+'.obj')
 $args=@('cc','-target','x86_64-windows-gnu','-c','-nostdlib','-fno-stack-protector','-Os','-Wall','-Wextra','-Werror')+$unitDefines
 if($p.Name -eq 'eye_target_watch'){$args+='-DGF2VR_FOCUS_MRT'}
 if($p.Name -in @('eye_pose_pool','xr_frame_prefetch','xr_ordered_frames')){$args+=@('-I',(Join-Path $root 'third_party/openxr/include'),'-Wno-unused-function')}
 $args+=@('-isystem',$zigHeaders,$src,'-MD','-MF',(Join-Path $Output ($p.Name+'.d')),'-o',$obj)
 Compile $args $p.Name;$objects[$p.Name]=(Get-FileHash -LiteralPath $obj).Hash
}
foreach($unit in @('hook','buffer','trampoline','hde64')){
 $relative=if($unit -eq 'hde64'){'src/hde/hde64.c'}else{'src/'+$unit+'.c'}
 $src=Join-Path $root ('third_party/minhook/'+$relative);$obj=Join-Path $Output ('mh-'+$unit+'.obj')
 Compile @('cc','-target','x86_64-windows-gnu','-c','-Os','-fno-stack-protector','-fno-builtin','-DNDEBUG',$src,'-MD','-MF',(Join-Path $Output ('mh-'+$unit+'.d')),'-o',$obj) ('mh-'+$unit)
 $objects['mh-'+$unit]=(Get-FileHash -LiteralPath $obj).Hash
}
$args=@($recipe.mainArgs|ForEach-Object {$_.Replace('SOURCE_ROOT',$root).Replace('OUTPUT_DIR',$Output).Replace('SDK_KERNEL32',$Kernel32Library)})
$args+=('-ffile-prefix-map='+(Join-Path $root 'src')+'=vrmod/src')
$dll=Join-Path $Output 'version.dll'
$args+=@('-MD','-MF',(Join-Path $Output 'native.d'),'-o',$dll)
Compile $args 'native'
$hash=(Get-FileHash -LiteralPath $dll).Hash
$report=[ordered]@{build=$recipe.build;dllSha256=$hash;expectedDllSha256=$recipe.expectedDllSha256;exactMatch=($hash -eq $recipe.expectedDllSha256);objects=$objects;compilerSha256=(Get-FileHash -LiteralPath $zigExe).Hash;args=$args}
[IO.File]::WriteAllText((Join-Path $Output 'build-result.json'),($report|ConvertTo-Json -Depth 6),[Text.UTF8Encoding]::new($false))
if(!$report.exactMatch){
 if(!$AllowSourceChanges){throw 'Source build differs from frozen release DLL; inspect recorded object/build hashes before packaging. For an intentional source change, rebuild with -AllowSourceChanges and generate matching desktop metadata.'}
 Write-Warning ('Intentional source build differs from the frozen release: '+$hash)
}else{'PASS exact release native reproduction '+$hash}
} finally {
 $env:ZIG_LOCAL_CACHE_DIR=$previousLocalCache
 $env:ZIG_GLOBAL_CACHE_DIR=$previousGlobalCache
}
