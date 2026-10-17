# インストール・切替・削除

[한국어](../ko/INSTALLATION.md) | [English](../en/INSTALLATION.md) | [日本語](../ja/INSTALLATION.md) | [简体中文](../zh-CN/INSTALLATION.md)

> **必須: VR を使用する前に、ゲーム設定のアンチエイリアスをオフにしてください。**

VR DLL と共通設定アプリの payload は 1.1.1 と同一です。この版はインストーラーの案内のみ更新します。


- [GF2VR-Native-SelfContained-Clean-1.1.2.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.2)
- [GF2VR-Native-SelfContained-Dirty-1.1.2.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.2)

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

Quest 3 と Galaxy XR の左右映像のずれは、解決を確認していません。Galaxy XR では VDXR・SteamVR の両方で報告されています。1.1.2 はインストーラーと文書の案内を更新し、VR DLL と設定アプリは 1.1.1 と同じです。アンチエイリアスをオフにする案内の追加は、両機種の問題解決を確認したという意味ではありません。
