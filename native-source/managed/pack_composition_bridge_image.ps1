[CmdletBinding()]
param([Parameter(Mandatory)][string]$OutputFolder)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Reflection.Metadata
$folder=[IO.Path]::GetFullPath($OutputFolder)
$source=Join-Path $folder 'GF2.Composition.CameraBridge.encoded.dll'
$record=Join-Path $folder 'package-check.json'
$output=Join-Path $folder 'composition_bridge_package.h'
$expected='52CF8E7BD9118943EBCC7ADF387F8D35C6984B7ECFA1669E5E3717B8787184A2'
$check=Get-Content -LiteralPath $record -Raw|ConvertFrom-Json
if($check.result -ne 'PASS' -or $check.encoded_sha256 -ne $expected -or
   (Get-FileHash -LiteralPath $source).Hash -ne $expected -or
   (Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'CameraScopeBridge.cs')).Hash -ne $check.source_sha256 -or
   (Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'build_composition_bridge.ps1')).Hash -ne $check.builder_sha256 -or
   $check.methods.Count -ne 13){throw 'Encoded bridge package differs from proven artifact'}
$bytes=[IO.File]::ReadAllBytes($source)
if($bytes.Length -ne 5120){throw 'Unexpected encoded bridge size'}
$stream=[IO.File]::OpenRead($source)
try{
 $pe=[Reflection.PortableExecutable.PEReader]::new($stream)
 $md=[Reflection.Metadata.PEReaderExtensions]::GetMetadataReader($pe)
 if($md.GetString($md.GetModuleDefinition().Name) -ne 'GF2.Composition.CameraBridge.dll' -or
    $md.GetString($md.GetAssemblyDefinition().Name) -ne 'GF2.Composition.CameraBridge'){
  throw 'Encoded bridge identity changed'
 }
}finally{$pe.Dispose();$stream.Dispose()}
$sb=[Text.StringBuilder]::new()
[void]$sb.AppendLine('#ifndef GF2_COMPOSITION_BRIDGE_PACKAGE_H')
[void]$sb.AppendLine('#define GF2_COMPOSITION_BRIDGE_PACKAGE_H')
[void]$sb.AppendLine('/* Generated from exact encoded bridge; do not edit. */')
[void]$sb.AppendLine('#define GF2_COMPOSITION_BRIDGE_PACKAGE_SIZE 5120')
[void]$sb.AppendLine('#define GF2_COMPOSITION_BRIDGE_PACKAGE_SHA256 "52CF8E7BD9118943EBCC7ADF387F8D35C6984B7ECFA1669E5E3717B8787184A2"')
[void]$sb.AppendLine('static const unsigned char gf2_composition_bridge_package[GF2_COMPOSITION_BRIDGE_PACKAGE_SIZE]={')
for($i=0;$i -lt $bytes.Length;$i+=16){
 $last=[Math]::Min($i+16,$bytes.Length)
 $row=for($j=$i;$j -lt $last;$j++){'0x'+$bytes[$j].ToString('x2')}
 [void]$sb.AppendLine(('    '+($row -join ',')+$(if($last -lt $bytes.Length){','}else{''})))
}
[void]$sb.AppendLine('};')
[void]$sb.AppendLine('#endif')
$content=$sb.ToString()
[IO.File]::WriteAllText($output,$content,[Text.UTF8Encoding]::new($false))
if((Get-Item $output).Length -lt 25000){throw 'Generated header incomplete'}
[ordered]@{result='PASS';source_sha256=$expected;header_sha256=(Get-FileHash $output).Hash;size=$bytes.Length;module='GF2.Composition.CameraBridge.dll';method_count=$check.methods.Count;limits='Embeddable exact encoded bytes only. No live load, execution, delegate or pass/resource capture.'}|ConvertTo-Json|Set-Content (Join-Path $folder 'package-embed-check.json') -Encoding utf8
"PASS: $($bytes.Length) exact encoded bridge bytes packaged for native diagnostic build."
