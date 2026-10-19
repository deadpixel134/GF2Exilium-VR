# 변경 사항

[한국어](../ko/RELEASE_NOTES.md) | [English](../en/RELEASE_NOTES.md) | [日本語](../ja/RELEASE_NOTES.md) | [简体中文](../zh-CN/RELEASE_NOTES.md)

## 1.1.4

**공통 사항**

- 수동 보조 패널 설정 추가

**Dirty 빌드**

- 캐릭터 터치 시 진동 추가
- 캐릭터 터치 관련 흔들림 및 설정 추가


## 1.1.3

1.1.3 업데이트: Clean 또는 Dirty ZIP 전체를 새 폴더에 풀고, 그 안의 GF2VR.Installer.exe로 기존 게임 위치에 설치하세요. 기존 1.1.2 설정 창의 자동 업데이트는 새 native 프로필을 검증하지 못해 거부됩니다. 기존 설치는 보존됩니다. DLL만 복사하거나 파일을 섞지 마세요.

1.1.3은 PR #1의 3DMigoto 기본 wrapper 모드 호환 변경을 Clean·Dirty에 함께 반영합니다. 게임의 wrapper는 유지하고 VR 복사·OpenXR에는 실제 D3D11 장치/context를 사용합니다. 변경된 DLL과 일치하는 설정 도구·인스톨러를 함께 제공합니다.

3DMigoto의 `[System]`에서 `hook`이 생략되었거나 주석 처리된 기본 wrapper 모드를 사용하세요. `hook=recommended` 및 `hook=all` 조합은 지원 범위 밖입니다. `calls`·`crash`는 기록/덤프 옵션이며 이 수정에 필수는 아닙니다. 스킨의 INI 문법·중복 규칙 경고는 별도 문제입니다.

> **필수: VR 사용 전 게임 설정에서 안티앨리어싱을 끄세요.**

PR 작성자는 Dirty 개발 빌드에서 VR·스킨 동시 작동을 세 번 연속 확인했다고 보고했습니다. 이는 작성자의 기록이며 새 1.1.3 바이너리의 HMD 수용을 대신하지 않습니다. 이번 릴리즈는 양쪽 native 빌드, 실제 소스의 COM 장치/실패 경로 검사, 설정 도구·인스톨러 빌드와 패키지 무결성을 검증합니다. 새 게임/HMD 실행은 하지 않았습니다.

기여: [rgbtl1248](https://github.com/rgbtl1248), [PR #1](https://github.com/deadpixel134/GF2Exilium-VR/pull/1).

## 1.1.2

> **필수: VR 사용 전 게임 설정에서 안티앨리어싱을 끄세요.**

1.1.2: 인스톨러 첫 화면과 README에 게임 설정에서 안티앨리어싱을 꺼야 한다는 안내를 추가했습니다. Clean·Dirty 모두 같은 안내를 표시합니다. 게임 설정을 자동으로 바꾸지 않으며 VR DLL·설정 프로그램·접촉 기능은 1.1.1과 같습니다.

Quest 3와 Galaxy XR에서 보고된 양안 영상 어긋남은 안티앨리어싱이 원인인 것으로 확인됐습니다. 게임 설정에서 안티앨리어싱을 끄고 VR을 사용하세요.

## 1.1.1


1.1.1: Clean/Dirty를 별도 ZIP으로 배포하고 흰색 손 표시는 공통으로 제공합니다. 접촉 변형은 Dirty 전용입니다. 비접촉 기능의 소스·버전·업데이트 경로를 공통으로 유지하며, 같은 버전의 명시적 종류 전환과 설정 보존을 지원합니다. 투영 보정의 수평축에만 의존하던 적용 조건을 안전한 수직축 근거까지 확대했습니다.

Clean/Dirty 전환·실패 복원 160개, 소유권 보호 525개, 폴더 배치 175개 검사를 새 Installer에서 통과했습니다. 공통 투영 보정은 183,984개 오프라인 검사와 기존 경로 회귀를 통과했습니다. 공개 소스에서 다시 빌드한 두 native DLL은 각각 배포 DLL과 바이트가 일치합니다. 설정 프로그램과 인스톨러는 필요한 .NET 10.0.12 런타임을 포함합니다. 오프라인·설치 검증과 실제 게임/HMD 수용은 별개입니다.

Quest 3와 Galaxy XR에서 보고된 양안 영상 어긋남은 안티앨리어싱이 원인인 것으로 확인됐습니다. 게임 설정에서 안티앨리어싱을 끄고 VR을 사용하세요.

[1.0.2](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.0.2) · [1.0.1](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.0.1) · [1.0.0](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.0.0)
