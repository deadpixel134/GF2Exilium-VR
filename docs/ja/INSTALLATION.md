# インストール・切り替え・削除

[한국어](../ko/INSTALLATION.md) | [English](../en/INSTALLATION.md) | [日本語](../ja/INSTALLATION.md) | [简体中文](../zh-CN/INSTALLATION.md)

[トップ](../../README.ja.md)

## 始める前の準備

- 正常に起動する **Haoplay 版または Steam 版のゲーム**。ゲーム本体は含みません。
- VR ヘッドセットとコントローラー、および接続済みの **SteamVR/OpenXR 環境**。
- ゲーム内の **アンチエイリアスを OFF**。Quest 3・Galaxy XR で報告された左右の映像ずれの原因として確認されています。

## パッケージの選択

[最新リリース](https://github.com/deadpixel134/GF2Exilium-VR/releases/latest)の **Assets** から Clean または Dirty の完全な ZIP を選びます。通常の VR・入力・白い手の表示は共通で、キャラクターの接触変形・振動は Dirty 専用です。一度に一種類だけインストールしてください。GitHub の `Source code` ZIP は使用しません。

## インストール手順

1. [最新リリース](https://github.com/deadpixel134/GF2Exilium-VR/releases/latest)から **Clean または Dirty の ZIP を一つ**選び、新しいフォルダーにすべて展開します。`Source code` ZIP はインストール用ではありません。
2. ゲームと既存の設定・インストーラーを終了します。展開した `GF2VR.Installer.exe` で `GF2_Exilium.exe` またはゲームフォルダーを選んでインストールします。
3. **設定を開く**からオプションを保存します。SteamVR とヘッドセットを準備し、普段の方法でゲームを起動します。

## インストールされるファイル

`version.dll` はゲーム実行ファイルの隣、OpenXR ローダーや設定ツールなどは `vrmod` 内に配置されます。元の `GameAssembly.dll`・`UnityPlayer.dll`・ゲームアセットは置き換えません。実行ファイルや DLL だけを移動したり、異なるパッケージを混ぜたりしないでください。

## Clean と Dirty の切り替え

希望する種類の最新 ZIP を新しいフォルダーにすべて展開し、そのインストーラーで同じゲーム場所へインストールします。二種類を重ねないでください。互換性のある設定は保持されます。Clean は Dirty の任意設定 `touch_interaction.json` を読みません。

## 削除と前のインストールの復元

ゲームを終了し、インストーラーで同じゲーム場所を選んで削除内容を確認します。記録された直前のインストールがあれば復元し、初回導入なら所有ファイルだけを削除します。設定や変更済み・所有権不明のファイルは保持します。インストール記録やバックアップを先に削除しないでください。中断した操作があれば中断復旧を使います。

[更新](UPDATE.md) · [トラブルシューティング](TROUBLESHOOTING.md)
