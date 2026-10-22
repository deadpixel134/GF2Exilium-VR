# Native runtime source

[한국어](README.ko.md) | [English](README.md) | [日本語](README.ja.md) | [简体中文](README.zh-CN.md)

## Source tree guide

`variants/common/` together with `variants/<Flavor>/overlay/` is the source of the current1.2.0 distributed DLLs, including Steam/Haoplay build profiles and runtime-derived hook targets. Use the reproduction commands in the1.2.0 section below.

`src/` with the root `build-native.ps1` and `native-build.json` is retained only for historical1.0.x `BOOTSTRAP-FLEX-001` reproduction (`6215CEAF…`). Its old Haoplay-only constants, for example `src/stereo/stereo_hooks.h:48`, are not used by the current build; runtime-derived targets in `variants/common/src/stereo/stereo_hooks.h` replace that route. Historical source bytes are intentionally unchanged.

The current source is now plain frozen Steam-capable code. Both DLLs remain byte-identical to1.2.0. The [collapsed source manifest](../release/common-source-manifest-1.2.0-collapsed.txt) records the new source identity; the original1.2.0 manifest and installed desktop contracts remain unchanged.

## 1.2.0 — Steam support and hand/contact compatibility fixes

Supports the Steam executable folder and existing Haoplay layout. Fixes compatibility after the game update that removed white hand indicators and character contact. Dirty handles each simultaneously visible supported Activity character independently. Clean excludes contact deformation.

Hardware validation covers Dirty hands/contact in Haoplay Main and Activity, and Steam Main launch/stereo. All Steam contexts and the new Clean DLL have not received separate HMD acceptance. Turn OFF in-game anti-aliasing.

An intact managed 1.1.6 installation can update to 1.2.0 by approving the Settings GUI notification, without GitHub CLI or sign-in. Flavor and user settings are preserved. For 1.1.4 or earlier, or old clients unable to check updates, extract the entire desired ZIP into a fresh folder and run its GF2VR.Installer.exe once. Close the game first.

The release version is 1.2.0. Internal native build IDs retaining 130 identify the exact validated DLLs from an earlier unpublished candidate. The normal Installer does not silently overwrite manually replaced or unowned files.

The current1.2.0 native recipe uses238 shared inputs under variants and per-flavor config/generated bridge headers. With Zig0.16.0 and Windows SDK10.0.26100.0 the command below reproduces the exact distributed DLLs and generates paired desktop-metadata. The root build-native.ps1 preserves historical1.0.2 reproduction. Regenerating the managed bridge requires metadata from your installed game and checks exact Haoplay/Steam profiles in game-reference-profiles.json. Game files are not included.

```powershell
./variants/build-variants.ps1 -Zig 'C:/Tools/zig/zig.exe' -Output './build-1.2.0'
./variants/managed/build_touch_bridge.ps1 -NativeEyeCapacity 3 -Flavor Dirty -GameRoot '<game executable folder>' -GameProfile Auto -OutputDirectory './managed-1.2.0'
```

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

[Native metadata](../release/1.1.5-native-metadata.json)

## 1.1.4

Both builds provide auxiliary-panel viewer-facing, position, rotation, screen tilt and maximum-size settings. Dirty adds character-contact vibration and separate breast/buttock jiggle settings.

[Build metadata](../docs/en/BUILD_VARIANT_METADATA.md) · [Native metadata](../release/1.1.4-native-metadata.json)


## 1.1.3 — Clean / Dirty

Current 1.1.3 native source uses shared code and per-flavor generated headers under variants. Rebuild both flavors with:

```powershell
./variants/build-variants.ps1 -Zig 'C:/Tools/zig/zig.exe' -Output './build-1.1.3'
```

Version 1.1.3 publishes the Clean/Dirty product sources and separate packages. native-source/variants/common holds shared native code; Clean/Dirty overlays contain the generated contact-specific headers; managed contains owned hand-display/contact code. The rest of the native-source root preserves the previous 1.0.2 source and reproduction recipe.

The reported binocular misalignment on Quest 3 and Galaxy XR was confirmed to be caused by anti-aliasing. Turn OFF anti-aliasing in the game's settings before using VR.

1.1.3 delivers the default-wrapper 3DMigoto interoperability changes from PR #1 in both Clean and Dirty. Game wrappers remain intact; VR copies and OpenXR use the native D3D11 device/context. Matching Configurator and Installer metadata ships with the DLLs.

[3DMigoto](../docs/en/3DMIGOTO_COMPATIBILITY.md) · [Build metadata](../docs/en/BUILD_VARIANT_METADATA.md)

---


This is the source closure for `BOOTSTRAP-FLEX-001`, the native runtime included in GF2VR 1.0.2. Its SHA-256 is:

```text
6215CEAFFFA46A3B89336DA73203122DD0095977206FDB395A1259F967B54771
```

Build on Windows x64 with PowerShell 7, Zig 0.16.0, and the Windows 10 SDK 10.0.26100.0 x64 `kernel32.lib`:

```powershell
./build-native.ps1 -Zig 'C:/Tools/zig/zig.exe' -Kernel32Library 'C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64/kernel32.lib' -Output './build'
```

The builder uses its output directory for Zig caches and restores the caller's cache settings afterward. It compiles the 11 runtime support units and four MinHook units, links the runtime, and rejects a DLL whose hash differs from the release. `build-result.json` records the object hashes, compiler hash, final arguments and DLL comparison. This build does not install or load the DLL and does not start the game or an XR runtime.

`native-build.json` records the exact build flags and source hashes. `src/` contains the current owned native code and the generated package headers consumed by this build. `managed/ActivityProducerEngineBridge.cs` is the owned managed bridge source corresponding to the embedded managed package; it is provided for inspection and development. Rebuilding this native DLL consumes the checked-in generated package and does not require game assemblies. Regenerating the managed package is a separate development operation requiring the matching legally obtained game assemblies; those assemblies and their assets are not distributed here.

The OpenXR and MinHook source subsets retain their licenses under `third_party/`. The game executable, Unity/GameAssembly binaries, game metadata, game assets, user configuration, logs and rollback files are not part of this source distribution.

The 1.0.2 path changes locate the loader beside the executable and the matching `vrmod` directory. The standard `GF2 Game/GF2_Exilium.exe` layout keeps `vrmod` in its existing parent folder. Other layouts use `vrmod` in the located executable's folder. `version.dll` is always installed beside that executable.
