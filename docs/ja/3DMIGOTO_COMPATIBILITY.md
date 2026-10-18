# 3DMigoto と併用する

[한국어](../ko/3DMIGOTO_COMPATIBILITY.md) | [English](../en/3DMIGOTO_COMPATIBILITY.md) | [日本語](../ja/3DMIGOTO_COMPATIBILITY.md) | [简体中文](../zh-CN/3DMIGOTO_COMPATIBILITY.md)

1.1.3 は PR #1 の 3DMigoto 既定 wrapper モードとの互換性変更を Clean・Dirty の両方に反映します。ゲームの wrapper を保持し、VR コピーと OpenXR には実際の D3D11 デバイス/context を使用します。DLL と一致する設定アプリ・インストーラーを同梱します。

> **必須: VR を使用する前に、ゲーム設定のアンチエイリアスをオフにしてください。**

## 設定

[System] の `hook` を省略またはコメントアウトした既定 wrapper モードを使用してください。`hook=recommended` と `hook=all` は対応範囲外です。`calls`・`crash` はログ/ダンプ用で、この修正に必須ではありません。スキンの INI 構文や重複ルール警告は別の問題です。

## 実装

- swapchain の公開 GetDevice を使用し、失敗時にはバックバッファの GetDevice を使用します。
- Unity wrapper と実際のデバイスを COM IUnknown 同一性で比較します。ゲームの wrapper は交換しません。
- 参照を保持した実際のデバイス/immediate context を VR に使用し、パネル・コピー先・手のカーソルにも同じ比較を適用します。
- 目の描画直前に context の COM 同一性を確認し、保持した実際の context の監視フックを更新します。
- 実際のデバイス/context に既知の 3DMigoto wrapper インターフェイスがあれば VR 初期化を中止します。理由 11/12 がこの検査を表します。通常のゲーム実行は維持します。

DLL だけのコピーではデスクトップ側の整合性情報が一致しません。ZIP 全体を展開し、新しいインストーラーで更新してください。開発者は[メタデータ生成手順](BUILD_VARIANT_METADATA.md)を参照してください。診断 native_graphics_isolated=1 は wrapper/native のアドレス差、0 は一致を示し、互換性成功の判定ではありません。

## 確認範囲

PR 作者は Dirty 開発ビルドで 3 回連続起動し、VR とスキンの同時動作を確認したと報告しています。これは投稿者の記録であり、新しい 1.1.3 バイナリの HMD 受け入れ確認ではありません。本リリースでは両 native ビルド、実際の変更ソースを使う COM 同一性/失敗経路、デスクトップビルド、パッケージ整合性を検証します。新たなゲーム/HMD 実行は行っていません。

Quest 3 と Galaxy XR で報告された左右映像のずれは、アンチエイリアスが原因であると確認されました。VR を使用する前に、ゲーム設定のアンチエイリアスをオフにしてください。

貢献: [rgbtl1248](https://github.com/rgbtl1248)、[PR #1](https://github.com/deadpixel134/GF2Exilium-VR/pull/1)。
