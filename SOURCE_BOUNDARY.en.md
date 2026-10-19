# Source contents and terms of use

[한국어](SOURCE_BOUNDARY.md) · [English](SOURCE_BOUNDARY.en.md) · [日本語](SOURCE_BOUNDARY.ja.md) · [简体中文](SOURCE_BOUNDARY.zh-CN.md)

## 1.1.6 — Automatic update fix

The settings GUI checks public GitHub releases at startup. Approve **Update** in the notification to download, verify and install. Starting with 1.1.6, GitHub CLI and sign-in are no longer required.

Fixed rejection of new native builds by the old clients frozen allowlist. The client verifies release/file hashes, build flavor and current ownership; the verified incoming Installer owns final native validation, backups and installation. Clean/Dirty flavor and settings are preserved.

For 1.1.4 or earlier, extract the complete desired Clean/Dirty ZIP into a new folder and run its new GF2VR.Installer.exe once. Existing 1.1.5 installations with working update checks can use the GUI updater. Users whose old version lacks its required GitHub CLI also need this one-time ZIP installation. After installing 1.1.6, GUI updates need neither CLI nor sign-in.

This release retains the exact 1.1.5 VR DLLs. Logging remains OFF by default; explicit ON and its permanent initial-cleanup exemption, panel and contact settings are preserved. Close the game and settings window before installation.

## 1.1.4

Both builds provide auxiliary-panel viewer-facing, position, rotation, screen tilt and maximum-size settings. Dirty adds character-contact vibration and separate breast/buttock jiggle settings.

[Source manifest](source-manifest.json) · [Native metadata](release/1.1.4-native-metadata.json)


Version 1.1.1 publishes the Clean/Dirty product sources and separate packages. native-source/variants/common holds shared native code; Clean/Dirty overlays contain the generated contact-specific headers; managed contains owned hand-display/contact code. The rest of the native-source root preserves the previous 1.0.2 source and reproduction recipe.


To install, use **GF2VR-Native-SelfContained-Clean-1.1.3.zip / GF2VR-Native-SelfContained-Dirty-1.1.3.zip** from the [1.1.1 release](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.3). GitHub's **Source code** ZIP contains unbuilt source code. Extracting it and copying it into the game folder does not install the mod.

## Contents of this repository

| Location | Contents |
|---|---|
| `native-source/` | Native VR module source, our managed bridge code and generated headers, build configuration and scripts, required third-party headers and notices |
| `desktop-source/` | Configurator and installer source, with Korean, English and Japanese resources |
| `docs/en/` | Installation, controls, settings, update and troubleshooting guides |
| `source-manifest.json` | List of published source files and integrity information |

Original game executables, DLLs and assets, development reference projects, user settings, logs, recovery files, credentials and development tool caches are excluded from distribution. Our bridge code and generated headers built by referencing game libraries are managed separately from the original game libraries.

## Release contents

The installation ZIP contains the native module, OpenXR loader, configurator, installer and third-party software notices. The configurator and installer ship with the required .NET 10.0.12 runtime, so a separate .NET installation is not required.

Use the runtime, configurator and installation metadata together as one versioned package. Mixing files from different versions can cause file checks or startup to fail. Original game files are checked in the user's legitimate game installation and are not included in the installation ZIP.

The standard `GF2 Game/GF2_Exilium.exe` layout keeps `vrmod` in its existing parent folder. Other layouts use `vrmod` in the located executable's folder. `version.dll` is always installed beside that executable. Original game files are read-only and fingerprint differences do not block installation. Product-package hashes, file ownership, running-game guards, previous-version restoration and interrupted-operation recovery checks remain enforced. [Verification scope](docs/en/VERIFICATION.md) separates practical testing from local distribution checks.

## Terms of use

A distribution license for the project source has not yet been specified. The repository being public does not by itself grant permission to freely redistribute it or distribute modified versions.

OpenXR, MinHook and Microsoft .NET are subject to their respective licenses and notices. See the corresponding notices in the installation ZIP and source, along with the [Microsoft .NET notices](DOTNET-NOTICES.txt). These documents do not replace the terms of use for GF2VR as a whole. Game-related trademarks and copyrighted works belong to their respective owners.
