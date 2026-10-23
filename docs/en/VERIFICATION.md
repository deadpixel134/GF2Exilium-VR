# Supported environments and verification

[한국어](../ko/VERIFICATION.md) | [English](../en/VERIFICATION.md) | [日本語](../ja/VERIFICATION.md) | [简体中文](../zh-CN/VERIFICATION.md)

[Home](../../README.en.md)

Platform compatibility, observations in the actual game and offline checks are separate kinds of evidence. Editing documentation or rebuilding source does not add HMD verification.

## Platforms and actual game observations

| Target | Confirmed scope |
|---|---|
| Haoplay · Dirty | Main/Activity hand display and contact paths |
| Steam · Dirty | Main launch and stereoscopic HMD output |
| Other Steam contexts · new Clean DLL | Separate HMD confirmation is still needed |

The previously tested setup is Meta Quest 2, a 4K monitor and Virtual Desktop. In-game anti-aliasing was identified as the cause of reported eye misalignment on Quest 3 and Galaxy XR and must be OFF. This list does not guarantee every headset or connection method.

## Feature limits

Contact is Dirty-only and requires supported bones for the relevant body region. It does not cover every character, model or screen. Actual buttock response in Dorm/Activity has not been separately confirmed. Frame rate depends on the scene, hardware and render settings.

3DMigoto support covers its default wrapper mode; see [compatibility](3DMIGOTO_COMPATIBILITY.md). Clean also runs inside the game process; its name does not guarantee undetectability or safety.

## Package and source checks

Clean/Dirty DLLs reproduced from public source match the shipped DLL bytes. Installation, switching, restore, settings/localized tools, package integrity and old-client update paths received technical checks. These do not establish every user environment or every scene’s HMD behavior. Version-specific records remain in [release notes](RELEASE_NOTES.md) and [GitHub releases](https://github.com/deadpixel134/GF2Exilium-VR/releases).
