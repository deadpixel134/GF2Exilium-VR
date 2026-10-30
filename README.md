# GF2VR

제작자: [@TBluebox12](https://x.com/TBluebox12)  
아카라이브 가상현실 채널: [가상현실 채널](https://arca.live/b/vrshits)  
후원: [buymeacoffee.com/vrshits](https://buymeacoffee.com/vrshits)  

[한국어](README.md) | [English](README.en.md) | [日本語](README.ja.md) | [简体中文](README.zh-CN.md)

소녀전선 2: 망명(GF2 Exilium)을 VR 헤드셋과 컨트롤러로 즐기기 위한 비공식 모드입니다. 일반 메뉴는 가상 화면으로, 지원되는 3D 장면은 입체 화면으로 표시합니다.

Haoplay와 Steam을 대상으로 합니다. 현재 3.0 업데이트 대응 빌드는 정적·자동 검증을 완료했으며, 실제 게임/HMD 동작은 아직 확인하지 않았습니다.

**[최신 릴리스 다운로드](https://github.com/deadpixel134/GF2Exilium-VR/releases/latest)**

> **필수: VR 사용 전 게임 설정에서 안티앨리어싱을 끄세요.**


[1.3.5 — Crewdeck 장면 전환 정리 수정](docs/release-notes/1.3.5.ko.md)

실제 게임/HMD에서 Crewdeck 해결 여부는 아직 확인하지 않았습니다. 모델 기반 회귀 검사 27개와 Clean/Dirty 빌드·패키지 검증을 완료했습니다.

## 무엇을 할 수 있나요?

머리 움직임으로 둘러보기, 스틱 이동·회전, 포인터 클릭과 드래그, 손에 여는 보조 패널을 제공합니다. 화질·월드 크기·입체감과 조작 방식을 설정할 수 있습니다. Dirty에는 지원 캐릭터의 가슴·엉덩이 접촉과 진동·흔들림 설정이 추가됩니다.

## 기여자

- [rgbtl1248](https://github.com/rgbtl1248) — 3DMigoto 호환성 및 빌드 도구 개선 ([PR #1](https://github.com/deadpixel134/GF2Exilium-VR/pull/1)).

## 3DMigoto 호환성

Clean·Dirty 모두 3DMigoto 기본 wrapper 모드와 호환됩니다([PR #1](https://github.com/deadpixel134/GF2Exilium-VR/pull/1)).

[3DMigoto](docs/ko/3DMIGOTO_COMPATIBILITY.md)

## 시작 전 준비

- 정상 실행되는 **Haoplay 또는 Steam판 게임**. 게임 본편은 포함하지 않습니다.
- VR 헤드셋·컨트롤러와 정상 연결된 **SteamVR/OpenXR 환경**.
- 게임 설정의 **안티앨리어싱 OFF**. Quest 3·Galaxy XR의 양안 영상 어긋남 원인으로 확인됐습니다.

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

둘 중 하나만 설치하세요. 두 빌드는 게임 실행 파일 옆의 같은 version.dll 한 개를 사용하며, OpenXR 로더·설정 프로그램 등 공통 지원 파일은 vrmod 아래에 설치합니다. 원본 GameAssembly.dll·UnityPlayer.dll·게임 에셋은 교체하지 않습니다. 두 빌드 모두 게임 프로세스에서 실행하므로 Clean이라는 이름이 미감지·안전을 보장하지 않습니다.

## 처음 설치하기

1. [최신 릴리스](https://github.com/deadpixel134/GF2Exilium-VR/releases/latest)에서 **Clean 또는 Dirty ZIP 하나**를 받아 새 폴더에 전부 풉니다. `Source code` ZIP은 설치 파일이 아닙니다.
2. 게임과 기존 설정·인스톨러를 종료합니다. 압축을 푼 폴더의 `GF2VR.Installer.exe`에서 `GF2_Exilium.exe` 또는 게임 폴더를 선택해 설치합니다.
3. **설정 열기**에서 옵션을 저장합니다. SteamVR와 헤드셋을 준비한 뒤 평소 방법으로 게임을 직접 실행합니다.

[설치·제거·복원](docs/ko/INSTALLATION.md) · [업데이트·Clean/Dirty 전환](docs/ko/UPDATE.md)

## 업데이트·복구·저장

Settings에서 새 버전을 확인하고 동의하면 업데이트합니다. 자동 적용이 불가능한 경우에도 버전 번호와 수동 설치 안내를 표시하고, 릴리스 페이지는 열기를 눌러야 열립니다. 버전별 알림을 다시 보지 않도록 선택할 수 있습니다.

유효한 설치 기록이 있는 삭제·격리 파일은 재설치할 수 있습니다. 변경 파일은 최신 ZIP의 Installer에서 **복구 설치**를 선택해 파일별 계획을 확인하고 동의하면 원본을 백업한 뒤 교체합니다. 소유가 확인되지 않은 파일은 기본 제외하며, 설치 기록·기존 설정·로그·백업은 보존합니다. 제품 설치 후 남은 후처리는 파일 재설치 없이 재시도할 수 있습니다.

설정 창은 저장 성공·실패·미저장 상태와 제목의 *****를 표시합니다. 저장 실패 시 변경은 편집 상태로 남으며, 미저장 상태에서 닫으면 확인합니다. 게임을 종료한 뒤 저장하고, 적용하려면 게임을 다시 시작하세요.

## 기본 조작

| 조작 | 동작 |
|---|---|
| 왼쪽 스틱 | VR 시점 이동 |
| 오른쪽 스틱 | 시야 회전. 기본은 한 번에 30°씩 도는 스냅 방식 |
| 오른손 포인터 | 게임 화면의 항목을 가리킵니다. |
| 오른손 Trigger 또는 A | 가리킨 항목 클릭 |
| 오른손 B | 뒤로 가기·취소 |
| 왼손 Grip | 3D 화면에서 보조 패널 열기·닫기 |

[게임 실행·전체 조작법](docs/ko/USAGE.md) · [설정 항목](docs/ko/SETTINGS.md)

## 호환성과 확인 범위

Haoplay는 Dirty의 Main·Activity 손 표시와 접촉, Steam은 Dirty의 Main 실행과 입체 화면을 실기로 확인했습니다. Steam의 다른 화면과 Clean의 새 DLL 실기는 아직 별도로 확인하지 않았습니다.

기존 실기 확인 환경: Meta Quest 2 · 4K 모니터 · Virtual Desktop.

Quest 3와 Galaxy XR에서 보고된 양안 영상 어긋남은 안티앨리어싱이 원인인 것으로 확인됐습니다. 게임 설정에서 안티앨리어싱을 끄고 VR을 사용하세요.

## 자세한 안내

- [설치·제거·복원](docs/ko/INSTALLATION.md)
- [게임 실행·전체 조작법](docs/ko/USAGE.md)
- [설정 항목](docs/ko/SETTINGS.md)
- [업데이트](docs/ko/UPDATE.md)
- [문제 해결](docs/ko/TROUBLESHOOTING.md)
- [로그 저장과 전달](docs/ko/LOGS.md)
- [다운로드·파일 구성](docs/ko/RELEASE_READINESS.md)
- [검증 범위](docs/ko/VERIFICATION.md)
- [3DMigoto 호환성](docs/ko/3DMIGOTO_COMPATIBILITY.md)
- [개발자 빌드 안내](docs/ko/BUILD_VARIANT_METADATA.md)
- [변경 이력](docs/ko/RELEASE_NOTES.md) · [GitHub Releases](https://github.com/deadpixel134/GF2Exilium-VR/releases)
- [소스 구성과 이용 조건](SOURCE_BOUNDARY.md)

> GF2VR은 비공식 팬 프로젝트입니다. 정식으로 설치한 게임이 필요하며, 게임의 상표·저작물은 각 권리자에게 있습니다.
