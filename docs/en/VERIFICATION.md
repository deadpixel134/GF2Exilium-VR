# Verified behavior and limitations

[한국어](../ko/VERIFICATION.md) · [English](../en/VERIFICATION.md) · [日本語](../ja/VERIFICATION.md) · [简体中文](../zh-CN/VERIFICATION.md)

This page describes the supported target and verification scope for official version 1.0.2. Practical user testing of the current implementation, local checks of distribution files, and verification on other PCs are distinct levels of evidence.

## Supported target

A legitimate game installation and VR environment on Windows x64 are required. **2.0.3936** is the practically tested baseline. Original-game SHA-256, file-size or cache differences do not block installation; this does not guarantee VR compatibility with other game versions.

## Configurator and installer

The following were checked in separate test folders:

- Saving and loading settings, defaults, and profile import/export
- Error guidance for invalid setting values and persistence of language selection
- Installation, updates, removal, previous-version restoration and interrupted-operation recovery
- Preservation of existing settings and unrelated files
- Progress display, operation logs and finding the matching configurator from the installer
- Three-language screens and button alignment in small windows and enlarged layouts

These checks did not change actual game files or user settings.

Version 1.0.2 passed 1,570 managed checks, actual GUI checks in three languages and automatic-package preflight from the release ZIP. Inert folders cover backup/restoration of identical receiptless files, modified-file protection, settings preservation and installation/restoration from 1.0.0 and 1.0.1. The native DLL is unchanged from 1.0.1; its existing 214 boundary checks and exact rebuild evidence are reused. No new reporter-PC or actual game/HMD run is claimed.

## Practical testing of the current implementation

The practical observations below describe the 1.0.0 baseline implementation. Version 1.0.2 changes installation discovery and path handling; post-change reporter-PC and game/HMD testing is not yet recorded.

Practical testing has covered stereoscopic rendering in supported scenes, auxiliary panels and UI controls, stick movement and turning, world grip, Activity UI visibility settings, and WASD/F/Escape input in game control mode. The implementation, including world-scale linking, left-stick-click viewpoint return, right-stick-click camera following, and the current configurator and installer screens, was frozen for the official release based on the user's confirmation that testing was complete.

For stuttering reported every 2–4 seconds in Activity scenes, calls that generated repeated camera warnings were fixed. Later game logs contained no occurrences of that warning, and the user selected the current implementation as the stable version. This does not guarantee a numerically measured minimum frame rate or uninterrupted behavior in every situation.

## Distribution-file checks and limits

The native distribution module preserves the rendering and input code of that implementation while locating files relative to the game installation folder. Path relocation, Windows API calls, exports and file integrity were checked locally. A fresh game/HMD run of this exact distribution binary has not been separately recorded.

The required .NET runtime is included, but the complete process from installation through HMD use has not been reverified on a fresh Windows PC without .NET. Testing also does not cover every scene transition or button combination.

## Updates and distribution

Updates from 1.0.0 to 1.0.2, previous-version restoration and interrupted-operation recovery were checked in inert folders using the **new 1.0.2 installer**. Automatic replacement through the old 1.0.0 configurator and completion on an actual user's PC have not been verified.

If problems occur, record the scene, controls, device environment and error message using the [troubleshooting guide](TROUBLESHOOTING.md). See the [release guide](RELEASE_READINESS.md) for downloads.
