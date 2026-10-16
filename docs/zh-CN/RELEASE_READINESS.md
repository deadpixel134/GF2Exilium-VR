# 下载与支持

[한국어](../ko/RELEASE_READINESS.md) · [English](../en/RELEASE_READINESS.md) · [日本語](../ja/RELEASE_READINESS.md) · [简体中文](RELEASE_READINESS.md)

## 正式版 1.0.2

请从 [GitHub 发布页](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.0.2)下载 **GF2VR-Native-SelfContained-1.0.2.zip**。公开下载无须登录 GitHub，也不需要额外令牌。

- [直接下载安装 ZIP](https://github.com/deadpixel134/GF2Exilium-VR/releases/download/v1.0.2/GF2VR-Native-SelfContained-1.0.2.zip)
- [安装、卸载与恢复指南](INSTALLATION.md)
- [当前发布列表](https://github.com/deadpixel134/GF2Exilium-VR/releases)

GitHub 单独提供的 **Source code** ZIP 不是安装包。请将发布 ZIP 全部解压到一个新文件夹，然后运行 **GF2VR.Installer.exe**。

## 支持环境

需要 Windows x64 PC 上合法安装的游戏及 VR 环境。**2.0.3936** 是实机验证的基准版本。原始游戏的 SHA-256、文件大小或缓存差异不会阻止安装，但这并不保证其他游戏版本的 VR 兼容性。 头显、控制器软件及 SteamVR/OpenXR 需要自行准备。

标准的 `GF2 Game/GF2_Exilium.exe` 结构保留原上级文件夹中的 `vrmod`。其他结构使用找到的可执行文件所在文件夹中的 `vrmod`。`version.dll` 始终安装在该可执行文件旁。

设置程序和安装程序支持韩语、英语和日语，随包提供所需的 .NET 运行时；程序界面目前不支持中文。游戏和 SteamVR 需要自行启动。已验证的行为与尚未验证的环境见[验证范围](VERIFICATION.md)。

## 更新

可下载新发布 ZIP，通过安装程序更新已有安装。设置程序的可选更新功能使用已安装的 **GitHub CLI（gh）**及其登录状态。不使用该功能，也能从公开发布页手动安装和更新。详情见[更新方法](UPDATE.md)。

## 使用条件与反馈

不包含原始游戏。项目源码的分发许可证尚未确定，第三方组件分别适用其自身许可证。请阅读[源码构成与使用条件](../../SOURCE_BOUNDARY.zh-CN.md)。

遇到问题时，请按照[故障排查指南](TROUBLESHOOTING.md)整理错误信息和发生步骤。社区位于 [Arca Live 虚拟现实频道](https://arca.live/b/vrshits)。

安装 ZIP 大小：**93,673,090 字节**。SHA-256：

```text
9E2BDE8C9C3DB46F98BE16305899169EB7B11658C6BC1D7772A00AA55FB4ADA9
```
