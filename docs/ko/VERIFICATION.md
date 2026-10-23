# 지원 환경과 확인 범위

[한국어](../ko/VERIFICATION.md) | [English](../en/VERIFICATION.md) | [日本語](../ja/VERIFICATION.md) | [简体中文](../zh-CN/VERIFICATION.md)

[처음으로](../../README.md)

플랫폼 호환성, 실제 게임에서 관찰한 결과, 오프라인 검증을 구분합니다. 문서 정리나 소스 빌드 성공이 새로운 HMD 실기 확인을 뜻하지는 않습니다.

## 플랫폼과 실제 게임 확인

| 대상 | 확인된 범위 |
|---|---|
| Haoplay · Dirty | Main·Activity의 손 표시와 접촉 경로 |
| Steam · Dirty | Main 실행과 HMD 입체 화면 |
| Steam의 다른 화면 · Clean의 새 DLL | 별도 HMD 확인이 필요함 |

기존 실기 확인 환경은 Meta Quest 2 · 4K 모니터 · Virtual Desktop입니다. Quest 3·Galaxy XR의 양안 어긋남은 게임 안티앨리어싱이 원인으로 확인되어 OFF가 필수입니다. 이 목록이 모든 헤드셋·연결 방식의 호환성 보장은 아닙니다.

## 기능별 제한

접촉은 Dirty 전용이며 해당 부위의 지원 뼈가 연결된 캐릭터에서 동작합니다. 모든 캐릭터·모델·화면을 보장하지 않으며 숙소·활동층의 실제 엉덩이 반응은 별도로 확인되지 않았습니다. 프레임 속도는 장면·장치·화질 설정에 따라 달라집니다.

3DMigoto는 기본 wrapper 모드가 지원 범위입니다. [호환성 안내](3DMIGOTO_COMPATIBILITY.md)를 확인하세요. Clean도 게임 프로세스에서 실행되며 이름이 미감지나 안전을 보장하지 않습니다.

## 패키지와 소스 검증

공개 소스로 재현한 Clean·Dirty DLL은 배포 DLL과 바이트가 일치합니다. 설치·전환·복원, 설정·다국어 도구, 패키지 무결성과 구버전 업데이트 경로를 기술적으로 검사했습니다. 이는 모든 사용자 환경이나 모든 장면의 HMD 동작 확인과는 별개입니다. 세부 버전 기록은 [변경 이력](RELEASE_NOTES.md)과 [GitHub 릴리스](https://github.com/deadpixel134/GF2Exilium-VR/releases)에 보관합니다.
