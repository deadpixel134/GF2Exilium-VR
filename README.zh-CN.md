# GF2VR

作者: [@TBluebox12](https://x.com/TBluebox12)  
Arca Live VR 社区: [VR 社区](https://arca.live/b/vrshits)  
赞助: [buymeacoffee.com/vrshits](https://buymeacoffee.com/vrshits)  

[한국어](README.md) | [English](README.en.md) | [日本語](README.ja.md) | [简体中文](README.zh-CN.md)

用于以 VR 游玩 GF2 Exilium 的非官方模组。

**[下载正式版 1.2.0](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.2.0)**

> **使用 VR 前必须在游戏设置中关闭抗锯齿。**

1.1.6修复GUI更新检查和批准后安装流程。1.1.4及更早版本，或旧GUI无法检查更新的环境，请此次使用新Clean/Dirty ZIP安装程序更新一次。之后GUI更新无需GitHub CLI或登录。

## 1.2.0 — Steam支持与手部、接触兼容性修复

支持Steam可执行文件目录及现有Haoplay目录结构。修复游戏更新后白色手部标记和角色接触消失的兼容性问题。Dirty独立处理活动层中同时可见的受支持角色；Clean不包含接触变形。

实机验证范围为Dirty在Haoplay Main、Activity的手部显示/接触，以及Steam Main的启动和立体显示。尚未单独确认Steam全部场景及新Clean DLL的HMD表现。请关闭游戏内抗锯齿。

完整的受管理1.1.6安装可在设置GUI中批准更新通知，升级至1.2.0，无需GitHub CLI或登录，保留版本类型及用户设置。1.1.4及更早版本或无法检查更新的旧客户端，请将所需ZIP完整解压到新目录，运行附带的GF2VR.Installer.exe安装一次。请先关闭游戏。界面语言仍为韩语、英语和日语。

发行版本为1.2.0。内部native构建ID中的130用于标识此前未公开候选中经过验证的同一DLL。常规安装程序不会静默覆盖手动替换或不属于本产品的文件。

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

## 贡献者

- [rgbtl1248](https://github.com/rgbtl1248) — 改进 3DMigoto 兼容性与构建工具（[PR #1](https://github.com/deadpixel134/GF2Exilium-VR/pull/1)）。

## 3DMigoto 兼容性

1.1.3 将 PR #1 的 3DMigoto 默认 wrapper 模式兼容性修改同时提供给 Clean 和 Dirty。保留游戏 wrapper，VR 复制与 OpenXR 使用实际 D3D11 设备/context。DLL 随附匹配的设置程序与安装程序元数据。

请使用默认 wrapper 模式，即省略或注释 [System] 中的 `hook`。`hook=recommended` 和 `hook=all` 不在支持范围内。`calls`、`crash` 用于日志/转储，并非此修复的必要选项。皮肤 INI 语法与重复规则警告属于其他问题。

[3DMigoto](docs/zh-CN/3DMIGOTO_COMPATIBILITY.md) · [Build metadata](docs/zh-CN/BUILD_VARIANT_METADATA.md)

## 选择 Clean 或 Dirty

| 功能 | Clean | Dirty |
|---|---|---|
| 常规 VR、立体画面、控制器输入和设置 | 包含 | 包含 |
| 双手位置的白色球体 | 包含 | 包含 |
| 角色胸部、臀部接触与变形 | 不包含 | 包含 |
| 辅助面板手动位置、旋转与倾斜设置 | 包含 | 包含 |
| 接触振动、晃动及分部位设置 | 不包含 | 包含 |

接触功能在对应部位的受支持骨骼已绑定的角色上生效。支持部位因角色模型而异；尚未确认宿舍和活动层中的实际臀部反应。

两个版本的常规 VR 源码、版本和更新保持一致。只有 Dirty 包含接触变形代码；Clean 在构建时排除该实现，而不是仅将其关闭。

- [GF2VR-Native-SelfContained-Clean-1.2.0.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.2.0)
- [GF2VR-Native-SelfContained-Dirty-1.2.0.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.2.0)

一次只安装一个版本。两者都在游戏可执行文件旁使用同一个 version.dll，OpenXR 加载器和设置程序等公共支持文件安装在 vrmod 下。不替换原有 GameAssembly.dll、UnityPlayer.dll 或游戏资源。两者均在游戏进程中运行，Clean 名称不代表不会被检测或保证安全。

## 安装、切换和移除

1. 将所需 Clean 或 Dirty ZIP 完整解压到新文件夹。GitHub 的 Source code ZIP 不是安装包。
2. 关闭游戏、设置和安装程序。运行新文件夹中的 GF2VR.Installer.exe，选择 GF2_Exilium.exe 或游戏文件夹。程序会自动找到随附安装包。
3. 确认版本类型和版本号后安装。Clean↔Dirty 切换也使用所需 ZIP 中的新安装程序。
4. 打开设置并保存，然后按平时方式自行启动游戏。手部球体默认开启，可在手部显示页面修改。

安装、切换和移除均保留用户设置。可选 touch_interaction.json 仅由 Dirty 使用。移除时如有记录的上一安装则恢复，否则仅删除本工具拥有的文件。已修改或归属不明的 DLL 会被保留并拒绝覆盖。不要单独复制 version.dll，也不要混用不同安装包的文件。

## 验证范围与限制

此前实际测试环境：Meta Quest 2 · 4K 显示器 · Virtual Desktop。

已确认 Quest 3 和 Galaxy XR 所报告的左右画面错位由抗锯齿导致。使用 VR 前，请在游戏设置中关闭抗锯齿。

## 使用指南

- [安装、移除与恢复](docs/zh-CN/INSTALLATION.md)
- [操作](docs/zh-CN/USAGE.md)
- [设置](docs/zh-CN/SETTINGS.md)
- [更新](docs/zh-CN/UPDATE.md)
- [故障排除](docs/zh-CN/TROUBLESHOOTING.md)
- [更新日志](docs/zh-CN/RELEASE_NOTES.md)
- [下载与内容](docs/zh-CN/RELEASE_READINESS.md)
- [验证范围](docs/zh-CN/VERIFICATION.md)
- [源码范围与使用条款](SOURCE_BOUNDARY.zh-CN.md)

> GF2VR 是非官方粉丝项目，需要合法安装的游戏。游戏商标及相关作品的权利属于各自权利人。
