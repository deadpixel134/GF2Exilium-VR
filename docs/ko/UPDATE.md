# 업데이트

[한국어](../ko/UPDATE.md) | [English](../en/UPDATE.md) | [日本語](../ja/UPDATE.md) | [简体中文](../zh-CN/UPDATE.md)

1.0.x에서 처음 Clean/Dirty로 이동할 때는 원하는 ZIP을 직접 받아 새 인스톨러를 사용하세요. 이전 자동 업데이트가 빌드 종류를 선택해 주지는 않습니다. 새 버전의 자동 업데이트는 현재 설치와 같은 종류의 검증된 패키지만 허용하며, 종류 전환은 수동 설치로 진행합니다. 설정의 업데이트 확인은 로그인된 GitHub CLI(gh)를 사용합니다. 접근·네트워크·검증 실패 시 기존 설치와 설정 접근을 유지합니다.

- [GF2VR-Native-SelfContained-Clean-1.1.1.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.1)
- [GF2VR-Native-SelfContained-Dirty-1.1.1.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.1)

설치·전환·제거는 사용자 설정을 보존합니다. Dirty의 touch_interaction.json은 선택 사항이며 Clean은 읽지 않습니다. 제거는 기록된 직전 설치가 있으면 복원하고, 최초 설치에서는 소유 파일만 제거합니다. 수정되거나 소유권을 확인할 수 없는 DLL은 보존하며 덮어쓰지 않습니다. version.dll만 따로 복사하거나 서로 다른 패키지의 파일을 섞지 마세요.

Galaxy XR에서 VDXR와 SteamVR 모두 좌우 영상이 어긋난다는 제보는 아직 해결 확인 전입니다. 1.1.1의 공통 투영 보정 적용 범위 개선은 오프라인 검증을 통과했지만, 이 증상의 해결이나 전체 헤드셋 호환성을 보장하지 않습니다. 접촉 기능은 이전 사용자 실기에서 확인했으며, 이 배포 바이너리 자체의 새 게임/HMD 실기는 진행하지 않았습니다.
