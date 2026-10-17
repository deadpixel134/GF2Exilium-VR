# Verification and limits

[한국어](../ko/VERIFICATION.md) | [English](../en/VERIFICATION.md) | [日本語](../ja/VERIFICATION.md) | [简体中文](../zh-CN/VERIFICATION.md)

> **Required before using VR: Turn OFF anti-aliasing in the game's settings.**

The VR DLLs and shared Configurator payload are identical to 1.1.1. This version updates Installer guidance only.


Previously tested setup: Meta Quest 2 · 4K monitor · Virtual Desktop.

The new Installer passed 160 variant/switch/rollback checks, 525 ownership checks and 175 layout checks. Shared projection policy passed 183,984 offline assertions plus existing-path regression. Rebuilding both native DLLs from the published source reproduces their exact release bytes. Installer and Configurator bundle the required .NET 10.0.12 runtime. Offline and installation checks are separate from actual game/HMD acceptance.

Binocular misalignment reported on Quest 3 and Galaxy XR remains unresolved. Galaxy XR reports cover both VDXR and SteamVR. Version 1.1.2 updates Installer/documentation guidance; its VR DLLs and Configurator are unchanged from 1.1.1. Adding the instruction to disable anti-aliasing does not establish that either headset report is fixed.

Install one variant at a time. Both use the same single version.dll beside the game executable; shared support files, including the OpenXR loader and Configurator, are installed under vrmod. Original GameAssembly.dll, UnityPlayer.dll and game assets are not replaced. Both variants run inside the game process; the name Clean does not guarantee nondetection or safety.
