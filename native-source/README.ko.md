# 네이티브 런타임 소스

[한국어](README.ko.md) | [English](README.md) | [日本語](README.ja.md) | [简体中文](README.zh-CN.md)

## 1.1.1 — Clean / Dirty

현재 1.1.1 네이티브 소스는 variants 아래의 공통 코드와 빌드별 생성 헤더로 구성합니다. 두 종류를 함께 재빌드하는 명령은 다음과 같습니다.

```powershell
./variants/build-variants.ps1 -Zig 'C:/Tools/zig/zig.exe' -Output './build-1.1.1'
```

1.1.1은 Clean/Dirty의 제품 소스와 별도 패키지를 공개합니다. native-source/variants/common은 공통 native 코드, Clean/Dirty overlay는 접촉 유무에 따른 생성 헤더, managed는 자체 제작 손 표시·접촉 코드입니다. 그 밖의 native-source 루트는 기존 1.0.2 소스·재현 스크립트를 보존합니다.

Galaxy XR에서 VDXR와 SteamVR 모두 좌우 영상이 어긋난다는 제보는 아직 해결 확인 전입니다. 1.1.1의 공통 투영 보정 적용 범위 개선은 오프라인 검증을 통과했지만, 이 증상의 해결이나 전체 헤드셋 호환성을 보장하지 않습니다. 접촉 기능은 이전 사용자 실기에서 확인했으며, 이 배포 바이너리 자체의 새 게임/HMD 실기는 진행하지 않았습니다.

---


GF2VR 1.0.2에 포함된 네이티브 런타임 `BOOTSTRAP-FLEX-001`을 빌드하는 데 필요한 소스 묶음입니다. DLL의 SHA-256은 다음과 같습니다.

```text
6215CEAFFFA46A3B89336DA73203122DD0095977206FDB395A1259F967B54771
```

Windows x64에서 PowerShell 7, Zig 0.16.0과 Windows 10 SDK 10.0.26100.0의 x64 `kernel32.lib`를 사용해 빌드합니다.

```powershell
./build-native.ps1 -Zig 'C:/Tools/zig/zig.exe' -Kernel32Library 'C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64/kernel32.lib' -Output './build'
```

빌더는 출력 디렉터리를 Zig 캐시에 사용하고 작업 후 호출자의 캐시 설정을 복원합니다. 런타임 지원 코드 11개와 MinHook 코드 4개를 컴파일하고 런타임을 링크한 뒤, 릴리즈와 해시가 다른 DLL은 거부합니다. `build-result.json`에는 객체 파일 해시, 컴파일러 해시, 최종 인수와 DLL 비교 결과가 기록됩니다. 이 빌드는 DLL을 설치하거나 로드하지 않으며 게임이나 XR 런타임을 실행하지 않습니다.

`native-build.json`에는 정확한 빌드 플래그와 소스 해시가 기록되어 있습니다. `src/`에는 현재 프로젝트 소유의 네이티브 코드와 빌드에서 사용하는 생성 패키지 헤더가 있습니다. `managed/ActivityProducerEngineBridge.cs`는 내장된 관리 코드 패키지에 대응하는 프로젝트 소유의 관리 브리지 소스이며 검토와 개발을 위해 제공합니다. 이 네이티브 DLL을 다시 빌드할 때는 저장소의 생성 패키지를 사용하므로 게임 어셈블리가 필요하지 않습니다. 관리 코드 패키지의 재생성은 별도의 개발 작업으로, 합법적으로 확보한 일치하는 게임 어셈블리가 필요합니다. 해당 어셈블리와 게임 에셋은 배포하지 않습니다.

OpenXR와 MinHook 소스 일부의 라이선스는 `third_party/` 아래에 유지되어 있습니다. 게임 실행 파일, Unity·GameAssembly 바이너리, 게임 메타데이터·에셋, 사용자 설정·로그·복구 파일은 이 소스 배포본에 포함되지 않습니다.

1.0.2의 경로 변경은 실행 파일 옆의 로더와 위 구성의 `vrmod`를 찾는 부분에 한정합니다. `GF2 Game/GF2_Exilium.exe` 구조에서는 기존 상위 폴더의 `vrmod`를 유지합니다. 그 외 구조에서는 찾은 실행 파일 폴더의 `vrmod`를 사용합니다. `version.dll`은 항상 해당 실행 파일 옆에 설치됩니다.
