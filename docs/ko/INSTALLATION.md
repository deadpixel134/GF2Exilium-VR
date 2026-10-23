# 설치·전환·제거

[한국어](../ko/INSTALLATION.md) | [English](../en/INSTALLATION.md) | [日本語](../ja/INSTALLATION.md) | [简体中文](../zh-CN/INSTALLATION.md)

[처음으로](../../README.md)

## 시작 전 준비

- 정상 실행되는 **Haoplay 또는 Steam판 게임**. 게임 본편은 포함하지 않습니다.
- VR 헤드셋·컨트롤러와 정상 연결된 **SteamVR/OpenXR 환경**.
- 게임 설정의 **안티앨리어싱 OFF**. Quest 3·Galaxy XR의 양안 영상 어긋남 원인으로 확인됐습니다.

## 패키지 선택

[최신 릴리스](https://github.com/deadpixel134/GF2Exilium-VR/releases/latest)의 **Assets**에서 Clean 또는 Dirty 전체 ZIP을 선택하세요. 일반 VR·컨트롤러 입력·흰색 손 표시는 공통이고, 캐릭터 접촉 변형·접촉 진동은 Dirty에만 포함됩니다. 둘 중 하나만 설치하세요. GitHub의 `Source code` ZIP은 사용하지 않습니다.

## 설치 순서

1. [최신 릴리스](https://github.com/deadpixel134/GF2Exilium-VR/releases/latest)에서 **Clean 또는 Dirty ZIP 하나**를 받아 새 폴더에 전부 풉니다. `Source code` ZIP은 설치 파일이 아닙니다.
2. 게임과 기존 설정·인스톨러를 종료합니다. 압축을 푼 폴더의 `GF2VR.Installer.exe`에서 `GF2_Exilium.exe` 또는 게임 폴더를 선택해 설치합니다.
3. **설정 열기**에서 옵션을 저장합니다. SteamVR와 헤드셋을 준비한 뒤 평소 방법으로 게임을 직접 실행합니다.

## 설치되는 파일

`version.dll`은 게임 실행 파일 옆에, OpenXR 로더·설정 프로그램 등 공통 파일은 `vrmod` 아래에 설치됩니다. 원본 `GameAssembly.dll`·`UnityPlayer.dll`·게임 에셋은 교체하지 않습니다. 실행 파일이나 DLL만 따로 옮기거나 다른 패키지 파일을 섞지 마세요.

## Clean과 Dirty 전환

원하는 종류의 최신 ZIP 전체를 새 폴더에 풀고 포함된 인스톨러를 실행해 같은 게임 위치에 설치하세요. 두 종류를 겹쳐 설치하지 않습니다. 호환되는 사용자 설정은 보존하며, Clean은 Dirty의 선택 설정 `touch_interaction.json`을 읽지 않습니다.

## 제거와 이전 설치 복원

게임을 종료하고 인스톨러에서 같은 게임 위치를 선택한 뒤 제거 안내를 확인하세요. 기록된 직전 설치가 있으면 복원하고, 최초 설치에서는 소유 파일만 제거합니다. 사용자 설정과 수정되었거나 소유권을 확인할 수 없는 파일은 보존합니다. 설치 기록·백업을 먼저 지우지 마세요. 중단된 작업이 표시되면 중단 복구를 사용합니다.

[업데이트](UPDATE.md) · [문제 해결](TROUBLESHOOTING.md)
