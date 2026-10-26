# GF2VR

作者: [@TBluebox12](https://x.com/TBluebox12)  
Arca Live VR 社区: [VR 社区](https://arca.live/b/vrshits)  
赞助: [buymeacoffee.com/vrshits](https://buymeacoffee.com/vrshits)  

[한국어](README.md) | [English](README.en.md) | [日本語](README.ja.md) | [简体中文](README.zh-CN.md)

通过 VR 头显和控制器游玩 GF2 Exilium 的非官方模组。普通菜单显示在虚拟屏幕上，支持的 3D 场景以立体 VR 显示。

**兼容Haoplay版本和Steam版本。** 两个平台使用相同的Clean/Dirty安装包。

**[下载最新版本](https://github.com/deadpixel134/GF2Exilium-VR/releases/latest)**

> **使用 VR 前必须在游戏设置中关闭抗锯齿。**

## 可以做什么？

支持随头部动作环顾、摇杆移动与转向、指针点击和拖动，以及在手边打开原始游戏画面的辅助面板。可调整画质、世界大小、立体感和操作。Dirty 还提供对支持角色胸部、臀部的接触，以及振动和晃动设置。

## PC 输出分辨率

设置程序可选择原生／3200×1800／2560×1440／1920×1080／1280×720，默认保持原生分辨率。推荐使用1280×720来降低GPU负担，代价是PC窗口和辅助面板较模糊。眼睛画质和固定渲染倍率单独保持。选择仅在VR中或始终应用，保存后重启游戏。

更新和移除会保留现有的performance.json。动态眼睛倍率和PC 3D renderScale在实测中未提升处理量，因此不支持。

## VR 姿态刷新

自 1.3.1 起，默认启用在眼睛渲染前重新读取头部姿态的 late re-locate；重新获取失败时使用原有姿态。如需关闭，请在 Settings 的画质·立体感标签页取消勾选 **眼睛渲染前重新读取头部姿态**，保存后重新启动游戏（即 `performance.json` 中的 `pose.lateRelocate`）。

## 贡献者

- [rgbtl1248](https://github.com/rgbtl1248) — 改进 3DMigoto 兼容性与构建工具（[PR #1](https://github.com/deadpixel134/GF2Exilium-VR/pull/1)）。

## 3DMigoto 兼容性

Clean与Dirty均兼容3DMigoto默认wrapper模式（[PR #1](https://github.com/deadpixel134/GF2Exilium-VR/pull/1)）。保留游戏wrapper，VR复制与OpenXR使用实际D3D11设备/context。DLL随附匹配的设置程序与安装程序元数据。

[3DMigoto](docs/zh-CN/3DMIGOTO_COMPATIBILITY.md)

## 开始前的准备

- 可正常运行的 **Haoplay 或 Steam 版游戏**。不包含游戏本体。
- VR 头显、控制器及已连接的 **SteamVR/OpenXR 环境**。
- 游戏内**抗锯齿设为 OFF**。已确认其是 Quest 3、Galaxy XR 所报告双眼画面错位的原因。

## 选择 Clean 或 Dirty

| 功能 | Clean | Dirty |
|---|---|---|
| 常规 VR、立体画面、控制器输入和设置 | 包含 | 包含 |
| 双手位置的白色球体 | 包含 | 包含 |
| 角色胸部、臀部接触与变形 | 不包含 | 包含 |
| 辅助面板手动位置、旋转与倾斜设置 | 包含 | 包含 |
| 接触振动、晃动及分部位设置 | 不包含 | 包含 |

接触功能在对应部位的受支持骨骼已绑定的角色上生效。支持部位因角色模型而异；尚未确认宿舍和活动层中的实际臀部反应。

两个版本的常规 VR 源码、版本和更新保持一致。只有 Dirty 包含接触变形代码；Clean 在构建时排除该实现，而不是仅将其关闭。

一次只安装一个版本。两者都在游戏可执行文件旁使用同一个 version.dll，OpenXR 加载器和设置程序等公共支持文件安装在 vrmod 下。不替换原有 GameAssembly.dll、UnityPlayer.dll 或游戏资源。两者均在游戏进程中运行，Clean 名称不代表不会被检测或保证安全。

## 首次安装

1. 从[最新发行版](https://github.com/deadpixel134/GF2Exilium-VR/releases/latest)下载 **Clean 或 Dirty 的一个完整 ZIP**，全部解压到新文件夹。`Source code` ZIP 不是安装包。
2. 关闭游戏及已有的设置程序、安装器。运行解压后的 `GF2VR.Installer.exe`，选择 `GF2_Exilium.exe` 或游戏文件夹并安装。
3. 打开设置并保存选项。准备好 SteamVR 和头显后，按平常方式自行启动游戏。

[安装、卸载与恢复](docs/zh-CN/INSTALLATION.md) · [更新与版本类型切换](docs/zh-CN/UPDATE.md)

## 更新、修复与保存

在Settings确认新版本提示后即可更新。无法自动安装时仍显示版本号及手动说明，只有点击「打开」才会打开发行页面。支持按版本跳过通知。

具有有效安装记录的缺失或隔离文件可重新安装。对于已更改文件，在最新ZIP的Installer选择**修复安装**，检查逐文件计划并确认后，先备份再替换。归属未知的文件默认排除，保留安装记录、设置、日志与既有备份。可单独重试未完成的后续处理，无需重新安装产品文件。

设置界面显示保存成功、保存失败、未保存状态及标题*****。保存失败时保留编辑内容，关闭未保存的更改时提示确认。请先关闭游戏再保存，并重启游戏使设置生效。

## 默认操作

| 操作 | 功能 |
|---|---|
| 左摇杆 | 移动 VR 视点 |
| 右摇杆 | 转动视角。默认采用每次 30° 的分段转向 |
| 右手指针 | 指向游戏画面中的项目。 |
| 右手 Trigger 或 A | 点击所指项目 |
| 右手 B | 返回、取消 |
| 左手 Grip | 在 3D 画面中打开或关闭辅助面板 |

[启动与完整操作](docs/zh-CN/USAGE.md) · [设置项目](docs/zh-CN/SETTINGS.md)

## 兼容性与验证范围

实机验证范围为Dirty在Haoplay Main、Activity的手部显示和角色接触，以及Steam Main的启动与立体显示。尚未单独验证Steam其他场景及新Clean DLL的HMD表现。

此前实际测试环境：Meta Quest 2 · 4K 显示器 · Virtual Desktop。

已确认 Quest 3 和 Galaxy XR 所报告的左右画面错位由抗锯齿导致。使用 VR 前，请在游戏设置中关闭抗锯齿。

## 详细指南

- [安装、卸载与恢复](docs/zh-CN/INSTALLATION.md)
- [启动与完整操作](docs/zh-CN/USAGE.md)
- [设置项目](docs/zh-CN/SETTINGS.md)
- [更新](docs/zh-CN/UPDATE.md)
- [故障排除](docs/zh-CN/TROUBLESHOOTING.md)
- [下载与文件组成](docs/zh-CN/RELEASE_READINESS.md)
- [验证范围](docs/zh-CN/VERIFICATION.md)
- [3DMigoto 兼容性](docs/zh-CN/3DMIGOTO_COMPATIBILITY.md)
- [开发者构建指南](docs/zh-CN/BUILD_VARIANT_METADATA.md)
- [更新记录](docs/zh-CN/RELEASE_NOTES.md) · [GitHub Releases](https://github.com/deadpixel134/GF2Exilium-VR/releases)
- [源码边界与使用条件](SOURCE_BOUNDARY.zh-CN.md)

> GF2VR 是非官方粉丝项目，需要合法安装的游戏。游戏商标及相关作品的权利属于各自权利人。
