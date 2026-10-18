# 更新内容

[한국어](../ko/RELEASE_NOTES.md) | [English](../en/RELEASE_NOTES.md) | [日本語](../ja/RELEASE_NOTES.md) | [简体中文](../zh-CN/RELEASE_NOTES.md)

## 1.1.3

更新至 1.1.3：将完整的 Clean 或 Dirty ZIP 解压到新文件夹，使用其中的 GF2VR.Installer.exe 选择现有游戏位置安装。旧 1.1.2 设置程序无法验证新的 native 配置，因此会拒绝自动暂存并保留当前安装。请勿只复制 DLL 或混用包文件。

1.1.3 将 PR #1 的 3DMigoto 默认 wrapper 模式兼容性修改同时提供给 Clean 和 Dirty。保留游戏 wrapper，VR 复制与 OpenXR 使用实际 D3D11 设备/context。DLL 随附匹配的设置程序与安装程序元数据。

请使用默认 wrapper 模式，即省略或注释 [System] 中的 `hook`。`hook=recommended` 和 `hook=all` 不在支持范围内。`calls`、`crash` 用于日志/转储，并非此修复的必要选项。皮肤 INI 语法与重复规则警告属于其他问题。

> **使用 VR 前必须在游戏设置中关闭抗锯齿。**

PR 作者报告在 Dirty 开发构建中连续启动三次，确认 VR 与皮肤同时工作。这是贡献者的记录，不代表新 1.1.3 二进制已通过 HMD 验收。本次发布验证两种 native 构建、使用实际修改源码的 COM 同一性/失败路径、桌面构建及包完整性。没有重新运行游戏或 HMD。

贡献：[rgbtl1248](https://github.com/rgbtl1248)，[PR #1](https://github.com/deadpixel134/GF2Exilium-VR/pull/1)。

## 1.1.2

> **使用 VR 前必须在游戏设置中关闭抗锯齿。**

1.1.2 在安装程序初始界面和 README 中增加必须在游戏设置中关闭抗锯齿的说明。Clean 与 Dirty 显示相同提示，不自动修改游戏设置。VR DLL、设置程序及接触行为与 1.1.1 相同。

已确认 Quest 3 和 Galaxy XR 所报告的左右画面错位由抗锯齿导致。使用 VR 前，请在游戏设置中关闭抗锯齿。

## 1.1.1


1.1.1 分别提供 Clean 和 Dirty ZIP。白色手部球体为公共功能，接触变形仅属 Dirty。常规功能使用公共源码、版本及更新契约，支持同版本显式切换并保留设置。投影修正适用条件从仅水平轴扩展至具有充分垂直轴依据的情况。

新安装程序通过了 160 项版本切换与失败恢复、525 项所有权以及 175 项目录布局检查。公共投影修正通过 183,984 项离线断言及原有路径回归检查。由公开源码重新构建的两个原生 DLL 与发行文件逐字节一致。安装和设置程序包含所需 .NET 10.0.12 运行时。离线与安装检查不能替代真实游戏/HMD 验收。

已确认 Quest 3 和 Galaxy XR 所报告的左右画面错位由抗锯齿导致。使用 VR 前，请在游戏设置中关闭抗锯齿。

[1.0.2](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.0.2) · [1.0.1](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.0.1) · [1.0.0](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.0.0)
