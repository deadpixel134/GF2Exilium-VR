# 更新

[한국어](../ko/UPDATE.md) | [English](../en/UPDATE.md) | [日本語](../ja/UPDATE.md) | [简体中文](../zh-CN/UPDATE.md)

1.0.x から移行するときは、希望する ZIP を手動でダウンロードして新しいインストーラーを使います。旧更新機能は種類を選びません。新しい自動更新は現在の種類と一致する検証済みパッケージのみ許可し、種類変更は手動インストールで行います。設定アプリの更新確認にはログイン済み GitHub CLI (gh) を使います。アクセス・通信・検証に失敗しても既存のインストールと設定利用を維持します。

- [GF2VR-Native-SelfContained-Clean-1.1.1.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.1)
- [GF2VR-Native-SelfContained-Dirty-1.1.1.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.1)

インストール・切替・削除ではユーザー設定を保持します。任意の touch_interaction.json は Dirty のみ使用します。削除時は記録された直前のインストールがあれば復元し、それ以外は所有ファイルだけを削除します。変更済み・所有権不明の DLL は上書きせず保持します。version.dll だけをコピーしたり、別パッケージのファイルを混ぜたりしないでください。

Galaxy XR の VDXR・SteamVR 両方で報告された左右映像のずれは、解決を確認していません。1.1.1 の共通投影補正の適用条件改善はオフライン検証済みですが、この問題の解決や全ヘッドセットへの対応を保証しません。接触機能は以前のユーザー実機テストで確認されましたが、今回の配布バイナリ自体では新たなゲーム/HMD 実行を行っていません。
