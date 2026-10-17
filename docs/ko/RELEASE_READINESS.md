# 정식 버전 1.1.1 다운로드

[한국어](../ko/RELEASE_READINESS.md) | [English](../en/RELEASE_READINESS.md) | [日本語](../ja/RELEASE_READINESS.md) | [简体中文](../zh-CN/RELEASE_READINESS.md)

- [GF2VR-Native-SelfContained-Clean-1.1.1.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.1)
- [GF2VR-Native-SelfContained-Dirty-1.1.1.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.1)

| 기능 | Clean | Dirty |
|---|---|---|
| 일반 VR·입체 화면·컨트롤러 입력·설정 | 포함 | 포함 |
| 흰색 양손 구 표시 | 포함 | 포함 |
| 캐릭터 가슴 접촉·변형 | 미포함 | 포함 |

두 빌드는 같은 일반 VR 소스와 버전으로 함께 업데이트합니다. 접촉 변형 코드만 Dirty에 포함하며, Clean은 단순히 접촉을 꺼 둔 버전이 아닙니다.

둘 중 하나만 설치하세요. 두 빌드는 게임 실행 파일 옆의 같은 version.dll 한 개를 사용하며, OpenXR 로더·설정 프로그램 등 공통 지원 파일은 vrmod 아래에 설치합니다. 원본 GameAssembly.dll·UnityPlayer.dll·게임 에셋은 교체하지 않습니다. 두 빌드 모두 게임 프로세스에서 실행하므로 Clean이라는 이름이 미감지·안전을 보장하지 않습니다.

설치·전환·제거는 사용자 설정을 보존합니다. Dirty의 touch_interaction.json은 선택 사항이며 Clean은 읽지 않습니다. 제거는 기록된 직전 설치가 있으면 복원하고, 최초 설치에서는 소유 파일만 제거합니다. 수정되거나 소유권을 확인할 수 없는 DLL은 보존하며 덮어쓰지 않습니다. version.dll만 따로 복사하거나 서로 다른 패키지의 파일을 섞지 마세요.

Clean/Dirty 전환·실패 복원 160개, 소유권 보호 525개, 폴더 배치 175개 검사를 새 Installer에서 통과했습니다. 공통 투영 보정은 183,984개 오프라인 검사와 기존 경로 회귀를 통과했습니다. 공개 소스에서 다시 빌드한 두 native DLL은 각각 배포 DLL과 바이트가 일치합니다. 설정 프로그램과 인스톨러는 필요한 .NET 10.0.12 런타임을 포함합니다. 오프라인·설치 검증과 실제 게임/HMD 수용은 별개입니다.

Galaxy XR에서 VDXR와 SteamVR 모두 좌우 영상이 어긋난다는 제보는 아직 해결 확인 전입니다. 1.1.1의 공통 투영 보정 적용 범위 개선은 오프라인 검증을 통과했지만, 이 증상의 해결이나 전체 헤드셋 호환성을 보장하지 않습니다. 접촉 기능은 이전 사용자 실기에서 확인했으며, 이 배포 바이너리 자체의 새 게임/HMD 실기는 진행하지 않았습니다.

CHECKSUMS.sha256 · GF2VR-Clean-update.json · GF2VR-Dirty-update.json
