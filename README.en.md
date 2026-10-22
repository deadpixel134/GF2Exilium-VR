# GF2VR

Creator: [@TBluebox12](https://x.com/TBluebox12)  
Arca VR community: [VR community](https://arca.live/b/vrshits)  
Support: [buymeacoffee.com/vrshits](https://buymeacoffee.com/vrshits)  

[한국어](README.md) | [English](README.en.md) | [日本語](README.ja.md) | [简体中文](README.zh-CN.md)

An unofficial mod for playing GF2 Exilium in VR.

**[Download release 1.2.0](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.2.0)**

> **Required before using VR: Turn OFF anti-aliasing in the game's settings.**

1.1.6 fixes GUI update checks and approved installation. For 1.1.4 or earlier, or an old GUI that cannot check updates, install once with the new Clean/Dirty ZIP installer. Subsequent GUI updates need neither GitHub CLI nor sign-in.

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

## Contributors

- [rgbtl1248](https://github.com/rgbtl1248) — 3DMigoto compatibility and build tooling improvements ([PR #1](https://github.com/deadpixel134/GF2Exilium-VR/pull/1)).

## 3DMigoto compatibility

1.1.3 delivers the default-wrapper 3DMigoto interoperability changes from PR #1 in both Clean and Dirty. Game wrappers remain intact; VR copies and OpenXR use the native D3D11 device/context. Matching Configurator and Installer metadata ships with the DLLs.

Use the default wrapper mode, with the [System] `hook` entry omitted or commented out. `hook=recommended` and `hook=all` are outside the supported configuration. `calls` and `crash` control logging/dumps and are not required by this fix. Skin INI syntax and duplicate-rule warnings are separate issues.

[3DMigoto](docs/en/3DMIGOTO_COMPATIBILITY.md) · [Build metadata](docs/en/BUILD_VARIANT_METADATA.md)

## Choose Clean or Dirty

| Feature | Clean | Dirty |
|---|---|---|
| Ordinary VR, stereo, controller input and settings | Included | Included |
| White spheres at both hand positions | Included | Included |
| Character breast and buttock contact/deformation | Excluded | Included |
| Manual auxiliary-panel position, rotation and tilt | Included | Included |
| Contact vibration, jiggle and regional settings | Excluded | Included |

Contact works when the character's supported bones for that region are bound. Supported regions vary by character model; actual buttock response in Dorm and Activity has not yet been confirmed.

Both variants share ordinary VR source, versions and updates. Only Dirty includes contact deformation code; Clean excludes that implementation rather than merely turning it off.

- [GF2VR-Native-SelfContained-Clean-1.2.0.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.2.0)
- [GF2VR-Native-SelfContained-Dirty-1.2.0.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.2.0)

Install one variant at a time. Both use the same single version.dll beside the game executable; shared support files, including the OpenXR loader and Configurator, are installed under vrmod. Original GameAssembly.dll, UnityPlayer.dll and game assets are not replaced. Both variants run inside the game process; the name Clean does not guarantee nondetection or safety.

## Install, switch and remove

1. Fully extract the desired Clean or Dirty ZIP into a fresh folder. GitHub Source code archives are not installers.
2. Close the game, Configurator and Installer. Run GF2VR.Installer.exe from the new folder and select GF2_Exilium.exe or the game folder. The bundled package is found automatically.
3. Confirm the variant and version, then install. To switch Clean↔Dirty, use the Installer from the desired ZIP.
4. Open Settings, save, and start the game through your normal route. Hand spheres default to enabled and can be changed in the Hands tab.

Installation, switching and removal preserve user settings. Optional touch_interaction.json is used only by Dirty. Remove restores a recorded previous installation when available; otherwise it removes only owned files. Modified or unowned DLLs are preserved and refused. Do not copy version.dll alone or mix files from different packages.

## Verification and limits

Previously tested setup: Meta Quest 2 · 4K monitor · Virtual Desktop.

The reported binocular misalignment on Quest 3 and Galaxy XR was confirmed to be caused by anti-aliasing. Turn OFF anti-aliasing in the game's settings before using VR.

## Guides

- [Installation, removal and restoration](docs/en/INSTALLATION.md)
- [Controls](docs/en/USAGE.md)
- [Settings](docs/en/SETTINGS.md)
- [Updates](docs/en/UPDATE.md)
- [Troubleshooting](docs/en/TROUBLESHOOTING.md)
- [Release notes](docs/en/RELEASE_NOTES.md)
- [Downloads and contents](docs/en/RELEASE_READINESS.md)
- [Verification](docs/en/VERIFICATION.md)
- [Source boundary and terms](SOURCE_BOUNDARY.en.md)

> GF2VR is an unofficial fan project. A legally installed copy of the game is required. Game trademarks and assets belong to their respective owners.
