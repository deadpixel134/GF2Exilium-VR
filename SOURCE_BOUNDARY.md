# 소스 구성과 이용 조건

[한국어](SOURCE_BOUNDARY.md) | [English](SOURCE_BOUNDARY.en.md) | [日本語](SOURCE_BOUNDARY.ja.md) | [简体中文](SOURCE_BOUNDARY.zh-CN.md)

1.1.1은 Clean/Dirty의 제품 소스와 별도 패키지를 공개합니다. native-source/variants/common은 공통 native 코드, Clean/Dirty overlay는 접촉 유무에 따른 생성 헤더, managed는 자체 제작 손 표시·접촉 코드입니다. 그 밖의 native-source 루트는 기존 1.0.2 소스·재현 스크립트를 보존합니다.


설치하려면 [1.1.1 릴리스](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.2)의 **GF2VR-Native-SelfContained-Clean-1.1.2.zip / GF2VR-Native-SelfContained-Dirty-1.1.2.zip**을 사용하세요. GitHub의 **Source code** ZIP에는 빌드 전 소스가 들어 있으며, 압축을 풀어 게임 폴더에 복사하는 것만으로 설치되지 않습니다.

## 저장소에 포함된 자료

| 위치 | 내용 |
|---|---|
| `native-source/` | 네이티브 VR 모듈의 소스, 자체 제작 관리 코드와 생성 헤더, 빌드 설정·스크립트, 필요한 외부 헤더와 고지 |
| `desktop-source/` | 설정 프로그램·인스톨러 소스와 한국어·영어·일본어 리소스 |
| `docs/ko/` | 설치·조작·설정·업데이트·문제 해결 안내 |
| `source-manifest.json` | 공개 소스의 파일 목록과 무결성 정보 |

원본 게임 실행 파일·DLL·에셋, 개발 참고 프로젝트, 사용자 설정·로그·복구 파일, 인증 정보와 개발 도구 캐시는 배포 대상에 포함하지 않습니다. 게임 라이브러리를 참조해 만든 자체 브리지 코드와 생성 헤더는 원본 게임 라이브러리와 구분해 관리합니다.

## 릴리스 구성

설치 ZIP에는 네이티브 모듈, OpenXR 로더, 설정 프로그램, 인스톨러와 외부 소프트웨어 고지가 들어 있습니다. 설정 프로그램과 인스톨러는 필요한 .NET 10.0.12 런타임을 함께 배포합니다. 별도의 .NET 설치를 요구하지 않는 구성입니다.

런타임·설정 프로그램·설치 메타데이터는 한 버전의 묶음으로 사용하세요. 일부 파일을 다른 버전의 파일과 섞으면 파일 검사나 실행이 실패할 수 있습니다. 원본 게임 파일은 사용자가 정식 설치한 게임에서 확인하며 설치 ZIP에 넣지 않습니다.

`GF2 Game/GF2_Exilium.exe` 구조에서는 기존 상위 폴더의 `vrmod`를 유지합니다. 그 외 구조에서는 찾은 실행 파일 폴더의 `vrmod`를 사용합니다. `version.dll`은 항상 해당 실행 파일 옆에 설치됩니다. 원본 게임 파일은 읽기만 하며 지문 차이는 설치 차단 조건이 아닙니다. 제품 패키지 해시, 파일 소유권, 실행 중인 게임 차단과 이전 버전 복원·중단 복구 검사는 유지합니다. 실기 확인과 배포용 바이너리의 로컬 검증은 [확인 범위](docs/ko/VERIFICATION.md)에 구분해 기록합니다.

## 이용 조건

프로젝트 소스의 배포 라이선스는 아직 정하지 않았습니다. 저장소가 공개되어 있다는 것만으로 자유로운 재배포나 변경 배포를 허용하는 것은 아닙니다.

OpenXR, MinHook 및 Microsoft .NET에는 각 소프트웨어의 라이선스와 고지가 적용됩니다. 설치 ZIP 및 소스의 해당 고지와 [Microsoft .NET 고지](DOTNET-NOTICES.txt)를 확인하세요. 이 문서들은 GF2VR 전체의 이용 조건을 대신하지 않습니다. 게임과 관련된 상표·저작물의 권리는 각 권리자에게 있습니다.
