# 保存与发送日志

[한국어](../ko/LOGS.md) | [English](../en/LOGS.md) | [日本語](../ja/LOGS.md) | [简体中文](../zh-CN/LOGS.md)

[首页](../../README.zh-CN.md)

反馈问题时附上 VR 模组的诊断日志，能大大帮助查找原因。日志记录默认关闭，请按以下步骤开启，再次复现问题后发送文件。

## 1. 开启日志保存

1. 关闭游戏。
2. 打开设置程序：在安装程序中点击 **打开设置**，或运行游戏文件夹中的 `vrmod/bin/GF2VR.Configurator/GF2VR.Configurator.exe`。
3. 在 **诊断** 选项卡中勾选 **自动记录日志**，然后点击 **保存**。
4. 重新启动游戏。保存的设置需要重启游戏后才会生效。

## 2. 复现问题

- 进入出现问题的画面，复现一次相同症状后关闭游戏。
- 每次运行游戏，日志都会追加到同一个文件末尾，一次运行可能超过 100MB。如果只想记录本次问题，请在启动游戏前把现有日志文件移到其他文件夹。
- 如果游戏突然退出，最后约 0.25 秒的内容可能不会被记录。

## 3. 打开日志文件夹

- **1.3.3 起：** 在设置程序的 **诊断** 选项卡中点击 **打开日志文件夹**，即可在资源管理器中打开日志文件夹。按钮下方会显示保存位置。如果还没有生成日志，会显示提示信息。
- **手动打开：** 位于设置程序顶部 **游戏文件夹** 所示路径下的 `vrmod\logs\bootstrap` 文件夹。
  - 如果 `GF2_Exilium.exe` 直接位于游戏文件夹中（如 Steam 版），路径为 `<游戏文件夹>\vrmod\logs\bootstrap`。
  - 如果 `GF2_Exilium.exe` 位于 `GF2 Game` 文件夹内（如 Haoplay 版），路径为 `GF2 Game` 上一级文件夹下的 `vrmod\logs\bootstrap`。

## 4. 选择要发送的文件

- 在以 `GF2VR-CLEAN-` 或 `GF2VR-DIRTY-` 开头的 `.jsonl` 文件中，选择 **修改日期最新的文件**。如果有同名的 `.inventory.jsonl` 文件，请一并发送。
- 文件名会随版本变化。例如在 1.3.3 中，Dirty 为 `GF2VR-DIRTY-133-006.jsonl`，Clean 为 `GF2VR-CLEAN-130-002.jsonl`（1.3.1 的 Dirty 为 `GF2VR-DIRTY-130-004.jsonl`）。
- 将选中的文件压缩为 ZIP。Windows 11：右键 → **压缩为** → **ZIP 文件**；Windows 10：右键 → **发送到** → **压缩(zipped)文件夹**。日志是文本，压缩后会小很多（例如约 100MB → 约 5MB）。

## 5. 发送给开发者

- 在 [GitHub Issues](https://github.com/deadpixel134/GF2Exilium-VR/issues/new) 新建 Issue，把 ZIP 文件拖到输入框中作为附件。GitHub 附件上限为 25MB。
- 也可以发布到 [Arca Live VR 社区](https://arca.live/b/vrshits)。
- 文件过大时，请上传到 Google Drive 等云存储，并附上分享链接。
- 请同时写明以下内容：
  - 类型（Clean/Dirty）和版本，Haoplay/Steam
  - 头显和连接方式（例如 Quest 3 + Virtual Desktop）
  - 出现问题的画面、复现步骤以及显示的错误信息
  - 问题发生的大致时间

## 6. 分享之前

- GitHub Issue 和 Arca Live 帖子所有人都能看到。
- 日志是 VR 模组的运行记录（事件、状态和性能数值）。请只发送日志文件，不要发送游戏原始文件、账号或认证信息，以及未被要求的其他文件。

## 7. 用完后关闭

确认结束后，请在 **诊断** 选项卡中取消勾选 **自动记录日志** 并保存。保持开启会让日志文件不断增大。已经发送的日志文件可以删除。

[设置](SETTINGS.md) · [故障排除](TROUBLESHOOTING.md)
