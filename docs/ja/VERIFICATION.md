# 対応環境と検証範囲

[한국어](../ko/VERIFICATION.md) | [English](../en/VERIFICATION.md) | [日本語](../ja/VERIFICATION.md) | [简体中文](../zh-CN/VERIFICATION.md)

[トップ](../../README.ja.md)

プラットフォーム互換性、実際のゲームでの観察、オフライン検査は別の証拠です。文書の整理やソースのビルド成功は新たな HMD 実機確認を意味しません。

## プラットフォームと実機確認

| 対象 | 確認範囲 |
|---|---|
| Haoplay · Dirty | Main・Activity の手の表示と接触経路 |
| Steam · Dirty | Main の起動と HMD の立体映像 |
| Steam の他画面・新しい Clean DLL | 別途 HMD 確認が必要 |

既存の確認環境は Meta Quest 2・4K モニター・Virtual Desktop です。Quest 3・Galaxy XR の左右ずれはゲーム内アンチエイリアスが原因として確認され、OFF が必須です。すべてのヘッドセット・接続方法を保証する一覧ではありません。

## 機能ごとの制限

接触は Dirty 専用で、該当部位の対応ボーンが必要です。全キャラクター・モデル・画面を保証しません。宿舎・活動層での実際の臀部反応は別途確認されていません。フレームレートはシーン・機器・画質設定で変わります。

3DMigoto は基本 wrapper モードが対象です。[互換性](3DMIGOTO_COMPATIBILITY.md)を確認してください。Clean もゲームプロセス内で動作し、名前は検出回避や安全を保証しません。

## パッケージとソースの検証

公開ソースから再現した Clean・Dirty DLL は配布 DLL とバイト一致します。導入・切り替え・復元、設定と多言語ツール、パッケージ整合性、旧クライアントの更新経路を技術的に検査しました。すべての利用環境やシーンの HMD 動作を確認したものではありません。版ごとの記録は[変更履歴](RELEASE_NOTES.md)と [GitHub リリース](https://github.com/deadpixel134/GF2Exilium-VR/releases)に保管します。
