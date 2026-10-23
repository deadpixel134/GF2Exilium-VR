# ダウンロードとファイル構成

[한국어](../ko/RELEASE_READINESS.md) | [English](../en/RELEASE_READINESS.md) | [日本語](../ja/RELEASE_READINESS.md) | [简体中文](../zh-CN/RELEASE_READINESS.md)

[トップ](../../README.ja.md)

[最新の正式リリース](https://github.com/deadpixel134/GF2Exilium-VR/releases/latest)の **Assets** からインストール用パッケージを取得します。実際の版とファイル名はそのページで確認してください。

| ファイル | 用途 |
|---|---|
| `GF2VR-Native-SelfContained-Clean-<version>.zip` | Clean の完全インストールパッケージ |
| `GF2VR-Native-SelfContained-Dirty-<version>.zip` | Dirty の完全インストールパッケージ |
| `CHECKSUMS.sha256` | 取得ファイルの SHA-256 確認用 |
| `GF2VR-Clean-update.json` / `GF2VR-Dirty-update.json` | 自動更新用メタデータ。手動導入には ZIP を使用します。 |
| GitHub `Source code` | 開発用のリポジトリ複製。インストールパッケージではありません。 |

完全な ZIP には `GF2VR.Installer.exe`、設定ツール、選択した種類の VR ファイルを含みます。必要な .NET ランタイムも同梱され、利用者によるビルドは不要です。

すべて展開して同梱インストーラーを使用してください。DLL だけのコピーや整合性検査の無効化は避けてください。[インストール](INSTALLATION.md) · [更新](UPDATE.md)
