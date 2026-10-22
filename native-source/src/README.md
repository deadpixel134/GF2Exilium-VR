# Historical native source / 역사적 네이티브 소스

[한국어](#korean) | [English](#english) | [日本語](#japanese) | [简体中文](#chinese)

<a id="korean"></a>

## 소스 트리 안내

`src/`와 루트 `build-native.ps1`, `native-build.json`은 과거1.0.x `BOOTSTRAP-FLEX-001`(`6215CEAF…`)의 역사적 재현 전용입니다. 이 트리의 Haoplay 전용 상수(예: `src/stereo/stereo_hooks.h:48`)는 현재 빌드에서 사용되지 않으며, `variants/common/src/stereo/stereo_hooks.h`의 런타임 유도 대상으로 대체되었습니다. 바이트 재현을 위해 기존 소스는 수정하지 않습니다.

<a id="english"></a>

## Source tree guide

`src/` with the root `build-native.ps1` and `native-build.json` is retained only for historical1.0.x `BOOTSTRAP-FLEX-001` reproduction (`6215CEAF…`). Its old Haoplay-only constants, for example `src/stereo/stereo_hooks.h:48`, are not used by the current build; runtime-derived targets in `variants/common/src/stereo/stereo_hooks.h` replace that route. Historical source bytes are intentionally unchanged.

<a id="japanese"></a>

## ソースツリー案内

`src/`とルートの`build-native.ps1`、`native-build.json`は過去1.0.xの`BOOTSTRAP-FLEX-001`（`6215CEAF…`）の歴史的再現専用です。旧Haoplay専用定数（例：`src/stereo/stereo_hooks.h:48`）は現在のビルドでは使わず、`variants/common/src/stereo/stereo_hooks.h`の実行時導出対象に置き換わっています。バイト再現のため既存ソースを変更しません。

<a id="chinese"></a>

## 源码目录说明

`src/`以及根目录的`build-native.ps1`、`native-build.json`仅用于历史1.0.x `BOOTSTRAP-FLEX-001`（`6215CEAF…`）的精确复现。旧Haoplay专用常量（如`src/stereo/stereo_hooks.h:48`）不用于当前构建，已由`variants/common/src/stereo/stereo_hooks.h`中的运行时推导目标替代。为保持字节复现，不修改历史源码。

