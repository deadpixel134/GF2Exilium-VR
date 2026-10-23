# 支持环境与验证范围

[한국어](../ko/VERIFICATION.md) | [English](../en/VERIFICATION.md) | [日本語](../ja/VERIFICATION.md) | [简体中文](../zh-CN/VERIFICATION.md)

[首页](../../README.zh-CN.md)

平台兼容性、实际游戏观察和离线检查是不同层次的证据。文档整理或源码构建成功不代表增加了头显实测。

## 平台与实际游戏观察

| 对象 | 确认范围 |
|---|---|
| Haoplay · Dirty | Main、Activity 的双手显示与接触路径 |
| Steam · Dirty | Main 启动与头显立体画面 |
| Steam 其他画面、新 Clean DLL | 仍需单独进行头显确认 |

既有实测环境为 Meta Quest 2、4K 显示器和 Virtual Desktop。Quest 3、Galaxy XR 的双眼错位已确认与游戏抗锯齿有关，必须设为 OFF。此列表不保证所有头显和连接方式。

## 功能限制

接触仅 Dirty 提供，需要对应部位的支持骨骼，不保证所有角色、模型和画面。宿舍、活动层的实际臀部反应尚未单独确认。帧率取决于场景、硬件和画质设置。

3DMigoto 支持默认 wrapper 模式，见[兼容性说明](3DMIGOTO_COMPATIBILITY.md)。Clean 同样在游戏进程内运行，名称不保证不被检测或安全。

## 安装包与源码检查

从公开源码重现的 Clean、Dirty DLL 与发布 DLL 字节一致。安装、切换、恢复、设置与多语言工具、安装包完整性及旧客户端更新路径经过技术检查；这不等同于所有用户环境或全部场景的头显实测。各版本记录保留在[更新记录](RELEASE_NOTES.md)和 [GitHub 发行版](https://github.com/deadpixel134/GF2Exilium-VR/releases)。
