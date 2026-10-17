#requires -Version 7.0
[CmdletBinding()]
param([ValidateSet('Clean','Dirty','Both')][string]$Flavor='Both',[string]$Zig='zig',[string]$Kernel32Library=([IO.Path]::Combine(${env:ProgramFiles(x86)},'Windows Kits/10/Lib/10.0.26100.0/um/x64/kernel32.lib')),[string]$Output,[switch]$AllowSourceChanges)
$ErrorActionPreference='Stop'
if(!$Output){$Output=Join-Path $PSScriptRoot 'build'}
$Output=[IO.Path]::GetFullPath($Output)
foreach($variant in $(if($Flavor -eq 'Both'){@('Clean','Dirty')}else{@($Flavor)})){
 $materialized=Join-Path $Output ($variant+'/materialized')
 if(Test-Path -LiteralPath $materialized){throw 'Use a fresh output; preserve existing build'}
 [IO.Directory]::CreateDirectory($materialized)|Out-Null
 Copy-Item -Path (Join-Path $PSScriptRoot 'common/*') -Destination $materialized -Recurse
 Copy-Item -Path (Join-Path $PSScriptRoot ($variant+'/overlay/*')) -Destination $materialized -Recurse -Force
 Copy-Item -LiteralPath (Join-Path $PSScriptRoot ($variant+'/build-config')) -Destination $materialized -Recurse
 Copy-Item -LiteralPath (Join-Path $PSScriptRoot ($variant+'/native-build.json')) -Destination $materialized
 Copy-Item -LiteralPath (Join-Path $PSScriptRoot '../third_party') -Destination $materialized -Recurse
 Copy-Item -LiteralPath (Join-Path $PSScriptRoot '../build-native.ps1') -Destination $materialized
 & (Join-Path $materialized 'build-native.ps1') -Zig $Zig -Kernel32Library $Kernel32Library -Output (Join-Path $Output ($variant+'/native')) -AllowSourceChanges:$AllowSourceChanges
 if(!$?){throw ('Failed '+$variant)}
}
if($Flavor -eq 'Both'){
 & (Join-Path $PSScriptRoot 'generate-desktop-metadata.ps1') -BuildOutput $Output
 if(!$?){throw 'Failed generating paired desktop metadata'}
}else{
 Write-Output 'Single-flavor build: paired desktop metadata was not generated.'
}
