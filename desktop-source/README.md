# Desktop source

[한국어](README.ko.md) | [English](README.md) | [日本語](README.ja.md) | [简体中文](README.zh-CN.md)

Both variants share ordinary VR source, versions and updates. Only Dirty includes contact deformation code; Clean excludes that implementation rather than merely turning it off.

Installation, switching and removal preserve user settings. Optional touch_interaction.json is used only by Dirty. Remove restores a recorded previous installation when available; otherwise it removes only owned files. Modified or unowned DLLs are preserved and refused. Do not copy version.dll alone or mix files from different packages.


The Configurator and Installer use Windows Forms, .NET SDK 10.0.112 and the official .NET 10.0.12 Windows x64 runtime packs. PowerShell 7 and Windows x64 are required. From this directory:

```powershell
./vrmod/tools/build_desktop_private_runtime.ps1 -Application Configurator -BuildId RELEASE-1.1.1 -ProductVersion 1.1.1
./vrmod/tools/build_desktop_private_runtime.ps1 -Application Installer -BuildId RELEASE-1.1.1 -ProductVersion 1.1.1
```

The first build restores the official runtime packs from NuGet using the checked-in source configuration. It creates a project-local cache, build output and the required .NET license notices under `vrmod/build/`. No game files or shared system .NET installation are changed. Both hosts include a private runtime; keep their app DLL, deps and runtimeconfig files alongside the EXE.

`runtime-contract.json` is an installation template for the current native module and seven configuration documents. The Installer validates the package and writes a contract bound to the user's selected installation folder. The template is not a personal configuration or an installed standalone Configurator. A native package additionally needs the exact native DLL, OpenXR loader, licenses and verified package manifest from the release. Do not substitute game files or install source-build output over a working installation without checking the complete package contract.

The app sources include local validation entry points (`--self-test`, `--verify-localization`). Installer package, portable-path and update fixtures require a complete package and an explicit previous-version package; they use inert game prerequisites and do not load the VR DLL. The release records distinguish these checks from real game/HMD acceptance.

Version 1.1.1 discovers the game executable across folder layouts and makes original-game fingerprint differences nonblocking. The standard `GF2 Game/GF2_Exilium.exe` layout keeps `vrmod` in its existing parent folder. Other layouts use `vrmod` in the located executable's folder. `version.dll` is always installed beside that executable. Product-package integrity and existing-file ownership checks remain enforced. `--self-test-flexible` exercises these installation paths with inert files.
