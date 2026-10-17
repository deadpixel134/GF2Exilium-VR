# GF2VR

[@TBluebox12](https://x.com/TBluebox12) · [가상현실 채널](https://arca.live/b/vrshits) · [buymeacoffee.com/vrshits](https://buymeacoffee.com/vrshits)

[한국어](README.md) | [English](README.en.md) | [日本語](README.ja.md) | [简体中文](README.zh-CN.md)

GF2 Exilium を VR で楽しむための非公式 MOD です。

**[正式版 1.1.2 をダウンロード](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.2)**

> **必須: VR を使用する前に、ゲーム設定のアンチエイリアスをオフにしてください。**

## Clean と Dirty の違い

| 機能 | Clean | Dirty |
|---|---|---|
| 通常の VR・立体表示・コントローラー入力・設定 | 含む | 含む |
| 両手の位置を示す白い球 | 含む | 含む |
| キャラクターの胸への接触・変形 | 含まない | 含む |

通常の VR のソース・バージョン・更新は両方で共通です。接触変形コードは Dirty のみで、Clean は単に機能をオフにしたものではなく実装を除外しています。

- [GF2VR-Native-SelfContained-Clean-1.1.2.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.2)
- [GF2VR-Native-SelfContained-Dirty-1.1.2.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.2)

同時にインストールするのは一方だけです。どちらもゲーム実行ファイルの隣に同じ version.dll 一つを配置し、OpenXR ローダーや設定アプリなどの共通ファイルは vrmod 配下に配置します。元の GameAssembly.dll・UnityPlayer.dll・ゲームアセットは交換しません。両方ともゲームプロセス内で動くため、Clean という名称は非検知や安全を保証しません。

## インストール・切替・削除

1. 希望する Clean または Dirty の ZIP 全体を新しいフォルダーに展開します。GitHub の Source code ZIP はインストーラーではありません。
2. ゲーム・設定・インストーラーを終了し、新しい GF2VR.Installer.exe で GF2_Exilium.exe またはゲームフォルダーを選びます。パッケージは自動検出します。
3. 種類とバージョンを確認してインストールします。Clean↔Dirty の切替も希望する ZIP のインストーラーで行います。
4. 設定を開いて保存し、通常の方法でゲームを起動します。手の球表示は初期値オンで、「手の表示」から変更できます。

インストール・切替・削除ではユーザー設定を保持します。任意の touch_interaction.json は Dirty のみ使用します。削除時は記録された直前のインストールがあれば復元し、それ以外は所有ファイルだけを削除します。変更済み・所有権不明の DLL は上書きせず保持します。version.dll だけをコピーしたり、別パッケージのファイルを混ぜたりしないでください。

## 検証範囲と制限

従来の実機確認環境: Meta Quest 2 · 4K モニター · Virtual Desktop。

Quest 3 と Galaxy XR の左右映像のずれは、解決を確認していません。Galaxy XR では VDXR・SteamVR の両方で報告されています。1.1.2 はインストーラーと文書の案内を更新し、VR DLL と設定アプリは 1.1.1 と同じです。アンチエイリアスをオフにする案内の追加は、両機種の問題解決を確認したという意味ではありません。

## ガイド

- [導入・削除・復元](docs/ja/INSTALLATION.md)
- [操作](docs/ja/USAGE.md)
- [設定](docs/ja/SETTINGS.md)
- [更新](docs/ja/UPDATE.md)
- [問題解決](docs/ja/TROUBLESHOOTING.md)
- [変更履歴](docs/ja/RELEASE_NOTES.md)
- [ダウンロード・内容](docs/ja/RELEASE_READINESS.md)
- [検証範囲](docs/ja/VERIFICATION.md)
- [ソース構成と利用条件](SOURCE_BOUNDARY.ja.md)

> GF2VR は非公式ファンプロジェクトです。正規にインストールしたゲームが必要です。ゲームの商標・著作物の権利は各権利者に帰属します。
