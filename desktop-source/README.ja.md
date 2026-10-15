# デスクトップアプリのソース

[한국어](README.ko.md) · [English](README.md) · [日本語](README.ja.md) · [简体中文](README.zh-CN.md)

設定プログラムとインストーラーは、Windows Forms、.NET SDK 10.0.112、公式の .NET 10.0.12 Windows x64 ランタイムパックを使用します。PowerShell 7 と Windows x64 が必要です。このディレクトリから、次のコマンドを実行します。

```powershell
./vrmod/tools/build_desktop_private_runtime.ps1 -Application Configurator -BuildId STABLE-DESKTOP-001 -ProductVersion 1.0.0
./vrmod/tools/build_desktop_private_runtime.ps1 -Application Installer -BuildId STABLE-DESKTOP-001 -ProductVersion 1.0.0
```

最初のビルドでは、リポジトリ内のソース設定を使い、NuGet から公式のランタイムパックを復元します。プロジェクト内のキャッシュ、ビルド出力、必要な .NET のライセンス告知を `vrmod/build/` に作成します。ゲームファイルやシステム共通の .NET インストールは変更しません。両方のアプリに専用ランタイムを含みます。アプリの DLL、deps、runtimeconfig ファイルは EXE と同じ場所に維持してください。

`runtime-contract.json` は、現在のネイティブモジュールと 7 つの設定ドキュメントに対応するインストール用テンプレートです。インストーラーはパッケージを検証し、ユーザーが選択したインストールフォルダーに結び付けた契約ファイルを書き込みます。このテンプレートは個人用の設定でも、インストール済みの単体設定プログラムでもありません。ネイティブパッケージには、さらにリリースの正確なネイティブ DLL、OpenXR ローダー、ライセンス、検証済みのパッケージマニフェストが必要です。パッケージ全体の契約を確認せずにゲームファイルを代用したり、正常なインストールにソースからのビルド出力を上書きしたりしないでください。

アプリのソースには、ローカル検証用のエントリーポイント（`--self-test`、`--verify-localization`）が含まれます。インストーラーのパッケージ・可搬パス・更新のテスト用構成には、完全なパッケージと明示的に指定した前バージョンのパッケージが必要です。これらは実際には動作しないゲーム前提ファイルを使用し、VR DLL を読み込みません。リリースの記録では、これらの検査と実際のゲーム/HMD による受け入れ確認を区別しています。
