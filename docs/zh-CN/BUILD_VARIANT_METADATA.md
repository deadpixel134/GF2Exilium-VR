# 为修改的 native 源码构建桌面工具

[한국어](../ko/BUILD_VARIANT_METADATA.md) | [English](../en/BUILD_VARIANT_METADATA.md) | [日本語](../ja/BUILD_VARIANT_METADATA.md) | [简体中文](../zh-CN/BUILD_VARIANT_METADATA.md)

## 1.1.4

[1.1.4 native metadata](../../release/1.1.4-native-metadata.json) · [Source manifest](../../source-manifest.json)


Configurator 和 Installer 严格校验 native DLL 哈希。请保留检查并使用两种 native 构建生成的同一份元数据。需要 Windows x64、PowerShell 7、Zig 0.16.0 和 Windows SDK。在仓库根目录执行以下命令。

正式 1.1.4 源码默认要求与发布哈希完全一致。仅在主动修改源码的开发构建中添加 -AllowSourceChanges。使用新的输出目录，同时构建两个版本。

```powershell
./native-source/variants/build-variants.ps1 -Flavor Both `
    -Zig <zig.exe> -Kernel32Library <kernel32.lib> -Output ./native-source/variants/build
```

desktop-metadata 包含实际 DLL build ID/哈希对应的 VariantProfiles.generated.cs、Clean/Dirty 各自的 runtime-contract.json、common-source-manifest.txt 和 metadata.json。只构建一种时不会生成。缺少 DLL、报告哈希不符、materialized 公共源码不一致都会被拒绝。

将同一份元数据传给 Installer 和两个 Configurator，每次使用独立 BuildId。直接使用 MSBuild 时设置 GF2VRVariantMetadataRoot 和 Configurator 的 GF2VRBuildFlavor。省略属性会使用已提交的正式发布契约，不要与修改后的 DLL 混用。

```powershell
$metadata = (Resolve-Path ./native-source/variants/build/desktop-metadata).Path
$desktop = './desktop-source/vrmod/tools/build_desktop_private_runtime.ps1'
& $desktop -Application Installer -BuildId SOURCE-INSTALLER-113 -ProductVersion 1.1.4 `
    -NativeMetadataRoot $metadata
& $desktop -Application Configurator -BuildId SOURCE-CLEAN-113 -ProductVersion 1.1.4 `
    -NativeMetadataRoot $metadata -NativeFlavor Clean
& $desktop -Application Configurator -BuildId SOURCE-DIRTY-113 -ProductVersion 1.1.4 `
    -NativeMetadataRoot $metadata -NativeFlavor Dirty
```

打包所选 DLL、匹配的 runtime contract 及使用相同 profiles 构建的 Installer。将 metadata.json 中的 native buildId/dllSha256/buildFlavor/commonCoreSha256 写入 manifest，重新计算全部 payload 大小/SHA-256，并通过正常的包校验。本生成器不创建 ZIP 或发布。Installer 将 INSTALL_ROOT/vrmod/config 绑定到选定配置目录；Configurator 拒绝未安装的模板是正常行为。

## commonCoreSha256

GF2VR-COMMON-SOURCE-MANIFEST-V1 将 common 下普通文件的相对路径规范为 /，并按 ordinal 顺序排列。以下头部与各行使用无 BOM 的 UTF-8；\t 表示 tab，\n 表示 LF。字节内容的大写 SHA-256 为 commonCoreSha256。这不是旧 AFBB… 标识的复现，不包含 overlay/编译输出，也不证明运行成功。DLL 仍由实际哈希单独校验。

```text
GF2VR-COMMON-SOURCE-MANIFEST-V1\n
relative/path\tUPPERCASE_FILE_SHA256\n
```
