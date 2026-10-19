[CmdletBinding()]
param([Parameter(Mandatory=$true)][ValidateSet(2,3)][int]$NativeEyeCapacity,[Parameter(Mandatory=$true)][string]$OutputDirectory,[ValidateSet('Clean','Dirty')][string]$Flavor='Dirty',[string]$GameRoot='')
$ErrorActionPreference='Stop'
Add-Type -AssemblyName Microsoft.CodeAnalysis.CSharp
Add-Type -AssemblyName System.Reflection.Metadata
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../../..'))
$folder=Join-Path $root 'vrmod/build/private/TOUCH-P3-006/managed'
if ($OutputDirectory) { $folder=[IO.Path]::GetFullPath($OutputDirectory) }
New-Item -ItemType Directory -Path $folder -Force|Out-Null
$source=Join-Path $PSScriptRoot 'ControllerDisplay.cs'
$dirty=Join-Path $PSScriptRoot 'dirty'
$sourceFiles=if($Flavor -eq 'Dirty'){@((Join-Path $dirty 'ContextActorContact.cs'),$source,(Join-Path $PSScriptRoot 'ShaderDiagnostics.cs'),(Join-Path $dirty 'BoneReadProbe.cs'),(Join-Path $dirty 'ContactRuntime.cs'),(Join-Path $dirty 'ContactHapticEdges.cs'),(Join-Path $dirty 'ContactConfig.cs'),(Join-Path $dirty 'ContactMaterial.cs'),(Join-Path $dirty 'ContactMotion.cs'),(Join-Path $dirty 'ContactDeformation.cs'))}else{@($source,(Join-Path $PSScriptRoot 'ShaderDiagnostics.cs'),(Join-Path $PSScriptRoot 'CleanContactAdapter.cs'))}
$generated=Join-Path $folder 'BuildContract.generated.cs'
$generatedText="#if GF2VR_TOUCH_PRIVATE`nnamespace GF2.Private { internal static class BuildContract { internal const int EyeCapacity = $NativeEyeCapacity; } }`n#endif`n"
[IO.File]::WriteAllText($generated,$generatedText,[Text.UTF8Encoding]::new($false))
$sourceFiles+=$generated
$sourceHashes=[ordered]@{}
foreach($file in $sourceFiles) { $sourceHashes[$file]=(Get-FileHash -LiteralPath $file).Hash }
if ($GameRoot) { $root=[IO.Path]::GetFullPath($GameRoot) }
$cache=Join-Path $root 'GF2 Game/GF2_Exilium_Data/LocalCache/Data/ClientRes_Windows/2.0.3936/Codes'
$expected=@{
 'Assembly-CSharp.dll.bytes'='C093192EF639AA9590850B0A85EFDF9C22BB9C71F098B2015D71E1DEEB38ECEC'
 'Assembly-CSharp-firstpass.dll.bytes'='785E9D4B998411B8C33AE34BE5BA0DE283800A697B6E7D3C46B98F7BE973D8B7'
 'mscorlib.dll.bytes'='C9955D65FDA5CAE9C66A0EF000168827B2D6D8863C98B7AF1E0212886738437B'
 'System.dll.bytes'='D4DBBF0FB7519C526953A979BCDAFABE8C3EEC2A4C499A64C2C2DABEE5A5B220'
 'System.Core.dll.bytes'='B02F22729B3CECA252F93BA4C82EBCFD7C24F944E658F16CB340660A74FB9331'
 'UnityEngine.CoreModule.dll.bytes'='7ED87B8E18B4764246E6BBB10DA4F4BB8841D4777C517530448E596D77E058EC'
 'UnityEngine.AnimationModule.dll.bytes'='889F21C06DF0FC0D1CCD25892CBA7551362EA2B31D51FCB672ED4F7C80A87D5F'
}
$refs=@(foreach($name in ($expected.Keys|Sort-Object)){
 $path=Join-Path $cache $name
 if((Get-FileHash $path).Hash -ne $expected[$name]){throw "Reference changed $name"}
 [Microsoft.CodeAnalysis.MetadataReference]::CreateFromFile($path)
})
$options=[Microsoft.CodeAnalysis.CSharp.CSharpCompilationOptions]::new([Microsoft.CodeAnalysis.OutputKind]::DynamicallyLinkedLibrary).
 WithOptimizationLevel([Microsoft.CodeAnalysis.OptimizationLevel]::Release).WithDeterministic($true).
 WithAssemblyIdentityComparer([Microsoft.CodeAnalysis.DesktopAssemblyIdentityComparer]::Default)
$symbols=[string[]]@('GF2VR_TOUCH_PRIVATE')
if($Flavor -eq 'Dirty'){$symbols+=@('GF2VR_DIRTY_CONTACT')}
$parse=[Microsoft.CodeAnalysis.CSharp.CSharpParseOptions]::Default.WithLanguageVersion([Microsoft.CodeAnalysis.CSharp.LanguageVersion]::CSharp7_3).WithPreprocessorSymbols([System.Collections.Generic.IEnumerable[string]][string[]]$symbols)
$trees=@(foreach($file in $sourceFiles) { [Microsoft.CodeAnalysis.CSharp.CSharpSyntaxTree]::ParseText((Get-Content $file -Raw),$parse,[IO.Path]::GetFileName($file)) })
$compilation=[Microsoft.CodeAnalysis.CSharp.CSharpCompilation]::Create('GF2.Private.TouchBridge',[Microsoft.CodeAnalysis.SyntaxTree[]]$trees,[Microsoft.CodeAnalysis.MetadataReference[]]$refs,$options)
$rawPath=Join-Path $folder 'GF2.Private.TouchBridge.raw.dll'
$stream=[IO.File]::Create($rawPath)
try{$result=$compilation.Emit($stream)}finally{$stream.Dispose()}
if(!$result.Success){throw ($result.Diagnostics -join "`n")}
$diagnostics=@($result.Diagnostics|ForEach-Object {$_.ToString()})
$bytes=[IO.File]::ReadAllBytes($rawPath);$encoded=[byte[]]$bytes.Clone();$touched=[bool[]]::new($bytes.Length)
$stream=[IO.File]::OpenRead($rawPath);$pe=[Reflection.PortableExecutable.PEReader]::new($stream)
$methods=@();$references=@();$nativeImports=@()
try{
 $md=[Reflection.Metadata.PEReaderExtensions]::GetMetadataReader($pe)
 if($md.GetString($md.GetAssemblyDefinition().Name) -ne 'GF2.Private.TouchBridge'){throw 'Not owned bridge assembly'}
 if(!$md.GetAssemblyDefinition().PublicKey.IsNil){throw 'Cannot encode signed image'}
 foreach($h in $md.AssemblyReferences){$n=$md.GetString($md.GetAssemblyReference($h).Name);if($n -notin @('mscorlib','System','System.Core','Assembly-CSharp','Assembly-CSharp-firstpass','UnityEngine.CoreModule','UnityEngine.AnimationModule')){throw "Unexpected dependency $n"};$references+=$n}
 foreach($h in $md.MethodDefinitions){
  $m=$md.GetMethodDefinition($h)
  if(($m.Attributes -band [Reflection.MethodAttributes]::PinvokeImpl) -or !$m.RelativeVirtualAddress -or ($m.ImplAttributes -band [Reflection.MethodImplAttributes]::InternalCall)){throw 'Owned engine bridge must contain ordinary IL only'}
  if(!$m.RelativeVirtualAddress){continue}
  $rva=$m.RelativeVirtualAddress;$offset=-1
  foreach($s in $pe.PEHeaders.SectionHeaders){if($rva -ge $s.VirtualAddress -and $rva -lt $s.VirtualAddress+$s.SizeOfRawData){$offset=$s.PointerToRawData+$rva-$s.VirtualAddress;break}}
  if($offset -lt 0){throw 'Unmapped method'}
  $body=[Reflection.Metadata.PEReaderExtensions]::GetMethodBody($pe,$rva)
  $il=$body.GetILBytes();$header=if(($bytes[$offset] -band 3) -eq 2){1}elseif(($bytes[$offset] -band 3) -eq 3){([BitConverter]::ToUInt16($bytes,$offset) -shr 12)*4}else{throw 'Invalid method header'}
  $start=$offset+$header;$locals=0
  if(!$body.LocalSignature.IsNil){$sig=$md.GetStandaloneSignature($body.LocalSignature);$reader=$md.GetBlobReader($sig.Signature);if($reader.ReadByte() -ne 7){throw 'Invalid local signature'};$locals=$reader.ReadCompressedInteger()}
  $eh=$body.ExceptionRegions.Length;$lc=if($locals){$locals}else{2};$ec=if($eh){$eh}else{2}
  [long]$product=[long]$il.Length*$lc*$ec
  if($product -gt 0x7fffffff -or $start+$il.Length -gt $bytes.Length){throw 'Outside proven encoding bounds'}
  $key=([long]($product*4294967297)).ToString([Globalization.CultureInfo]::InvariantCulture)
  for($i=0;$i -lt $il.Length;$i++){
   if($touched[$start+$i] -or $bytes[$start+$i] -ne $il[$i]){throw 'Body overlap or offset mismatch'}
   $touched[$start+$i]=$true;$encoded[$start+$i]=$il[$i] -bxor [byte][char]$key[$i%$key.Length]
  }
  $methods+=@{token=[Reflection.Metadata.Ecma335.MetadataTokens]::GetToken([Reflection.Metadata.EntityHandle]$h);name=$md.GetString($m.Name);offset=$start;length=$il.Length;locals=$locals;eh=$eh;key=$key}
 }
}finally{$pe.Dispose();$stream.Dispose()}
if($methods.Count -lt 15 -or $nativeImports.Count){throw 'Missing ordinary-IL bridge methods'}
# Verify inverse transform against compiler output, including every untouched byte.
$roundtrip=[byte[]]$encoded.Clone()
foreach($m in $methods){for($i=0;$i -lt $m.length;$i++){$roundtrip[$m.offset+$i]=$encoded[$m.offset+$i] -bxor [byte][char]$m.key[$i%$m.key.Length]}}
for($i=0;$i -lt $bytes.Length;$i++){if($roundtrip[$i] -ne $bytes[$i] -or (!$touched[$i] -and $encoded[$i] -ne $bytes[$i])){throw 'Packaging changed non-IL bytes or inverse mismatch'}}
$out=Join-Path $folder 'GF2.Private.TouchBridge.dll';[IO.File]::WriteAllBytes($out,$encoded)
[ordered]@{result='PASS';source_sha256=(Get-FileHash $source).Hash;builder_sha256=(Get-FileHash $PSCommandPath).Hash;native_imports=$nativeImports;compiler=[Microsoft.CodeAnalysis.CSharp.CSharpCompilation].Assembly.FullName;compiler_diagnostics=$diagnostics;identity_policy='Roslyn DesktopAssemblyIdentityComparer for exact cached metadata. Compiler unification is not live binding proof.';reference_hashes=$expected;assembly_references=$references;raw_sha256=(Get-FileHash $rawPath).Hash;encoded_sha256=(Get-FileHash $out).Hash;methods=$methods;limits='Offline exact metadata/encoded owned IL only. No owned external/InternalCall/PInvoke/custom MonoBehaviour or I/O; contact writes are isolated to the private self-restorer. Real engine binding, callback timing, visible contact/restoration and accepted opaque display preservation require P3 runtime evidence.'}|ConvertTo-Json -Depth 5|Set-Content (Join-Path $folder 'package-check.json') -Encoding utf8
Write-Output "PASS: owned bridge compiled against exact game metadata; $($methods.Count) method bodies encoded with exact byte-for-byte inverse."
$packageHeader=Join-Path $folder 'gf2_touch_bridge_package.h'
$packageLines=@('#ifndef GF2_PRIVATE_TOUCH_BRIDGE_PACKAGE_H','#define GF2_PRIVATE_TOUCH_BRIDGE_PACKAGE_H',('/* Exact owned encoded package SHA256: '+(Get-FileHash $out).Hash+' */'),'#ifdef GF2VR_TOUCH_PRIVATE',('#define GF2_TOUCH_BRIDGE_CAPACITY '+$NativeEyeCapacity),'static const unsigned char gf2_touch_bridge_package[]={')
for($i=0;$i -lt $encoded.Length;$i+=24){$last=[Math]::Min($i+23,$encoded.Length-1);$packageLines+=(' '+(($encoded[$i..$last]|ForEach-Object{'0x{0:x2}' -f $_}) -join ',')+',')}
$packageLines+=@('};','#endif','#endif')
[IO.File]::WriteAllText($packageHeader,($packageLines -join "`n")+"`n",[Text.UTF8Encoding]::new($false))

$contractCode=@'
using System;
using System.Collections.Generic;
using Microsoft.CodeAnalysis;
public static class TouchPrivateContract {
 public static string[] Check(Compilation c) {
  var t=c.GetTypeByMetadataName("GF2.Private.ControllerDisplay");
  if(t==null||!t.IsStatic)throw new InvalidOperationException("Expected public static private bridge");
  var expected=new Dictionary<string,string> {
   {"Prepare","void|UnityEngine.Camera|long|long|ref int"},
   {"Draw","void|UnityEngine.Camera|UnityEngine.Camera|UnityEngine.Camera|UnityEngine.Vector3|UnityEngine.Vector3|float|float|int|long|long|long|ref int"},
   {"Reset","void|ref int"},
   {"Snapshot","string|UnityEngine.Camera|int"},
   {"Diagnostics","string"},
   {"ConfigureContact","void|byte[]|ref int"},
   {"ServiceContact","void|ref int"},
   {"ContactDiagnostics","string"}
  };
  if(c.GetTypeByMetadataName("GF2.Private.ContactHapticEdges")!=null)expected.Add("DrainContactHaptics","void|long|long|ref int");
  var found=new List<string>();
  foreach(var a in t.GetMembers())if(a is IMethodSymbol m && m.DeclaredAccessibility==Accessibility.Public) {
   string wanted;if(!m.IsStatic||!expected.TryGetValue(m.Name,out wanted))throw new InvalidOperationException("Unexpected public contract "+m.Name);
   string actual=m.ReturnType.ToDisplayString(SymbolDisplayFormat.CSharpErrorMessageFormat);
   foreach(var p in m.Parameters)actual+="|"+(p.RefKind==RefKind.Ref?"ref ":p.RefKind==RefKind.None?"":"INVALID ")+p.Type.ToDisplayString(SymbolDisplayFormat.CSharpErrorMessageFormat);
   if(actual!=wanted)throw new InvalidOperationException("Contract mismatch "+m.Name+" "+actual);
   found.Add(m.Name+"|"+actual);expected.Remove(m.Name);
  }
  if(expected.Count!=0)throw new InvalidOperationException("Missing public contract");
  return found.ToArray();
 }
}
'@
Add-Type -TypeDefinition $contractCode -CompilerOptions '/nowarn:1701' -ReferencedAssemblies @([Microsoft.CodeAnalysis.Compilation].Assembly.Location,[Microsoft.CodeAnalysis.CSharp.CSharpCompilation].Assembly.Location,[Collections.Immutable.ImmutableArray].Assembly.Location,(Join-Path $PSHOME 'ref/System.Collections.dll'))
$contract=[TouchPrivateContract]::Check($compilation)
$offTrees=@(foreach($file in $sourceFiles) { [Microsoft.CodeAnalysis.CSharp.CSharpSyntaxTree]::ParseText((Get-Content $file -Raw),$parse.WithPreprocessorSymbols([string[]]@()),[IO.Path]::GetFileName($file)) })
$offCompilation=[Microsoft.CodeAnalysis.CSharp.CSharpCompilation]::Create('GF2.Private.TouchBridge.MacroOffWitness',[Microsoft.CodeAnalysis.SyntaxTree[]]$offTrees,[Microsoft.CodeAnalysis.MetadataReference[]]$refs,$options)
if($offCompilation.GetTypeByMetadataName('GF2.Private.ControllerDisplay')){throw 'Macro OFF still contains feature type'}
$offErrors=@($offCompilation.GetDiagnostics() | Where-Object Severity -eq Error)
if($offErrors.Count){throw ($offErrors -join "`n")}
foreach($name in $expected.Keys){if((Get-FileHash -LiteralPath (Join-Path $cache $name)).Hash -ne $expected[$name]){throw "Original reference mutated $name"}}
foreach($file in $sourceFiles) { if((Get-FileHash -LiteralPath $file).Hash -cne $sourceHashes[$file]) { throw 'Source changed during managed compilation' } }
[ordered]@{
 result='PASS_OFFLINE_NOT_RUNTIME_ACCEPTANCE'
 assembly='GF2.Private.TouchBridge'
 type='GF2.Private.ControllerDisplay'
 public_static_contract=$contract
 native_eye_capacity=$NativeEyeCapacity
 generated_contract_sha256=(Get-FileHash -LiteralPath $generated).Hash
 source_hashes=$sourceHashes
 source_sha256=(Get-FileHash -LiteralPath $source).Hash
 builder_sha256=(Get-FileHash -LiteralPath $PSCommandPath).Hash
 package_header_sha256=(Get-FileHash -LiteralPath $packageHeader).Hash
 raw_sha256=(Get-FileHash -LiteralPath $rawPath).Hash
 encoded_sha256=(Get-FileHash -LiteralPath $out).Hash
 encoded_bytes=$encoded.Length
 ordinary_il_methods=$methods.Count
 exact_inverse_and_untouched_bytes=$true
 frozen_reference_hashes=$expected
 original_inputs_unchanged=$true
 macro_off_type_absent=$true
 native_imports=@()
 custom_monobehaviour_types=0
 limits='Native assembly-load/binding and Application.onBeforeRender timing remain runtime boundaries. Offline fixtures and owned hot IL inspection are separate from visible contact/restoration, opaque display/depth/PC preservation and performance acceptance. Engine-internal allocations/cost remain UNKNOWN.'
}|ConvertTo-Json -Depth 7|Set-Content -LiteralPath (Join-Path $folder 'build-result.json') -Encoding utf8

