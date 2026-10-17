# 검증 범위와 제한

[한국어](../ko/VERIFICATION.md) | [English](../en/VERIFICATION.md) | [日本語](../ja/VERIFICATION.md) | [简体中文](../zh-CN/VERIFICATION.md)

> **필수: VR 사용 전 게임 설정에서 안티앨리어싱을 끄세요.**

VR DLL과 공통 설정 프로그램 payload는 1.1.1과 동일합니다. 이 버전은 인스톨러 안내만 갱신합니다.


기존 실기 확인 환경: Meta Quest 2 · 4K 모니터 · Virtual Desktop.

Clean/Dirty 전환·실패 복원 160개, 소유권 보호 525개, 폴더 배치 175개 검사를 새 Installer에서 통과했습니다. 공통 투영 보정은 183,984개 오프라인 검사와 기존 경로 회귀를 통과했습니다. 공개 소스에서 다시 빌드한 두 native DLL은 각각 배포 DLL과 바이트가 일치합니다. 설정 프로그램과 인스톨러는 필요한 .NET 10.0.12 런타임을 포함합니다. 오프라인·설치 검증과 실제 게임/HMD 수용은 별개입니다.

Quest 3와 Galaxy XR의 양안 영상 어긋남 제보는 아직 해결 확인 전입니다. Galaxy XR에서는 VDXR와 SteamVR 모두 제보됐습니다. 1.1.2는 인스톨러·문서 안내를 추가한 버전이며, VR DLL과 설정 프로그램은 1.1.1과 같습니다. 안티앨리어싱 해제 안내를 추가한 것이 두 기기의 해결 확인을 뜻하지는 않습니다.

둘 중 하나만 설치하세요. 두 빌드는 게임 실행 파일 옆의 같은 version.dll 한 개를 사용하며, OpenXR 로더·설정 프로그램 등 공통 지원 파일은 vrmod 아래에 설치합니다. 원본 GameAssembly.dll·UnityPlayer.dll·게임 에셋은 교체하지 않습니다. 두 빌드 모두 게임 프로세스에서 실행하므로 Clean이라는 이름이 미감지·안전을 보장하지 않습니다.
