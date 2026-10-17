# 验证范围与限制

[한국어](../ko/VERIFICATION.md) | [English](../en/VERIFICATION.md) | [日本語](../ja/VERIFICATION.md) | [简体中文](../zh-CN/VERIFICATION.md)

此前实际测试环境：Meta Quest 2 · 4K 显示器 · Virtual Desktop。

新安装程序通过了 160 项版本切换与失败恢复、525 项所有权以及 175 项目录布局检查。公共投影修正通过 183,984 项离线断言及原有路径回归检查。由公开源码重新构建的两个原生 DLL 与发行文件逐字节一致。安装和设置程序包含所需 .NET 10.0.12 运行时。离线与安装检查不能替代真实游戏/HMD 验收。

Galaxy XR 在 VDXR 和 SteamVR 下均有左右画面错位报告，目前尚未确认解决。1.1.1 的公共投影修正适用条件改进已通过离线检查，但不代表修复了该报告，也不保证所有头显兼容。接触功能曾在此前用户实机测试中确认；此次发行二进制本身未重新进行游戏/HMD 实测。

一次只安装一个版本。两者都在游戏可执行文件旁使用同一个 version.dll，OpenXR 加载器和设置程序等公共支持文件安装在 vrmod 下。不替换原有 GameAssembly.dll、UnityPlayer.dll 或游戏资源。两者均在游戏进程中运行，Clean 名称不代表不会被检测或保证安全。
