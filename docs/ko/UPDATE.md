# 업데이트

[한국어](../ko/UPDATE.md) | [English](../en/UPDATE.md) | [日本語](../ja/UPDATE.md) | [简体中文](../zh-CN/UPDATE.md)

> **필수: VR 사용 전 게임 설정에서 안티앨리어싱을 끄세요.**

VR DLL과 공통 설정 프로그램 payload는 1.1.1과 동일합니다. 이 버전은 인스톨러 안내만 갱신합니다.


1.0.x에서 처음 Clean/Dirty로 이동할 때는 원하는 ZIP을 직접 받아 새 인스톨러를 사용하세요. 이전 자동 업데이트가 빌드 종류를 선택해 주지는 않습니다. 새 버전의 자동 업데이트는 현재 설치와 같은 종류의 검증된 패키지만 허용하며, 종류 전환은 수동 설치로 진행합니다. 설정의 업데이트 확인은 로그인된 GitHub CLI(gh)를 사용합니다. 접근·네트워크·검증 실패 시 기존 설치와 설정 접근을 유지합니다.

- [GF2VR-Native-SelfContained-Clean-1.1.2.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.2)
- [GF2VR-Native-SelfContained-Dirty-1.1.2.zip](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.1.2)

설치·전환·제거는 사용자 설정을 보존합니다. Dirty의 touch_interaction.json은 선택 사항이며 Clean은 읽지 않습니다. 제거는 기록된 직전 설치가 있으면 복원하고, 최초 설치에서는 소유 파일만 제거합니다. 수정되거나 소유권을 확인할 수 없는 DLL은 보존하며 덮어쓰지 않습니다. version.dll만 따로 복사하거나 서로 다른 패키지의 파일을 섞지 마세요.

Quest 3와 Galaxy XR에서 보고된 양안 영상 어긋남은 안티앨리어싱이 원인인 것으로 확인됐습니다. 게임 설정에서 안티앨리어싱을 끄고 VR을 사용하세요.
