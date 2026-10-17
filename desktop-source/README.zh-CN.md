# 桌面程序源码

[한국어](README.ko.md) · [English](README.md) · [日本語](README.ja.md) · [简体中文](README.zh-CN.md)

VR DLL 与公共设置程序 payload 和 1.1.1 完全相同。此版本仅更新安装程序提示。


两个版本的常规 VR 源码、版本和更新保持一致。只有 Dirty 包含接触变形代码；Clean 在构建时排除该实现，而不是仅将其关闭。

安装、切换和移除均保留用户设置。可选 touch_interaction.json 仅由 Dirty 使用。移除时如有记录的上一安装则恢复，否则仅删除本工具拥有的文件。已修改或归属不明的 DLL 会被保留并拒绝覆盖。不要单独复制 version.dll，也不要混用不同安装包的文件。


设置程序与安装程序使用 Windows Forms、.NET SDK 10.0.112，以及官方 .NET 10.0.12 Windows x64 运行时包。需要 PowerShell 7 与 Windows x64。从本目录运行：

```powershell
./vrmod/tools/build_desktop_private_runtime.ps1 -Application Configurator -BuildId RELEASE-1.1.1 -ProductVersion 1.1.1
./vrmod/tools/build_desktop_private_runtime.ps1 -Application Installer -BuildId RELEASE-1.1.2 -ProductVersion 1.1.2
```

首次构建会使用仓库中的源配置，从 NuGet 还原官方运行时包，并在项目本地的 `vrmod/build/` 下创建缓存、构建输出及所需的 .NET 许可证声明。不会修改游戏文件或系统共享的 .NET 安装。两个宿主程序均包含私有运行时；请将应用 DLL、deps 与 runtimeconfig 文件保留在 EXE 旁。

`runtime-contract.json` 是适用于当前原生模块与七份配置文档的安装模板。安装程序会验证软件包，并写入绑定用户所选安装文件夹的契约。该模板不是个人配置，也不是已安装的独立设置程序。完整原生软件包还需要发布版本中准确匹配的原生 DLL、OpenXR 加载器、许可证及经过验证的软件包清单。请勿用游戏文件代替，也不要在未核对完整软件包契约的情况下，将源码构建输出覆盖到正常运行的安装中。

应用源码包含本地验证入口（`--self-test`、`--verify-localization`）。安装程序的软件包、可迁移路径与更新测试夹具需要完整软件包，以及明确指定的上一版本软件包；它们使用不会执行的模拟游戏前置文件，不加载 VR DLL。发布记录区分这些检查与真实游戏/HMD 验收。

应用界面目前仅支持韩语、英语和日语，不支持中文。本文件的简体中文翻译不表示已经添加中文界面资源。

1.1.1 跨文件夹结构查找游戏可执行文件，原始游戏指纹差异不再阻止安装。标准的 `GF2 Game/GF2_Exilium.exe` 结构保留原上级文件夹中的 `vrmod`。其他结构使用找到的可执行文件所在文件夹中的 `vrmod`。`version.dll` 始终安装在该可执行文件旁。 继续执行产品软件包完整性及已有文件所有权检查。`--self-test-flexible` 使用不执行的模拟文件验证这些安装路径。
