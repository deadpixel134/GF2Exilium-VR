# Changes

[한국어](../ko/RELEASE_NOTES.md) | [English](../en/RELEASE_NOTES.md) | [日本語](../ja/RELEASE_NOTES.md) | [简体中文](../zh-CN/RELEASE_NOTES.md)

## 1.3.4 — Game update compatibility

Adds API-name bindings for Haoplay 3.0.5388 and Steam 3.0.5400. The updated games renamed internal APIs, so the previous bindings could no longer find APIs required for VR initialization.

The actual game/HMD has NOT been tested. Bindings were checked against both game DLLs through static analysis, with automated API lookup, hook safety, build and package checks. Additional runtime compatibility issues may remain.

[1.3.4](../release-notes/1.3.4.en.md)

## 1.2.0 — Steam support and hand/contact compatibility fixes

Supports the Steam executable folder and existing Haoplay layout. Fixes compatibility after the game update that removed white hand indicators and character contact. Dirty handles each simultaneously visible supported Activity character independently. Clean excludes contact deformation.

Hardware validation covers Dirty hands/contact in Haoplay Main and Activity, and Steam Main launch/stereo. All Steam contexts and the new Clean DLL have not received separate HMD acceptance. Turn OFF in-game anti-aliasing.

An intact managed 1.1.6 installation can update to 1.2.0 by approving the Settings GUI notification, without GitHub CLI or sign-in. Flavor and user settings are preserved. For 1.1.4 or earlier, or old clients unable to check updates, extract the entire desired ZIP into a fresh folder and run its GF2VR.Installer.exe once. Close the game first.

The release version is 1.2.0. Internal native build IDs retaining 130 identify the exact validated DLLs from an earlier unpublished candidate. The normal Installer does not silently overwrite manually replaced or unowned files.

## 1.1.6 — Automatic update fix

The settings GUI checks public GitHub releases at startup. Approve **Update** in the notification to download, verify and install. Starting with 1.1.6, GitHub CLI and sign-in are no longer required.

Fixed rejection of new native builds by the old clients frozen allowlist. The client verifies release/file hashes, build flavor and current ownership; the verified incoming Installer owns final native validation, backups and installation. Clean/Dirty flavor and settings are preserved.

For 1.1.4 or earlier, extract the complete desired Clean/Dirty ZIP into a new folder and run its new GF2VR.Installer.exe once. Existing 1.1.5 installations with working update checks can use the GUI updater. Users whose old version lacks its required GitHub CLI also need this one-time ZIP installation. After installing 1.1.6, GUI updates need neither CLI nor sign-in.

This release retains the exact 1.1.5 VR DLLs. Logging remains OFF by default; explicit ON and its permanent initial-cleanup exemption, panel and contact settings are preserved. Close the game and settings window before installation.

## 1.1.5 — Logging controls

Added **Diagnostics → Automatically record logs** to the settings GUI in both Clean and Dirty. The default is **OFF**, including when older settings omit this option. Save and restart the game to apply.

On the first update installation, previous automatic VR logs are cleaned once for users with logging OFF. **Users with logging ON keep their existing logs and are excluded from this initial cleanup.** Switching OFF later does not trigger a deferred deletion. Settings, other mods and manually collected evidence are preserved.

Enable logging when troubleshooting. With logging ON, the same build log is appended and may grow over time.

For 1.1.5, extract the entire desired Clean or Dirty ZIP into a new folder and use its new GF2VR.Installer.exe. Older settings programs may reject the new native profile. Existing settings are preserved.

## 1.1.4

**Common**

- Added manual auxiliary panel settings

**Dirty build**

- Added vibration on character contact
- Added character-contact jiggle and settings


## 1.1.3

Updating to 1.1.3: extract the complete Clean or Dirty ZIP into a new folder and use its GF2VR.Installer.exe with your existing game location. The old 1.1.2 Configurator rejects automatic staging because it cannot validate the new native profile, preserving the current installation. Do not copy only the DLL or mix package files.

1.1.3 delivers the default-wrapper 3DMigoto interoperability changes from PR #1 in both Clean and Dirty. Game wrappers remain intact; VR copies and OpenXR use the native D3D11 device/context. Matching Configurator and Installer metadata ships with the DLLs.

Use the default wrapper mode, with the [System] `hook` entry omitted or commented out. `hook=recommended` and `hook=all` are outside the supported configuration. `calls` and `crash` control logging/dumps and are not required by this fix. Skin INI syntax and duplicate-rule warnings are separate issues.

> **Required before using VR: Turn OFF anti-aliasing in the game's settings.**

The PR author reported simultaneous VR and skin rendering on three consecutive launches of a Dirty development build. That is contributor evidence, not HMD acceptance of these new 1.1.3 binaries. This release verifies both native builds, COM identity/failure paths using the actual changed source, desktop builds and package integrity. No new game/HMD run was performed.

Contribution: [rgbtl1248](https://github.com/rgbtl1248), [PR #1](https://github.com/deadpixel134/GF2Exilium-VR/pull/1).

## 1.1.2

> **Required before using VR: Turn OFF anti-aliasing in the game's settings.**

1.1.2 adds a requirement to turn off anti-aliasing in game settings to the initial Installer screen and README. Both Clean and Dirty show the same guidance. Game settings are not changed automatically. VR DLLs, Configurator and contact behavior are unchanged from 1.1.1.

The reported binocular misalignment on Quest 3 and Galaxy XR was confirmed to be caused by anti-aliasing. Turn OFF anti-aliasing in the game's settings before using VR.

## 1.1.1


1.1.1 distributes separate Clean and Dirty ZIPs. Both include white hand spheres; contact deformation is Dirty-only. Ordinary functionality shares source, version and update contracts. Explicit same-version flavor switching preserves settings. Projection correction eligibility now accepts a sufficient vertical-axis witness as well as the existing horizontal witness.

The new Installer passed 160 variant/switch/rollback checks, 525 ownership checks and 175 layout checks. Shared projection policy passed 183,984 offline assertions plus existing-path regression. Rebuilding both native DLLs from the published source reproduces their exact release bytes. Installer and Configurator bundle the required .NET 10.0.12 runtime. Offline and installation checks are separate from actual game/HMD acceptance.

The reported binocular misalignment on Quest 3 and Galaxy XR was confirmed to be caused by anti-aliasing. Turn OFF anti-aliasing in the game's settings before using VR.

[1.0.2](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.0.2) · [1.0.1](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.0.1) · [1.0.0](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.0.0)
