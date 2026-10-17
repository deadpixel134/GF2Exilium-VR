# Changes

[한국어](../ko/RELEASE_NOTES.md) | [English](../en/RELEASE_NOTES.md) | [日本語](../ja/RELEASE_NOTES.md) | [简体中文](../zh-CN/RELEASE_NOTES.md)

## 1.1.2

> **Required before using VR: Turn OFF anti-aliasing in the game's settings.**

1.1.2 adds a requirement to turn off anti-aliasing in game settings to the initial Installer screen and README. Both Clean and Dirty show the same guidance. Game settings are not changed automatically. VR DLLs, Configurator and contact behavior are unchanged from 1.1.1.

The reported binocular misalignment on Quest 3 and Galaxy XR was confirmed to be caused by anti-aliasing. Turn OFF anti-aliasing in the game's settings before using VR.

## 1.1.1


1.1.1 distributes separate Clean and Dirty ZIPs. Both include white hand spheres; contact deformation is Dirty-only. Ordinary functionality shares source, version and update contracts. Explicit same-version flavor switching preserves settings. Projection correction eligibility now accepts a sufficient vertical-axis witness as well as the existing horizontal witness.

The new Installer passed 160 variant/switch/rollback checks, 525 ownership checks and 175 layout checks. Shared projection policy passed 183,984 offline assertions plus existing-path regression. Rebuilding both native DLLs from the published source reproduces their exact release bytes. Installer and Configurator bundle the required .NET 10.0.12 runtime. Offline and installation checks are separate from actual game/HMD acceptance.

The reported binocular misalignment on Quest 3 and Galaxy XR was confirmed to be caused by anti-aliasing. Turn OFF anti-aliasing in the game's settings before using VR.

[1.0.2](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.0.2) · [1.0.1](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.0.1) · [1.0.0](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.0.0)
