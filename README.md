# GF2 VR — 0.4.0 로컬 설치 후보

Windows x64용 인스톨러·설정 앱과 `version.dll` 기반 VR 모드를 준비한 개발 후보입니다. **깨끗한 Windows/게임에서의 실행 및 새 패키지의 VR 사용자 수용은 아직입니다.** 현재 개발 설치를 자동으로 교체하지 않습니다.

배포 후보 파일은 `GF2VR-Native-SelfContained-0.4.0.zip`입니다. 인스톨러와 설정 앱에 .NET10.0.12 런타임을 포함해 별도 .NET 설치를 실행하지 않습니다. ZIP 전체를 풀고 `GF2VR.Installer.exe`를 실행합니다. 실행 파일 옆의 DLL·deps·runtimeconfig와 payload를 함께 유지하세요.

- [설치·업데이트·제거·복구](docs/ko/INSTALLATION.md)
- [설정 GUI와 저장·프로필](docs/ko/SETTINGS.md)
- [게임 실행과 기본 조작](docs/ko/USAGE.md)
- [문제 해결](docs/ko/TROUBLESHOOTING.md)
- [0.4.0 변경과 검증 범위](docs/ko/RELEASE_NOTES.md)
- [소스·배포 범위](SOURCE_BOUNDARY.md)

인스톨러와 설정 GUI는 한국어·영어·일본어를 제공합니다. 이 문서 초안은 한국어로 작성했습니다. 저장소는 [deadpixel134/GF2Exilium-VR](https://github.com/deadpixel134/GF2Exilium-VR)이며 현재 private·본인용입니다. 바이너리 다운로드·자동 업데이트·지원 문의 채널은 아직 제공하지 않습니다.

파일 일치 확인용 ZIP SHA256:
`613FE54F203391B42DB43EFBED5C245FF004BD3247AD37B1DB22ECB819F410BA`

이 해시는 배포자 인증 서명이 아닙니다. 게임 파일·개인 설정·로그·복구 자료는 이 문서/소스 스냅샷에 포함하지 않습니다. 현재 native runtime은 앞선 수용 빌드에서 경로와 Windows 함수 전달을 수정한 `BOOTSTRAP-PORTABLE-001`이며, 이 버전 자체의 실제 게임/VR 수용을 이전 결과로 대신하지 않습니다.

설정 GUI 시작 시 업데이트를 알리고 사용자 확인 후 설치하는 기능은 향후 요구사항입니다. 현재 체크/다운로드를 구현했다고 안내하지 않습니다.
