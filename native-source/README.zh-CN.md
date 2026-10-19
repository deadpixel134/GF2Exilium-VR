# 原生运行时源码

[한국어](README.ko.md) · [English](README.md) · [日本語](README.ja.md) · [简体中文](README.zh-CN.md)

## 1.1.6 — 修复自动更新

设置GUI启动时检查公开GitHub发行版。批准通知中的 **更新** 后，将下载、校验并安装。从1.1.6开始，无需GitHub CLI或登录。

修复旧客户端使用固定白名单拒绝新构建的问题。客户端校验发行版与文件哈希、版本类型及当前文件归属，再由经过验证的新安装程序完成最终构建校验、备份和安装。保留Clean/Dirty类型与用户设置。

1.1.4及更早版本需要此次手动更新一次：将所选Clean/Dirty ZIP完整解压到新文件夹，并运行其中的新GF2VR.Installer.exe。更新检查正常的1.1.5环境可直接使用GUI更新。旧版所需GitHub CLI缺失的用户也需此次通过ZIP安装一次。安装1.1.6之后，GUI更新无需CLI或登录。

本版保留与1.1.5完全相同的VR DLL。保留日志默认关闭、明确开启设置及首次清理排除状态，以及面板和接触设置。安装前关闭游戏与设置窗口。界面语言仍为韩语、英语和日语。

## 1.1.5 — 日志记录设置

Clean和Dirty均在设置GUI中新增 **诊断 → 自动记录日志**。默认 **关闭**，旧设置中缺少此项时也保持关闭。保存后重启游戏生效。设置程序的界面语言仍为韩语、英语和日语。

首次更新安装时，仅为日志关闭的用户清理一次旧VR自动日志。**日志开启的用户保留已有日志，并排除在此次首次清理之外。** 之后改为关闭也不会延迟删除旧日志。保留设置、其他模组文件和手动收集的资料。

排查问题时再开启日志。开启后会继续追加到同一构建的日志文件，容量可能持续增加。

更新1.1.5时，请将所选Clean或Dirty ZIP完整解压到新文件夹，并使用随附的新GF2VR.Installer.exe安装。旧设置程序可能拒绝新的native配置。保留已有设置。

[Native metadata](../release/1.1.5-native-metadata.json)

## 1.1.4

两个版本均可设置辅助面板的朝向用户、位置、旋转、画面倾斜和最大尺寸。Dirty增加角色接触振动，以及胸部、臀部分别调整的晃动设置。

[Build metadata](../docs/zh-CN/BUILD_VARIANT_METADATA.md) · [Native metadata](../release/1.1.4-native-metadata.json)


## 1.1.3 — Clean / Dirty

当前 1.1.3 使用 variants 下的公共代码及各类型生成头文件。一起重新构建两个版本：

```powershell
./variants/build-variants.ps1 -Zig 'C:/Tools/zig/zig.exe' -Output './build-1.1.3'
```

1.1.3 公开 Clean/Dirty 产品源码及独立安装包。native-source/variants/common 为公共原生代码，Clean/Dirty overlay 为接触功能差异生成的头文件，managed 为自有手部显示及接触代码。native-source 根目录其余部分保留此前 1.0.2 的源码与重建脚本。

已确认 Quest 3 和 Galaxy XR 所报告的左右画面错位由抗锯齿导致。使用 VR 前，请在游戏设置中关闭抗锯齿。

1.1.3 将 PR #1 的 3DMigoto 默认 wrapper 模式兼容性修改同时提供给 Clean 和 Dirty。保留游戏 wrapper，VR 复制与 OpenXR 使用实际 D3D11 设备/context。DLL 随附匹配的设置程序与安装程序元数据。

[3DMigoto](../docs/zh-CN/3DMIGOTO_COMPATIBILITY.md) · [Build metadata](../docs/zh-CN/BUILD_VARIANT_METADATA.md)

---


这里是 GF2VR 1.0.2 中原生运行时 `BOOTSTRAP-FLEX-001` 的完整构建源码集。其 SHA-256 为：

```text
6215CEAFFFA46A3B89336DA73203122DD0095977206FDB395A1259F967B54771
```

在 Windows x64 上使用 PowerShell 7、Zig 0.16.0，以及 Windows 10 SDK 10.0.26100.0 的 x64 `kernel32.lib` 构建：

```powershell
./build-native.ps1 -Zig 'C:/Tools/zig/zig.exe' -Kernel32Library 'C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64/kernel32.lib' -Output './build'
```

构建脚本使用输出目录存放 Zig 缓存，并在完成后恢复调用者的缓存设置。它编译 11 个运行时支持单元及 4 个 MinHook 单元，链接运行时；如果生成的 DLL 哈希与发布版本不同，便会拒绝该文件。`build-result.json` 记录目标文件哈希、编译器哈希、最终参数与 DLL 比较结果。此构建不会安装或加载 DLL，也不会启动游戏或 XR 运行时。

`native-build.json` 记录准确的构建参数与源码哈希。`src/` 包含当前自有原生代码及此构建使用的已生成软件包头文件。`managed/ActivityProducerEngineBridge.cs` 是与嵌入式托管软件包对应的自有托管桥接源码，提供给开发者审阅和开发。重新构建此原生 DLL 会使用仓库中已生成的软件包，不需要游戏程序集。重新生成托管软件包是独立的开发操作，需要合法取得且版本匹配的游戏程序集；这些程序集及其资源不在此处提供。

OpenXR 与 MinHook 的源码子集在 `third_party/` 中保留其许可证。游戏可执行文件、Unity/GameAssembly 二进制文件、游戏元数据、游戏资源、用户配置、日志和回滚文件不属于此次源码分发的内容。

1.0.2 的路径修改用于查找可执行文件旁的加载器及对应的 `vrmod` 目录。标准的 `GF2 Game/GF2_Exilium.exe` 结构保留原上级文件夹中的 `vrmod`。其他结构使用找到的可执行文件所在文件夹中的 `vrmod`。`version.dll` 始终安装在该可执行文件旁。
