# 源码构成与使用条件

[한국어](SOURCE_BOUNDARY.md) · [English](SOURCE_BOUNDARY.en.md) · [日本語](SOURCE_BOUNDARY.ja.md) · [简体中文](SOURCE_BOUNDARY.zh-CN.md)

安装时请使用 [1.0.2 发布页](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.0.2)中的 **GF2VR-Native-SelfContained-1.0.2.zip**。GitHub 的 **Source code** ZIP 包含尚未构建的源码，仅解压并复制到游戏文件夹并不能完成安装。

## 仓库包含的资料

| 位置 | 内容 |
|---|---|
| `native-source/` | 原生 VR 模块源码、自行编写的托管代码与生成头文件、构建配置与脚本，以及所需的第三方头文件和声明 |
| `desktop-source/` | 设置程序、安装程序源码，以及韩语、英语、日语资源；程序界面目前不支持中文 |
| `docs/zh-CN/` | 安装、操作、设置、更新与故障排查的简体中文指南 |
| `source-manifest.json` | 公开源码的文件列表与完整性信息 |

原始游戏可执行文件、DLL、资源、开发参考项目、用户设置、日志、恢复文件、认证信息和开发工具缓存不包含在分发内容中。引用游戏库生成的自有桥接代码与生成头文件，和原始游戏库分开管理。

## 发布包构成

安装 ZIP 包含原生模块、OpenXR 加载器、设置程序、安装程序与第三方软件声明。设置程序和安装程序随包提供所需的 .NET 10.0.12 运行时，无须单独安装 .NET。

运行时、设置程序及安装元数据应作为同一版本的一整套文件使用。混用不同版本的部分文件可能导致文件检查或运行失败。原始游戏文件从用户合法安装的游戏中核对，不放入安装 ZIP。

标准的 `GF2 Game/GF2_Exilium.exe` 结构保留原上级文件夹中的 `vrmod`。其他结构使用找到的可执行文件所在文件夹中的 `vrmod`。`version.dll` 始终安装在该可执行文件旁。 原始游戏文件仅被读取，指纹差异不会阻止安装。继续执行产品软件包哈希、文件所有权、游戏运行时阻止操作、恢复上一版本及中断恢复检查。 [验证范围](docs/zh-CN/VERIFICATION.md)区分了实机测试与发布文件的本地检查。

## 使用条件

项目源码的分发许可证尚未确定。仓库公开并不等于允许自由再分发或分发修改版本。

OpenXR、MinHook 和 Microsoft .NET 分别适用其自身的许可证与声明。请查看安装 ZIP 及源码内的相关声明，以及 [Microsoft .NET 声明](DOTNET-NOTICES.txt)。这些文件不能替代 GF2VR 整体的使用条件。游戏相关商标及作品的权利归各自权利人所有。
