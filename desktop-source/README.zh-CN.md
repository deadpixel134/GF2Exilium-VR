# 桌面程序源码

[한국어](README.ko.md) · [English](README.md) · [日本語](README.ja.md) · [简体中文](README.zh-CN.md)

设置程序与安装程序使用 Windows Forms、.NET SDK 10.0.112，以及官方 .NET 10.0.12 Windows x64 运行时包。需要 PowerShell 7 与 Windows x64。从本目录运行：

```powershell
./vrmod/tools/build_desktop_private_runtime.ps1 -Application Configurator -BuildId STABLE-DESKTOP-001 -ProductVersion 1.0.0
./vrmod/tools/build_desktop_private_runtime.ps1 -Application Installer -BuildId STABLE-DESKTOP-001 -ProductVersion 1.0.0
```

首次构建会使用仓库中的源配置，从 NuGet 还原官方运行时包，并在项目本地的 `vrmod/build/` 下创建缓存、构建输出及所需的 .NET 许可证声明。不会修改游戏文件或系统共享的 .NET 安装。两个宿主程序均包含私有运行时；请将应用 DLL、deps 与 runtimeconfig 文件保留在 EXE 旁。

`runtime-contract.json` 是适用于当前原生模块与七份配置文档的安装模板。安装程序会验证软件包，并写入绑定用户所选安装文件夹的契约。该模板不是个人配置，也不是已安装的独立设置程序。完整原生软件包还需要发布版本中准确匹配的原生 DLL、OpenXR 加载器、许可证及经过验证的软件包清单。请勿用游戏文件代替，也不要在未核对完整软件包契约的情况下，将源码构建输出覆盖到正常运行的安装中。

应用源码包含本地验证入口（`--self-test`、`--verify-localization`）。安装程序的软件包、可迁移路径与更新测试夹具需要完整软件包，以及明确指定的上一版本软件包；它们使用不会执行的模拟游戏前置文件，不加载 VR DLL。发布记录区分这些检查与真实游戏/HMD 验收。

应用界面目前仅支持韩语、英语和日语，不支持中文。本文件的简体中文翻译不表示已经添加中文界面资源。
