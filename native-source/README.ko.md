# 네이티브 런타임 소스

[한국어](README.ko.md) | [English](README.md) | [日本語](README.ja.md) | [简体中文](README.zh-CN.md)

## 소스 트리 안내

`variants/common/`과 `variants/<Flavor>/overlay/`가 현재1.2.0 배포 DLL의 소스입니다. Steam/Haoplay 빌드 프로필과 런타임 유도 훅 대상을 포함합니다. 재현 명령은 아래1.2.0 절을 참고하세요.

`src/`와 루트 `build-native.ps1`, `native-build.json`은 과거1.0.x `BOOTSTRAP-FLEX-001`(`6215CEAF…`)의 역사적 재현 전용입니다. 이 트리의 Haoplay 전용 상수(예: `src/stereo/stereo_hooks.h:48`)는 현재 빌드에서 사용되지 않으며, `variants/common/src/stereo/stereo_hooks.h`의 런타임 유도 대상으로 대체되었습니다. 바이트 재현을 위해 기존 소스는 수정하지 않습니다.

`variants/common/`의 `GF2VR_STEAM_COMPAT` 래퍼는 과거 참조 해시 재현을 위한 것으로, `#else` 분기는 배포 DLL에 포함되지 않습니다.

## 1.2.0 — Steam 지원 및 손·접촉 호환성 수정

Steam판 실행 파일 폴더와 기존 Haoplay 설치 구조를 지원합니다. 게임 업데이트 후 흰색 손 표시와 캐릭터 접촉이 사라지던 호환성 문제를 수정했습니다. Dirty는 활동층에서 동시에 보이는 지원 캐릭터들을 각각 접촉 대상으로 처리합니다. Clean에는 접촉 변형이 포함되지 않습니다.

실기 확인 범위는 Dirty의 Haoplay Main·Activity 손 표시/접촉과 Steam Main 실행·입체 화면입니다. Steam의 모든 화면과 Clean의 새 DLL 실기는 별도로 확인되지 않았습니다. 게임 내 안티앨리어싱은 OFF로 설정하세요.

정상적인 1.1.6 관리 설치는 설정 GUI의 업데이트 알림에서 승인해 1.2.0으로 업데이트할 수 있습니다. GitHub CLI·로그인이 필요하지 않으며 Clean·Dirty 종류와 사용자 설정을 유지합니다. 1.1.4 이하 또는 구버전 업데이트 확인이 안 되는 환경은 원하는 ZIP 전체를 새 폴더에 풀고 포함된 GF2VR.Installer.exe로 한 번 설치하세요. 게임을 종료한 뒤 진행하세요.

릴리즈 버전은 1.2.0입니다. 이전 미공개 후보의 내부 native 빌드 ID에 남은 130은 동일한 검증 DLL의 식별자입니다. 일반 설치 도구는 수동 교체되거나 소유권이 없는 파일을 자동 덮어쓰지 않습니다.

현재 1.2.0 native 빌드는 variants의 공통 소스 238개와 종류별 설정·생성 브리지 헤더를 사용합니다. Zig 0.16.0과 Windows SDK 10.0.26100.0으로 아래 명령을 실행하면 배포 DLL과 정확히 일치하며, 두 빌드의 일치하는 desktop-metadata가 생성됩니다. 루트 build-native.ps1은 과거 1.0.2 재현용입니다. 관리 브리지를 다시 생성하려면 현재 설치된 게임의 메타데이터가 필요하며, game-reference-profiles.json의 정확한 Haoplay/Steam 프로필을 검사합니다. 게임 파일은 저장소에 포함하지 않습니다.

```powershell
./variants/build-variants.ps1 -Zig 'C:/Tools/zig/zig.exe' -Output './build-1.2.0'
./variants/managed/build_touch_bridge.ps1 -NativeEyeCapacity 3 -Flavor Dirty -GameRoot '<game executable folder>' -GameProfile Auto -OutputDirectory './managed-1.2.0'
```

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


## 1.1.3 — Clean / Dirty

현재 1.1.3 네이티브 소스는 variants 아래의 공통 코드와 빌드별 생성 헤더로 구성합니다. 두 종류를 함께 재빌드하는 명령은 다음과 같습니다.

```powershell
./variants/build-variants.ps1 -Zig 'C:/Tools/zig/zig.exe' -Output './build-1.1.3'
```

1.1.3은 Clean/Dirty의 제품 소스와 별도 패키지를 공개합니다. native-source/variants/common은 공통 native 코드, Clean/Dirty overlay는 접촉 유무에 따른 생성 헤더, managed는 자체 제작 손 표시·접촉 코드입니다. 그 밖의 native-source 루트는 기존 1.0.2 소스·재현 스크립트를 보존합니다.

Quest 3와 Galaxy XR에서 보고된 양안 영상 어긋남은 안티앨리어싱이 원인인 것으로 확인됐습니다. 게임 설정에서 안티앨리어싱을 끄고 VR을 사용하세요.

1.1.3은 PR #1의 3DMigoto 기본 wrapper 모드 호환 변경을 Clean·Dirty에 함께 반영합니다. 게임의 wrapper는 유지하고 VR 복사·OpenXR에는 실제 D3D11 장치/context를 사용합니다. 변경된 DLL과 일치하는 설정 도구·인스톨러를 함께 제공합니다.

[3DMigoto](../docs/ko/3DMIGOTO_COMPATIBILITY.md) · [Build metadata](../docs/ko/BUILD_VARIANT_METADATA.md)

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
