# 다운로드와 파일 구성

[한국어](../ko/RELEASE_READINESS.md) | [English](../en/RELEASE_READINESS.md) | [日本語](../ja/RELEASE_READINESS.md) | [简体中文](../zh-CN/RELEASE_READINESS.md)

[처음으로](../../README.md)

설치 파일은 [최신 정식 릴리스](https://github.com/deadpixel134/GF2Exilium-VR/releases/latest)의 **Assets**에서 받으세요. 릴리스 페이지에서 실제 버전과 파일 이름을 확인할 수 있습니다.

| 파일 | 용도 |
|---|---|
| `GF2VR-Native-SelfContained-Clean-<version>.zip` | Clean 전체 설치 패키지 |
| `GF2VR-Native-SelfContained-Dirty-<version>.zip` | Dirty 전체 설치 패키지 |
| `CHECKSUMS.sha256` | 다운로드 파일의 SHA-256 확인용 |
| `GF2VR-Clean-update.json` / `GF2VR-Dirty-update.json` | 자동 업데이트 도구가 사용하는 메타데이터. 수동 설치에는 ZIP을 사용합니다. |
| GitHub `Source code` | 개발용 저장소 사본. 설치 패키지가 아닙니다. |

전체 ZIP에는 `GF2VR.Installer.exe`, 설정 도구와 선택한 종류의 VR 파일이 함께 들어 있습니다. 필요한 .NET 런타임을 포함하므로 일반 사용자가 별도로 빌드할 필요가 없습니다.

압축을 전부 풀고 포함된 인스톨러로 설치하세요. DLL만 받거나 파일 검사를 끄지 마세요. [설치](INSTALLATION.md) · [업데이트](UPDATE.md)
