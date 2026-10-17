# Building desktop tools for changed native sources

The Configurator validates the installed `version.dll` against its runtime
contract. The Installer also validates the native DLL against compiled variant
profiles. Replacing a native DLL alone leaves those checks pointing at the old
build and disables settings. Keep these checks enabled and build matching
metadata instead.

## Generate a paired native build

Use PowerShell 7 from the repository root. Supply the compiler and Windows SDK
paths appropriate to the build machine:

```powershell
./native-source/variants/build-variants.ps1 -Flavor Both -AllowSourceChanges `
    -Zig <zig.exe> -Kernel32Library <kernel32.lib> -Output ./native-source/variants/build
```

`-AllowSourceChanges` is an explicit source-development option. Without it,
native builds still require exact reproduction of the checked-in release hash.
The build report always records the actual hash and whether it matches that
frozen release. Compilation failures still stop either mode.

After both native builds succeed, `desktop-metadata/` contains:

- `VariantProfiles.generated.cs`, with the hashes of the actual Clean and Dirty
  DLLs and their build IDs;
- `Clean/runtime-contract.json` and `Dirty/runtime-contract.json`, each with the
  matching DLL hash and the portable `INSTALL_ROOT/vrmod/config` placeholder;
- `common-source-manifest.txt` and `metadata.json`, documenting source identity.

A single-flavor invocation does not generate paired metadata. The metadata
generator rejects missing DLLs, build-report hash mismatches, and common source
files that differ between the current common tree and either materialized build.
Use a fresh output directory when rebuilding. The generator can also be invoked
directly with `-BuildOutput` after a completed paired build.

## Consume the generated files

Pass the metadata directory to **both** desktop builds. Build one Configurator
per selected native flavor, with distinct `BuildId` values and output folders:

```powershell
$metadata = (Resolve-Path ./native-source/variants/build/desktop-metadata).Path
$desktop = './desktop-source/vrmod/tools/build_desktop_private_runtime.ps1'
& $desktop -Application Installer -BuildId SOURCE-INSTALLER-001 `
    -NativeMetadataRoot $metadata
& $desktop -Application Configurator -BuildId SOURCE-CLEAN-001 `
    -NativeMetadataRoot $metadata -NativeFlavor Clean
& $desktop -Application Configurator -BuildId SOURCE-DIRTY-001 `
    -NativeMetadataRoot $metadata -NativeFlavor Dirty
```

Direct MSBuild users can set `GF2VRVariantMetadataRoot` and, for the Configurator,
`GF2VRBuildFlavor=Clean` or `Dirty`. Omitting the metadata property preserves
the existing checked-in desktop build inputs. Do not mix that legacy output
with a changed native DLL.

Package the selected native DLL, its matching Configurator contract, and the
Installer compiled with the same generated profiles. Set the package manifest's
`native.buildId`, `native.dllSha256`, `native.buildFlavor`, and
`native.commonCoreSha256` from `metadata.json`; regenerate payload byte counts
and SHA-256 values after assembling the package. These steps must use the normal
package validator. This script does not create a release archive or edit frozen
release manifests. The existing Installer binds the placeholder to the user's
selected configuration directory and records the bound contract in its receipt.
The uninstalled portable contract is intentionally rejected by the Configurator
until that binding has occurred.

## Source fingerprint definition

New metadata uses `GF2VR-COMMON-SOURCE-MANIFEST-V1`. This is a **new source
fingerprint rule**, not a reproduction of the legacy `AFBB...` release identity.
Enumerate every regular file below `native-source/variants/common`, sorted by
relative path using ordinal comparison. Paths use `/`. The manifest bytes are
UTF-8 without a BOM, with exactly this header and one line per file:

```text
GF2VR-COMMON-SOURCE-MANIFEST-V1\n
relative/path\tUPPERCASE_FILE_SHA256\n
```

Here `\t` and `\n` denote a tab and LF byte. `commonCoreSha256` is the uppercase
SHA-256 of those manifest bytes. It identifies the common source snapshot; it
does not attest to runtime behavior, and it excludes flavor overlays and compiler
outputs. DLL identity remains independently pinned by the actual output hashes.

No game assets, external graphics injector files, skins, machine-specific
configuration paths, or installed user files are inputs to this generation.
