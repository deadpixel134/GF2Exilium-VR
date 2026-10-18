# GF2VR 1.1.3 — ダウンロード

[한국어](../ko/RELEASE_READINESS.md) | [English](../en/RELEASE_READINESS.md) | [日本語](../ja/RELEASE_READINESS.md) | [简体中文](../zh-CN/RELEASE_READINESS.md)

> **必須: VR を使用する前に、ゲーム設定のアンチエイリアスをオフにしてください。**

1.1.3 への更新: Clean または Dirty の ZIP 全体を新しいフォルダーに展開し、その GF2VR.Installer.exe で既存ゲームの場所にインストールしてください。旧 1.1.2 設定アプリの自動更新は新しい native プロファイルを検証できず拒否します。既存インストールは保持されます。DLL だけのコピーやファイルの混在は避けてください。

1.1.3 は PR #1 の 3DMigoto 既定 wrapper モードとの互換性変更を Clean・Dirty の両方に反映します。ゲームの wrapper を保持し、VR コピーと OpenXR には実際の D3D11 デバイス/context を使用します。DLL と一致する設定アプリ・インストーラーを同梱します。


- [GF2VR-Native-SelfContained-Clean-1.1.3.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.3)
- [GF2VR-Native-SelfContained-Dirty-1.1.3.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.3)

| 機能 | Clean | Dirty |
|---|---|---|
| 通常の VR・立体表示・コントローラー入力・設定 | 含む | 含む |
| 両手の位置を示す白い球 | 含む | 含む |
| キャラクターの胸への接触・変形 | 含まない | 含む |

通常の VR のソース・バージョン・更新は両方で共通です。接触変形コードは Dirty のみで、Clean は単に機能をオフにしたものではなく実装を除外しています。

同時にインストールするのは一方だけです。どちらもゲーム実行ファイルの隣に同じ version.dll 一つを配置し、OpenXR ローダーや設定アプリなどの共通ファイルは vrmod 配下に配置します。元の GameAssembly.dll・UnityPlayer.dll・ゲームアセットは交換しません。両方ともゲームプロセス内で動くため、Clean という名称は非検知や安全を保証しません。

インストール・切替・削除ではユーザー設定を保持します。任意の touch_interaction.json は Dirty のみ使用します。削除時は記録された直前のインストールがあれば復元し、それ以外は所有ファイルだけを削除します。変更済み・所有権不明の DLL は上書きせず保持します。version.dll だけをコピーしたり、別パッケージのファイルを混ぜたりしないでください。

新しいインストーラーは種類切替・失敗復元 160 件、所有権 525 件、配置 175 件の検証に合格しました。共通投影補正はオフライン検証 183,984 件と既存経路の回帰検査に合格しています。公開ソースから両ネイティブ DLL を再構築し、配布バイトとの完全一致を確認しました。設定とインストーラーは必要な .NET 10.0.12 を同梱します。オフライン・インストール検証は実ゲーム/HMD の受入とは別です。

Quest 3 と Galaxy XR で報告された左右映像のずれは、アンチエイリアスが原因であると確認されました。VR を使用する前に、ゲーム設定のアンチエイリアスをオフにしてください。

CHECKSUMS.sha256 · GF2VR-Clean-update.json · GF2VR-Dirty-update.json
