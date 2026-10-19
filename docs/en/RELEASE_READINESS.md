# GF2VR 1.1.3 — Downloads

[한국어](../ko/RELEASE_READINESS.md) | [English](../en/RELEASE_READINESS.md) | [日本語](../ja/RELEASE_READINESS.md) | [简体中文](../zh-CN/RELEASE_READINESS.md)

## 1.1.4

Both builds provide auxiliary-panel viewer-facing, position, rotation, screen tilt and maximum-size settings. Dirty adds character-contact vibration and separate breast/buttock jiggle settings.

Both native DLLs reproduce byte for byte from source. Passed: 4,517 native placement/configuration checks, 706 Clean and 941 Dirty GUI checks, 236 regional-default/prior-actual-IL explicit-profile parity checks, 236 actual GUI-default JSON checks, 159 install/switch/failure-rollback checks and 11 exact prior-install restoration checks. The bridge contains 131 verified methods; contact hot IL covers 46 methods. These are offline, inert-fixture and personal-install checks; no new game/HMD run is claimed as acceptance.


> **Required before using VR: Turn OFF anti-aliasing in the game's settings.**

Updating to 1.1.3: extract the complete Clean or Dirty ZIP into a new folder and use its GF2VR.Installer.exe with your existing game location. The old 1.1.2 Configurator rejects automatic staging because it cannot validate the new native profile, preserving the current installation. Do not copy only the DLL or mix package files.

1.1.3 delivers the default-wrapper 3DMigoto interoperability changes from PR #1 in both Clean and Dirty. Game wrappers remain intact; VR copies and OpenXR use the native D3D11 device/context. Matching Configurator and Installer metadata ships with the DLLs.


- [GF2VR-Native-SelfContained-Clean-1.1.3.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.3)
- [GF2VR-Native-SelfContained-Dirty-1.1.3.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.3)

| Feature | Clean | Dirty |
|---|---|---|
| Ordinary VR, stereo, controller input and settings | Included | Included |
| White spheres at both hand positions | Included | Included |
| Character breast contact and deformation | Excluded | Included |

Both variants share ordinary VR source, versions and updates. Only Dirty includes contact deformation code; Clean excludes that implementation rather than merely turning it off.

Install one variant at a time. Both use the same single version.dll beside the game executable; shared support files, including the OpenXR loader and Configurator, are installed under vrmod. Original GameAssembly.dll, UnityPlayer.dll and game assets are not replaced. Both variants run inside the game process; the name Clean does not guarantee nondetection or safety.

Installation, switching and removal preserve user settings. Optional touch_interaction.json is used only by Dirty. Remove restores a recorded previous installation when available; otherwise it removes only owned files. Modified or unowned DLLs are preserved and refused. Do not copy version.dll alone or mix files from different packages.

The new Installer passed 160 variant/switch/rollback checks, 525 ownership checks and 175 layout checks. Shared projection policy passed 183,984 offline assertions plus existing-path regression. Rebuilding both native DLLs from the published source reproduces their exact release bytes. Installer and Configurator bundle the required .NET 10.0.12 runtime. Offline and installation checks are separate from actual game/HMD acceptance.

The reported binocular misalignment on Quest 3 and Galaxy XR was confirmed to be caused by anti-aliasing. Turn OFF anti-aliasing in the game's settings before using VR.

CHECKSUMS.sha256 · GF2VR-Clean-update.json · GF2VR-Dirty-update.json
