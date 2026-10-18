# レビューと検証

[한국어](../ko/VERIFICATION.md) | [English](../en/VERIFICATION.md) | [日本語](../ja/VERIFICATION.md) | [简体中文](../zh-CN/VERIFICATION.md)

> **必須: VR を使用する前に、ゲーム設定のアンチエイリアスをオフにしてください。**

PR 作者は Dirty 開発ビルドで 3 回連続起動し、VR とスキンの同時動作を確認したと報告しています。これは投稿者の記録であり、新しい 1.1.3 バイナリの HMD 受け入れ確認ではありません。本リリースでは両 native ビルド、実際の変更ソースを使う COM 同一性/失敗経路、デスクトップビルド、パッケージ整合性を検証します。新たなゲーム/HMD 実行は行っていません。

Quest 3 と Galaxy XR で報告された左右映像のずれは、アンチエイリアスが原因であると確認されました。VR を使用する前に、ゲーム設定のアンチエイリアスをオフにしてください。

3DMigoto 既定 wrapper モードとの互換性変更と一致するビルドメタデータを追加します。通常 VR と手の表示は共通、接触変形は Dirty 専用です。既存設定と所有権/復元保護を保持します。

[3DMigoto](3DMIGOTO_COMPATIBILITY.md) · [Build metadata](BUILD_VARIANT_METADATA.md)

貢献: [rgbtl1248](https://github.com/rgbtl1248)、[PR #1](https://github.com/deadpixel134/GF2Exilium-VR/pull/1)。

1.1.3 への更新: Clean または Dirty の ZIP 全体を新しいフォルダーに展開し、その GF2VR.Installer.exe で既存ゲームの場所にインストールしてください。旧 1.1.2 設定アプリの自動更新は新しい native プロファイルを検証できず拒否します。既存インストールは保持されます。DLL だけのコピーやファイルの混在は避けてください。

1.1.3 の検証: COM 47件、Clean/Dirty のインストール・切替・復元160件、所有権525件、フォルダー構成175件、パッケージ位置12件、インストーラーUX19件、設定アプリ各577件、1.1.2 → 1.1.3 → 完全復元28件が通過しました。オフライン・模擬ファイルの検査であり、ゲーム/HMDの受け入れ確認とは別です。
