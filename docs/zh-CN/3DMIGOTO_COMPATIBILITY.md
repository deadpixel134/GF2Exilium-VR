# 与 3DMigoto 一起使用

[한국어](../ko/3DMIGOTO_COMPATIBILITY.md) | [English](../en/3DMIGOTO_COMPATIBILITY.md) | [日本語](../ja/3DMIGOTO_COMPATIBILITY.md) | [简体中文](../zh-CN/3DMIGOTO_COMPATIBILITY.md)

[首页](../../README.zh-CN.md)

Clean、Dirty 均支持 3DMigoto 默认 wrapper 模式。保留游戏 wrapper，VR 使用实际 D3D11 device/context。仅使用 VR 时不需要额外安装 3DMigoto。

> **使用 VR 前必须在游戏设置中关闭抗锯齿。**

## 配置

请使用默认 wrapper 模式，即省略或注释 [System] 中的 `hook`。`hook=recommended` 和 `hook=all` 不在支持范围内。`calls`、`crash` 用于日志/转储，并非此修复的必要选项。皮肤 INI 语法与重复规则警告属于其他问题。

## 验证范围

基于 [rgbtl1248](https://github.com/rgbtl1248) 的 [PR #1](https://github.com/deadpixel134/GF2Exilium-VR/pull/1)。贡献者对 Dirty 开发版同时运行 VR、皮肤的报告与安装包技术检查属于不同证据，不保证所有皮肤与设置组合。当前游戏、头显确认范围见[验证说明](VERIFICATION.md)。

<details>
<summary>开发者实现说明</summary>

## 实现

- 通过 swapchain 的公开 GetDevice 获取实际设备，失败时使用后台缓冲区的 GetDevice。
- 通过 COM IUnknown 同一性比较 Unity wrapper 与实际设备，不替换游戏 wrapper。
- VR 使用持有引用的实际设备/immediate context；面板、目标纹理及手部光标采用相同的设备比较规则。
- 在眼部绘制前比较 context 的 COM 同一性，并更新持有的实际 context 的监视钩子。
- 如果实际设备/context 暴露已知的 3DMigoto wrapper 接口，则停止 VR 初始化。原因 11/12 对应此检查，游戏仍可正常运行。

只复制 DLL 会使桌面完整性元数据不匹配。请完整解压 ZIP 并使用其中的新安装程序更新。开发者请参阅[元数据构建步骤](BUILD_VARIANT_METADATA.md)。诊断 native_graphics_isolated=1 表示 wrapper/native 地址不同，0 表示相同，并非兼容成功的判定。

</details>
