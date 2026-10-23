# 安装、切换与卸载

[한국어](../ko/INSTALLATION.md) | [English](../en/INSTALLATION.md) | [日本語](../ja/INSTALLATION.md) | [简体中文](../zh-CN/INSTALLATION.md)

[首页](../../README.zh-CN.md)

## 开始前的准备

- 可正常运行的 **Haoplay 或 Steam 版游戏**。不包含游戏本体。
- VR 头显、控制器及已连接的 **SteamVR/OpenXR 环境**。
- 游戏内**抗锯齿设为 OFF**。已确认其是 Quest 3、Galaxy XR 所报告双眼画面错位的原因。

## 选择安装包

在[最新发行版](https://github.com/deadpixel134/GF2Exilium-VR/releases/latest)的 **Assets** 中选择 Clean 或 Dirty 的完整 ZIP。常规 VR、输入和白色双手标记为两者共有；角色接触变形、接触振动仅 Dirty 包含。一次只安装一种。不要使用 GitHub 的 `Source code` ZIP。

## 安装步骤

1. 从[最新发行版](https://github.com/deadpixel134/GF2Exilium-VR/releases/latest)下载 **Clean 或 Dirty 的一个完整 ZIP**，全部解压到新文件夹。`Source code` ZIP 不是安装包。
2. 关闭游戏及已有的设置程序、安装器。运行解压后的 `GF2VR.Installer.exe`，选择 `GF2_Exilium.exe` 或游戏文件夹并安装。
3. 打开设置并保存选项。准备好 SteamVR 和头显后，按平常方式自行启动游戏。

## 安装的文件

`version.dll` 安装到游戏可执行文件旁，OpenXR 加载器、设置程序等公共文件放在 `vrmod` 下。不会替换原始 `GameAssembly.dll`、`UnityPlayer.dll` 或游戏资源。请保持完整安装包，不要单独移动程序、DLL 或混用不同安装包的文件。

## 切换 Clean 与 Dirty

将目标类型的最新完整 ZIP 解压到新文件夹，用其中的安装器选择相同游戏位置安装。不要叠加安装两种类型。兼容的用户设置会保留；Clean 不读取 Dirty 的可选 `touch_interaction.json`。

## 卸载与恢复上一次安装

关闭游戏，在安装器中选择相同游戏位置并查看卸载说明。有记录的上一次安装会被恢复，首次安装则只移除自有文件。用户设置和被修改或无法确认归属的文件会保留。不要先删除安装记录和备份。若有中断操作，请使用中断恢复。

[更新](UPDATE.md) · [故障排除](TROUBLESHOOTING.md)
