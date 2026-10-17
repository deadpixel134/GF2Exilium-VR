# Verification and limits

[한국어](../ko/VERIFICATION.md) | [English](../en/VERIFICATION.md) | [日本語](../ja/VERIFICATION.md) | [简体中文](../zh-CN/VERIFICATION.md)

Previously tested setup: Meta Quest 2 · 4K monitor · Virtual Desktop.

The new Installer passed 160 variant/switch/rollback checks, 525 ownership checks and 175 layout checks. Shared projection policy passed 183,984 offline assertions plus existing-path regression. Rebuilding both native DLLs from the published source reproduces their exact release bytes. Installer and Configurator bundle the required .NET 10.0.12 runtime. Offline and installation checks are separate from actual game/HMD acceptance.

Reported binocular misalignment on Galaxy XR with both VDXR and SteamVR remains unresolved. The shared projection eligibility improvement in 1.1.1 passed offline checks, but does not establish a fix for that report or support for every headset. Contact was observed in an earlier user runtime test; these exact distribution binaries have not had a new game/HMD run.

Install one variant at a time. Both use the same single version.dll beside the game executable; shared support files, including the OpenXR loader and Configurator, are installed under vrmod. Original GameAssembly.dll, UnityPlayer.dll and game assets are not replaced. Both variants run inside the game process; the name Clean does not guarantee nondetection or safety.
