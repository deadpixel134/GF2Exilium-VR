# 변경 사항

[한국어](../ko/RELEASE_NOTES.md) | [English](../en/RELEASE_NOTES.md) | [日本語](../ja/RELEASE_NOTES.md) | [简体中文](../zh-CN/RELEASE_NOTES.md)

## 1.1.2

> **필수: VR 사용 전 게임 설정에서 안티앨리어싱을 끄세요.**

1.1.2: 인스톨러 첫 화면과 README에 게임 설정에서 안티앨리어싱을 꺼야 한다는 안내를 추가했습니다. Clean·Dirty 모두 같은 안내를 표시합니다. 게임 설정을 자동으로 바꾸지 않으며 VR DLL·설정 프로그램·접촉 기능은 1.1.1과 같습니다.

Quest 3와 Galaxy XR에서 보고된 양안 영상 어긋남은 안티앨리어싱이 원인인 것으로 확인됐습니다. 게임 설정에서 안티앨리어싱을 끄고 VR을 사용하세요.

## 1.1.1


1.1.1: Clean/Dirty를 별도 ZIP으로 배포하고 흰색 손 표시는 공통으로 제공합니다. 접촉 변형은 Dirty 전용입니다. 비접촉 기능의 소스·버전·업데이트 경로를 공통으로 유지하며, 같은 버전의 명시적 종류 전환과 설정 보존을 지원합니다. 투영 보정의 수평축에만 의존하던 적용 조건을 안전한 수직축 근거까지 확대했습니다.

Clean/Dirty 전환·실패 복원 160개, 소유권 보호 525개, 폴더 배치 175개 검사를 새 Installer에서 통과했습니다. 공통 투영 보정은 183,984개 오프라인 검사와 기존 경로 회귀를 통과했습니다. 공개 소스에서 다시 빌드한 두 native DLL은 각각 배포 DLL과 바이트가 일치합니다. 설정 프로그램과 인스톨러는 필요한 .NET 10.0.12 런타임을 포함합니다. 오프라인·설치 검증과 실제 게임/HMD 수용은 별개입니다.

Quest 3와 Galaxy XR에서 보고된 양안 영상 어긋남은 안티앨리어싱이 원인인 것으로 확인됐습니다. 게임 설정에서 안티앨리어싱을 끄고 VR을 사용하세요.

[1.0.2](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.0.2) · [1.0.1](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.0.1) · [1.0.0](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.0.0)
