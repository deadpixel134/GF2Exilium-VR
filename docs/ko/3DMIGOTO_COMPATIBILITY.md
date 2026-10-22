# 3DMigoto와 함께 사용하기

[한국어](../ko/3DMIGOTO_COMPATIBILITY.md) | [English](../en/3DMIGOTO_COMPATIBILITY.md) | [日本語](../ja/3DMIGOTO_COMPATIBILITY.md) | [简体中文](../zh-CN/3DMIGOTO_COMPATIBILITY.md)

1.1.3은 PR #1의 3DMigoto 기본 wrapper 모드 호환 변경을 Clean·Dirty에 함께 반영합니다. 게임의 wrapper는 유지하고 VR 복사·OpenXR에는 실제 D3D11 장치/context를 사용합니다. 변경된 DLL과 일치하는 설정 도구·인스톨러를 함께 제공합니다.

> **필수: VR 사용 전 게임 설정에서 안티앨리어싱을 끄세요.**

## 1.2.0 — Steam 지원 및 손·접촉 호환성 수정

Steam판 실행 파일 폴더와 기존 Haoplay 설치 구조를 지원합니다. 게임 업데이트 후 흰색 손 표시와 캐릭터 접촉이 사라지던 호환성 문제를 수정했습니다. Dirty는 활동층에서 동시에 보이는 지원 캐릭터들을 각각 접촉 대상으로 처리합니다. Clean에는 접촉 변형이 포함되지 않습니다.

실기 확인 범위는 Dirty의 Haoplay Main·Activity 손 표시/접촉과 Steam Main 실행·입체 화면입니다. Steam의 모든 화면과 Clean의 새 DLL 실기는 별도로 확인되지 않았습니다. 게임 내 안티앨리어싱은 OFF로 설정하세요.

정상적인 1.1.6 관리 설치는 설정 GUI의 업데이트 알림에서 승인해 1.2.0으로 업데이트할 수 있습니다. GitHub CLI·로그인이 필요하지 않으며 Clean·Dirty 종류와 사용자 설정을 유지합니다. 1.1.4 이하 또는 구버전 업데이트 확인이 안 되는 환경은 원하는 ZIP 전체를 새 폴더에 풀고 포함된 GF2VR.Installer.exe로 한 번 설치하세요. 게임을 종료한 뒤 진행하세요.

릴리즈 버전은 1.2.0입니다. 이전 미공개 후보의 내부 native 빌드 ID에 남은 130은 동일한 검증 DLL의 식별자입니다. 일반 설치 도구는 수동 교체되거나 소유권이 없는 파일을 자동 덮어쓰지 않습니다.

## 설정

3DMigoto의 `[System]`에서 `hook`이 생략되었거나 주석 처리된 기본 wrapper 모드를 사용하세요. `hook=recommended` 및 `hook=all` 조합은 지원 범위 밖입니다. `calls`·`crash`는 기록/덤프 옵션이며 이 수정에 필수는 아닙니다. 스킨의 INI 문법·중복 규칙 경고는 별도 문제입니다.

## 구현

- swapchain의 공개 GetDevice를 사용하고, 실패하면 백버퍼의 GetDevice를 사용합니다.
- Unity wrapper와 실제 장치는 COM IUnknown 동일성으로 확인합니다. 게임 wrapper를 교체하지 않습니다.
- VR은 참조를 보유한 실제 장치와 immediate context를 사용합니다. 패널·목적지·손 커서 장치도 동일하게 비교합니다.
- 눈 렌더링 직전에는 context의 COM 동일성을 확인하고 보유한 실제 context의 감시 후크를 갱신합니다.
- 실제 장치/context에서 알려진 3DMigoto wrapper 인터페이스가 발견되면 VR 초기화를 중단합니다. 이유 11/12는 이 검사에 해당합니다. 게임의 일반 실행은 유지합니다.

DLL만 복사하면 설정 도구의 무결성 검사와 맞지 않습니다. 전체 ZIP을 풀어 새 인스톨러로 업데이트하세요. 개발자는 [메타데이터 빌드 절차](BUILD_VARIANT_METADATA.md)를 사용하세요. 진단 native_graphics_isolated의 1은 wrapper/native 주소가 다름을, 0은 같음을 뜻하며 호환성 성공 판정은 아닙니다.

## 확인 범위

PR 작성자는 Dirty 개발 빌드에서 VR·스킨 동시 작동을 세 번 연속 확인했다고 보고했습니다. 이는 작성자의 기록이며 새 1.1.3 바이너리의 HMD 수용을 대신하지 않습니다. 이번 릴리즈는 양쪽 native 빌드, 실제 소스의 COM 장치/실패 경로 검사, 설정 도구·인스톨러 빌드와 패키지 무결성을 검증합니다. 새 게임/HMD 실행은 하지 않았습니다.

Quest 3와 Galaxy XR에서 보고된 양안 영상 어긋남은 안티앨리어싱이 원인인 것으로 확인됐습니다. 게임 설정에서 안티앨리어싱을 끄고 VR을 사용하세요.

기여: [rgbtl1248](https://github.com/rgbtl1248), [PR #1](https://github.com/deadpixel134/GF2Exilium-VR/pull/1).
