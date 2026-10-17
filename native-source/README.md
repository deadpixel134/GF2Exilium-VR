# Native runtime source

[한국어](README.ko.md) | [English](README.md) | [日本語](README.ja.md) | [简体中文](README.zh-CN.md)

## 1.1.1 — Clean / Dirty

Current 1.1.1 native source uses shared code and per-flavor generated headers under variants. Rebuild both flavors with:

```powershell
./variants/build-variants.ps1 -Zig 'C:/Tools/zig/zig.exe' -Output './build-1.1.1'
```

Version 1.1.1 publishes the Clean/Dirty product sources and separate packages. native-source/variants/common holds shared native code; Clean/Dirty overlays contain the generated contact-specific headers; managed contains owned hand-display/contact code. The rest of the native-source root preserves the previous 1.0.2 source and reproduction recipe.

Reported binocular misalignment on Galaxy XR with both VDXR and SteamVR remains unresolved. The shared projection eligibility improvement in 1.1.1 passed offline checks, but does not establish a fix for that report or support for every headset. Contact was observed in an earlier user runtime test; these exact distribution binaries have not had a new game/HMD run.

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
