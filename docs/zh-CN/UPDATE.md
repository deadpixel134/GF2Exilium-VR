# 更新

[한국어](../ko/UPDATE.md) | [English](../en/UPDATE.md) | [日本語](../ja/UPDATE.md) | [简体中文](../zh-CN/UPDATE.md)

## 1.1.4

更新至1.1.4：将Clean或Dirty ZIP完整解压到新文件夹，使用其中的GF2VR.Installer.exe选择现有游戏位置安装。旧版1.1.3设置程序无法验证新的native配置，因此此次请使用新ZIP内的安装程序。保留已有设置。请勿只复制DLL或混用包文件。


> **使用 VR 前必须在游戏设置中关闭抗锯齿。**

更新至 1.1.3：将完整的 Clean 或 Dirty ZIP 解压到新文件夹，使用其中的 GF2VR.Installer.exe 选择现有游戏位置安装。旧 1.1.2 设置程序无法验证新的 native 配置，因此会拒绝自动暂存并保留当前安装。请勿只复制 DLL 或混用包文件。

1.1.3 将 PR #1 的 3DMigoto 默认 wrapper 模式兼容性修改同时提供给 Clean 和 Dirty。保留游戏 wrapper，VR 复制与 OpenXR 使用实际 D3D11 设备/context。DLL 随附匹配的设置程序与安装程序元数据。


从 1.0.x 首次迁移到 Clean/Dirty 时，请手动下载所需 ZIP 并使用新安装程序，旧更新程序不会替您选择类型。新版自动更新只接受与当前类型一致且验证通过的安装包；切换类型请手动安装。设置中的更新检查使用已登录的 GitHub CLI (gh)。访问、网络或校验失败时保留现有安装和设置访问。

- [GF2VR-Native-SelfContained-Clean-1.1.3.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.3)
- [GF2VR-Native-SelfContained-Dirty-1.1.3.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.3)

安装、切换和移除均保留用户设置。可选 touch_interaction.json 仅由 Dirty 使用。移除时如有记录的上一安装则恢复，否则仅删除本工具拥有的文件。已修改或归属不明的 DLL 会被保留并拒绝覆盖。不要单独复制 version.dll，也不要混用不同安装包的文件。

已确认 Quest 3 和 Galaxy XR 所报告的左右画面错位由抗锯齿导致。使用 VR 前，请在游戏设置中关闭抗锯齿。
