# GF2VR 1.1.3 — 下载

[한국어](../ko/RELEASE_READINESS.md) | [English](../en/RELEASE_READINESS.md) | [日本語](../ja/RELEASE_READINESS.md) | [简体中文](../zh-CN/RELEASE_READINESS.md)

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
