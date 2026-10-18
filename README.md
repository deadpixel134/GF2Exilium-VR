# GF2VR

제작자: [@TBluebox12](https://x.com/TBluebox12)  
아카라이브 가상현실 채널: [가상현실 채널](https://arca.live/b/vrshits)  
후원: [buymeacoffee.com/vrshits](https://buymeacoffee.com/vrshits)  

[한국어](README.md) | [English](README.en.md) | [日本語](README.ja.md) | [简体中文](README.zh-CN.md)

GF2 Exilium을 VR로 즐기기 위한 비공식 모드입니다.

**[정식 버전 1.1.3 다운로드](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.3)**

> **필수: VR 사용 전 게임 설정에서 안티앨리어싱을 끄세요.**

1.1.3 업데이트: Clean 또는 Dirty ZIP 전체를 새 폴더에 풀고, 그 안의 GF2VR.Installer.exe로 기존 게임 위치에 설치하세요. 기존 1.1.2 설정 창의 자동 업데이트는 새 native 프로필을 검증하지 못해 거부됩니다. 기존 설치는 보존됩니다. DLL만 복사하거나 파일을 섞지 마세요.

## 기여자

- [rgbtl1248](https://github.com/rgbtl1248) — 3DMigoto 호환성 및 빌드 도구 개선 ([PR #1](https://github.com/deadpixel134/GF2Exilium-VR/pull/1)).

## 3DMigoto 호환성

1.1.3은 PR #1의 3DMigoto 기본 wrapper 모드 호환 변경을 Clean·Dirty에 함께 반영합니다. 게임의 wrapper는 유지하고 VR 복사·OpenXR에는 실제 D3D11 장치/context를 사용합니다. 변경된 DLL과 일치하는 설정 도구·인스톨러를 함께 제공합니다.

3DMigoto의 `[System]`에서 `hook`이 생략되었거나 주석 처리된 기본 wrapper 모드를 사용하세요. `hook=recommended` 및 `hook=all` 조합은 지원 범위 밖입니다. `calls`·`crash`는 기록/덤프 옵션이며 이 수정에 필수는 아닙니다. 스킨의 INI 문법·중복 규칙 경고는 별도 문제입니다.

[3DMigoto](docs/ko/3DMIGOTO_COMPATIBILITY.md) · [Build metadata](docs/ko/BUILD_VARIANT_METADATA.md)

## Clean과 Dirty 선택

| 기능 | Clean | Dirty |
|---|---|---|
| 일반 VR·입체 화면·컨트롤러 입력·설정 | 포함 | 포함 |
| 흰색 양손 구 표시 | 포함 | 포함 |
| 캐릭터 가슴·엉덩이 접촉 및 변형 | 미포함 | 포함 |

접촉은 해당 부위의 지원 뼈가 연결된 캐릭터에서 작동합니다. 캐릭터 모델에 따라 지원 부위가 다르며, 숙소·활동층에서의 실제 엉덩이 반응은 아직 확인되지 않았습니다.

두 빌드는 같은 일반 VR 소스와 버전으로 함께 업데이트합니다. 접촉 변형 코드만 Dirty에 포함하며, Clean은 단순히 접촉을 꺼 둔 버전이 아닙니다.

- [GF2VR-Native-SelfContained-Clean-1.1.3.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.3)
- [GF2VR-Native-SelfContained-Dirty-1.1.3.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.3)

둘 중 하나만 설치하세요. 두 빌드는 게임 실행 파일 옆의 같은 version.dll 한 개를 사용하며, OpenXR 로더·설정 프로그램 등 공통 지원 파일은 vrmod 아래에 설치합니다. 원본 GameAssembly.dll·UnityPlayer.dll·게임 에셋은 교체하지 않습니다. 두 빌드 모두 게임 프로세스에서 실행하므로 Clean이라는 이름이 미감지·안전을 보장하지 않습니다.

## 설치·전환·제거

1. 원하는 Clean 또는 Dirty ZIP을 새 폴더에 전부 풉니다. GitHub Source code ZIP은 설치 파일이 아닙니다.
2. 게임·설정·인스톨러를 종료합니다. 새 폴더의 GF2VR.Installer.exe에서 GF2_Exilium.exe 또는 게임 폴더를 선택합니다. 패키지 폴더는 자동으로 찾습니다.
3. 설치 종류와 버전을 확인하고 설치합니다. Clean↔Dirty 전환도 원하는 쪽 ZIP의 새 인스톨러로 진행합니다.
4. 설정 열기에서 저장한 뒤 게임을 평소 방법으로 직접 실행합니다. 흰색 손 표시는 기본 켜짐이며 손 표시 탭에서 바꿀 수 있습니다.

설치·전환·제거는 사용자 설정을 보존합니다. Dirty의 touch_interaction.json은 선택 사항이며 Clean은 읽지 않습니다. 제거는 기록된 직전 설치가 있으면 복원하고, 최초 설치에서는 소유 파일만 제거합니다. 수정되거나 소유권을 확인할 수 없는 DLL은 보존하며 덮어쓰지 않습니다. version.dll만 따로 복사하거나 서로 다른 패키지의 파일을 섞지 마세요.

## 검증 범위와 제한

기존 실기 확인 환경: Meta Quest 2 · 4K 모니터 · Virtual Desktop.

Quest 3와 Galaxy XR에서 보고된 양안 영상 어긋남은 안티앨리어싱이 원인인 것으로 확인됐습니다. 게임 설정에서 안티앨리어싱을 끄고 VR을 사용하세요.

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
