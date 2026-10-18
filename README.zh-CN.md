# GF2VR

作者: [@TBluebox12](https://x.com/TBluebox12)  
Arca Live VR 社区: [VR 社区](https://arca.live/b/vrshits)  
赞助: [buymeacoffee.com/vrshits](https://buymeacoffee.com/vrshits)  

[한국어](README.md) | [English](README.en.md) | [日本語](README.ja.md) | [简体中文](README.zh-CN.md)

用于以 VR 游玩 GF2 Exilium 的非官方模组。

**[下载正式版 1.1.3](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.3)**

> **使用 VR 前必须在游戏设置中关闭抗锯齿。**

更新至 1.1.3：将完整的 Clean 或 Dirty ZIP 解压到新文件夹，使用其中的 GF2VR.Installer.exe 选择现有游戏位置安装。旧 1.1.2 设置程序无法验证新的 native 配置，因此会拒绝自动暂存并保留当前安装。请勿只复制 DLL 或混用包文件。

## 3DMigoto 兼容性

1.1.3 将 PR #1 的 3DMigoto 默认 wrapper 模式兼容性修改同时提供给 Clean 和 Dirty。保留游戏 wrapper，VR 复制与 OpenXR 使用实际 D3D11 设备/context。DLL 随附匹配的设置程序与安装程序元数据。

请使用默认 wrapper 模式，即省略或注释 [System] 中的 `hook`。`hook=recommended` 和 `hook=all` 不在支持范围内。`calls`、`crash` 用于日志/转储，并非此修复的必要选项。皮肤 INI 语法与重复规则警告属于其他问题。

[3DMigoto](docs/zh-CN/3DMIGOTO_COMPATIBILITY.md) · [Build metadata](docs/zh-CN/BUILD_VARIANT_METADATA.md)

## 选择 Clean 或 Dirty

| 功能 | Clean | Dirty |
|---|---|---|
| 常规 VR、立体画面、控制器输入和设置 | 包含 | 包含 |
| 双手位置的白色球体 | 包含 | 包含 |
| 角色胸部、臀部接触与变形 | 不包含 | 包含 |

接触功能在对应部位的受支持骨骼已绑定的角色上生效。支持部位因角色模型而异；尚未确认宿舍和活动层中的实际臀部反应。

两个版本的常规 VR 源码、版本和更新保持一致。只有 Dirty 包含接触变形代码；Clean 在构建时排除该实现，而不是仅将其关闭。

- [GF2VR-Native-SelfContained-Clean-1.1.3.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.3)
- [GF2VR-Native-SelfContained-Dirty-1.1.3.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.3)

一次只安装一个版本。两者都在游戏可执行文件旁使用同一个 version.dll，OpenXR 加载器和设置程序等公共支持文件安装在 vrmod 下。不替换原有 GameAssembly.dll、UnityPlayer.dll 或游戏资源。两者均在游戏进程中运行，Clean 名称不代表不会被检测或保证安全。

## 安装、切换和移除

1. 将所需 Clean 或 Dirty ZIP 完整解压到新文件夹。GitHub 的 Source code ZIP 不是安装包。
2. 关闭游戏、设置和安装程序。运行新文件夹中的 GF2VR.Installer.exe，选择 GF2_Exilium.exe 或游戏文件夹。程序会自动找到随附安装包。
3. 确认版本类型和版本号后安装。Clean↔Dirty 切换也使用所需 ZIP 中的新安装程序。
4. 打开设置并保存，然后按平时方式自行启动游戏。手部球体默认开启，可在手部显示页面修改。

安装、切换和移除均保留用户设置。可选 touch_interaction.json 仅由 Dirty 使用。移除时如有记录的上一安装则恢复，否则仅删除本工具拥有的文件。已修改或归属不明的 DLL 会被保留并拒绝覆盖。不要单独复制 version.dll，也不要混用不同安装包的文件。

## 验证范围与限制

此前实际测试环境：Meta Quest 2 · 4K 显示器 · Virtual Desktop。

已确认 Quest 3 和 Galaxy XR 所报告的左右画面错位由抗锯齿导致。使用 VR 前，请在游戏设置中关闭抗锯齿。

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
