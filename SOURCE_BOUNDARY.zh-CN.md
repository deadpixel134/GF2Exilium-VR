# 源码构成与使用条件

[한국어](SOURCE_BOUNDARY.md) · [English](SOURCE_BOUNDARY.en.md) · [日本語](SOURCE_BOUNDARY.ja.md) · [简体中文](SOURCE_BOUNDARY.zh-CN.md)

## 1.2.0 — Steam支持与手部、接触兼容性修复

支持Steam可执行文件目录及现有Haoplay目录结构。修复游戏更新后白色手部标记和角色接触消失的兼容性问题。Dirty独立处理活动层中同时可见的受支持角色；Clean不包含接触变形。

实机验证范围为Dirty在Haoplay Main、Activity的手部显示/接触，以及Steam Main的启动和立体显示。尚未单独确认Steam全部场景及新Clean DLL的HMD表现。请关闭游戏内抗锯齿。

完整的受管理1.1.6安装可在设置GUI中批准更新通知，升级至1.2.0，无需GitHub CLI或登录，保留版本类型及用户设置。1.1.4及更早版本或无法检查更新的旧客户端，请将所需ZIP完整解压到新目录，运行附带的GF2VR.Installer.exe安装一次。请先关闭游戏。界面语言仍为韩语、英语和日语。

发行版本为1.2.0。内部native构建ID中的130用于标识此前未公开候选中经过验证的同一DLL。常规安装程序不会静默覆盖手动替换或不属于本产品的文件。

## 1.1.6 — 修复自动更新

设置GUI启动时检查公开GitHub发行版。批准通知中的 **更新** 后，将下载、校验并安装。从1.1.6开始，无需GitHub CLI或登录。

修复旧客户端使用固定白名单拒绝新构建的问题。客户端校验发行版与文件哈希、版本类型及当前文件归属，再由经过验证的新安装程序完成最终构建校验、备份和安装。保留Clean/Dirty类型与用户设置。

1.1.4及更早版本需要此次手动更新一次：将所选Clean/Dirty ZIP完整解压到新文件夹，并运行其中的新GF2VR.Installer.exe。更新检查正常的1.1.5环境可直接使用GUI更新。旧版所需GitHub CLI缺失的用户也需此次通过ZIP安装一次。安装1.1.6之后，GUI更新无需CLI或登录。

本版保留与1.1.5完全相同的VR DLL。保留日志默认关闭、明确开启设置及首次清理排除状态，以及面板和接触设置。安装前关闭游戏与设置窗口。界面语言仍为韩语、英语和日语。

## 1.1.4

两个版本均可设置辅助面板的朝向用户、位置、旋转、画面倾斜和最大尺寸。Dirty增加角色接触振动，以及胸部、臀部分别调整的晃动设置。

[Source manifest](source-manifest.json) · [Native metadata](release/1.1.4-native-metadata.json)


1.1.1 公开 Clean/Dirty 产品源码及独立安装包。native-source/variants/common 为公共原生代码，Clean/Dirty overlay 为接触功能差异生成的头文件，managed 为自有手部显示及接触代码。native-source 根目录其余部分保留此前 1.0.2 的源码与重建脚本。


安装时请使用 [1.1.1 发布页](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.3)中的 **GF2VR-Native-SelfContained-Clean-1.1.3.zip / GF2VR-Native-SelfContained-Dirty-1.1.3.zip**。GitHub 的 **Source code** ZIP 包含尚未构建的源码，仅解压并复制到游戏文件夹并不能完成安装。

## 仓库包含的资料

| 位置 | 内容 |
|---|---|
| `native-source/` | 原生 VR 模块源码、自行编写的托管代码与生成头文件、构建配置与脚本，以及所需的第三方头文件和声明 当前源码：`variants/`；历史复现目录：`src/`。 |
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
