# GF2VR

제작자: [@TBluebox12](https://x.com/TBluebox12)  
아카라이브 가상현실 채널: [가상현실 채널](https://arca.live/b/vrshits)  
후원: [buymeacoffee.com/vrshits](https://buymeacoffee.com/vrshits)  

[한국어](README.md) | [English](README.en.md) | [日本語](README.ja.md) | [简体中文](README.zh-CN.md)

GF2 Exilium을 VR로 즐기기 위한 비공식 모드입니다.

**[정식 버전 1.2.0 다운로드](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.2.0)**

> **필수: VR 사용 전 게임 설정에서 안티앨리어싱을 끄세요.**

## 1.2.0 — Steam 지원 및 손·접촉 호환성 수정

Steam판 실행 파일 폴더와 기존 Haoplay 설치 구조를 지원합니다. 게임 업데이트 후 흰색 손 표시와 캐릭터 접촉이 사라지던 호환성 문제를 수정했습니다. Dirty는 활동층에서 동시에 보이는 지원 캐릭터들을 각각 접촉 대상으로 처리합니다. Clean에는 접촉 변형이 포함되지 않습니다.

실기 확인 범위는 Dirty의 Haoplay Main·Activity 손 표시/접촉과 Steam Main 실행·입체 화면입니다. Steam의 모든 화면과 Clean의 새 DLL 실기는 별도로 확인되지 않았습니다. 게임 내 안티앨리어싱은 OFF로 설정하세요.

정상적인 1.1.6 관리 설치는 설정 GUI의 업데이트 알림에서 승인해 1.2.0으로 업데이트할 수 있습니다. GitHub CLI·로그인이 필요하지 않으며 Clean·Dirty 종류와 사용자 설정을 유지합니다. 1.1.4 이하 또는 구버전 업데이트 확인이 안 되는 환경은 원하는 ZIP 전체를 새 폴더에 풀고 포함된 GF2VR.Installer.exe로 한 번 설치하세요. 게임을 종료한 뒤 진행하세요.

릴리즈 버전은 1.2.0입니다. 이전 미공개 후보의 내부 native 빌드 ID에 남은 130은 동일한 검증 DLL의 식별자입니다. 일반 설치 도구는 수동 교체되거나 소유권이 없는 파일을 자동 덮어쓰지 않습니다.

## 1.1.6 — 자동 업데이트 수정

설정 GUI 실행 시 공개 GitHub 릴리즈를 자동 확인하고, 알림의 **업데이트**를 승인하면 다운로드·검증·설치합니다. 1.1.6부터 GitHub CLI 설치나 로그인이 필요하지 않습니다.

새 빌드를 구버전의 고정 허용 목록으로 검사하던 문제를 수정했습니다. 기존 프로그램은 릴리즈·파일 해시·설치 종류와 소유권을 확인하고, 검증된 새 인스톨러가 최종 빌드 검증과 백업·설치를 처리합니다. Clean·Dirty 종류와 설정을 유지합니다.

1.1.4 이하는 이번 한 번 원하는 Clean·Dirty ZIP 전체를 새 폴더에 풀고 새 GF2VR.Installer.exe로 업데이트하세요. 기존 1.1.5에서 업데이트 확인이 동작하는 환경은 GUI 업데이트를 사용할 수 있습니다. 구버전에 필요한 GitHub CLI가 없는 경우도 이번 한 번 새 ZIP으로 설치하면 됩니다. 1.1.6을 설치한 뒤부터는 CLI·로그인 없이 GUI에서 업데이트합니다.

이번 버전은 VR DLL을 1.1.5와 동일하게 유지합니다. 로그 기본값 OFF, 명시적 ON 설정과 최초 정리 제외, 보조패널·접촉 설정을 보존합니다. 게임과 설정 창을 종료한 상태에서 업데이트하세요.

## 1.1.5 — 로그 기록 설정

Clean·Dirty 공통으로 설정 GUI의 **진단 → 로그 자동 기록**을 추가했습니다. 기본값은 **OFF**이며 기존 설정에 해당 값이 없어도 OFF입니다. 저장 후 게임을 다시 실행하면 적용됩니다.

이번 버전으로 처음 업데이트 설치할 때 로그가 OFF인 사용자의 이전 VR 자동 로그를 한 번만 정리합니다. **로그 ON 사용자는 기존 로그를 보존하며 이 최초 정리에서 제외**됩니다. 이후 OFF로 바꿔도 과거 로그를 뒤늦게 삭제하지 않습니다. 설정·다른 모드 파일·수동 수집 자료는 보존합니다.

문제 분석이 필요할 때만 로그 기록을 켜세요. ON에서는 같은 빌드의 로그에 계속 추가하므로 용량이 커질 수 있습니다.

1.1.5 업데이트는 원하는 Clean·Dirty ZIP 전체를 새 폴더에 풀고 포함된 새 GF2VR.Installer.exe로 진행하세요. 이전 버전 설정 창은 새 native 프로필을 거부할 수 있습니다. 사용자 설정은 보존됩니다.

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
| 수동 보조패널 방향·위치·회전·기울기 설정 | 포함 | 포함 |
| 터치 진동·흔들림 및 부위별 설정 | 미포함 | 포함 |

접촉은 해당 부위의 지원 뼈가 연결된 캐릭터에서 작동합니다. 캐릭터 모델에 따라 지원 부위가 다르며, 숙소·활동층에서의 실제 엉덩이 반응은 아직 확인되지 않았습니다.

두 빌드는 같은 일반 VR 소스와 버전으로 함께 업데이트합니다. 접촉 변형 코드만 Dirty에 포함하며, Clean은 단순히 접촉을 꺼 둔 버전이 아닙니다.

- [GF2VR-Native-SelfContained-Clean-1.2.0.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.2.0)
- [GF2VR-Native-SelfContained-Dirty-1.2.0.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.2.0)

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
