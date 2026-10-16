# Source contents and terms of use

[한국어](SOURCE_BOUNDARY.md) · [English](SOURCE_BOUNDARY.en.md) · [日本語](SOURCE_BOUNDARY.ja.md) · [简体中文](SOURCE_BOUNDARY.zh-CN.md)

To install, use **GF2VR-Native-SelfContained-1.0.1.zip** from the [1.0.1 release](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.0.1). GitHub's **Source code** ZIP contains unbuilt source code. Extracting it and copying it into the game folder does not install the mod.

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
