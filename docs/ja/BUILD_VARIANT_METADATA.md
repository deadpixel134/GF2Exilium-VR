# 変更した native ソースとデスクトップツールのビルド

[한국어](../ko/BUILD_VARIANT_METADATA.md) | [English](../en/BUILD_VARIANT_METADATA.md) | [日本語](../ja/BUILD_VARIANT_METADATA.md) | [简体中文](../zh-CN/BUILD_VARIANT_METADATA.md)

Configurator と Installer は native DLL のハッシュを厳密に検査します。検査を無効化せず、両 native ビルドが生成した同じメタデータを使用してください。Windows x64、PowerShell 7、Zig 0.16.0、Windows SDK が必要です。以下はリポジトリのルートで実行します。

正式 1.1.3 ソースの既定ビルドは配布ハッシュの完全一致を要求します。意図的にソースを変更した開発時だけ -AllowSourceChanges を追加してください。新しい出力フォルダーで両方を一緒にビルドします。

```powershell
./native-source/variants/build-variants.ps1 -Flavor Both `
    -Zig <zig.exe> -Kernel32Library <kernel32.lib> -Output ./native-source/variants/build
```

desktop-metadata に実 DLL の build ID/ハッシュを持つ VariantProfiles.generated.cs、Clean/Dirty ごとの runtime-contract.json、common-source-manifest.txt、metadata.json が生成されます。単独ビルドでは生成しません。DLL 不足、報告ハッシュ不一致、materialized 共通ソースの差異は拒否します。

同じメタデータを Installer と両 Configurator に渡し、それぞれ固有の BuildId を使います。MSBuild では GF2VRVariantMetadataRoot と Configurator の GF2VRBuildFlavor を設定します。省略時はチェックインされた正式契約を使うため、変更済み DLL と混ぜないでください。

```powershell
$metadata = (Resolve-Path ./native-source/variants/build/desktop-metadata).Path
$desktop = './desktop-source/vrmod/tools/build_desktop_private_runtime.ps1'
& $desktop -Application Installer -BuildId SOURCE-INSTALLER-113 -ProductVersion 1.1.3 `
    -NativeMetadataRoot $metadata
& $desktop -Application Configurator -BuildId SOURCE-CLEAN-113 -ProductVersion 1.1.3 `
    -NativeMetadataRoot $metadata -NativeFlavor Clean
& $desktop -Application Configurator -BuildId SOURCE-DIRTY-113 -ProductVersion 1.1.3 `
    -NativeMetadataRoot $metadata -NativeFlavor Dirty
```

選択した DLL、一致する runtime contract、同じ profiles でビルドした Installer をまとめます。metadata.json の native buildId/dllSha256/buildFlavor/commonCoreSha256 を manifest に反映し、全 payload のサイズ/SHA-256 を再計算して通常のパッケージ検証を通します。この生成器は ZIP やリリースを作りません。Installer は INSTALL_ROOT/vrmod/config を選択先にバインドします。インストール前のテンプレートを Configurator が拒否するのは正常です。

## commonCoreSha256

GF2VR-COMMON-SOURCE-MANIFEST-V1 は common 配下の通常ファイルの相対パスを / に正規化し、ordinal 順で並べます。以下のヘッダーと行を BOM なし UTF-8 で記録します。\t は tab、\n は LF です。このバイト列の大文字 SHA-256 が commonCoreSha256 です。旧 AFBB… の再現ではなく、overlay・コンパイル出力・実行成功も証明しません。DLL は別の実ハッシュで検査します。

```text
GF2VR-COMMON-SOURCE-MANIFEST-V1\n
relative/path\tUPPERCASE_FILE_SHA256\n
```
