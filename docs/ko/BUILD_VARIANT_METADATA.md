# 변경된 native 소스와 데스크톱 도구 빌드

[한국어](../ko/BUILD_VARIANT_METADATA.md) | [English](../en/BUILD_VARIANT_METADATA.md) | [日本語](../ja/BUILD_VARIANT_METADATA.md) | [简体中文](../zh-CN/BUILD_VARIANT_METADATA.md)

## 1.2.0 — Steam 지원 및 손·접촉 호환성 수정

Steam판 실행 파일 폴더와 기존 Haoplay 설치 구조를 지원합니다. 게임 업데이트 후 흰색 손 표시와 캐릭터 접촉이 사라지던 호환성 문제를 수정했습니다. Dirty는 활동층에서 동시에 보이는 지원 캐릭터들을 각각 접촉 대상으로 처리합니다. Clean에는 접촉 변형이 포함되지 않습니다.

실기 확인 범위는 Dirty의 Haoplay Main·Activity 손 표시/접촉과 Steam Main 실행·입체 화면입니다. Steam의 모든 화면과 Clean의 새 DLL 실기는 별도로 확인되지 않았습니다. 게임 내 안티앨리어싱은 OFF로 설정하세요.

정상적인 1.1.6 관리 설치는 설정 GUI의 업데이트 알림에서 승인해 1.2.0으로 업데이트할 수 있습니다. GitHub CLI·로그인이 필요하지 않으며 Clean·Dirty 종류와 사용자 설정을 유지합니다. 1.1.4 이하 또는 구버전 업데이트 확인이 안 되는 환경은 원하는 ZIP 전체를 새 폴더에 풀고 포함된 GF2VR.Installer.exe로 한 번 설치하세요. 게임을 종료한 뒤 진행하세요.

릴리즈 버전은 1.2.0입니다. 이전 미공개 후보의 내부 native 빌드 ID에 남은 130은 동일한 검증 DLL의 식별자입니다. 일반 설치 도구는 수동 교체되거나 소유권이 없는 파일을 자동 덮어쓰지 않습니다.

## 1.1.4

[1.1.4 native metadata](../../release/1.1.4-native-metadata.json) · [Source manifest](../../source-manifest.json)


Configurator와 Installer는 native DLL 해시를 엄격히 검사합니다. 검사 조건을 끄지 말고 두 native 빌드에서 생성한 동일한 메타데이터를 사용하세요. Windows x64, PowerShell 7, Zig 0.16.0과 Windows SDK가 필요합니다. 아래 명령은 저장소 루트에서 실행합니다.

정식 1.1.4 소스의 기본 빌드는 배포 해시와 정확히 일치해야 합니다. 의도적으로 소스를 바꾼 개발 빌드에만 -AllowSourceChanges를 추가하세요. 새 출력 폴더를 사용하고 두 종류를 함께 빌드하세요.

```powershell
./native-source/variants/build-variants.ps1 -Flavor Both `
    -Zig <zig.exe> -Kernel32Library <kernel32.lib> -Output ./native-source/variants/build
```

desktop-metadata에는 실제 DLL의 build ID/해시를 담은 VariantProfiles.generated.cs, Clean/Dirty 각각의 runtime-contract.json, common-source-manifest.txt와 metadata.json이 생성됩니다. 한 종류만 빌드하면 생성하지 않습니다. 누락된 DLL·보고서 해시 불일치·materialized 공통 소스 불일치는 거부합니다.

같은 메타데이터를 Installer와 두 Configurator에 전달하세요. 각 BuildId는 고유해야 합니다. 직접 MSBuild를 쓸 때는 GF2VRVariantMetadataRoot와 Configurator의 GF2VRBuildFlavor를 지정합니다. 속성을 생략하면 체크인된 정식 릴리즈 계약을 사용하므로 변경한 DLL과 섞지 마세요.

```powershell
$metadata = (Resolve-Path ./native-source/variants/build/desktop-metadata).Path
$desktop = './desktop-source/vrmod/tools/build_desktop_private_runtime.ps1'
& $desktop -Application Installer -BuildId SOURCE-INSTALLER-113 -ProductVersion 1.1.4 `
    -NativeMetadataRoot $metadata
& $desktop -Application Configurator -BuildId SOURCE-CLEAN-113 -ProductVersion 1.1.4 `
    -NativeMetadataRoot $metadata -NativeFlavor Clean
& $desktop -Application Configurator -BuildId SOURCE-DIRTY-113 -ProductVersion 1.1.4 `
    -NativeMetadataRoot $metadata -NativeFlavor Dirty
```

선택한 DLL·해당 runtime contract·같은 profiles로 빌드한 Installer를 묶으세요. metadata.json의 native buildId/dllSha256/buildFlavor/commonCoreSha256를 package manifest에 반영하고 모든 payload 크기·SHA-256을 다시 계산한 뒤 정상 패키지 검사를 통과해야 합니다. 이 생성기는 ZIP이나 릴리즈를 만들지 않습니다. INSTALL_ROOT/vrmod/config는 설치할 때 선택한 경로로 바인딩하며, 설치 전 템플릿을 Configurator가 거부하는 것은 정상입니다.

## commonCoreSha256

GF2VR-COMMON-SOURCE-MANIFEST-V1은 common 아래 일반 파일의 상대 경로를 /로 정규화하고 ordinal 순서로 정렬합니다. UTF-8 BOM 없이 아래 header와 각 행을 기록합니다. \t는 tab, \n은 LF입니다. 이 바이트의 대문자 SHA-256이 commonCoreSha256입니다. 예전 AFBB… 규칙의 재현이 아니며 overlay·컴파일 출력·실행 성공을 증명하지 않습니다. DLL은 별도 실제 해시로 검증합니다.

```text
GF2VR-COMMON-SOURCE-MANIFEST-V1\n
relative/path\tUPPERCASE_FILE_SHA256\n
```
