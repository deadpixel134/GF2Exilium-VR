# 0.4.0 — 로컬 후보

2026-10-11 기준 문서입니다. 공개 릴리스/다운로드/서명된 지원 버전이 아닙니다.

0.4.0은 인스톨러와 설정 앱에 공식 Microsoft .NET10.0.12 Windows x64 runtime을 포함합니다. 기존 설정 기능/소유 파일 목록을 유지하며0.3.0에서 업데이트한 뒤 직전 파일로 복구하는 임시 검증을 수행했습니다. 설정 앱은18개 필드, 저장/다시 읽기/기본값/프로필 가져오기·내보내기와3언어를 제공합니다.

native VR DLL은0.3.0과 같은 `BOOTSTRAP-PORTABLE-001`입니다. 마지막 사용자 수용 native 출력에서 설치 루트와 시스템 version.dll 경로를 수정했으며, 그 새 DLL의 깨끗한 게임/VR 수용은 아직입니다. 렌더/입력/pose 기능을 이번 .NET 배포 작업에서 수정하지 않았습니다.

| 증거 수준 | 상태 |
| --- | --- |
| 두 desktop 앱의 private runtime과3언어 화면 | 로컬 통과 |
| 설치/업데이트/이전 버전 복구/제거 | inert game·실제 제품 바이트의 임시 폴더 확인 |
| 추출된 ZIP의 실제 desktop 실행 / 읽기 preflight | 로컬 통과 |
| 현재 개발 설치의 DLL/설정/Gate 보존 | 확인 |
| .NET이 없는 깨끗한 Windows VM | 미확인 |
| 새 native DLL의 깨끗한 게임 PC/VR 수용 | 미확인 |
| 목표 fresh110/s30초, 전체 UI/기능 milestone | 미완료 |
| GitHub 다운로드·자동 업데이트·배포자 인증 | 미구현/미설정 |

패키지 ZIP은16개 허용 항목과 그 해시를 검증했습니다. SHA256:
`613FE54F203391B42DB43EFBED5C245FF004BD3247AD37B1DB22ECB819F410BA`.

OpenXR/MinHook 라이선스와 .NET license/notices를 포함합니다. .NET private runtime은 새 배포본을 만들어야 갱신됩니다. 게임 원본/개인 설정/reference source/로그/복구 자료는 배포하지 않습니다.
