# 与 3DMigoto 一起使用

[한국어](../ko/3DMIGOTO_COMPATIBILITY.md) | [English](../en/3DMIGOTO_COMPATIBILITY.md) | [日本語](../ja/3DMIGOTO_COMPATIBILITY.md) | [简体中文](../zh-CN/3DMIGOTO_COMPATIBILITY.md)

1.1.3 将 PR #1 的 3DMigoto 默认 wrapper 模式兼容性修改同时提供给 Clean 和 Dirty。保留游戏 wrapper，VR 复制与 OpenXR 使用实际 D3D11 设备/context。DLL 随附匹配的设置程序与安装程序元数据。

> **使用 VR 前必须在游戏设置中关闭抗锯齿。**

## 1.2.0 — Steam支持与手部、接触兼容性修复

支持Steam可执行文件目录及现有Haoplay目录结构。修复游戏更新后白色手部标记和角色接触消失的兼容性问题。Dirty独立处理活动层中同时可见的受支持角色；Clean不包含接触变形。

实机验证范围为Dirty在Haoplay Main、Activity的手部显示/接触，以及Steam Main的启动和立体显示。尚未单独确认Steam全部场景及新Clean DLL的HMD表现。请关闭游戏内抗锯齿。

完整的受管理1.1.6安装可在设置GUI中批准更新通知，升级至1.2.0，无需GitHub CLI或登录，保留版本类型及用户设置。1.1.4及更早版本或无法检查更新的旧客户端，请将所需ZIP完整解压到新目录，运行附带的GF2VR.Installer.exe安装一次。请先关闭游戏。界面语言仍为韩语、英语和日语。

发行版本为1.2.0。内部native构建ID中的130用于标识此前未公开候选中经过验证的同一DLL。常规安装程序不会静默覆盖手动替换或不属于本产品的文件。

## 配置

请使用默认 wrapper 模式，即省略或注释 [System] 中的 `hook`。`hook=recommended` 和 `hook=all` 不在支持范围内。`calls`、`crash` 用于日志/转储，并非此修复的必要选项。皮肤 INI 语法与重复规则警告属于其他问题。

## 实现

- 通过 swapchain 的公开 GetDevice 获取实际设备，失败时使用后台缓冲区的 GetDevice。
- 通过 COM IUnknown 同一性比较 Unity wrapper 与实际设备，不替换游戏 wrapper。
- VR 使用持有引用的实际设备/immediate context；面板、目标纹理及手部光标采用相同的设备比较规则。
- 在眼部绘制前比较 context 的 COM 同一性，并更新持有的实际 context 的监视钩子。
- 如果实际设备/context 暴露已知的 3DMigoto wrapper 接口，则停止 VR 初始化。原因 11/12 对应此检查，游戏仍可正常运行。

只复制 DLL 会使桌面完整性元数据不匹配。请完整解压 ZIP 并使用其中的新安装程序更新。开发者请参阅[元数据构建步骤](BUILD_VARIANT_METADATA.md)。诊断 native_graphics_isolated=1 表示 wrapper/native 地址不同，0 表示相同，并非兼容成功的判定。

## 验证范围

PR 作者报告在 Dirty 开发构建中连续启动三次，确认 VR 与皮肤同时工作。这是贡献者的记录，不代表新 1.1.3 二进制已通过 HMD 验收。本次发布验证两种 native 构建、使用实际修改源码的 COM 同一性/失败路径、桌面构建及包完整性。没有重新运行游戏或 HMD。

已确认 Quest 3 和 Galaxy XR 所报告的左右画面错位由抗锯齿导致。使用 VR 前，请在游戏设置中关闭抗锯齿。

贡献：[rgbtl1248](https://github.com/rgbtl1248)，[PR #1](https://github.com/deadpixel134/GF2Exilium-VR/pull/1)。
