# ダウンロードとサポート

[한국어](../ko/RELEASE_READINESS.md) · [English](../en/RELEASE_READINESS.md) · [日本語](RELEASE_READINESS.md) · [简体中文](../zh-CN/RELEASE_READINESS.md)

## 正式版 1.0.1

[GitHub リリースページ](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.0.1)から **GF2VR-Native-SelfContained-1.0.1.zip** をダウンロードしてください。公開ダウンロードに GitHub へのログインや別途のトークンは必要ありません。

- [インストール ZIP を直接ダウンロード](https://github.com/deadpixel134/GF2Exilium-VR/releases/download/v1.0.1/GF2VR-Native-SelfContained-1.0.1.zip)
- [インストール・削除・復元ガイド](INSTALLATION.md)
- [現在のリリース一覧](https://github.com/deadpixel134/GF2Exilium-VR/releases)

GitHub が別途提供する **Source code** ZIP はインストール用ではありません。リリース ZIP の内容をすべて新しいフォルダーに展開し、**GF2VR.Installer.exe** を実行してください。

## 対応環境

Windows x64 PC に正規のゲームと VR 環境が必要です。**2.0.3936** は実機確認の基準ビルドです。元のゲームの SHA-256・ファイルサイズ・キャッシュの違いだけでインストールを止めませんが、他のゲームバージョンでの VR 動作を保証するものではありません。 ヘッドセット・コントローラー用ソフトウェアと SteamVR/OpenXR は別途用意してください。

標準の `GF2 Game/GF2_Exilium.exe` 構成では、従来どおり親フォルダーの `vrmod` を使います。それ以外の構成では、見つかった実行ファイルのフォルダーにある `vrmod` を使います。`version.dll` は常にその実行ファイルの隣にインストールされます。

設定プログラムとインストーラーは韓国語・英語・日本語に対応し、必要な .NET ランタイムを同梱しています。ゲームと SteamVR はユーザーが手動で起動します。確認済みの動作と未検証の環境は、[確認範囲](VERIFICATION.md)に記載しています。

## 更新

新しいリリース ZIP をダウンロードし、インストーラーから既存のインストールを更新できます。設定プログラムの任意の更新機能は、インストール済みの **GitHub CLI（gh）** とそのログイン状態を使用します。この機能を使わなくても、公開リリースの手動インストール・更新は可能です。詳しくは、[更新方法](UPDATE.md)を確認してください。

## 利用条件と問い合わせ

元のゲームは含まれていません。プロジェクトソースの配布ライセンスはまだ定めておらず、外部コンポーネントにはそれぞれのライセンスが適用されます。[ソースの構成と利用条件](../../SOURCE_BOUNDARY.ja.md)を確認してください。

問題が発生した場合は、[トラブルシューティング](TROUBLESHOOTING.md)に従ってエラーメッセージと発生までの手順をまとめてください。コミュニティは、[Arca Live バーチャルリアリティチャンネル](https://arca.live/b/vrshits)にあります。

インストール ZIP のサイズ：**93,661,818 バイト**。SHA-256：

```text
07E4365C7EA15047BAD95833318864EA30BC1A8AE4E9413C44FE0084F6CABB73
```
