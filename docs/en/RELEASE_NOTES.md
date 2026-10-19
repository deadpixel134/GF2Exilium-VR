# Changes

[한국어](../ko/RELEASE_NOTES.md) | [English](../en/RELEASE_NOTES.md) | [日本語](../ja/RELEASE_NOTES.md) | [简体中文](../zh-CN/RELEASE_NOTES.md)

## 1.1.4

**Common**

- Added manual auxiliary panel settings

**Dirty build**

- Added vibration on character contact
- Added character-contact jiggle and settings


## 1.1.3

Updating to 1.1.3: extract the complete Clean or Dirty ZIP into a new folder and use its GF2VR.Installer.exe with your existing game location. The old 1.1.2 Configurator rejects automatic staging because it cannot validate the new native profile, preserving the current installation. Do not copy only the DLL or mix package files.

1.1.3 delivers the default-wrapper 3DMigoto interoperability changes from PR #1 in both Clean and Dirty. Game wrappers remain intact; VR copies and OpenXR use the native D3D11 device/context. Matching Configurator and Installer metadata ships with the DLLs.

Use the default wrapper mode, with the [System] `hook` entry omitted or commented out. `hook=recommended` and `hook=all` are outside the supported configuration. `calls` and `crash` control logging/dumps and are not required by this fix. Skin INI syntax and duplicate-rule warnings are separate issues.

> **Required before using VR: Turn OFF anti-aliasing in the game's settings.**

The PR author reported simultaneous VR and skin rendering on three consecutive launches of a Dirty development build. That is contributor evidence, not HMD acceptance of these new 1.1.3 binaries. This release verifies both native builds, COM identity/failure paths using the actual changed source, desktop builds and package integrity. No new game/HMD run was performed.

Contribution: [rgbtl1248](https://github.com/rgbtl1248), [PR #1](https://github.com/deadpixel134/GF2Exilium-VR/pull/1).

## 1.1.2

> **Required before using VR: Turn OFF anti-aliasing in the game's settings.**

1.1.2 adds a requirement to turn off anti-aliasing in game settings to the initial Installer screen and README. Both Clean and Dirty show the same guidance. Game settings are not changed automatically. VR DLLs, Configurator and contact behavior are unchanged from 1.1.1.

The reported binocular misalignment on Quest 3 and Galaxy XR was confirmed to be caused by anti-aliasing. Turn OFF anti-aliasing in the game's settings before using VR.

## 1.1.1


1.1.1 distributes separate Clean and Dirty ZIPs. Both include white hand spheres; contact deformation is Dirty-only. Ordinary functionality shares source, version and update contracts. Explicit same-version flavor switching preserves settings. Projection correction eligibility now accepts a sufficient vertical-axis witness as well as the existing horizontal witness.

The new Installer passed 160 variant/switch/rollback checks, 525 ownership checks and 175 layout checks. Shared projection policy passed 183,984 offline assertions plus existing-path regression. Rebuilding both native DLLs from the published source reproduces their exact release bytes. Installer and Configurator bundle the required .NET 10.0.12 runtime. Offline and installation checks are separate from actual game/HMD acceptance.

The reported binocular misalignment on Quest 3 and Galaxy XR was confirmed to be caused by anti-aliasing. Turn OFF anti-aliasing in the game's settings before using VR.

[1.0.2](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.0.2) · [1.0.1](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.0.1) · [1.0.0](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.0.0)
