# 更新

[한국어](../ko/UPDATE.md) | [English](../en/UPDATE.md) | [日本語](../ja/UPDATE.md) | [简体中文](../zh-CN/UPDATE.md)

> **必須: VR を使用する前に、ゲーム設定のアンチエイリアスをオフにしてください。**

VR DLL と共通設定アプリの payload は 1.1.1 と同一です。この版はインストーラーの案内のみ更新します。


1.0.x から移行するときは、希望する ZIP を手動でダウンロードして新しいインストーラーを使います。旧更新機能は種類を選びません。新しい自動更新は現在の種類と一致する検証済みパッケージのみ許可し、種類変更は手動インストールで行います。設定アプリの更新確認にはログイン済み GitHub CLI (gh) を使います。アクセス・通信・検証に失敗しても既存のインストールと設定利用を維持します。

- [GF2VR-Native-SelfContained-Clean-1.1.2.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.2)
- [GF2VR-Native-SelfContained-Dirty-1.1.2.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.2)

インストール・切替・削除ではユーザー設定を保持します。任意の touch_interaction.json は Dirty のみ使用します。削除時は記録された直前のインストールがあれば復元し、それ以外は所有ファイルだけを削除します。変更済み・所有権不明の DLL は上書きせず保持します。version.dll だけをコピーしたり、別パッケージのファイルを混ぜたりしないでください。

Quest 3 と Galaxy XR の左右映像のずれは、解決を確認していません。Galaxy XR では VDXR・SteamVR の両方で報告されています。1.1.2 はインストーラーと文書の案内を更新し、VR DLL と設定アプリは 1.1.1 と同じです。アンチエイリアスをオフにする案内の追加は、両機種の問題解決を確認したという意味ではありません。
