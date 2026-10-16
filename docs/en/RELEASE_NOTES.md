# Recent changes

[한국어](../ko/RELEASE_NOTES.md) · [English](../en/RELEASE_NOTES.md) · [日本語](../ja/RELEASE_NOTES.md) · [简体中文](../zh-CN/RELEASE_NOTES.md)

## 1.0.2 · October 16, 2026

- Removes package-folder selection. Fully extract the ZIP and select only the game folder; installation files are found beside the installer automatically.
- Allows receiptless files that exactly match the current package to be backed up and installed. Removal restores the files that existed beforehand.
- Preserves modified files and different DLLs, with separate actionable messages for missing ownership and changed files.
- VR rendering and headset behavior are unchanged. Local automated and GUI checks cover this change; no new reporter-PC or actual game/HMD run is claimed.

For an existing installation, fully extract the 1.0.2 ZIP into a new folder and use its installer with the existing game location. Settings and intact installation records are preserved.

## 1.0.1 · October 16, 2026

- Finds `GF2_Exilium.exe` across renamed and nested folders. Select the executable's folder or a containing installation folder; narrow the selection if several copies are found.
- Original-game SHA-256, file-size or cache differences no longer block installation. **2.0.3936** remains the practically tested baseline, without a new VR compatibility promise for other versions.
- Standard `GF2 Game` layouts keep `vrmod` in the existing parent folder; other layouts use the executable folder's `vrmod`. `version.dll` is installed beside the executable.
- Keeps product-file hash and ownership checks, running-game guards, previous-version restoration and interrupted-operation recovery.

To upgrade from 1.0.0, download the 1.0.1 ZIP and use its included installer. If the old configurator rejects the package, this manual route preserves settings and installation records.

Local installation, update, recovery and path checks passed. No new post-change run on the reporter's PC or in the actual game/HMD is recorded. See [Verification scope](VERIFICATION.md) and [Installation](INSTALLATION.md).

## 1.0.0 · October 15, 2026

The first official version is released based on confirmation that practical testing of the current implementation is complete. Start with the [installation ZIP and download guide](RELEASE_READINESS.md).

### VR display and controls

- Includes stereoscopic rendering, head tracking, front and auxiliary panels, and controller controls in supported 3D scenes.
- Provides world-scale-linked stereo depth and movement speed, left-stick-click viewpoint return, and right-stick-click camera following.
- Fixes repeated camera-stack warning calls identified during the investigation of periodic Activity-scene stuttering.
- Organizes distribution paths so that configuration and runtime files are located relative to the game installation folder.

### Configurator

- Aligns button heights and spacing, and fixes buttons being partially clipped in small windows.
- Aligns the vertical positions of the game-folder field and Browse button.
- Removes the separate feature-support tab and places the necessary descriptions beside each setting.
- Adds automatic linking of stereo depth and movement speed to world scale, along with startup camera-following settings.
- Organizes settings into six tabs: Quality and stereo depth, Movement and turning, World grip, Activity UI, Buttons, and World scale.

### Installer

- Provides the native VR module, configurator and required .NET runtime in one installation package.
- Checks supported game and package files, preserving user settings and unrelated files.
- Distinguishes installation, update, reinstallation and previous-version restoration according to installation status.
- Improves configurator launching, progress display and operation logs.
- Fixes Browse button alignment for the game-folder and package-folder fields.

### Guides

The guides have been revised to explain folder selection, applying settings and using profiles, two-handed controller controls, updates and recovery. See [Troubleshooting](TROUBLESHOOTING.md) for common situations.

See [Verified behavior and limitations](VERIFICATION.md) for device compatibility and the distinction between distribution-binary checks and practical testing. Close the game before updating, removing or recovering the installation.
