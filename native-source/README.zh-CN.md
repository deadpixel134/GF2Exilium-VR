# 原生运行时源码

[한국어](README.ko.md) · [English](README.md) · [日本語](README.ja.md) · [简体中文](README.zh-CN.md)

## 1.1.1 — Clean / Dirty

当前 1.1.1 使用 variants 下的公共代码及各类型生成头文件。一起重新构建两个版本：

```powershell
./variants/build-variants.ps1 -Zig 'C:/Tools/zig/zig.exe' -Output './build-1.1.1'
```

1.1.1 公开 Clean/Dirty 产品源码及独立安装包。native-source/variants/common 为公共原生代码，Clean/Dirty overlay 为接触功能差异生成的头文件，managed 为自有手部显示及接触代码。native-source 根目录其余部分保留此前 1.0.2 的源码与重建脚本。

已确认 Quest 3 和 Galaxy XR 所报告的左右画面错位由抗锯齿导致。使用 VR 前，请在游戏设置中关闭抗锯齿。

---


这里是 GF2VR 1.0.2 中原生运行时 `BOOTSTRAP-FLEX-001` 的完整构建源码集。其 SHA-256 为：

```text
6215CEAFFFA46A3B89336DA73203122DD0095977206FDB395A1259F967B54771
```

在 Windows x64 上使用 PowerShell 7、Zig 0.16.0，以及 Windows 10 SDK 10.0.26100.0 的 x64 `kernel32.lib` 构建：

```powershell
./build-native.ps1 -Zig 'C:/Tools/zig/zig.exe' -Kernel32Library 'C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64/kernel32.lib' -Output './build'
```

构建脚本使用输出目录存放 Zig 缓存，并在完成后恢复调用者的缓存设置。它编译 11 个运行时支持单元及 4 个 MinHook 单元，链接运行时；如果生成的 DLL 哈希与发布版本不同，便会拒绝该文件。`build-result.json` 记录目标文件哈希、编译器哈希、最终参数与 DLL 比较结果。此构建不会安装或加载 DLL，也不会启动游戏或 XR 运行时。

`native-build.json` 记录准确的构建参数与源码哈希。`src/` 包含当前自有原生代码及此构建使用的已生成软件包头文件。`managed/ActivityProducerEngineBridge.cs` 是与嵌入式托管软件包对应的自有托管桥接源码，提供给开发者审阅和开发。重新构建此原生 DLL 会使用仓库中已生成的软件包，不需要游戏程序集。重新生成托管软件包是独立的开发操作，需要合法取得且版本匹配的游戏程序集；这些程序集及其资源不在此处提供。

OpenXR 与 MinHook 的源码子集在 `third_party/` 中保留其许可证。游戏可执行文件、Unity/GameAssembly 二进制文件、游戏元数据、游戏资源、用户配置、日志和回滚文件不属于此次源码分发的内容。

1.0.2 的路径修改用于查找可执行文件旁的加载器及对应的 `vrmod` 目录。标准的 `GF2 Game/GF2_Exilium.exe` 结构保留原上级文件夹中的 `vrmod`。其他结构使用找到的可执行文件所在文件夹中的 `vrmod`。`version.dll` 始终安装在该可执行文件旁。
