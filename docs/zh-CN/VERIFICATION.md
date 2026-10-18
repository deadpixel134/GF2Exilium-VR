# 审查与验证

[한국어](../ko/VERIFICATION.md) | [English](../en/VERIFICATION.md) | [日本語](../ja/VERIFICATION.md) | [简体中文](../zh-CN/VERIFICATION.md)

> **使用 VR 前必须在游戏设置中关闭抗锯齿。**

PR 作者报告在 Dirty 开发构建中连续启动三次，确认 VR 与皮肤同时工作。这是贡献者的记录，不代表新 1.1.3 二进制已通过 HMD 验收。本次发布验证两种 native 构建、使用实际修改源码的 COM 同一性/失败路径、桌面构建及包完整性。没有重新运行游戏或 HMD。

已确认 Quest 3 和 Galaxy XR 所报告的左右画面错位由抗锯齿导致。使用 VR 前，请在游戏设置中关闭抗锯齿。

增加 3DMigoto 默认 wrapper 模式兼容性修改及匹配的构建元数据。普通 VR 与手部显示保持共用，接触变形仅属于 Dirty。保留现有设置和安装归属/恢复保护。

[3DMigoto](3DMIGOTO_COMPATIBILITY.md) · [Build metadata](BUILD_VARIANT_METADATA.md)

贡献：[rgbtl1248](https://github.com/rgbtl1248)，[PR #1](https://github.com/deadpixel134/GF2Exilium-VR/pull/1)。

更新至 1.1.3：将完整的 Clean 或 Dirty ZIP 解压到新文件夹，使用其中的 GF2VR.Installer.exe 选择现有游戏位置安装。旧 1.1.2 设置程序无法验证新的 native 配置，因此会拒绝自动暂存并保留当前安装。请勿只复制 DLL 或混用包文件。

1.1.3 验证通过：COM 47项、Clean/Dirty 安装/切换/恢复160项、归属保护525项、目录结构175项、包位置12项、安装界面19项、设置程序各577项，以及1.1.2 → 1.1.3 → 精确恢复28项。这些是离线/模拟文件测试，不等同于游戏或HMD验收。
