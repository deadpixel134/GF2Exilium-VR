# 更新

[한국어](../ko/UPDATE.md) | [English](../en/UPDATE.md) | [日本語](../ja/UPDATE.md) | [简体中文](../zh-CN/UPDATE.md)

从 1.0.x 首次迁移到 Clean/Dirty 时，请手动下载所需 ZIP 并使用新安装程序，旧更新程序不会替您选择类型。新版自动更新只接受与当前类型一致且验证通过的安装包；切换类型请手动安装。设置中的更新检查使用已登录的 GitHub CLI (gh)。访问、网络或校验失败时保留现有安装和设置访问。

- [GF2VR-Native-SelfContained-Clean-1.1.1.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.1)
- [GF2VR-Native-SelfContained-Dirty-1.1.1.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.1)

安装、切换和移除均保留用户设置。可选 touch_interaction.json 仅由 Dirty 使用。移除时如有记录的上一安装则恢复，否则仅删除本工具拥有的文件。已修改或归属不明的 DLL 会被保留并拒绝覆盖。不要单独复制 version.dll，也不要混用不同安装包的文件。

Galaxy XR 在 VDXR 和 SteamVR 下均有左右画面错位报告，目前尚未确认解决。1.1.1 的公共投影修正适用条件改进已通过离线检查，但不代表修复了该报告，也不保证所有头显兼容。接触功能曾在此前用户实机测试中确认；此次发行二进制本身未重新进行游戏/HMD 实测。
