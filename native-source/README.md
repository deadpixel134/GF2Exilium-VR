# Native runtime source

[한국어](README.ko.md) | [English](README.md) | [日本語](README.ja.md) | [简体中文](README.zh-CN.md)

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
