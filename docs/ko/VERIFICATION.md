# 검토 및 검증

[한국어](../ko/VERIFICATION.md) | [English](../en/VERIFICATION.md) | [日本語](../ja/VERIFICATION.md) | [简体中文](../zh-CN/VERIFICATION.md)

## 1.1.4

Clean/Dirty native가 각각 동일 소스 재빌드와 바이트 단위로 일치합니다. native 배치·설정 4,517개, GUI Clean 706개/Dirty 941개, 부위별 기본값과 직전 실제 IL의 명시적 설정 동작 일치 236개, 실제 GUI 기본 JSON 236개, 설치·전환·실패 복원 159개와 직전 설치의 정확한 복원 11개를 통과했습니다. bridge 131개 메서드와 contact hot IL 46개를 검사했습니다. 오프라인·모의 파일·개인용 설치 검증이며 새 게임/HMD 실행을 수용 증거로 주장하지 않습니다.


> **필수: VR 사용 전 게임 설정에서 안티앨리어싱을 끄세요.**

PR 작성자는 Dirty 개발 빌드에서 VR·스킨 동시 작동을 세 번 연속 확인했다고 보고했습니다. 이는 작성자의 기록이며 새 1.1.3 바이너리의 HMD 수용을 대신하지 않습니다. 이번 릴리즈는 양쪽 native 빌드, 실제 소스의 COM 장치/실패 경로 검사, 설정 도구·인스톨러 빌드와 패키지 무결성을 검증합니다. 새 게임/HMD 실행은 하지 않았습니다.

Quest 3와 Galaxy XR에서 보고된 양안 영상 어긋남은 안티앨리어싱이 원인인 것으로 확인됐습니다. 게임 설정에서 안티앨리어싱을 끄고 VR을 사용하세요.

3DMigoto 기본 wrapper 모드 호환 변경과 일치하는 빌드 메타데이터를 제공합니다. 일반 VR·손 표시는 공통이며 접촉 변형은 Dirty 전용입니다. 기존 설정과 설치 소유권/복원 보호를 유지합니다.

[3DMigoto](3DMIGOTO_COMPATIBILITY.md) · [Build metadata](BUILD_VARIANT_METADATA.md)

기여: [rgbtl1248](https://github.com/rgbtl1248), [PR #1](https://github.com/deadpixel134/GF2Exilium-VR/pull/1).

1.1.3 업데이트: Clean 또는 Dirty ZIP 전체를 새 폴더에 풀고, 그 안의 GF2VR.Installer.exe로 기존 게임 위치에 설치하세요. 기존 1.1.2 설정 창의 자동 업데이트는 새 native 프로필을 검증하지 못해 거부됩니다. 기존 설치는 보존됩니다. DLL만 복사하거나 파일을 섞지 마세요.

1.1.3 검증: COM 47개, Clean/Dirty 설치·전환·복원 160개, 소유권 525개, 폴더 배치 175개, 패키지 위치 12개, 인스톨러 UX 19개, 설정 도구 각 577개, 1.1.2 → 1.1.3 → 정확한 복원 28개를 통과했습니다. 오프라인·모의 파일 검사이며 실제 게임/HMD 수용과 구분합니다.
