# Updating

[한국어](../ko/UPDATE.md) | [English](../en/UPDATE.md) | [日本語](../ja/UPDATE.md) | [简体中文](../zh-CN/UPDATE.md)

## 1.1.5 — Logging controls

Added **Diagnostics → Automatically record logs** to the settings GUI in both Clean and Dirty. The default is **OFF**, including when older settings omit this option. Save and restart the game to apply.

On the first update installation, previous automatic VR logs are cleaned once for users with logging OFF. **Users with logging ON keep their existing logs and are excluded from this initial cleanup.** Switching OFF later does not trigger a deferred deletion. Settings, other mods and manually collected evidence are preserved.

Enable logging when troubleshooting. With logging ON, the same build log is appended and may grow over time.

For 1.1.5, extract the entire desired Clean or Dirty ZIP into a new folder and use its new GF2VR.Installer.exe. Older settings programs may reject the new native profile. Existing settings are preserved.

## 1.1.4

Updating to 1.1.4: extract the entire Clean or Dirty ZIP into a new folder and install with its GF2VR.Installer.exe at your existing game location. The old 1.1.3 Configurator cannot validate the new native profile, so use the installer from the new ZIP for this update. Existing settings are preserved. Do not copy only the DLL or mix package files.


> **Required before using VR: Turn OFF anti-aliasing in the game's settings.**

Updating to 1.1.3: extract the complete Clean or Dirty ZIP into a new folder and use its GF2VR.Installer.exe with your existing game location. The old 1.1.2 Configurator rejects automatic staging because it cannot validate the new native profile, preserving the current installation. Do not copy only the DLL or mix package files.

1.1.3 delivers the default-wrapper 3DMigoto interoperability changes from PR #1 in both Clean and Dirty. Game wrappers remain intact; VR copies and OpenXR use the native D3D11 device/context. Matching Configurator and Installer metadata ships with the DLLs.


When moving from 1.0.x to Clean/Dirty, manually download the desired ZIP and use its new Installer. The older updater does not choose a variant. New variant updates accept only verified packages matching the installed flavor; switching flavors is a manual install. Settings update discovery uses a logged-in GitHub CLI (gh). Access, network or verification failure preserves the existing installation and access to settings.

- [GF2VR-Native-SelfContained-Clean-1.1.3.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.3)
- [GF2VR-Native-SelfContained-Dirty-1.1.3.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.3)

Installation, switching and removal preserve user settings. Optional touch_interaction.json is used only by Dirty. Remove restores a recorded previous installation when available; otherwise it removes only owned files. Modified or unowned DLLs are preserved and refused. Do not copy version.dll alone or mix files from different packages.

The reported binocular misalignment on Quest 3 and Galaxy XR was confirmed to be caused by anti-aliasing. Turn OFF anti-aliasing in the game's settings before using VR.
