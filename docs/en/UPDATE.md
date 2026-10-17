# Updating

[한국어](../ko/UPDATE.md) | [English](../en/UPDATE.md) | [日本語](../ja/UPDATE.md) | [简体中文](../zh-CN/UPDATE.md)

When moving from 1.0.x to Clean/Dirty, manually download the desired ZIP and use its new Installer. The older updater does not choose a variant. New variant updates accept only verified packages matching the installed flavor; switching flavors is a manual install. Settings update discovery uses a logged-in GitHub CLI (gh). Access, network or verification failure preserves the existing installation and access to settings.

- [GF2VR-Native-SelfContained-Clean-1.1.1.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.1)
- [GF2VR-Native-SelfContained-Dirty-1.1.1.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.1)

Installation, switching and removal preserve user settings. Optional touch_interaction.json is used only by Dirty. Remove restores a recorded previous installation when available; otherwise it removes only owned files. Modified or unowned DLLs are preserved and refused. Do not copy version.dll alone or mix files from different packages.

Reported binocular misalignment on Galaxy XR with both VDXR and SteamVR remains unresolved. The shared projection eligibility improvement in 1.1.1 passed offline checks, but does not establish a fix for that report or support for every headset. Contact was observed in an earlier user runtime test; these exact distribution binaries have not had a new game/HMD run.
