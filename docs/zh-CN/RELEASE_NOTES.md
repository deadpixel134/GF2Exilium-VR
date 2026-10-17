# 更新内容

[한국어](../ko/RELEASE_NOTES.md) | [English](../en/RELEASE_NOTES.md) | [日本語](../ja/RELEASE_NOTES.md) | [简体中文](../zh-CN/RELEASE_NOTES.md)

## 1.1.2

> **使用 VR 前必须在游戏设置中关闭抗锯齿。**

1.1.2 在安装程序初始界面和 README 中增加必须在游戏设置中关闭抗锯齿的说明。Clean 与 Dirty 显示相同提示，不自动修改游戏设置。VR DLL、设置程序及接触行为与 1.1.1 相同。

已确认 Quest 3 和 Galaxy XR 所报告的左右画面错位由抗锯齿导致。使用 VR 前，请在游戏设置中关闭抗锯齿。

## 1.1.1


1.1.1 分别提供 Clean 和 Dirty ZIP。白色手部球体为公共功能，接触变形仅属 Dirty。常规功能使用公共源码、版本及更新契约，支持同版本显式切换并保留设置。投影修正适用条件从仅水平轴扩展至具有充分垂直轴依据的情况。

新安装程序通过了 160 项版本切换与失败恢复、525 项所有权以及 175 项目录布局检查。公共投影修正通过 183,984 项离线断言及原有路径回归检查。由公开源码重新构建的两个原生 DLL 与发行文件逐字节一致。安装和设置程序包含所需 .NET 10.0.12 运行时。离线与安装检查不能替代真实游戏/HMD 验收。

已确认 Quest 3 和 Galaxy XR 所报告的左右画面错位由抗锯齿导致。使用 VR 前，请在游戏设置中关闭抗锯齿。

[1.0.2](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.0.2) · [1.0.1](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.0.1) · [1.0.0](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.0.0)
