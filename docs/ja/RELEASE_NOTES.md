# 変更内容

[한국어](../ko/RELEASE_NOTES.md) | [English](../en/RELEASE_NOTES.md) | [日本語](../ja/RELEASE_NOTES.md) | [简体中文](../zh-CN/RELEASE_NOTES.md)

1.1.1 は Clean・Dirty を別 ZIP として配布します。白い手の球表示は共通、接触変形は Dirty 専用です。通常機能のソース・版・更新契約を共通化し、同一バージョンの明示的切替と設定保持に対応します。投影補正の適用条件を、従来の水平軸だけでなく十分な垂直軸の根拠まで拡張しました。Galaxy XR 問題の解決確認ではありません。

新しいインストーラーは種類切替・失敗復元 160 件、所有権 525 件、配置 175 件の検証に合格しました。共通投影補正はオフライン検証 183,984 件と既存経路の回帰検査に合格しています。公開ソースから両ネイティブ DLL を再構築し、配布バイトとの完全一致を確認しました。設定とインストーラーは必要な .NET 10.0.12 を同梱します。オフライン・インストール検証は実ゲーム/HMD の受入とは別です。

Galaxy XR の VDXR・SteamVR 両方で報告された左右映像のずれは、解決を確認していません。1.1.1 の共通投影補正の適用条件改善はオフライン検証済みですが、この問題の解決や全ヘッドセットへの対応を保証しません。接触機能は以前のユーザー実機テストで確認されましたが、今回の配布バイナリ自体では新たなゲーム/HMD 実行を行っていません。

[1.0.2](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.0.2) · [1.0.1](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.0.1) · [1.0.0](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.0.0)
