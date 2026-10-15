# Desktop source

The Configurator and Installer use Windows Forms, .NET SDK 10.0.112 and the official .NET 10.0.12 Windows x64 runtime packs. PowerShell 7 and Windows x64 are required. From this directory:

```powershell
./vrmod/tools/build_desktop_private_runtime.ps1 -Application Configurator -BuildId STABLE-DESKTOP-001 -ProductVersion 1.0.0
./vrmod/tools/build_desktop_private_runtime.ps1 -Application Installer -BuildId STABLE-DESKTOP-001 -ProductVersion 1.0.0
```

The first build restores the official runtime packs from NuGet using the checked-in source configuration. It creates a project-local cache, build output and the required .NET license notices under `vrmod/build/`. No game files or shared system .NET installation are changed. Both hosts include a private runtime; keep their app DLL, deps and runtimeconfig files alongside the EXE.

`runtime-contract.json` is an installation template for the current native module and seven configuration documents. The Installer validates the package and writes a contract bound to the user's selected installation folder. The template is not a personal configuration or an installed standalone Configurator. A native package additionally needs the exact native DLL, OpenXR loader, licenses and verified package manifest from the release. Do not substitute game files or install source-build output over a working installation without checking the complete package contract.

The app sources include local validation entry points (`--self-test`, `--verify-localization`). Installer package, portable-path and update fixtures require a complete package and an explicit previous-version package; they use inert game prerequisites and do not load the VR DLL. The release records distinguish these checks from real game/HMD acceptance.
