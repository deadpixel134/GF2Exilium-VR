# 安装、切换和移除

[한국어](../ko/INSTALLATION.md) | [English](../en/INSTALLATION.md) | [日本語](../ja/INSTALLATION.md) | [简体中文](../zh-CN/INSTALLATION.md)

- [GF2VR-Native-SelfContained-Clean-1.1.1.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.1)
- [GF2VR-Native-SelfContained-Dirty-1.1.1.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.1)

| 功能 | Clean | Dirty |
|---|---|---|
| 常规 VR、立体画面、控制器输入和设置 | 包含 | 包含 |
| 双手位置的白色球体 | 包含 | 包含 |
| 角色胸部接触与变形 | 不包含 | 包含 |

1. 将所需 Clean 或 Dirty ZIP 完整解压到新文件夹。GitHub 的 Source code ZIP 不是安装包。
2. 关闭游戏、设置和安装程序。运行新文件夹中的 GF2VR.Installer.exe，选择 GF2_Exilium.exe 或游戏文件夹。程序会自动找到随附安装包。
3. 确认版本类型和版本号后安装。Clean↔Dirty 切换也使用所需 ZIP 中的新安装程序。
4. 打开设置并保存，然后按平时方式自行启动游戏。手部球体默认开启，可在手部显示页面修改。

一次只安装一个版本。两者都在游戏可执行文件旁使用同一个 version.dll，OpenXR 加载器和设置程序等公共支持文件安装在 vrmod 下。不替换原有 GameAssembly.dll、UnityPlayer.dll 或游戏资源。两者均在游戏进程中运行，Clean 名称不代表不会被检测或保证安全。

安装、切换和移除均保留用户设置。可选 touch_interaction.json 仅由 Dirty 使用。移除时如有记录的上一安装则恢复，否则仅删除本工具拥有的文件。已修改或归属不明的 DLL 会被保留并拒绝覆盖。不要单独复制 version.dll，也不要混用不同安装包的文件。

Galaxy XR 在 VDXR 和 SteamVR 下均有左右画面错位报告，目前尚未确认解决。1.1.1 的公共投影修正适用条件改进已通过离线检查，但不代表修复了该报告，也不保证所有头显兼容。接触功能曾在此前用户实机测试中确认；此次发行二进制本身未重新进行游戏/HMD 实测。
