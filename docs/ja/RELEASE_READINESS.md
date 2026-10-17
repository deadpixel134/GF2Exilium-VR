# 正式版 1.1.1 をダウンロード

[한국어](../ko/RELEASE_READINESS.md) | [English](../en/RELEASE_READINESS.md) | [日本語](../ja/RELEASE_READINESS.md) | [简体中文](../zh-CN/RELEASE_READINESS.md)

- [GF2VR-Native-SelfContained-Clean-1.1.1.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.1)
- [GF2VR-Native-SelfContained-Dirty-1.1.1.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.1)

| 機能 | Clean | Dirty |
|---|---|---|
| 通常の VR・立体表示・コントローラー入力・設定 | 含む | 含む |
| 両手の位置を示す白い球 | 含む | 含む |
| キャラクターの胸への接触・変形 | 含まない | 含む |

通常の VR のソース・バージョン・更新は両方で共通です。接触変形コードは Dirty のみで、Clean は単に機能をオフにしたものではなく実装を除外しています。

同時にインストールするのは一方だけです。どちらもゲーム実行ファイルの隣に同じ version.dll 一つを配置し、OpenXR ローダーや設定アプリなどの共通ファイルは vrmod 配下に配置します。元の GameAssembly.dll・UnityPlayer.dll・ゲームアセットは交換しません。両方ともゲームプロセス内で動くため、Clean という名称は非検知や安全を保証しません。

インストール・切替・削除ではユーザー設定を保持します。任意の touch_interaction.json は Dirty のみ使用します。削除時は記録された直前のインストールがあれば復元し、それ以外は所有ファイルだけを削除します。変更済み・所有権不明の DLL は上書きせず保持します。version.dll だけをコピーしたり、別パッケージのファイルを混ぜたりしないでください。

新しいインストーラーは種類切替・失敗復元 160 件、所有権 525 件、配置 175 件の検証に合格しました。共通投影補正はオフライン検証 183,984 件と既存経路の回帰検査に合格しています。公開ソースから両ネイティブ DLL を再構築し、配布バイトとの完全一致を確認しました。設定とインストーラーは必要な .NET 10.0.12 を同梱します。オフライン・インストール検証は実ゲーム/HMD の受入とは別です。

Galaxy XR の VDXR・SteamVR 両方で報告された左右映像のずれは、解決を確認していません。1.1.1 の共通投影補正の適用条件改善はオフライン検証済みですが、この問題の解決や全ヘッドセットへの対応を保証しません。接触機能は以前のユーザー実機テストで確認されましたが、今回の配布バイナリ自体では新たなゲーム/HMD 実行を行っていません。

CHECKSUMS.sha256 · GF2VR-Clean-update.json · GF2VR-Dirty-update.json
