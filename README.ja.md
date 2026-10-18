# GF2VR

制作者: [@TBluebox12](https://x.com/TBluebox12)  
Arca Live VR チャンネル: [VR チャンネル](https://arca.live/b/vrshits)  
支援: [buymeacoffee.com/vrshits](https://buymeacoffee.com/vrshits)  

[한국어](README.md) | [English](README.en.md) | [日本語](README.ja.md) | [简体中文](README.zh-CN.md)

GF2 Exilium を VR で楽しむための非公式 MOD です。

**[正式版 1.1.3 をダウンロード](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.3)**

> **必須: VR を使用する前に、ゲーム設定のアンチエイリアスをオフにしてください。**

1.1.3 への更新: Clean または Dirty の ZIP 全体を新しいフォルダーに展開し、その GF2VR.Installer.exe で既存ゲームの場所にインストールしてください。旧 1.1.2 設定アプリの自動更新は新しい native プロファイルを検証できず拒否します。既存インストールは保持されます。DLL だけのコピーやファイルの混在は避けてください。

## 貢献者

- [rgbtl1248](https://github.com/rgbtl1248) — 3DMigoto 互換性とビルドツールの改善 ([PR #1](https://github.com/deadpixel134/GF2Exilium-VR/pull/1))。

## 3DMigoto との互換性

1.1.3 は PR #1 の 3DMigoto 既定 wrapper モードとの互換性変更を Clean・Dirty の両方に反映します。ゲームの wrapper を保持し、VR コピーと OpenXR には実際の D3D11 デバイス/context を使用します。DLL と一致する設定アプリ・インストーラーを同梱します。

[System] の `hook` を省略またはコメントアウトした既定 wrapper モードを使用してください。`hook=recommended` と `hook=all` は対応範囲外です。`calls`・`crash` はログ/ダンプ用で、この修正に必須ではありません。スキンの INI 構文や重複ルール警告は別の問題です。

[3DMigoto](docs/ja/3DMIGOTO_COMPATIBILITY.md) · [Build metadata](docs/ja/BUILD_VARIANT_METADATA.md)

## Clean と Dirty の違い

| 機能 | Clean | Dirty |
|---|---|---|
| 通常の VR・立体表示・コントローラー入力・設定 | 含む | 含む |
| 両手の位置を示す白い球 | 含む | 含む |
| キャラクターの胸・臀部への接触と変形 | 含まない | 含む |

接触は、その部位の対応ボーンが接続されたキャラクターで動作します。対応部位はキャラクターモデルによって異なり、宿舎・活動層での実際の臀部の反応はまだ確認されていません。

通常の VR のソース・バージョン・更新は両方で共通です。接触変形コードは Dirty のみで、Clean は単に機能をオフにしたものではなく実装を除外しています。

- [GF2VR-Native-SelfContained-Clean-1.1.3.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.3)
- [GF2VR-Native-SelfContained-Dirty-1.1.3.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.3)

同時にインストールするのは一方だけです。どちらもゲーム実行ファイルの隣に同じ version.dll 一つを配置し、OpenXR ローダーや設定アプリなどの共通ファイルは vrmod 配下に配置します。元の GameAssembly.dll・UnityPlayer.dll・ゲームアセットは交換しません。両方ともゲームプロセス内で動くため、Clean という名称は非検知や安全を保証しません。

## インストール・切替・削除

1. 希望する Clean または Dirty の ZIP 全体を新しいフォルダーに展開します。GitHub の Source code ZIP はインストーラーではありません。
2. ゲーム・設定・インストーラーを終了し、新しい GF2VR.Installer.exe で GF2_Exilium.exe またはゲームフォルダーを選びます。パッケージは自動検出します。
3. 種類とバージョンを確認してインストールします。Clean↔Dirty の切替も希望する ZIP のインストーラーで行います。
4. 設定を開いて保存し、通常の方法でゲームを起動します。手の球表示は初期値オンで、「手の表示」から変更できます。

インストール・切替・削除ではユーザー設定を保持します。任意の touch_interaction.json は Dirty のみ使用します。削除時は記録された直前のインストールがあれば復元し、それ以外は所有ファイルだけを削除します。変更済み・所有権不明の DLL は上書きせず保持します。version.dll だけをコピーしたり、別パッケージのファイルを混ぜたりしないでください。

## 検証範囲と制限

従来の実機確認環境: Meta Quest 2 · 4K モニター · Virtual Desktop。

Quest 3 と Galaxy XR で報告された左右映像のずれは、アンチエイリアスが原因であると確認されました。VR を使用する前に、ゲーム設定のアンチエイリアスをオフにしてください。

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
