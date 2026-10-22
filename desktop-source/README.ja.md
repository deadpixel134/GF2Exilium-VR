# デスクトップアプリのソース

[한국어](README.ko.md) · [English](README.md) · [日本語](README.ja.md) · [简体中文](README.zh-CN.md)

## 1.2.0 — Steam対応と手・接触の互換性修正

Steam版の実行ファイルフォルダーと既存のHaoplay構成に対応します。ゲーム更新後に白い手の表示とキャラクター接触が失われる互換性問題を修正しました。Dirtyは活動層で同時に見える対応キャラクターを個別に扱います。Cleanには接触変形を含みません。

実機確認はDirtyのHaoplay Main・Activityの手表示/接触とSteam Mainの起動・立体表示です。Steamの全画面と新しいClean DLLのHMD動作は別途未確認です。ゲーム内アンチエイリアシングをオフにしてください。

正常な管理下の1.1.6は設定GUIの通知を承認して1.2.0へ更新できます。GitHub CLI・ログインは不要で、種類とユーザー設定を保持します。1.1.4以前または更新確認できない旧版では、希望するZIP全体を新しいフォルダーに展開し、付属のGF2VR.Installer.exeで一度インストールしてください。先にゲームを終了してください。

リリース版は1.2.0です。内部nativeビルドIDの130は以前の未公開候補で実機確認した同一DLLの識別子です。通常のインストーラーは手動交換・所有権のないファイルを無断で上書きしません。

```powershell
./vrmod/tools/build_desktop_private_runtime.ps1 -Application Installer -ProductVersion 1.2.0 -BuildId RELEASE-INSTALLER-120 -NativeMetadataRoot '../native-source/build-1.2.0/desktop-metadata'
./vrmod/tools/build_desktop_private_runtime.ps1 -Application Configurator -ProductVersion 1.2.0 -BuildId RELEASE-GUI-CLEAN-120 -NativeMetadataRoot '../native-source/build-1.2.0/desktop-metadata' -NativeFlavor Clean
./vrmod/tools/build_desktop_private_runtime.ps1 -Application Configurator -ProductVersion 1.2.0 -BuildId RELEASE-GUI-DIRTY-120 -NativeMetadataRoot '../native-source/build-1.2.0/desktop-metadata' -NativeFlavor Dirty
```

## 1.1.6 — 自動更新の修正

設定GUIの起動時に公開GitHubリリースを確認します。通知の **更新** を承認するとダウンロード・検証・インストールします。1.1.6以降はGitHub CLIのインストールやログインが不要です。

旧クライアントの固定許可リストが新しいビルドを拒否する問題を修正しました。クライアントはリリース・ファイルのハッシュ、種類、現在の所有権を確認し、検証済みの新インストーラーが最終ビルド検証・バックアップ・インストールを担当します。Clean・Dirtyの種類と設定は維持します。

1.1.4以前では今回一度だけ、希望するClean・Dirty ZIP全体を新しいフォルダーに展開し、付属の新しいGF2VR.Installer.exeで更新してください。更新確認が動作する1.1.5環境ではGUI更新を利用できます。旧版に必要なGitHub CLIがない場合も今回一度ZIPから更新してください。1.1.6導入後はCLI・ログイン不要でGUIから更新できます。

VR DLLは1.1.5と同一です。ログの初期値OFF、明示的ON設定と初回整理の除外、パネル・接触設定を維持します。インストール前にゲームと設定画面を閉じてください。

## 1.1.5 — ログ記録設定

Clean・Dirty共通で設定GUIに **診断 → ログを自動記録** を追加しました。初期値は **OFF** で、旧設定に項目がない場合もOFFです。保存後にゲームを再起動すると適用されます。

初回の更新インストール時に、ログOFFのユーザーの以前のVR自動ログを一度だけ整理します。**ログONのユーザーは既存ログを保持し、この初回整理から除外されます。** 後でOFFに変えても過去のログを遅れて削除しません。設定・他のMOD・手動で収集した資料は保持します。

問題の調査時にログを有効にしてください。ONでは同じビルドのログに追記され、容量が増加することがあります。

1.1.5では必要なCleanまたはDirtyのZIP全体を新しいフォルダーに展開し、付属の新しいGF2VR.Installer.exeで更新してください。旧設定アプリは新しいnativeプロファイルを拒否する場合があります。既存設定は保持されます。

[Native metadata](../release/1.1.5-native-metadata.json)

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
