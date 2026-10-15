# Verified behavior and limitations

[한국어](../ko/VERIFICATION.md) · [English](../en/VERIFICATION.md) · [日本語](../ja/VERIFICATION.md) · [简体中文](../zh-CN/VERIFICATION.md)

This page describes the supported target and verification scope for official version 1.0.0. Practical user testing of the current implementation, local checks of distribution files, and verification on other PCs are distinct levels of evidence.

## Supported target

Targets GF2 Exilium **2.0.3936** for Windows x64 PCs. The installer checks original game-file fingerprints. If a game update changes those files, a new compatible release may be required. There is no compatibility list covering every HMD, controller or connection method.

## Configurator and installer

The following were checked in separate test folders:

- Saving and loading settings, defaults, and profile import/export
- Error guidance for invalid setting values and persistence of language selection
- Installation, updates, removal, previous-version restoration and interrupted-operation recovery
- Preservation of existing settings and unrelated files
- Progress display, operation logs and finding the matching configurator from the installer
- Three-language screens and button alignment in small windows and enlarged layouts

These checks did not change actual game files or user settings.

## Practical testing of the current implementation

Practical testing has covered stereoscopic rendering in supported scenes, auxiliary panels and UI controls, stick movement and turning, world grip, Activity UI visibility settings, and WASD/F/Escape input in game control mode. The implementation, including world-scale linking, left-stick-click viewpoint return, right-stick-click camera following, and the current configurator and installer screens, was frozen for the official release based on the user's confirmation that testing was complete.

For stuttering reported every 2–4 seconds in Activity scenes, calls that generated repeated camera warnings were fixed. Later game logs contained no occurrences of that warning, and the user selected the current implementation as the stable version. This does not guarantee a numerically measured minimum frame rate or uninterrupted behavior in every situation.

## Distribution-file checks and limits

The native distribution module preserves the rendering and input code of that implementation while locating files relative to the game installation folder. Path relocation, Windows API calls, exports and file integrity were checked locally. A fresh game/HMD run of this exact distribution binary has not been separately recorded.

The required .NET runtime is included, but the complete process from installation through HMD use has not been reverified on a fresh Windows PC without .NET. Testing also does not cover every scene transition or button combination.

## Updates and distribution

Package checks, installation, updates and previous-version restoration were checked in test folders. As 1.0.0 is the first official release, there is no completed real-user update history to a later official version yet. Existing-installation checks and recovery procedures are also used when downloading and applying a new version.

If problems occur, record the scene, controls, device environment and error message using the [troubleshooting guide](TROUBLESHOOTING.md). See the [release guide](RELEASE_READINESS.md) for downloads.
