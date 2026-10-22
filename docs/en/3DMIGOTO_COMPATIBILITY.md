# Using GF2VR with 3DMigoto

[한국어](../ko/3DMIGOTO_COMPATIBILITY.md) | [English](../en/3DMIGOTO_COMPATIBILITY.md) | [日本語](../ja/3DMIGOTO_COMPATIBILITY.md) | [简体中文](../zh-CN/3DMIGOTO_COMPATIBILITY.md)

1.1.3 delivers the default-wrapper 3DMigoto interoperability changes from PR #1 in both Clean and Dirty. Game wrappers remain intact; VR copies and OpenXR use the native D3D11 device/context. Matching Configurator and Installer metadata ships with the DLLs.

> **Required before using VR: Turn OFF anti-aliasing in the game's settings.**

## 1.2.0 — Steam support and hand/contact compatibility fixes

Supports the Steam executable folder and existing Haoplay layout. Fixes compatibility after the game update that removed white hand indicators and character contact. Dirty handles each simultaneously visible supported Activity character independently. Clean excludes contact deformation.

Hardware validation covers Dirty hands/contact in Haoplay Main and Activity, and Steam Main launch/stereo. All Steam contexts and the new Clean DLL have not received separate HMD acceptance. Turn OFF in-game anti-aliasing.

An intact managed 1.1.6 installation can update to 1.2.0 by approving the Settings GUI notification, without GitHub CLI or sign-in. Flavor and user settings are preserved. For 1.1.4 or earlier, or old clients unable to check updates, extract the entire desired ZIP into a fresh folder and run its GF2VR.Installer.exe once. Close the game first.

The release version is 1.2.0. Internal native build IDs retaining 130 identify the exact validated DLLs from an earlier unpublished candidate. The normal Installer does not silently overwrite manually replaced or unowned files.

## Configuration

Use the default wrapper mode, with the [System] `hook` entry omitted or commented out. `hook=recommended` and `hook=all` are outside the supported configuration. `calls` and `crash` control logging/dumps and are not required by this fix. Skin INI syntax and duplicate-rule warnings are separate issues.

## Implementation

- Obtain the native device through public swapchain GetDevice, falling back to the backbuffer GetDevice if needed.
- Compare the Unity wrapper and native device through canonical COM IUnknown identity. Do not replace game wrappers.
- Retain the native device/immediate context for VR. Use the same identity check for panel, destination and controller cursor textures.
- At the pre-eye-draw boundary, compare context COM identity and refresh hooks on the retained native context.
- If the supposed native device/context exposes a known 3DMigoto wrapper interface, stop VR initialization. Reasons 11/12 identify this check; ordinary game execution remains available.

Copying a DLL alone leaves desktop integrity metadata mismatched. Extract the whole ZIP and update with its new Installer. Developers should follow [the metadata procedure](BUILD_VARIANT_METADATA.md). Diagnostic native_graphics_isolated=1 means wrapper/native addresses differ; 0 means they match. It is not a compatibility-success verdict.

## Evidence and limits

The PR author reported simultaneous VR and skin rendering on three consecutive launches of a Dirty development build. That is contributor evidence, not HMD acceptance of these new 1.1.3 binaries. This release verifies both native builds, COM identity/failure paths using the actual changed source, desktop builds and package integrity. No new game/HMD run was performed.

The reported binocular misalignment on Quest 3 and Galaxy XR was confirmed to be caused by anti-aliasing. Turn OFF anti-aliasing in the game's settings before using VR.

Contribution: [rgbtl1248](https://github.com/rgbtl1248), [PR #1](https://github.com/deadpixel134/GF2Exilium-VR/pull/1).
