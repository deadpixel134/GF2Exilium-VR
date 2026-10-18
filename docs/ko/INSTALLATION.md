# 설치·전환·제거

[한국어](../ko/INSTALLATION.md) | [English](../en/INSTALLATION.md) | [日本語](../ja/INSTALLATION.md) | [简体中文](../zh-CN/INSTALLATION.md)

> **필수: VR 사용 전 게임 설정에서 안티앨리어싱을 끄세요.**

1.1.3 업데이트: Clean 또는 Dirty ZIP 전체를 새 폴더에 풀고, 그 안의 GF2VR.Installer.exe로 기존 게임 위치에 설치하세요. 기존 1.1.2 설정 창의 자동 업데이트는 새 native 프로필을 검증하지 못해 거부됩니다. 기존 설치는 보존됩니다. DLL만 복사하거나 파일을 섞지 마세요.

1.1.3은 PR #1의 3DMigoto 기본 wrapper 모드 호환 변경을 Clean·Dirty에 함께 반영합니다. 게임의 wrapper는 유지하고 VR 복사·OpenXR에는 실제 D3D11 장치/context를 사용합니다. 변경된 DLL과 일치하는 설정 도구·인스톨러를 함께 제공합니다.


- [GF2VR-Native-SelfContained-Clean-1.1.3.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.3)
- [GF2VR-Native-SelfContained-Dirty-1.1.3.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.3)

| 기능 | Clean | Dirty |
|---|---|---|
| 일반 VR·입체 화면·컨트롤러 입력·설정 | 포함 | 포함 |
| 흰색 양손 구 표시 | 포함 | 포함 |
| 캐릭터 가슴 접촉·변형 | 미포함 | 포함 |

1. 원하는 Clean 또는 Dirty ZIP을 새 폴더에 전부 풉니다. GitHub Source code ZIP은 설치 파일이 아닙니다.
2. 게임·설정·인스톨러를 종료합니다. 새 폴더의 GF2VR.Installer.exe에서 GF2_Exilium.exe 또는 게임 폴더를 선택합니다. 패키지 폴더는 자동으로 찾습니다.
3. 설치 종류와 버전을 확인하고 설치합니다. Clean↔Dirty 전환도 원하는 쪽 ZIP의 새 인스톨러로 진행합니다.
4. 설정 열기에서 저장한 뒤 게임을 평소 방법으로 직접 실행합니다. 흰색 손 표시는 기본 켜짐이며 손 표시 탭에서 바꿀 수 있습니다.

둘 중 하나만 설치하세요. 두 빌드는 게임 실행 파일 옆의 같은 version.dll 한 개를 사용하며, OpenXR 로더·설정 프로그램 등 공통 지원 파일은 vrmod 아래에 설치합니다. 원본 GameAssembly.dll·UnityPlayer.dll·게임 에셋은 교체하지 않습니다. 두 빌드 모두 게임 프로세스에서 실행하므로 Clean이라는 이름이 미감지·안전을 보장하지 않습니다.

설치·전환·제거는 사용자 설정을 보존합니다. Dirty의 touch_interaction.json은 선택 사항이며 Clean은 읽지 않습니다. 제거는 기록된 직전 설치가 있으면 복원하고, 최초 설치에서는 소유 파일만 제거합니다. 수정되거나 소유권을 확인할 수 없는 DLL은 보존하며 덮어쓰지 않습니다. version.dll만 따로 복사하거나 서로 다른 패키지의 파일을 섞지 마세요.

Quest 3와 Galaxy XR에서 보고된 양안 영상 어긋남은 안티앨리어싱이 원인인 것으로 확인됐습니다. 게임 설정에서 안티앨리어싱을 끄고 VR을 사용하세요.
