# インストール・切替・削除

[한국어](../ko/INSTALLATION.md) | [English](../en/INSTALLATION.md) | [日本語](../ja/INSTALLATION.md) | [简体中文](../zh-CN/INSTALLATION.md)

- [GF2VR-Native-SelfContained-Clean-1.1.1.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.1)
- [GF2VR-Native-SelfContained-Dirty-1.1.1.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.1)

| 機能 | Clean | Dirty |
|---|---|---|
| 通常の VR・立体表示・コントローラー入力・設定 | 含む | 含む |
| 両手の位置を示す白い球 | 含む | 含む |
| キャラクターの胸への接触・変形 | 含まない | 含む |

1. 希望する Clean または Dirty の ZIP 全体を新しいフォルダーに展開します。GitHub の Source code ZIP はインストーラーではありません。
2. ゲーム・設定・インストーラーを終了し、新しい GF2VR.Installer.exe で GF2_Exilium.exe またはゲームフォルダーを選びます。パッケージは自動検出します。
3. 種類とバージョンを確認してインストールします。Clean↔Dirty の切替も希望する ZIP のインストーラーで行います。
4. 設定を開いて保存し、通常の方法でゲームを起動します。手の球表示は初期値オンで、「手の表示」から変更できます。

同時にインストールするのは一方だけです。どちらもゲーム実行ファイルの隣に同じ version.dll 一つを配置し、OpenXR ローダーや設定アプリなどの共通ファイルは vrmod 配下に配置します。元の GameAssembly.dll・UnityPlayer.dll・ゲームアセットは交換しません。両方ともゲームプロセス内で動くため、Clean という名称は非検知や安全を保証しません。

インストール・切替・削除ではユーザー設定を保持します。任意の touch_interaction.json は Dirty のみ使用します。削除時は記録された直前のインストールがあれば復元し、それ以外は所有ファイルだけを削除します。変更済み・所有権不明の DLL は上書きせず保持します。version.dll だけをコピーしたり、別パッケージのファイルを混ぜたりしないでください。

Galaxy XR の VDXR・SteamVR 両方で報告された左右映像のずれは、解決を確認していません。1.1.1 の共通投影補正の適用条件改善はオフライン検証済みですが、この問題の解決や全ヘッドセットへの対応を保証しません。接触機能は以前のユーザー実機テストで確認されましたが、今回の配布バイナリ自体では新たなゲーム/HMD 実行を行っていません。
