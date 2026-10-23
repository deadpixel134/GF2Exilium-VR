# Downloads and package files

[한국어](../ko/RELEASE_READINESS.md) | [English](../en/RELEASE_READINESS.md) | [日本語](../ja/RELEASE_READINESS.md) | [简体中文](../zh-CN/RELEASE_READINESS.md)

[Home](../../README.en.md)

Download installation packages under **Assets** on the [latest regular release](https://github.com/deadpixel134/GF2Exilium-VR/releases/latest). Check that page for the actual version and filenames.

| File | Purpose |
|---|---|
| `GF2VR-Native-SelfContained-Clean-<version>.zip` | Full Clean installation package |
| `GF2VR-Native-SelfContained-Dirty-<version>.zip` | Full Dirty installation package |
| `CHECKSUMS.sha256` | SHA-256 checksums for the downloads |
| `GF2VR-Clean-update.json` / `GF2VR-Dirty-update.json` | Metadata used by the updater. Use the ZIP for manual installation. |
| GitHub `Source code` | Repository snapshot for development, not an installation package. |

Each full ZIP includes `GF2VR.Installer.exe`, the configurator and VR files for that variant. The required .NET runtime is bundled; players do not need to build the project.

Extract everything and use the included installer. Do not copy only the DLL or disable integrity checks. [Installation](INSTALLATION.md) · [Updating](UPDATE.md)
