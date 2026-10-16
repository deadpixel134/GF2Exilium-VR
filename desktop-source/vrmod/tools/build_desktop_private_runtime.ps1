#requires -Version 7.0
[CmdletBinding()]
param([ValidateSet('Configurator','Installer')][string]$Application='Configurator',
    [ValidatePattern('^[A-Z0-9-]+$')][string]$BuildId='FLEX-DESKTOP-001',
    [ValidatePattern('^[0-9]+\.[0-9]+\.[0-9]+$')][string]$ProductVersion='1.0.1')
$ErrorActionPreference='Stop'
$taskRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$evidence=Join-Path $taskRoot ('vrmod/build/offline/'+$BuildId)
if(Test-Path -LiteralPath (Join-Path $evidence 'desktop-ready.json')) { throw 'Preserve frozen desktop build; select a new BuildId.' }
$dotnet=Join-Path $env:ProgramFiles 'dotnet/dotnet.exe'
$project=Join-Path $taskRoot ('vrmod/src/GF2VR.'+$Application+'/GF2VR.'+$Application+'.csproj')
$category=if($Application -eq 'Configurator'){'settings'}else{'installer'}
$output=Join-Path $taskRoot ('vrmod/build/'+$category+'/'+$BuildId)
$publishOutput=Join-Path $output ('publish-'+[Guid]::NewGuid().ToString('N'))
$packCache=Join-Path $taskRoot 'vrmod/build/desktop-nuget-cache'
$httpCache=Join-Path $taskRoot 'vrmod/build/desktop-nuget-http-cache'
$targets=Join-Path $taskRoot 'vrmod/tools/desktop_private_runtime.targets'
New-Item -ItemType Directory -Path $evidence -Force | Out-Null
$notices=Join-Path $evidence 'DOTNET-NOTICES.txt'
$noticeSources=@(
    (Join-Path $packCache 'microsoft.netcore.app.runtime.win-x64/10.0.12/LICENSE.TXT'),
    (Join-Path $packCache 'microsoft.netcore.app.runtime.win-x64/10.0.12/THIRD-PARTY-NOTICES.TXT'),
    (Join-Path $packCache 'microsoft.windowsdesktop.app.runtime.win-x64/10.0.12/LICENSE')
)
function Notices {
    $content="Microsoft .NET10.0.12 Windows x64 private runtime: package-supplied licenses/notices`n`n"
    foreach($source in $noticeSources) {
        if(-not(Test-Path -LiteralPath $source)) { throw 'Restore official runtime packs before creating notices.' }
        $content+='=== '+(Split-Path (Split-Path $source -Parent) -Parent | Split-Path -Leaf)+' / '+(Split-Path $source -Leaf)+" ===`n"
        $content+=[IO.File]::ReadAllText($source)+"`n`n"
    }
    [IO.File]::WriteAllText($notices,$content,[Text.UTF8Encoding]::new($false))
}
$previousPackages=$env:NUGET_PACKAGES
$previousHttpCache=$env:NUGET_HTTP_CACHE_PATH
$previousCliHome=$env:DOTNET_CLI_HOME
$previousTelemetry=$env:DOTNET_CLI_TELEMETRY_OPTOUT
$previousCertificate=$env:DOTNET_GENERATE_ASPNET_CERTIFICATE
$previousFirstRun=$env:DOTNET_SKIP_FIRST_TIME_EXPERIENCE
try {
    $env:NUGET_PACKAGES=$packCache
    $env:NUGET_HTTP_CACHE_PATH=$httpCache
    $env:DOTNET_CLI_HOME=Join-Path $taskRoot 'vrmod/build/dotnet-cli-home'
    $env:DOTNET_CLI_TELEMETRY_OPTOUT='1'
    $env:DOTNET_GENERATE_ASPNET_CERTIFICATE='false'
    $env:DOTNET_SKIP_FIRST_TIME_EXPERIENCE='1'
    if(@($noticeSources | Where-Object {-not(Test-Path -LiteralPath $_)}).Count) {
        & $dotnet restore $project -r win-x64 -p:RuntimeFrameworkVersion=10.0.12 -p:SelfContained=true -p:PublishSingleFile=true --configfile (Join-Path $taskRoot 'vrmod/tools/desktop_runtime_nuget.config') --nologo -v minimal 2>&1 |
            Out-File -LiteralPath (Join-Path $evidence ($Application+'-restore.log')) -Encoding UTF8
        if($LASTEXITCODE -ne 0) { throw 'Official runtime restore failed.' }
    }
    Notices
    $args=@('publish',$project,'-c','Release','-r','win-x64','--self-contained','true','-o',$publishOutput,
        '-p:RuntimeFrameworkVersion=10.0.12','-p:PublishSingleFile=true','-p:IncludeNativeLibrariesForSelfExtract=true',
        '-p:EnableCompressionInSingleFile=true','-p:PublishTrimmed=false','-p:DebugType=None','-p:DebugSymbols=false',
        ('-p:CustomAfterMicrosoftCommonTargets='+$targets),'-p:RestoreIgnoreFailedSources=false',
        ('-p:GF2VRDesktopNotices='+$notices),
        '--configfile',(Join-Path $taskRoot 'vrmod/tools/desktop_runtime_nuget.config'),'--nologo','-v','minimal')
    $args+=('-p:Version='+$ProductVersion)
    & $dotnet @args 2>&1 | Out-File -LiteralPath (Join-Path $evidence ($Application+'-publish.log')) -Encoding UTF8
    if($LASTEXITCODE -ne 0) { throw ('Private desktop publish failed. See '+$Application+'-publish.log') }
    $names=@('.exe','.dll','.deps.json','.runtimeconfig.json') | ForEach-Object {'GF2VR.'+$Application+$_}
    if($Application -eq 'Configurator') { $names+='runtime-contract.json' }
    $files=@(Get-ChildItem -LiteralPath $publishOutput -File | ForEach-Object {
        [ordered]@{path=$_.Name;bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash}
    })
    if(@($files | Where-Object {$_.path -cnotin $names}).Count -or $files.Count -ne $names.Count) { throw 'Unexpected self-contained layout; stop before package integration.' }
    foreach($name in $names) { Copy-Item -LiteralPath (Join-Path $publishOutput $name) -Destination (Join-Path $output $name) -Force }
    $report=[ordered]@{build=$BuildId;application=$Application;runtime='10.0.12';rid='win-x64';selfContained=$true;nativeSelfExtraction=$true;mainAssemblyExternal=$true;trimmed=$false;output=$output;args=$args;files=$files}
    $report | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $evidence ($Application+'-build.json')) -Encoding UTF8
    Write-Output ($Application+' private-runtime host built: '+$output)
} finally {
    $env:NUGET_PACKAGES=$previousPackages; $env:NUGET_HTTP_CACHE_PATH=$previousHttpCache; $env:DOTNET_CLI_HOME=$previousCliHome
    $env:DOTNET_CLI_TELEMETRY_OPTOUT=$previousTelemetry; $env:DOTNET_GENERATE_ASPNET_CERTIFICATE=$previousCertificate; $env:DOTNET_SKIP_FIRST_TIME_EXPERIENCE=$previousFirstRun
}
