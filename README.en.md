# GF2VR

Creator: [@TBluebox12](https://x.com/TBluebox12)  
Arca VR community: [VR community](https://arca.live/b/vrshits)  
Support: [buymeacoffee.com/vrshits](https://buymeacoffee.com/vrshits)  

[한국어](README.md) | [English](README.en.md) | [日本語](README.ja.md) | [简体中文](README.zh-CN.md)

An unofficial mod for playing GF2 Exilium with a VR headset and controllers. Menus appear on a virtual screen, while supported 3D scenes use stereoscopic VR.

Targets Haoplay and Steam. The current 3.0 compatibility build has passed static and automated checks; actual game/HMD operation has not yet been verified.

**[Download the latest release](https://github.com/deadpixel134/GF2Exilium-VR/releases/latest)**

> **Required before using VR: Turn OFF anti-aliasing in the game's settings.**

## What does it do?

Look around with your headset, move and turn with the sticks, click and drag with a pointer, and open the original game screen on a hand panel. Configure render quality, world scale, stereo depth and controls. Dirty also adds supported-character breast/buttock contact, haptics and motion settings.

## Contributors

- [rgbtl1248](https://github.com/rgbtl1248) — 3DMigoto compatibility and build tooling improvements ([PR #1](https://github.com/deadpixel134/GF2Exilium-VR/pull/1)).

## 3DMigoto compatibility

Both Clean and Dirty support the default 3DMigoto wrapper mode ([PR #1](https://github.com/deadpixel134/GF2Exilium-VR/pull/1)).

[3DMigoto](docs/en/3DMIGOTO_COMPATIBILITY.md)

## Before you start

- A working **Haoplay or Steam installation** of the game. The game is not included.
- A VR headset and controllers with a working **SteamVR/OpenXR connection**.
- In-game **anti-aliasing OFF**. It was identified as the cause of reported eye misalignment on Quest 3 and Galaxy XR.

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

Install one variant at a time. Both use the same single version.dll beside the game executable; shared support files, including the OpenXR loader and Configurator, are installed under vrmod. Original GameAssembly.dll, UnityPlayer.dll and game assets are not replaced. Both variants run inside the game process; the name Clean does not guarantee nondetection or safety.

## First installation

1. Download **one Clean or Dirty ZIP** from the [latest release](https://github.com/deadpixel134/GF2Exilium-VR/releases/latest) and extract the entire archive to a fresh folder. The `Source code` ZIP is not an installer package.
2. Close the game and any existing configurator/installer. Run the extracted `GF2VR.Installer.exe`, select `GF2_Exilium.exe` or the game folder, and install.
3. Open settings and save your options. Prepare SteamVR and your headset, then launch the game through your usual method.

[Installation, removal and restore](docs/en/INSTALLATION.md) · [Updates and variant switching](docs/en/UPDATE.md)

## Updates, repair and saving

Confirm an available update in Settings to install it. If automatic installation is unavailable, the version and manual instructions remain visible; the release page opens only with Open. Notifications can be skipped per version.

Missing or quarantined files with a valid installation record can be reinstalled. For changed files, choose **Repair** in the Installer from the latest ZIP, review the per-file plan and confirm. Originals are backed up first; files with unknown ownership are excluded by default. Installation records, settings, logs and backups are preserved. Unfinished final steps can be retried without reinstalling product files.

Settings displays saved, failed-save and unsaved states and a title *****. Failed saves retain your edits, and closing with unsaved changes asks for confirmation. Close the game before saving, then restart it to apply changes.

## Default controls

| Control | Action |
|---|---|
| Left stick | Move the VR viewpoint |
| Right stick | Turn the view. The default is snap turning in 30° steps. |
| Right-hand pointer | Point at items on the game screen. |
| Right Trigger or A | Click the pointed item |
| Right B | Back / cancel |
| Left Grip | Open or close the auxiliary panel in 3D scenes |

[Startup and full controls](docs/en/USAGE.md) · [Settings](docs/en/SETTINGS.md)

## Compatibility and verification

Hardware validation covers Dirty hands/contact in Haoplay Main and Activity, and Dirty launch/stereo in Steam Main. Other Steam contexts and the new Clean DLL have not received separate HMD validation.

Previously tested setup: Meta Quest 2 · 4K monitor · Virtual Desktop.

The reported binocular misalignment on Quest 3 and Galaxy XR was confirmed to be caused by anti-aliasing. Turn OFF anti-aliasing in the game's settings before using VR.

## Detailed guides

- [Install, remove and restore](docs/en/INSTALLATION.md)
- [Startup and full controls](docs/en/USAGE.md)
- [Settings](docs/en/SETTINGS.md)
- [Updating](docs/en/UPDATE.md)
- [Troubleshooting](docs/en/TROUBLESHOOTING.md)
- [Saving and sending logs](docs/en/LOGS.md)
- [Downloads and package files](docs/en/RELEASE_READINESS.md)
- [Verification scope](docs/en/VERIFICATION.md)
- [3DMigoto compatibility](docs/en/3DMIGOTO_COMPATIBILITY.md)
- [Developer build guide](docs/en/BUILD_VARIANT_METADATA.md)
- [Release history](docs/en/RELEASE_NOTES.md) · [GitHub Releases](https://github.com/deadpixel134/GF2Exilium-VR/releases)
- [Source boundary and terms](SOURCE_BOUNDARY.en.md)

> GF2VR is an unofficial fan project. A legally installed copy of the game is required. Game trademarks and assets belong to their respective owners.
