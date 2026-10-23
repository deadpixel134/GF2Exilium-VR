# Using 3DMigoto alongside GF2VR

[한국어](../ko/3DMIGOTO_COMPATIBILITY.md) | [English](../en/3DMIGOTO_COMPATIBILITY.md) | [日本語](../ja/3DMIGOTO_COMPATIBILITY.md) | [简体中文](../zh-CN/3DMIGOTO_COMPATIBILITY.md)

[Home](../../README.en.md)

Clean and Dirty support 3DMigoto’s default wrapper mode. The game wrapper stays in place while VR uses the underlying D3D11 device/context. 3DMigoto is not required for VR alone.

> **Required before using VR: Turn OFF anti-aliasing in the game's settings.**

## Configuration

Use the default wrapper mode, with the [System] `hook` entry omitted or commented out. `hook=recommended` and `hook=all` are outside the supported configuration. `calls` and `crash` control logging/dumps and are not required by this fix. Skin INI syntax and duplicate-rule warnings are separate issues.

## Verification scope

Based on [PR #1](https://github.com/deadpixel134/GF2Exilium-VR/pull/1) by [rgbtl1248](https://github.com/rgbtl1248). The contributor’s VR/skin observations with a Dirty development build are distinct from package-level technical checks. Every skin/settings combination is not guaranteed. See [verification](VERIFICATION.md) for actual game/HMD scope.

<details>
<summary>Technical implementation notes</summary>

## Implementation

- Obtain the native device through public swapchain GetDevice, falling back to the backbuffer GetDevice if needed.
- Compare the Unity wrapper and native device through canonical COM IUnknown identity. Do not replace game wrappers.
- Retain the native device/immediate context for VR. Use the same identity check for panel, destination and controller cursor textures.
- At the pre-eye-draw boundary, compare context COM identity and refresh hooks on the retained native context.
- If the supposed native device/context exposes a known 3DMigoto wrapper interface, stop VR initialization. Reasons 11/12 identify this check; ordinary game execution remains available.

Copying a DLL alone leaves desktop integrity metadata mismatched. Extract the whole ZIP and update with its new Installer. Developers should follow [the metadata procedure](BUILD_VARIANT_METADATA.md). Diagnostic native_graphics_isolated=1 means wrapper/native addresses differ; 0 means they match. It is not a compatibility-success verdict.

</details>
