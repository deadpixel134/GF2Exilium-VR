# 로컬 소스·배포 경계

이 폴더는 private 저장소에 준비한 **데스크톱 도구 소스 스냅샷과 문서 초안**입니다. 전체 게임-root를 Git 저장소나 release payload로 취급하지 않습니다. 전체 native VR 모드의 재빌드 가능한 소스 릴리스도 아닙니다.

## 허용 범위

`source-manifest.json`의 명시된 항목만 소스 ZIP에 들어갑니다.

- 현재0.4.0 근거에 묶인 Installer/Configurator 프로젝트·C#·Directory.Build.props·ko/en/ja 자원.
- desktop private runtime publish targets, 공식 NuGet 설정, desktop build 스크립트.
- 이 폴더의 README와 한국어 사용자 문서, 배포 경계, package-supplied .NET notices.
- Configurator의 source runtime-contract는 개발 root/설치 진단 값을 복사하지 않고0.4.0 제품의 `INSTALL_ROOT/vrmod/config` template로 대체. 설치 후 실제 계약 생성이 필요.

game-root 위치에 묶인 개발 launcher, 현재 사용자의 설정 파일, 실제 fixture 결과/로그, frozen rollback 디렉터리는 복사하지 않습니다. 기존 machine-local profile을 인식하는 **제품 코드 상수**와 source runtime-contract/개인 설정 파일은 구분합니다.

## 제외 / 아직 완전하지 않은 부분

게임 실행 파일·UnityPlayer/GameAssembly·게임 assets/metadata/cache/Codes, reference 프로젝트, 로그, 개인 runtime/UI 설정, rollback/transaction 자료, 인증 정보, NuGet/SDK cache, VS/Codex/Manager 기록, screenshot은 포함하지 않습니다. runtime/패키지 ZIP은 문서/소스 ZIP에 다시 넣지 않습니다.

native VR source/encoded managed package/전체 빌드 및 third-party/native SDK source의 publication 계약은 아직 여기서 확정하지 않았습니다. desktop 빌드를 지원하는 코드 스냅샷이며 native 재빌드·0.4.0 전체 package 재생성을 약속하지 않습니다. 제품 ZIP의 hash와 코드 스냅샷의 파일 hash를 동일한 수용/인증으로 취급하지 않습니다.

Microsoft source나 runtime pack 자체를 소스 스냅샷에 복사하지 않습니다. desktop build 스크립트는 공식 NuGet에서 필요한 팩을 받아 notices를 생성합니다. 포함된 .NET 고지는 그 패키지에서 온 것입니다. 프로젝트 자체의 source 배포 라이선스는 별도로 결정해야 하며, third-party MIT 고지를 이 프로젝트 전체의 라이선스로 표시하지 않습니다.

## desktop 빌드 범위

지원 .NET10 SDK와 PowerShell7이 있는 Windows x64 개발 환경에서 스냅샷 root의 `vrmod/tools/build_desktop_private_runtime.ps1 -Application Configurator` 및 `-Application Installer`로 desktop 앱을 publish할 수 있습니다. 네트워크 접근이 필요할 수 있으며 NuGet source는 공식nuget.org로 한정합니다. 앱을 실제 게임에서 사용하려면 검증된 provider와 올바른 설치 계약이 필요합니다. 이 문서 작업에서는 source ZIP으로 native/game을 실행하거나 재빌드하지 않습니다.

## 외부 배포를 선택할 때

저장소 이름/owner/private visibility, private release 접근 방식, 게시할 완전한 소스 범위와 프로젝트 라이선스, 실제 지원/수용 상태를 먼저 결정해야 합니다. token/private key/자격 증명을 앱·소스·문서에 넣지 않습니다. 원본 게임 폴더에서 바로 git add/push하는 절차는 없습니다. 현재 저장소는 deadpixel134/GF2Exilium-VR(private·본인용)입니다. 공개 전환, native binary release, 다운로드/updater endpoint와 프로젝트 라이선스는 아직 정하지 않았습니다.
