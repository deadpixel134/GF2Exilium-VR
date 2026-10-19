# Review and verification

[한국어](../ko/VERIFICATION.md) | [English](../en/VERIFICATION.md) | [日本語](../ja/VERIFICATION.md) | [简体中文](../zh-CN/VERIFICATION.md)

## 1.1.5 — Logging controls

Added **Diagnostics → Automatically record logs** to the settings GUI in both Clean and Dirty. The default is **OFF**, including when older settings omit this option. Save and restart the game to apply.

On the first update installation, previous automatic VR logs are cleaned once for users with logging OFF. **Users with logging ON keep their existing logs and are excluded from this initial cleanup.** Switching OFF later does not trigger a deferred deletion. Settings, other mods and manually collected evidence are preserved.

Enable logging when troubleshooting. With logging ON, the same build log is appended and may grow over time.

For 1.1.5, extract the entire desired Clean or Dirty ZIP into a new folder and use its new GF2VR.Installer.exe. Older settings programs may reject the new native profile. Existing settings are preserved.

## 1.1.4

Both native DLLs reproduce byte for byte from source. Passed: 4,517 native placement/configuration checks, 706 Clean and 941 Dirty GUI checks, 236 regional-default/prior-actual-IL explicit-profile parity checks, 236 actual GUI-default JSON checks, 159 install/switch/failure-rollback checks and 11 exact prior-install restoration checks. The bridge contains 131 verified methods; contact hot IL covers 46 methods. These are offline, inert-fixture and personal-install checks; no new game/HMD run is claimed as acceptance.


> **Required before using VR: Turn OFF anti-aliasing in the game's settings.**

The PR author reported simultaneous VR and skin rendering on three consecutive launches of a Dirty development build. That is contributor evidence, not HMD acceptance of these new 1.1.3 binaries. This release verifies both native builds, COM identity/failure paths using the actual changed source, desktop builds and package integrity. No new game/HMD run was performed.

The reported binocular misalignment on Quest 3 and Galaxy XR was confirmed to be caused by anti-aliasing. Turn OFF anti-aliasing in the game's settings before using VR.

Adds compatibility with default-wrapper 3DMigoto and matching generated build metadata. Ordinary VR and hands stay common; contact deformation remains Dirty-only. Existing settings and installation ownership/rollback protections are retained.

[3DMigoto](3DMIGOTO_COMPATIBILITY.md) · [Build metadata](BUILD_VARIANT_METADATA.md)

Contribution: [rgbtl1248](https://github.com/rgbtl1248), [PR #1](https://github.com/deadpixel134/GF2Exilium-VR/pull/1).

Updating to 1.1.3: extract the complete Clean or Dirty ZIP into a new folder and use its GF2VR.Installer.exe with your existing game location. The old 1.1.2 Configurator rejects automatic staging because it cannot validate the new native profile, preserving the current installation. Do not copy only the DLL or mix package files.

1.1.3: COM 47; Clean/Dirty install-switch-rollback 160; ownership 525; layout 175; package location 12; Installer UX 19; Configurator 577 × 2; 1.1.2 → 1.1.3 → exact rollback 28. Offline/inert fixtures; game/HMD acceptance is separate.
