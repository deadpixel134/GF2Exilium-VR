# GF2VR 1.1.3 — 下载

[한국어](../ko/RELEASE_READINESS.md) | [English](../en/RELEASE_READINESS.md) | [日本語](../ja/RELEASE_READINESS.md) | [简体中文](../zh-CN/RELEASE_READINESS.md)

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

## 1.1.4

两个版本均可设置辅助面板的朝向用户、位置、旋转、画面倾斜和最大尺寸。Dirty增加角色接触振动，以及胸部、臀部分别调整的晃动设置。

两个native DLL均与源码重编译逐字节一致。通过4,517项native布局/配置检查、Clean706项/Dirty941项GUI检查、236项分部位默认值及先前真实IL显式配置一致性检查、236项真实GUI默认JSON检查、159项安装/切换/失败恢复检查，以及11项先前安装精确恢复检查。验证bridge131个方法、接触hot IL46个方法。这些是离线、不可执行fixture和个人安装检查，不声称进行了新的游戏/HMD验收。


> **使用 VR 前必须在游戏设置中关闭抗锯齿。**

更新至 1.1.3：将完整的 Clean 或 Dirty ZIP 解压到新文件夹，使用其中的 GF2VR.Installer.exe 选择现有游戏位置安装。旧 1.1.2 设置程序无法验证新的 native 配置，因此会拒绝自动暂存并保留当前安装。请勿只复制 DLL 或混用包文件。

1.1.3 将 PR #1 的 3DMigoto 默认 wrapper 模式兼容性修改同时提供给 Clean 和 Dirty。保留游戏 wrapper，VR 复制与 OpenXR 使用实际 D3D11 设备/context。DLL 随附匹配的设置程序与安装程序元数据。


- [GF2VR-Native-SelfContained-Clean-1.1.3.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.3)
- [GF2VR-Native-SelfContained-Dirty-1.1.3.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.3)

| 功能 | Clean | Dirty |
|---|---|---|
| 常规 VR、立体画面、控制器输入和设置 | 包含 | 包含 |
| 双手位置的白色球体 | 包含 | 包含 |
| 角色胸部接触与变形 | 不包含 | 包含 |

两个版本的常规 VR 源码、版本和更新保持一致。只有 Dirty 包含接触变形代码；Clean 在构建时排除该实现，而不是仅将其关闭。

一次只安装一个版本。两者都在游戏可执行文件旁使用同一个 version.dll，OpenXR 加载器和设置程序等公共支持文件安装在 vrmod 下。不替换原有 GameAssembly.dll、UnityPlayer.dll 或游戏资源。两者均在游戏进程中运行，Clean 名称不代表不会被检测或保证安全。

安装、切换和移除均保留用户设置。可选 touch_interaction.json 仅由 Dirty 使用。移除时如有记录的上一安装则恢复，否则仅删除本工具拥有的文件。已修改或归属不明的 DLL 会被保留并拒绝覆盖。不要单独复制 version.dll，也不要混用不同安装包的文件。

新安装程序通过了 160 项版本切换与失败恢复、525 项所有权以及 175 项目录布局检查。公共投影修正通过 183,984 项离线断言及原有路径回归检查。由公开源码重新构建的两个原生 DLL 与发行文件逐字节一致。安装和设置程序包含所需 .NET 10.0.12 运行时。离线与安装检查不能替代真实游戏/HMD 验收。

已确认 Quest 3 和 Galaxy XR 所报告的左右画面错位由抗锯齿导致。使用 VR 前，请在游戏设置中关闭抗锯齿。

CHECKSUMS.sha256 · GF2VR-Clean-update.json · GF2VR-Dirty-update.json
