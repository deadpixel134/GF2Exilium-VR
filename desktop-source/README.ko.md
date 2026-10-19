# 데스크톱 앱 소스

[한국어](README.ko.md) | [English](README.md) | [日本語](README.ja.md) | [简体中文](README.zh-CN.md)

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

[Native metadata](../release/1.1.5-native-metadata.json)

## 1.1.4

Clean·Dirty 공통으로 보조패널의 사용자 바라보기, 위치·회전·화면 기울기·최대 크기를 설정할 수 있습니다. Dirty에는 캐릭터 터치 진동과 가슴·엉덩이별 흔들림 설정을 추가했습니다.

[Build metadata](../docs/ko/BUILD_VARIANT_METADATA.md) · [Native metadata](../release/1.1.4-native-metadata.json)


1.1.3은 PR #1의 3DMigoto 기본 wrapper 모드 호환 변경을 Clean·Dirty에 함께 반영합니다. 게임의 wrapper는 유지하고 VR 복사·OpenXR에는 실제 D3D11 장치/context를 사용합니다. 변경된 DLL과 일치하는 설정 도구·인스톨러를 함께 제공합니다.


두 빌드는 같은 일반 VR 소스와 버전으로 함께 업데이트합니다. 접촉 변형 코드만 Dirty에 포함하며, Clean은 단순히 접촉을 꺼 둔 버전이 아닙니다.

설치·전환·제거는 사용자 설정을 보존합니다. Dirty의 touch_interaction.json은 선택 사항이며 Clean은 읽지 않습니다. 제거는 기록된 직전 설치가 있으면 복원하고, 최초 설치에서는 소유 파일만 제거합니다. 수정되거나 소유권을 확인할 수 없는 DLL은 보존하며 덮어쓰지 않습니다. version.dll만 따로 복사하거나 서로 다른 패키지의 파일을 섞지 마세요.


설정 프로그램과 인스톨러는 Windows Forms, .NET SDK 10.0.112와 공식 .NET 10.0.12 Windows x64 런타임 팩을 사용합니다. PowerShell 7과 Windows x64가 필요합니다. 이 디렉터리에서 다음 명령을 실행하세요.

```powershell
./vrmod/tools/build_desktop_private_runtime.ps1 -Application Configurator -BuildId RELEASE-CLEAN-113 -ProductVersion 1.1.4
./vrmod/tools/build_desktop_private_runtime.ps1 -Application Installer -BuildId RELEASE-INSTALLER-113 -ProductVersion 1.1.4
```

첫 빌드는 저장소에 포함된 패키지 소스 설정으로 NuGet의 공식 런타임 팩을 복원합니다. 프로젝트 내부 캐시, 빌드 출력과 필요한 .NET 라이선스 고지를 `vrmod/build/` 아래에 만듭니다. 게임 파일이나 공용 시스템 .NET 설치는 변경하지 않습니다. 두 실행 프로그램 모두 전용 런타임을 포함하므로 앱 DLL, deps, runtimeconfig 파일을 EXE 옆에 함께 유지하세요.

`runtime-contract.json`은 현재 네이티브 모듈과 설정 문서 8개를 위한 설치 템플릿입니다. 인스톨러는 패키지를 검증하고 사용자가 선택한 설치 폴더에 연결된 계약 파일을 기록합니다. 이 템플릿은 개인 설정이나 설치가 완료된 독립 실행형 설정 프로그램이 아닙니다. 네이티브 패키지에는 릴리즈와 정확히 일치하는 네이티브 DLL, OpenXR 로더, 라이선스와 검증된 패키지 매니페스트도 필요합니다. 전체 패키지 계약을 확인하지 않고 게임 파일을 대체하거나 소스 빌드 출력을 정상 작동 중인 설치에 덮어쓰지 마세요.

앱 소스에는 로컬 검증 진입점(`--self-test`, `--verify-localization`)이 있습니다. 인스톨러의 패키지·이동 경로·업데이트 검증에는 전체 패키지와 명시적인 이전 버전 패키지가 필요합니다. 이 검증은 실행되지 않는 게임 대체 파일을 사용하며 VR DLL을 로드하지 않습니다. 릴리즈 기록에서는 이 검사와 실제 게임·HMD 수용을 구분합니다.

1.1.1은 폴더 구조와 관계없이 게임 실행 파일을 찾고, 원본 게임의 지문 차이를 설치 차단 조건에서 제외합니다. `GF2 Game/GF2_Exilium.exe` 구조에서는 기존 상위 폴더의 `vrmod`를 유지합니다. 그 외 구조에서는 찾은 실행 파일 폴더의 `vrmod`를 사용합니다. `version.dll`은 항상 해당 실행 파일 옆에 설치됩니다. 제품 패키지 무결성과 기존 파일 소유권 검사는 유지합니다. `--self-test-flexible`은 이 설치 경로의 게임 미실행 검증입니다.

[Build metadata](../docs/ko/BUILD_VARIANT_METADATA.md)
