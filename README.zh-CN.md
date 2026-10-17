# GF2VR

[@TBluebox12](https://x.com/TBluebox12) · [가상현실 채널](https://arca.live/b/vrshits) · [buymeacoffee.com/vrshits](https://buymeacoffee.com/vrshits)

[한국어](README.md) | [English](README.en.md) | [日本語](README.ja.md) | [简体中文](README.zh-CN.md)

用于以 VR 游玩 GF2 Exilium 的非官方模组。

**[下载正式版 1.1.1](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.1)**

## 选择 Clean 或 Dirty

| 功能 | Clean | Dirty |
|---|---|---|
| 常规 VR、立体画面、控制器输入和设置 | 包含 | 包含 |
| 双手位置的白色球体 | 包含 | 包含 |
| 角色胸部接触与变形 | 不包含 | 包含 |

两个版本的常规 VR 源码、版本和更新保持一致。只有 Dirty 包含接触变形代码；Clean 在构建时排除该实现，而不是仅将其关闭。

- [GF2VR-Native-SelfContained-Clean-1.1.1.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.1)
- [GF2VR-Native-SelfContained-Dirty-1.1.1.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.1)

一次只安装一个版本。两者都在游戏可执行文件旁使用同一个 version.dll，OpenXR 加载器和设置程序等公共支持文件安装在 vrmod 下。不替换原有 GameAssembly.dll、UnityPlayer.dll 或游戏资源。两者均在游戏进程中运行，Clean 名称不代表不会被检测或保证安全。

## 安装、切换和移除

1. 将所需 Clean 或 Dirty ZIP 完整解压到新文件夹。GitHub 的 Source code ZIP 不是安装包。
2. 关闭游戏、设置和安装程序。运行新文件夹中的 GF2VR.Installer.exe，选择 GF2_Exilium.exe 或游戏文件夹。程序会自动找到随附安装包。
3. 确认版本类型和版本号后安装。Clean↔Dirty 切换也使用所需 ZIP 中的新安装程序。
4. 打开设置并保存，然后按平时方式自行启动游戏。手部球体默认开启，可在手部显示页面修改。

安装、切换和移除均保留用户设置。可选 touch_interaction.json 仅由 Dirty 使用。移除时如有记录的上一安装则恢复，否则仅删除本工具拥有的文件。已修改或归属不明的 DLL 会被保留并拒绝覆盖。不要单独复制 version.dll，也不要混用不同安装包的文件。

## 验证范围与限制

此前实际测试环境：Meta Quest 2 · 4K 显示器 · Virtual Desktop。

Galaxy XR 在 VDXR 和 SteamVR 下均有左右画面错位报告，目前尚未确认解决。1.1.1 的公共投影修正适用条件改进已通过离线检查，但不代表修复了该报告，也不保证所有头显兼容。接触功能曾在此前用户实机测试中确认；此次发行二进制本身未重新进行游戏/HMD 实测。

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
