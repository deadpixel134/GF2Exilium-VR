# 桌面程序源码

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


1.1.3 将 PR #1 的 3DMigoto 默认 wrapper 模式兼容性修改同时提供给 Clean 和 Dirty。保留游戏 wrapper，VR 复制与 OpenXR 使用实际 D3D11 设备/context。DLL 随附匹配的设置程序与安装程序元数据。


两个版本的常规 VR 源码、版本和更新保持一致。只有 Dirty 包含接触变形代码；Clean 在构建时排除该实现，而不是仅将其关闭。

安装、切换和移除均保留用户设置。可选 touch_interaction.json 仅由 Dirty 使用。移除时如有记录的上一安装则恢复，否则仅删除本工具拥有的文件。已修改或归属不明的 DLL 会被保留并拒绝覆盖。不要单独复制 version.dll，也不要混用不同安装包的文件。


设置程序与安装程序使用 Windows Forms、.NET SDK 10.0.112，以及官方 .NET 10.0.12 Windows x64 运行时包。需要 PowerShell 7 与 Windows x64。从本目录运行：

```powershell
./vrmod/tools/build_desktop_private_runtime.ps1 -Application Configurator -BuildId RELEASE-CLEAN-113 -ProductVersion 1.1.4
./vrmod/tools/build_desktop_private_runtime.ps1 -Application Installer -BuildId RELEASE-INSTALLER-113 -ProductVersion 1.1.4
```

首次构建会使用仓库中的源配置，从 NuGet 还原官方运行时包，并在项目本地的 `vrmod/build/` 下创建缓存、构建输出及所需的 .NET 许可证声明。不会修改游戏文件或系统共享的 .NET 安装。两个宿主程序均包含私有运行时；请将应用 DLL、deps 与 runtimeconfig 文件保留在 EXE 旁。

`runtime-contract.json` 是适用于当前原生模块与八份配置文档的安装模板。安装程序会验证软件包，并写入绑定用户所选安装文件夹的契约。该模板不是个人配置，也不是已安装的独立设置程序。完整原生软件包还需要发布版本中准确匹配的原生 DLL、OpenXR 加载器、许可证及经过验证的软件包清单。请勿用游戏文件代替，也不要在未核对完整软件包契约的情况下，将源码构建输出覆盖到正常运行的安装中。

应用源码包含本地验证入口（`--self-test`、`--verify-localization`）。安装程序的软件包、可迁移路径与更新测试夹具需要完整软件包，以及明确指定的上一版本软件包；它们使用不会执行的模拟游戏前置文件，不加载 VR DLL。发布记录区分这些检查与真实游戏/HMD 验收。

应用界面目前仅支持韩语、英语和日语，不支持中文。本文件的简体中文翻译不表示已经添加中文界面资源。

1.1.1 跨文件夹结构查找游戏可执行文件，原始游戏指纹差异不再阻止安装。标准的 `GF2 Game/GF2_Exilium.exe` 结构保留原上级文件夹中的 `vrmod`。其他结构使用找到的可执行文件所在文件夹中的 `vrmod`。`version.dll` 始终安装在该可执行文件旁。 继续执行产品软件包完整性及已有文件所有权检查。`--self-test-flexible` 使用不执行的模拟文件验证这些安装路径。

[Build metadata](../docs/zh-CN/BUILD_VARIANT_METADATA.md)
