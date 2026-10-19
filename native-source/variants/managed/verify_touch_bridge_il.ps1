param([string]$InputAssembly,[string]$OutputDirectory,[ValidateSet('Clean','Dirty')][string]$Flavor='Dirty')
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Reflection.Metadata
$code=@'

using System;
using System.IO;
using System.Text;
using System.Globalization;
using System.Collections.Generic;
using System.Reflection;
using System.Reflection.Emit;
using System.Reflection.Metadata;
using System.Reflection.Metadata.Ecma335;
using System.Reflection.PortableExecutable;
public sealed class TouchILSignature : ISignatureTypeProvider<string, object> {
 public string GetArrayType(string element,ArrayShape shape){return element+"[rank="+shape.Rank+"]";}
 public string GetByReferenceType(string element){return element+"&";}
 public string GetFunctionPointerType(MethodSignature<string> signature){return "fn("+String.Join(",",signature.ParameterTypes)+")->"+signature.ReturnType;}
 public string GetGenericInstantiation(string generic,System.Collections.Immutable.ImmutableArray<string> args){return generic+"<"+String.Join(",",args)+">";}
 public string GetGenericMethodParameter(object context,int index){return "!!"+index;}
 public string GetGenericTypeParameter(object context,int index){return "!"+index;}
 public string GetModifiedType(string modifier,string unmodified,bool required){return (required?"modreq(":"modopt(")+modifier+")"+unmodified;}
 public string GetPinnedType(string element){return element+" pinned";}
 public string GetPointerType(string element){return element+"*";}
 public string GetPrimitiveType(PrimitiveTypeCode code){return code.ToString();}
 public string GetSZArrayType(string element){return element+"[]";}
 public string GetTypeFromDefinition(MetadataReader reader,TypeDefinitionHandle handle,byte raw){return TouchOwnedBridgeIL.Entity(reader,handle);}
 public string GetTypeFromReference(MetadataReader reader,TypeReferenceHandle handle,byte raw){return TouchOwnedBridgeIL.Entity(reader,handle);}
 public string GetTypeFromSpecification(MetadataReader reader,object context,TypeSpecificationHandle handle,byte raw){return reader.GetTypeSpecification(handle).DecodeSignature(this,context);}
}
public static class TouchOwnedBridgeIL {
 static readonly Dictionary<ushort,OpCode> Codes=new Dictionary<ushort,OpCode>();
 static TouchOwnedBridgeIL(){ foreach(var f in typeof(OpCodes).GetFields(BindingFlags.Public|BindingFlags.Static)) if(f.FieldType==typeof(OpCode)){var o=(OpCode)f.GetValue(null);Codes[unchecked((ushort)o.Value)]=o;} }
 static string TypeName(MetadataReader m,TypeDefinitionHandle h){var t=m.GetTypeDefinition(h);var p=t.GetDeclaringType();return p.IsNil?(m.GetString(t.Namespace)+"."+m.GetString(t.Name)).TrimStart('.'):TypeName(m,p)+"+"+m.GetString(t.Name);}
 public static string Entity(MetadataReader m,EntityHandle h){
  switch(h.Kind){
   case HandleKind.TypeDefinition:return TypeName(m,(TypeDefinitionHandle)h);
   case HandleKind.TypeReference:var t=m.GetTypeReference((TypeReferenceHandle)h);return (m.GetString(t.Namespace)+"."+m.GetString(t.Name)).TrimStart('.');
   case HandleKind.MethodDefinition:var a=m.GetMethodDefinition((MethodDefinitionHandle)h);return TypeName(m,a.GetDeclaringType())+"."+m.GetString(a.Name);
   case HandleKind.FieldDefinition:var b=m.GetFieldDefinition((FieldDefinitionHandle)h);return TypeName(m,b.GetDeclaringType())+"."+m.GetString(b.Name);
   case HandleKind.MemberReference:var c=m.GetMemberReference((MemberReferenceHandle)h);return Entity(m,c.Parent)+"."+m.GetString(c.Name);
   case HandleKind.MethodSpecification:var d=m.GetMethodSpecification((MethodSpecificationHandle)h);return Entity(m,d.Method)+"<...>";
   case HandleKind.TypeSpecification:return m.GetTypeSpecification((TypeSpecificationHandle)h).DecodeSignature(new TouchILSignature(),(object)null);
   case HandleKind.StandaloneSignature:m.GetStandaloneSignature((StandaloneSignatureHandle)h);return "Signature#"+MetadataTokens.GetToken(h).ToString("X8");
   default:throw new InvalidDataException("Unexpected token kind "+h.Kind);
  }
 }
 static string Token(MetadataReader m,int token,OperandType kind){
  int table=(int)((uint)token>>24);
  if(kind==OperandType.InlineString){if(table!=0x70)throw new InvalidDataException("Not string token");return "\""+m.GetUserString(MetadataTokens.UserStringHandle(token&0xffffff)).Replace("\r","\\r").Replace("\n","\\n")+"\"";}
  if(kind==OperandType.InlineMethod && table!=0x06&&table!=0x0a&&table!=0x2b)throw new InvalidDataException("Not method token");
  if(kind==OperandType.InlineField && table!=0x04&&table!=0x0a)throw new InvalidDataException("Not field token");
  if(kind==OperandType.InlineType && table!=0x01&&table!=0x02&&table!=0x1b)throw new InvalidDataException("Not type token");
  return Entity(m,MetadataTokens.EntityHandle(token));
 }
 static string Decode(MetadataReader m,byte[] b){
  int p=0;OpCode last=default;var s=new StringBuilder();var starts=new HashSet<int>();var branches=new List<int>();
  while(p<b.Length){
   int at=p;starts.Add(at);ushort v=b[p++];if(v==0xfe){if(p>=b.Length)throw new InvalidDataException("Truncated opcode");v=(ushort)(0xfe00|b[p++]);}
   if(!Codes.TryGetValue(v,out var op)||op.Name.StartsWith("prefix"))throw new InvalidDataException("Bad opcode "+v.ToString("X4"));
   last=op;int size=0;string value="";
   switch(op.OperandType){
    case OperandType.InlineNone:break;
    case OperandType.ShortInlineI:case OperandType.ShortInlineVar:case OperandType.ShortInlineBrTarget:size=1;break;
    case OperandType.InlineVar:size=2;break;
    case OperandType.InlineI:case OperandType.ShortInlineR:case OperandType.InlineBrTarget:case OperandType.InlineField:case OperandType.InlineMethod:case OperandType.InlineSig:case OperandType.InlineString:case OperandType.InlineTok:case OperandType.InlineType:size=4;break;
    case OperandType.InlineI8:case OperandType.InlineR:size=8;break;
    case OperandType.InlineSwitch:
     if(p+4>b.Length)throw new InvalidDataException("Switch header");
     int n=BitConverter.ToInt32(b,p);if(n<0||n>(b.Length-p-4)/4)throw new InvalidDataException("Switch count");
     size=4+n*4;for(int i=0;i<n;i++)branches.Add(p+size+BitConverter.ToInt32(b,p+4+i*4));value=n+" targets";break;
    default:throw new InvalidDataException("Unsupported operand");
   }
   if(p+size>b.Length)throw new InvalidDataException("Operand beyond method");
   if(size!=0&&op.OperandType!=OperandType.InlineSwitch){
    switch(op.OperandType){
     case OperandType.InlineField:case OperandType.InlineMethod:case OperandType.InlineSig:case OperandType.InlineString:case OperandType.InlineTok:case OperandType.InlineType:value=Token(m,BitConverter.ToInt32(b,p),op.OperandType);break;
     case OperandType.ShortInlineBrTarget:int a=p+size+(sbyte)b[p];branches.Add(a);value="IL_"+a.ToString("X4");break;
     case OperandType.InlineBrTarget:int c=p+size+BitConverter.ToInt32(b,p);branches.Add(c);value="IL_"+c.ToString("X4");break;
     case OperandType.ShortInlineR:value=BitConverter.ToSingle(b,p).ToString("R",CultureInfo.InvariantCulture);break;
     case OperandType.InlineR:value=BitConverter.ToDouble(b,p).ToString("R",CultureInfo.InvariantCulture);break;
     default:value=size==1?((sbyte)b[p]).ToString():size==2?BitConverter.ToUInt16(b,p).ToString():size==4?BitConverter.ToInt32(b,p).ToString():BitConverter.ToInt64(b,p).ToString();break;
    }
   }
   s.Append("IL_").Append(at.ToString("X4")).Append(": ").Append(op.Name).Append(" ").Append(value).AppendLine();p+=size;
  }
  if(last.FlowControl!=FlowControl.Return && last.FlowControl!=FlowControl.Throw && last.FlowControl!=FlowControl.Branch && last.Name!="endfinally" && last.Name!="endfilter")throw new InvalidDataException("Missing method terminator");
  foreach(int target in branches)if(target!=b.Length&&!starts.Contains(target))throw new InvalidDataException("Branch not on instruction");
  return s.ToString();
 }
 public static string Inspect(string path,string output){
  using(var s=File.OpenRead(path))using(var pe=new PEReader(s)){
   var m=pe.GetMetadataReader();var text=new StringBuilder();var summary=new StringBuilder();
   foreach(var th in m.TypeDefinitions){
    string type=TypeName(m,th);if(!type.StartsWith("GF2.Private."))continue;
    foreach(var h in m.GetTypeDefinition(th).GetMethods()){
     var method=m.GetMethodDefinition(h);if(method.RelativeVirtualAddress==0)continue;
     var body=pe.GetMethodBody(method.RelativeVirtualAddress);var raw=body.GetILBytes();int locals=0;
     if(!body.LocalSignature.IsNil){var br=m.GetBlobReader(m.GetStandaloneSignature(body.LocalSignature).Signature);if(br.ReadByte()!=7)throw new InvalidDataException("Locals signature");locals=br.ReadCompressedInteger();}
     long product=(long)raw.Length*(locals==0?2:locals)*(body.ExceptionRegions.Length==0?2:body.ExceptionRegions.Length);
     string key=(product*4294967297L).ToString(CultureInfo.InvariantCulture);var decoded=(byte[])raw.Clone();
     for(int i=0;i<decoded.Length;i++)decoded[i]^=(byte)key[i%key.Length];
     string rawText=null,decodedText=null,rawError="",decodedError="";
     try{rawText=Decode(m,raw);}catch(Exception e){rawError=e.Message;}
     try{decodedText=Decode(m,decoded);}catch(Exception e){decodedError=e.Message;}
     if((rawText==null)==(decodedText==null)){summary.AppendLine(type+"."+m.GetString(method.Name)+" AMBIGUOUS_OR_INVALID raw="+rawError+" xor="+decodedError);continue;}
     string mode=decodedText!=null?"ENCODED_GAME_IL_XOR_VALIDATED":"RAW_VALIDATED";
     string name=type+"."+m.GetString(method.Name);
     summary.AppendLine(name+" token=0x"+MetadataTokens.GetToken(h).ToString("X8")+" mode="+mode+" bytes="+raw.Length+" locals="+locals+" EH="+body.ExceptionRegions.Length);
     text.AppendLine("### "+name+" token=0x"+MetadataTokens.GetToken(h).ToString("X8")+" "+mode+" ILbytes="+raw.Length+" locals="+locals+" EH="+body.ExceptionRegions.Length);
     var signature=method.DecodeSignature(new TouchILSignature(),(object)null);
     text.Append("SIGNATURE ").Append(signature.Header.RawValue).Append(" generic=").Append(signature.GenericParameterCount).Append(" required=").Append(signature.RequiredParameterCount).Append(" return=").Append(signature.ReturnType).Append(" args=").AppendLine(String.Join(",",signature.ParameterTypes));
     text.Append("ATTRS ").Append(method.Attributes).Append(" ").Append(method.ImplAttributes).Append(" initlocals=").Append(body.LocalVariablesInitialized).Append(" maxstack=").Append(body.MaxStack).AppendLine();
     if(!body.LocalSignature.IsNil)text.Append("LOCALS ").AppendLine(String.Join(",",m.GetStandaloneSignature(body.LocalSignature).DecodeLocalSignature(new TouchILSignature(),(object)null)));
     foreach(var region in body.ExceptionRegions)text.Append("EH ").Append(region.Kind).Append(" try=").Append(region.TryOffset).Append(",").Append(region.TryLength).Append(" handler=").Append(region.HandlerOffset).Append(",").Append(region.HandlerLength).Append(" filter=").Append(region.FilterOffset).Append(" type=").Append(region.CatchType.IsNil?"":Entity(m,region.CatchType)).AppendLine();
     text.Append(decodedText??rawText).AppendLine();
    }
   }
   File.WriteAllText(output,text.ToString());return summary.ToString();
  }
 }
}

'@
$refs=@([Reflection.Metadata.MetadataReader].Assembly.Location,[Collections.Immutable.ImmutableArray].Assembly.Location,(Join-Path $PSHOME 'ref/System.Collections.dll'),(Join-Path $PSHOME 'ref/System.Reflection.Primitives.dll'))
Add-Type -TypeDefinition $code -ReferencedAssemblies $refs
$path=Join-Path $PWD 'vrmod/build/private/TOUCH-P3-006/managed/GF2.Private.TouchBridge.raw.dll'
$out=Join-Path $PWD 'vrmod/build/private/TOUCH-P3-006/managed'
if($InputAssembly){$path=$InputAssembly};if($OutputDirectory){$out=$OutputDirectory};New-Item -ItemType Directory -Path $out -Force|Out-Null
$summary=[TouchOwnedBridgeIL]::Inspect($path,(Join-Path $out 'managed-il.txt'))
[IO.File]::WriteAllText((Join-Path $out 'managed-il-validation.txt'),$summary,[Text.UTF8Encoding]::new($false))
$summary

$sections=(Get-Content -LiteralPath (Join-Path $out 'managed-il.txt') -Raw) -split '(?m)^### '
$hot=@()
foreach($part in $sections){
 if($part -match '^GF2.Private.ControllerDisplay\.(Draw|Queue|Eligible|Finite|OnOwner|Live) '){
  $first=($part -split "`n")[0]
  $bad=@(($part -split "`n")|Where-Object {$_ -match ': (newobj|newarr|box|ldstr) '})
  $hot += [ordered]@{method=$first;forbidden_allocation_string_opcodes=$bad;pass=($bad.Count -eq 0)}
 }
}
$hot|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $out 'hot-il-validation.json') -Encoding utf8
$expectedHot=if($Flavor -eq 'Clean'){5}else{7}
if(@($hot|Where-Object {$_.pass -ne $true}).Count -gt 0 -or $hot.Count -ne $expectedHot){throw 'Hot transitive IL allocation/string check failed'}
if($Flavor -eq 'Clean'){
 $all=[IO.File]::ReadAllText((Join-Path $out 'managed-il.txt'))
 if($all -match 'ContactSettings|ContactBeforeRender|BeginContact|BoneReadProbe|ContactHapticEdges|DrainContactHaptics|UnityEngine\.Animator|UnityEngine\.SkinnedMeshRenderer|add_onBeforeRender|set_localPosition|set_localRotation'){throw 'Clean contains forbidden contact implementation'}
}
if($summary -match 'AMBIGUOUS_OR_INVALID'){throw 'Ambiguous or invalid owned method body'}
Write-Output ('Hot transitive IL PASS methods='+$hot.Count+'; engine internal allocations are not measured')

# Walk every owned call reachable from the two contact entry points. Inspect
# all overloads sharing a resolved name; this is conservative for Finite.
# Binding/configuration/event subscription/diagnostics are genuinely cold and
# are not roots. If a hot method starts calling one, the walk includes it and
# its allocation/search instructions fail this proof.
$byName=@{}
foreach($part in $sections){
 if($part -match '^(GF2\.Private\.[^ ]+) token='){
  $name=$Matches[1];if(!$byName.ContainsKey($name)){$byName[$name]=@()};$byName[$name]+=$part
 }
}
$contactRoots=@('GF2.Private.ControllerDisplay.ContactBeforeRender','GF2.Private.ControllerDisplay.CacheContactHands')
if($byName.ContainsKey('GF2.Private.ControllerDisplay.DrainContactHaptics')){$contactRoots+='GF2.Private.ControllerDisplay.DrainContactHaptics'}
if($byName.ContainsKey($contactRoots[0])){
 $pending=[Collections.Generic.Queue[string]]::new();$seen=[Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
 foreach($name in $contactRoots){$pending.Enqueue($name)}
 $contactChecks=@()
 while($pending.Count){
  $name=$pending.Dequeue();if(!$seen.Add($name)){continue}
  if(!$byName.ContainsKey($name)){throw ('Unresolved owned hot call: '+$name)}
  $overload=0
  foreach($part in $byName[$name]){
   $bad=@(($part -split "`n")|Where-Object {$_ -match ': (newobj|newarr|box|ldstr|ldftn|ldvirtftn) ' -or $_ -match ': (call|callvirt) .*(FindObjects|GetComponentsInChildren|System\.IO\.|StringBuilder|add_onBeforeRender|remove_onBeforeRender)'})
   # Exact SDK value-type constructors produce stack values; newobj alone
   # does not imply a GC allocation. Keep all reference constructors forbidden.
   $bad=@($bad|Where-Object {$_ -notmatch ': newobj UnityEngine\.(Vector3|Quaternion)\.\.ctor\s*$'})
   $calls=@([regex]::Matches($part,'(?m)^IL_[0-9A-F]+: (?:call|callvirt) (GF2\.Private\.[^\r\n]+)')|ForEach-Object {$_.Groups[1].Value.Trim()})
   foreach($call in $calls){$pending.Enqueue($call)}
   $contactChecks+=[ordered]@{method=$name;overload=$overload++;pass=($bad.Count -eq 0);forbidden_instructions=$bad;owned_calls=$calls}
  }
 }
 $contactChecks|ConvertTo-Json -Depth 8|Set-Content -LiteralPath (Join-Path $out 'contact-hot-il-validation.json') -Encoding utf8
 if(@($contactChecks|Where-Object pass -NE $true).Count -or $seen.Count -lt 16){throw 'Contact hot transitive IL allocation/string/search/event-mutation check failed'}
 Write-Output ('Contact hot transitive IL PASS methods='+$contactChecks.Count+'; engine internal allocations are not measured')
}


