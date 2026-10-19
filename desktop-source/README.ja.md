# デスクトップアプリのソース

[한국어](README.ko.md) · [English](README.md) · [日本語](README.ja.md) · [简体中文](README.zh-CN.md)

## 1.1.4

両ビルドで補助パネルのユーザー向き・位置・回転・画面の傾き・最大サイズを設定できます。Dirtyはキャラクター接触時の振動と、胸・臀部別の揺れ設定を追加します。

[Build metadata](../docs/ja/BUILD_VARIANT_METADATA.md) · [Native metadata](../release/1.1.4-native-metadata.json)


1.1.3 は PR #1 の 3DMigoto 既定 wrapper モードとの互換性変更を Clean・Dirty の両方に反映します。ゲームの wrapper を保持し、VR コピーと OpenXR には実際の D3D11 デバイス/context を使用します。DLL と一致する設定アプリ・インストーラーを同梱します。


通常の VR のソース・バージョン・更新は両方で共通です。接触変形コードは Dirty のみで、Clean は単に機能をオフにしたものではなく実装を除外しています。

インストール・切替・削除ではユーザー設定を保持します。任意の touch_interaction.json は Dirty のみ使用します。削除時は記録された直前のインストールがあれば復元し、それ以外は所有ファイルだけを削除します。変更済み・所有権不明の DLL は上書きせず保持します。version.dll だけをコピーしたり、別パッケージのファイルを混ぜたりしないでください。


設定プログラムとインストーラーは、Windows Forms、.NET SDK 10.0.112、公式の .NET 10.0.12 Windows x64 ランタイムパックを使用します。PowerShell 7 と Windows x64 が必要です。このディレクトリから、次のコマンドを実行します。

```powershell
./vrmod/tools/build_desktop_private_runtime.ps1 -Application Configurator -BuildId RELEASE-CLEAN-113 -ProductVersion 1.1.4
./vrmod/tools/build_desktop_private_runtime.ps1 -Application Installer -BuildId RELEASE-INSTALLER-113 -ProductVersion 1.1.4
```

最初のビルドでは、リポジトリ内のソース設定を使い、NuGet から公式のランタイムパックを復元します。プロジェクト内のキャッシュ、ビルド出力、必要な .NET のライセンス告知を `vrmod/build/` に作成します。ゲームファイルやシステム共通の .NET インストールは変更しません。両方のアプリに専用ランタイムを含みます。アプリの DLL、deps、runtimeconfig ファイルは EXE と同じ場所に維持してください。

`runtime-contract.json` は、現在のネイティブモジュールと 8 つの設定ドキュメントに対応するインストール用テンプレートです。インストーラーはパッケージを検証し、ユーザーが選択したインストールフォルダーに結び付けた契約ファイルを書き込みます。このテンプレートは個人用の設定でも、インストール済みの単体設定プログラムでもありません。ネイティブパッケージには、さらにリリースの正確なネイティブ DLL、OpenXR ローダー、ライセンス、検証済みのパッケージマニフェストが必要です。パッケージ全体の契約を確認せずにゲームファイルを代用したり、正常なインストールにソースからのビルド出力を上書きしたりしないでください。

アプリのソースには、ローカル検証用のエントリーポイント（`--self-test`、`--verify-localization`）が含まれます。インストーラーのパッケージ・可搬パス・更新のテスト用構成には、完全なパッケージと明示的に指定した前バージョンのパッケージが必要です。これらは実際には動作しないゲーム前提ファイルを使用し、VR DLL を読み込みません。リリースの記録では、これらの検査と実際のゲーム/HMD による受け入れ確認を区別しています。

1.1.1 はフォルダー構成にかかわらずゲーム実行ファイルを探し、元ゲームの指紋の違いではインストールを止めません。標準の `GF2 Game/GF2_Exilium.exe` 構成では、従来どおり親フォルダーの `vrmod` を使います。それ以外の構成では、見つかった実行ファイルのフォルダーにある `vrmod` を使います。`version.dll` は常にその実行ファイルの隣にインストールされます。 製品パッケージの整合性と既存ファイルの所有権検査は維持します。`--self-test-flexible` はゲームを起動せずにこれらのインストール経路を検証します。

[Build metadata](../docs/ja/BUILD_VARIANT_METADATA.md)
