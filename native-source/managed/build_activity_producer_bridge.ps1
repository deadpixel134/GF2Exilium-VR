[CmdletBinding()]
param([Parameter(Mandatory)][string]$ReferenceDirectory,[Parameter(Mandatory)][string]$OutputFolder,[string]$HeaderOutput='')
$ErrorActionPreference='Stop'
Add-Type -AssemblyName Microsoft.CodeAnalysis.CSharp
Add-Type -AssemblyName System.Reflection.Metadata
$folder=[IO.Path]::GetFullPath($OutputFolder)
New-Item -ItemType Directory -Path $folder -Force|Out-Null
$source=Join-Path $PSScriptRoot 'ActivityProducerEngineBridge.cs'
$cache=[IO.Path]::GetFullPath($ReferenceDirectory)
$expected=@{
 'System.dll.bytes'='D4DBBF0FB7519C526953A979BCDAFABE8C3EEC2A4C499A64C2C2DABEE5A5B220'
 'mscorlib.dll.bytes'='C9955D65FDA5CAE9C66A0EF000168827B2D6D8863C98B7AF1E0212886738437B'
 'UnityEngine.CoreModule.dll.bytes'='7ED87B8E18B4764246E6BBB10DA4F4BB8841D4777C517530448E596D77E058EC'
 'Unity.RenderPipelines.Universal.Runtime.dll.bytes'='A00455A9110E3373CCF0DC581AFCF4687FDE18BA6D645530973D55B52D5ECD85'
 'System.Core.dll.bytes'='B02F22729B3CECA252F93BA4C82EBCFD7C24F944E658F16CB340660A74FB9331'
 'Unity.RenderPipelines.Core.Runtime.dll.bytes'='F8D2D86210646B371883D1363F8BA85033B5D0CCF1B72EED25F4A15FF4F50BF4'
}
$refs=@(foreach($name in ($expected.Keys|Sort-Object)){
 $path=Join-Path $cache $name
 if((Get-FileHash $path).Hash -ne $expected[$name]){throw "Reference changed $name"}
 [Microsoft.CodeAnalysis.MetadataReference]::CreateFromFile($path)
})
$options=[Microsoft.CodeAnalysis.CSharp.CSharpCompilationOptions]::new([Microsoft.CodeAnalysis.OutputKind]::DynamicallyLinkedLibrary).
 WithOptimizationLevel([Microsoft.CodeAnalysis.OptimizationLevel]::Release).WithDeterministic($true).
 WithAssemblyIdentityComparer([Microsoft.CodeAnalysis.DesktopAssemblyIdentityComparer]::Default)
$parse=[Microsoft.CodeAnalysis.CSharp.CSharpParseOptions]::Default.WithLanguageVersion([Microsoft.CodeAnalysis.CSharp.LanguageVersion]::CSharp7_3)
$tree=[Microsoft.CodeAnalysis.CSharp.CSharpSyntaxTree]::ParseText((Get-Content $source -Raw),$parse,'ProducerScopeBridge.cs')
$compilation=[Microsoft.CodeAnalysis.CSharp.CSharpCompilation]::Create('GF2.Performance.ProducerBridge',[Microsoft.CodeAnalysis.SyntaxTree[]]@($tree),[Microsoft.CodeAnalysis.MetadataReference[]]$refs,$options)
$rawPath=Join-Path $folder 'GF2.Performance.ProducerBridge.raw.dll'
$stream=[IO.File]::Create($rawPath)
try{$result=$compilation.Emit($stream)}finally{$stream.Dispose()}
if(!$result.Success){throw ($result.Diagnostics -join "`n")}
$diagnostics=@($result.Diagnostics|ForEach-Object {$_.ToString()})
$bytes=[IO.File]::ReadAllBytes($rawPath);$encoded=[byte[]]$bytes.Clone();$touched=[bool[]]::new($bytes.Length)
$stream=[IO.File]::OpenRead($rawPath);$pe=[Reflection.PortableExecutable.PEReader]::new($stream)
$methods=@();$references=@();$nativeImports=@()
try{
 $md=[Reflection.Metadata.PEReaderExtensions]::GetMetadataReader($pe)
 if($md.GetString($md.GetAssemblyDefinition().Name) -ne 'GF2.Performance.ProducerBridge'){throw 'Not owned bridge assembly'}
 if(!$md.GetAssemblyDefinition().PublicKey.IsNil){throw 'Cannot encode signed image'}
 foreach($h in $md.AssemblyReferences){$n=$md.GetString($md.GetAssemblyReference($h).Name);if($n -notin @('mscorlib','System','System.Core','UnityEngine.CoreModule','Unity.RenderPipelines.Universal.Runtime','Unity.RenderPipelines.Core.Runtime')){throw "Unexpected dependency $n"};$references+=$n}
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
$expectedBodies=23
if($methods.Count -ne $expectedBodies -or $nativeImports.Count){throw 'Missing ordinary-IL bridge methods'}
# Verify inverse transform against compiler output, including every untouched byte.
$roundtrip=[byte[]]$encoded.Clone()
foreach($m in $methods){for($i=0;$i -lt $m.length;$i++){$roundtrip[$m.offset+$i]=$encoded[$m.offset+$i] -bxor [byte][char]$m.key[$i%$m.key.Length]}}
for($i=0;$i -lt $bytes.Length;$i++){if($roundtrip[$i] -ne $bytes[$i] -or (!$touched[$i] -and $encoded[$i] -ne $bytes[$i])){throw 'Packaging changed non-IL bytes or inverse mismatch'}}
$out=Join-Path $folder 'GF2.Performance.ProducerBridge.encoded.dll';[IO.File]::WriteAllBytes($out,$encoded)
[ordered]@{result='PASS';source_sha256=(Get-FileHash $source).Hash;builder_sha256=(Get-FileHash $PSCommandPath).Hash;native_imports=$nativeImports;compiler=[Microsoft.CodeAnalysis.CSharp.CSharpCompilation].Assembly.FullName;compiler_diagnostics=$diagnostics;identity_policy='Roslyn DesktopAssemblyIdentityComparer for exact cached metadata. Compiler unification is not live binding proof.';reference_hashes=$expected;assembly_references=$references;raw_sha256=(Get-FileHash $rawPath).Hash;encoded_sha256=(Get-FileHash $out).Hash;methods=$methods;limits='Offline exact metadata/encoded IL only. Owned ordinary IL only, including scalar transport through existing GL wrapper. No owned external/InternalCall/PInvoke/delegate Marshal or shared registry write. Actual engine identity/field APIs/subscriber/queue delivery and120 remain unproved.'}|ConvertTo-Json -Depth 5|Set-Content (Join-Path $folder 'package-check.json') -Encoding utf8
Write-Output "PASS: owned bridge compiled against exact game metadata; $($methods.Count) method bodies encoded with exact byte-for-byte inverse."
$packageHeader=if($HeaderOutput){[IO.Path]::GetFullPath($HeaderOutput)}else{Join-Path $folder 'activity_producer_engine_bridge_baseline_package.h'}
$packageLines=@('#ifndef GF2_PRODUCER_BRIDGE_PACKAGE_H','#define GF2_PRODUCER_BRIDGE_PACKAGE_H',('/* Exact owned encoded package SHA256: '+(Get-FileHash $out).Hash+' */'),('#define GF2_ACTIVITY_PRODUCER_BRIDGE_METHODS '+$methods.Count),('#define GF2_ACTIVITY_PRODUCER_BRIDGE_BYTES '+$encoded.Length),'static const unsigned char gf2_producer_bridge_package[]={')
for($i=0;$i -lt $encoded.Length;$i+=24){$last=[Math]::Min($i+23,$encoded.Length-1);$packageLines+=(' '+(($encoded[$i..$last]|ForEach-Object{'0x{0:x2}' -f $_}) -join ',')+',')}
$packageLines+=@('};','#endif')
[IO.File]::WriteAllText($packageHeader,($packageLines -join "`n")+"`n",[Text.UTF8Encoding]::new($false))
