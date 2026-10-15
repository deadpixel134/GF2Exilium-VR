# ネイティブランタイムのソース

[한국어](README.ko.md) · [English](README.md) · [日本語](README.ja.md) · [简体中文](README.zh-CN.md)

これは、GF2VR 1.0.0 に含まれるネイティブランタイム `BOOTSTRAP-STABLE-001` のビルドに必要なソース一式です。SHA-256 は次のとおりです。

```text
F5281E05D0BEA0D83A0B895B2D02558B37C500FCE384739DDE643C182E1A695D
```

Windows x64 上で、PowerShell 7、Zig 0.16.0、Windows 10 SDK 10.0.26100.0 の x64 `kernel32.lib` を使用してビルドします。

```powershell
./build-native.ps1 -Zig 'C:/Tools/zig/zig.exe' -Kernel32Library 'C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64/kernel32.lib' -Output './build'
```

ビルダーは出力ディレクトリを Zig のキャッシュに使用し、終了後に呼び出し元のキャッシュ設定を復元します。11 個のランタイム補助ユニットと 4 個の MinHook ユニットをコンパイルし、ランタイムをリンクします。生成した DLL のハッシュがリリースと異なる場合は拒否します。`build-result.json` に、オブジェクトのハッシュ、コンパイラーのハッシュ、最終的な引数、DLL の比較結果を記録します。このビルドは DLL をインストール・読み込みせず、ゲームや XR ランタイムを起動しません。

`native-build.json` には、正確なビルドフラグとソースのハッシュを記録しています。`src/` には、現在の自作ネイティブコードと、このビルドが使用する生成済みパッケージヘッダーが含まれます。`managed/ActivityProducerEngineBridge.cs` は、埋め込まれたマネージドパッケージに対応する自作ブリッジのソースで、確認と開発のために提供しています。このネイティブ DLL の再ビルドは、リポジトリに含まれる生成済みパッケージを使用するため、ゲームのアセンブリを必要としません。マネージドパッケージの再生成は別の開発作業であり、合法的に入手した対応バージョンのゲームアセンブリが必要です。それらのアセンブリやアセットは、ここでは配布していません。

OpenXR と MinHook のソースの必要部分には、`third_party/` 内にそれぞれのライセンスを保持しています。ゲームの実行ファイル、Unity/GameAssembly のバイナリ、ゲームのメタデータ、ゲームのアセット、ユーザー設定、ログ、復元用ファイルは、このソース配布には含まれません。
