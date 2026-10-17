# GF2VR

[@TBluebox12](https://x.com/TBluebox12) · [가상현실 채널](https://arca.live/b/vrshits) · [buymeacoffee.com/vrshits](https://buymeacoffee.com/vrshits)

[한국어](README.md) | [English](README.en.md) | [日本語](README.ja.md) | [简体中文](README.zh-CN.md)

An unofficial mod for playing GF2 Exilium in VR.

**[Download release 1.1.1](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.1)**

## Choose Clean or Dirty

| Feature | Clean | Dirty |
|---|---|---|
| Ordinary VR, stereo, controller input and settings | Included | Included |
| White spheres at both hand positions | Included | Included |
| Character breast contact and deformation | Excluded | Included |

Both variants share ordinary VR source, versions and updates. Only Dirty includes contact deformation code; Clean excludes that implementation rather than merely turning it off.

- [GF2VR-Native-SelfContained-Clean-1.1.1.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.1)
- [GF2VR-Native-SelfContained-Dirty-1.1.1.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.1)

Install one variant at a time. Both use the same single version.dll beside the game executable; shared support files, including the OpenXR loader and Configurator, are installed under vrmod. Original GameAssembly.dll, UnityPlayer.dll and game assets are not replaced. Both variants run inside the game process; the name Clean does not guarantee nondetection or safety.

## Install, switch and remove

1. Fully extract the desired Clean or Dirty ZIP into a fresh folder. GitHub Source code archives are not installers.
2. Close the game, Configurator and Installer. Run GF2VR.Installer.exe from the new folder and select GF2_Exilium.exe or the game folder. The bundled package is found automatically.
3. Confirm the variant and version, then install. To switch Clean↔Dirty, use the Installer from the desired ZIP.
4. Open Settings, save, and start the game through your normal route. Hand spheres default to enabled and can be changed in the Hands tab.

Installation, switching and removal preserve user settings. Optional touch_interaction.json is used only by Dirty. Remove restores a recorded previous installation when available; otherwise it removes only owned files. Modified or unowned DLLs are preserved and refused. Do not copy version.dll alone or mix files from different packages.

## Verification and limits

Previously tested setup: Meta Quest 2 · 4K monitor · Virtual Desktop.

Reported binocular misalignment on Galaxy XR with both VDXR and SteamVR remains unresolved. The shared projection eligibility improvement in 1.1.1 passed offline checks, but does not establish a fix for that report or support for every headset. Contact was observed in an earlier user runtime test; these exact distribution binaries have not had a new game/HMD run.

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
