# 데스크톱 앱 소스

[한국어](README.ko.md) | [English](README.md) | [日本語](README.ja.md) | [简体中文](README.zh-CN.md)

설정 프로그램과 인스톨러는 Windows Forms, .NET SDK 10.0.112와 공식 .NET 10.0.12 Windows x64 런타임 팩을 사용합니다. PowerShell 7과 Windows x64가 필요합니다. 이 디렉터리에서 다음 명령을 실행하세요.

```powershell
./vrmod/tools/build_desktop_private_runtime.ps1 -Application Configurator -BuildId STABLE-DESKTOP-001 -ProductVersion 1.0.0
./vrmod/tools/build_desktop_private_runtime.ps1 -Application Installer -BuildId STABLE-DESKTOP-001 -ProductVersion 1.0.0
```

첫 빌드는 저장소에 포함된 패키지 소스 설정으로 NuGet의 공식 런타임 팩을 복원합니다. 프로젝트 내부 캐시, 빌드 출력과 필요한 .NET 라이선스 고지를 `vrmod/build/` 아래에 만듭니다. 게임 파일이나 공용 시스템 .NET 설치는 변경하지 않습니다. 두 실행 프로그램 모두 전용 런타임을 포함하므로 앱 DLL, deps, runtimeconfig 파일을 EXE 옆에 함께 유지하세요.

`runtime-contract.json`은 현재 네이티브 모듈과 설정 문서 7개를 위한 설치 템플릿입니다. 인스톨러는 패키지를 검증하고 사용자가 선택한 설치 폴더에 연결된 계약 파일을 기록합니다. 이 템플릿은 개인 설정이나 설치가 완료된 독립 실행형 설정 프로그램이 아닙니다. 네이티브 패키지에는 릴리즈와 정확히 일치하는 네이티브 DLL, OpenXR 로더, 라이선스와 검증된 패키지 매니페스트도 필요합니다. 전체 패키지 계약을 확인하지 않고 게임 파일을 대체하거나 소스 빌드 출력을 정상 작동 중인 설치에 덮어쓰지 마세요.

앱 소스에는 로컬 검증 진입점(`--self-test`, `--verify-localization`)이 있습니다. 인스톨러의 패키지·이동 경로·업데이트 검증에는 전체 패키지와 명시적인 이전 버전 패키지가 필요합니다. 이 검증은 실행되지 않는 게임 대체 파일을 사용하며 VR DLL을 로드하지 않습니다. 릴리즈 기록에서는 이 검사와 실제 게임·HMD 수용을 구분합니다.
