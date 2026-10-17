# 3DMigoto와 함께 사용하기

이 변경은 VR 모드가 그래픽 장치 wrapper와 실제 D3D11 장치를 구분하도록 합니다. 게임의 wrapper를 유지하면서 VR 복사와 OpenXR에는 실제 장치와 immediate context를 사용합니다. 장치를 비교할 때는 주소만 비교하지 않고 COM `IUnknown` 동일성을 확인합니다.

## 사용자 설정

3DMigoto는 기본 wrapper 모드를 사용하세요. `[System]`의 `hook` 항목이 생략되었거나 주석 처리되어 있으면 추가 변경이 필요 없습니다. 추가 함수 후킹을 켜는 `hook=recommended` 또는 `hook=all` 조합은 이번 지원 범위에 포함하지 않습니다.

`calls`와 `crash`는 기록·덤프 관련 설정입니다. 이번 호환성 수정을 위해 켜야 하는 항목이 아닙니다. 스킨별 INI 문법이나 중복 규칙 경고는 해당 스킨에서 별도로 확인하세요.

## 개발자 참고

- swapchain의 공개 `GetDevice`로 실제 장치를 얻습니다. 실패하면 백버퍼의 `GetDevice`를 사용합니다.
- Unity 장치와 실제 장치의 COM 동일성을 확인한 뒤 실제 장치/context를 VR에 보관합니다. Unity의 장치·context wrapper는 교체하지 않습니다.
- 패널, 목적지 텍스처, 컨트롤러 커서 텍스처의 장치 비교에도 동일한 기준을 사용합니다.
- 눈 렌더링 직전에도 wrapper와 실제 context의 COM 동일성을 확인하고, 실제 context의 그래픽 함수 주소가 바뀌면 감시 후크를 갱신합니다.
- 실제 장치/context에서 알려진 3DMigoto wrapper 인터페이스가 발견되면 설치를 거부합니다. 실패 이유 11/12는 추가 함수 후킹이 남은 조합을 뜻합니다.
- `native_graphics_isolated` 진단의 값 1은 Unity wrapper와 VR의 실제 장치 주소가 분리된 경우이며, 0은 처음부터 같은 주소였던 경우입니다.

설정 프로그램과 인스톨러는 DLL 해시를 검증합니다. 변경된 DLL만 복사하면 설정 도구가 `runtimeError`를 표시하고 입력을 잠글 수 있습니다. [빌드 메타데이터 생성 절차](../en/BUILD_VARIANT_METADATA.md)에 따라 실제 DLL과 일치하는 runtime contract·variant profile·패키지 manifest를 함께 만들면 기존 무결성 검사를 유지할 수 있습니다.

## 확인 범위

최신 Dirty 1.1.2 수정본에서 사용자가 3회 연속 재실행하며 VR 화면과 스킨의 동시 작동을 확인했습니다. 실행 중 바뀌는 그래픽 함수 주소가 수정 후 감시 대상에 포함되는 것도 확인했습니다. 기존 독립 D3D11 검사에서는 wrapper 명령 실행, native 복사·GPU 완료 확인·readback을 확인했고, 설정 정보 보정 후 Configurator의 실제 설치 검사도 통과했습니다. 이 설명은 로컬 확인 결과이며 모든 파생 버전·스킨·헤드셋의 호환성을 보장하지 않습니다. Clean의 실제 게임/HMD 실행은 확인하지 않았습니다.
