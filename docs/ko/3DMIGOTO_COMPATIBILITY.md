# 3DMigoto와 함께 사용하기

[한국어](../ko/3DMIGOTO_COMPATIBILITY.md) | [English](../en/3DMIGOTO_COMPATIBILITY.md) | [日本語](../ja/3DMIGOTO_COMPATIBILITY.md) | [简体中文](../zh-CN/3DMIGOTO_COMPATIBILITY.md)

[처음으로](../../README.md)

Clean·Dirty 모두 3DMigoto의 기본 wrapper 모드와 호환됩니다. 게임 wrapper는 유지하고 VR은 실제 D3D11 장치/context를 사용합니다. VR만 사용한다면 3DMigoto를 별도로 설치할 필요는 없습니다.

> **필수: VR 사용 전 게임 설정에서 안티앨리어싱을 끄세요.**

## 설정

3DMigoto의 `[System]`에서 `hook`이 생략되었거나 주석 처리된 기본 wrapper 모드를 사용하세요. `hook=recommended` 및 `hook=all` 조합은 지원 범위 밖입니다. `calls`·`crash`는 기록/덤프 옵션이며 이 수정에 필수는 아닙니다. 스킨의 INI 문법·중복 규칙 경고는 별도 문제입니다.

## 확인 범위

기여자 [rgbtl1248](https://github.com/rgbtl1248)의 [PR #1](https://github.com/deadpixel134/GF2Exilium-VR/pull/1)에 기반합니다. PR 작성자의 Dirty 개발 빌드 VR·스킨 동시 실행 보고와 패키지 기술 검증을 구분합니다. 모든 스킨·설정 조합을 보장하지 않습니다. 현재 게임/HMD 확인 범위는 [검증 안내](VERIFICATION.md)를 참고하세요.

<details>
<summary>개발자용 구현 참고</summary>

## 구현

- swapchain의 공개 GetDevice를 사용하고, 실패하면 백버퍼의 GetDevice를 사용합니다.
- Unity wrapper와 실제 장치는 COM IUnknown 동일성으로 확인합니다. 게임 wrapper를 교체하지 않습니다.
- VR은 참조를 보유한 실제 장치와 immediate context를 사용합니다. 패널·목적지·손 커서 장치도 동일하게 비교합니다.
- 눈 렌더링 직전에는 context의 COM 동일성을 확인하고 보유한 실제 context의 감시 후크를 갱신합니다.
- 실제 장치/context에서 알려진 3DMigoto wrapper 인터페이스가 발견되면 VR 초기화를 중단합니다. 이유 11/12는 이 검사에 해당합니다. 게임의 일반 실행은 유지합니다.

DLL만 복사하면 설정 도구의 무결성 검사와 맞지 않습니다. 전체 ZIP을 풀어 새 인스톨러로 업데이트하세요. 개발자는 [메타데이터 빌드 절차](BUILD_VARIANT_METADATA.md)를 사용하세요. 진단 native_graphics_isolated의 1은 wrapper/native 주소가 다름을, 0은 같음을 뜻하며 호환성 성공 판정은 아닙니다.

</details>
