# GF2VR

[@TBluebox12](https://x.com/TBluebox12) · [가상현실 채널](https://arca.live/b/vrshits) · [buymeacoffee.com/vrshits](https://buymeacoffee.com/vrshits)

[한국어](README.md) | [English](README.en.md) | [日本語](README.ja.md) | [简体中文](README.zh-CN.md)

GF2 Exilium을 VR로 즐기기 위한 비공식 모드입니다.

**[정식 버전 1.1.2 다운로드](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.2)**

> **필수: VR 사용 전 게임 설정에서 안티앨리어싱을 끄세요.**

## Clean과 Dirty 선택

| 기능 | Clean | Dirty |
|---|---|---|
| 일반 VR·입체 화면·컨트롤러 입력·설정 | 포함 | 포함 |
| 흰색 양손 구 표시 | 포함 | 포함 |
| 캐릭터 가슴 접촉·변형 | 미포함 | 포함 |

두 빌드는 같은 일반 VR 소스와 버전으로 함께 업데이트합니다. 접촉 변형 코드만 Dirty에 포함하며, Clean은 단순히 접촉을 꺼 둔 버전이 아닙니다.

- [GF2VR-Native-SelfContained-Clean-1.1.2.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.2)
- [GF2VR-Native-SelfContained-Dirty-1.1.2.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.2)

둘 중 하나만 설치하세요. 두 빌드는 게임 실행 파일 옆의 같은 version.dll 한 개를 사용하며, OpenXR 로더·설정 프로그램 등 공통 지원 파일은 vrmod 아래에 설치합니다. 원본 GameAssembly.dll·UnityPlayer.dll·게임 에셋은 교체하지 않습니다. 두 빌드 모두 게임 프로세스에서 실행하므로 Clean이라는 이름이 미감지·안전을 보장하지 않습니다.

## 설치·전환·제거

1. 원하는 Clean 또는 Dirty ZIP을 새 폴더에 전부 풉니다. GitHub Source code ZIP은 설치 파일이 아닙니다.
2. 게임·설정·인스톨러를 종료합니다. 새 폴더의 GF2VR.Installer.exe에서 GF2_Exilium.exe 또는 게임 폴더를 선택합니다. 패키지 폴더는 자동으로 찾습니다.
3. 설치 종류와 버전을 확인하고 설치합니다. Clean↔Dirty 전환도 원하는 쪽 ZIP의 새 인스톨러로 진행합니다.
4. 설정 열기에서 저장한 뒤 게임을 평소 방법으로 직접 실행합니다. 흰색 손 표시는 기본 켜짐이며 손 표시 탭에서 바꿀 수 있습니다.

설치·전환·제거는 사용자 설정을 보존합니다. Dirty의 touch_interaction.json은 선택 사항이며 Clean은 읽지 않습니다. 제거는 기록된 직전 설치가 있으면 복원하고, 최초 설치에서는 소유 파일만 제거합니다. 수정되거나 소유권을 확인할 수 없는 DLL은 보존하며 덮어쓰지 않습니다. version.dll만 따로 복사하거나 서로 다른 패키지의 파일을 섞지 마세요.

## 검증 범위와 제한

기존 실기 확인 환경: Meta Quest 2 · 4K 모니터 · Virtual Desktop.

Quest 3와 Galaxy XR의 양안 영상 어긋남 제보는 아직 해결 확인 전입니다. Galaxy XR에서는 VDXR와 SteamVR 모두 제보됐습니다. 1.1.2는 인스톨러·문서 안내를 추가한 버전이며, VR DLL과 설정 프로그램은 1.1.1과 같습니다. 안티앨리어싱 해제 안내를 추가한 것이 두 기기의 해결 확인을 뜻하지는 않습니다.

## 사용 안내

- [설치·제거·복원](docs/ko/INSTALLATION.md)
- [기본 조작](docs/ko/USAGE.md)
- [설정](docs/ko/SETTINGS.md)
- [업데이트](docs/ko/UPDATE.md)
- [문제 해결](docs/ko/TROUBLESHOOTING.md)
- [변경 사항](docs/ko/RELEASE_NOTES.md)
- [다운로드·파일 구성](docs/ko/RELEASE_READINESS.md)
- [검증 범위](docs/ko/VERIFICATION.md)
- [소스 구성과 이용 조건](SOURCE_BOUNDARY.md)

> GF2VR은 비공식 팬 프로젝트입니다. 정식으로 설치한 게임이 필요하며, 게임의 상표·저작물은 각 권리자에게 있습니다.
