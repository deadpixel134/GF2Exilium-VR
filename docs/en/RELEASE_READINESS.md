# Downloads and support

[한국어](../ko/RELEASE_READINESS.md) · [English](../en/RELEASE_READINESS.md) · [日本語](../ja/RELEASE_READINESS.md) · [简体中文](../zh-CN/RELEASE_READINESS.md)

## Official version 1.0.2

Download **GF2VR-Native-SelfContained-1.0.2.zip** from the [GitHub release page](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.0.2). Public downloads do not require a GitHub login or a separate token.

- [Download the installation ZIP](https://github.com/deadpixel134/GF2Exilium-VR/releases/download/v1.0.2/GF2VR-Native-SelfContained-1.0.2.zip)
- [Installation, removal and recovery guide](INSTALLATION.md)
- [Current releases](https://github.com/deadpixel134/GF2Exilium-VR/releases)

GitHub's separate **Source code** ZIP is not an installer. Extract the entire release ZIP into a new folder, then run **GF2VR.Installer.exe**.

## Supported environment

A legitimate game installation and VR environment on Windows x64 are required. **2.0.3936** is the practically tested baseline. Original-game SHA-256, file-size or cache differences do not block installation; this does not guarantee VR compatibility with other game versions. Obtain headset/controller software and SteamVR/OpenXR separately.

The standard `GF2 Game/GF2_Exilium.exe` layout keeps `vrmod` in its existing parent folder. Other layouts use `vrmod` in the located executable's folder. `version.dll` is always installed beside that executable.

The configurator and installer support Korean, English and Japanese and include the required .NET runtime. Start the game and SteamVR yourself. See [Verification scope](VERIFICATION.md) for verified behavior and environments not yet tested.

## Updates

Download a new release ZIP and use its installer to update your existing installation. The configurator's optional update feature uses the installed **GitHub CLI (gh)** and its login state. Manual installation and updates from the public release remain available without this feature. See [Updating](UPDATE.md) for details.

## Terms of use and help

The original game is not included. A distribution license for the project source has not yet been specified, and third-party components are subject to their own licenses. See [Source contents and terms of use](../../SOURCE_BOUNDARY.en.md).

If a problem occurs, follow [Troubleshooting](TROUBLESHOOTING.md) to record the error message and steps that led to it. You can find the community at the [Arca.live Virtual Reality channel](https://arca.live/b/vrshits).

Installation ZIP size: **93,673,090 bytes**. SHA-256:

```text
9E2BDE8C9C3DB46F98BE16305899169EB7B11658C6BC1D7772A00AA55FB4ADA9
```
