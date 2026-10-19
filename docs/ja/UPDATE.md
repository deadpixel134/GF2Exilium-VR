# 更新

[한국어](../ko/UPDATE.md) | [English](../en/UPDATE.md) | [日本語](../ja/UPDATE.md) | [简体中文](../zh-CN/UPDATE.md)

## 1.1.4

1.1.4への更新: CleanまたはDirtyのZIP全体を新しいフォルダーに展開し、そのGF2VR.Installer.exeで既存ゲームの場所にインストールしてください。旧1.1.3設定アプリは新しいnativeプロファイルを検証できないため、今回は新しいZIPのインストーラーを使用します。既存設定を保持します。DLLのみのコピーやパッケージの混在は避けてください。


> **必須: VR を使用する前に、ゲーム設定のアンチエイリアスをオフにしてください。**

1.1.3 への更新: Clean または Dirty の ZIP 全体を新しいフォルダーに展開し、その GF2VR.Installer.exe で既存ゲームの場所にインストールしてください。旧 1.1.2 設定アプリの自動更新は新しい native プロファイルを検証できず拒否します。既存インストールは保持されます。DLL だけのコピーやファイルの混在は避けてください。

1.1.3 は PR #1 の 3DMigoto 既定 wrapper モードとの互換性変更を Clean・Dirty の両方に反映します。ゲームの wrapper を保持し、VR コピーと OpenXR には実際の D3D11 デバイス/context を使用します。DLL と一致する設定アプリ・インストーラーを同梱します。


1.0.x から移行するときは、希望する ZIP を手動でダウンロードして新しいインストーラーを使います。旧更新機能は種類を選びません。新しい自動更新は現在の種類と一致する検証済みパッケージのみ許可し、種類変更は手動インストールで行います。設定アプリの更新確認にはログイン済み GitHub CLI (gh) を使います。アクセス・通信・検証に失敗しても既存のインストールと設定利用を維持します。

- [GF2VR-Native-SelfContained-Clean-1.1.3.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.3)
- [GF2VR-Native-SelfContained-Dirty-1.1.3.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.3)

インストール・切替・削除ではユーザー設定を保持します。任意の touch_interaction.json は Dirty のみ使用します。削除時は記録された直前のインストールがあれば復元し、それ以外は所有ファイルだけを削除します。変更済み・所有権不明の DLL は上書きせず保持します。version.dll だけをコピーしたり、別パッケージのファイルを混ぜたりしないでください。

Quest 3 と Galaxy XR で報告された左右映像のずれは、アンチエイリアスが原因であると確認されました。VR を使用する前に、ゲーム設定のアンチエイリアスをオフにしてください。
