# 소스·배포 경계

기존 비공개 저장소의 데스크톱 도구 소스만 갱신합니다. 명시적 파일 목록은 source-manifest.json에 있습니다. 게임 설치 폴더 자체를 Git 저장소로 취급하지 않습니다.

포함: 두WinForms프로젝트의 C#/프로젝트/구조화된 ko/en/ja 자원, 세 desktop 빌드 도구, 비작동 runtime-contract template, 사용자 문서, 공식.NET고지. 제외: 게임/네이티브/참조 프로젝트 소스·바이너리·encoded리소스·SDK/cache·개인 설정·로그·rollback·agent/Manager자료·자격 증명. 제품 코드의 기존 로컬 경로 호환 상수는 사용자 설정 파일과 별개입니다.

## Windows 개발 빌드

.NET10 SDK가 있는 Windows에서 저장소 루트 기준으로 다음을 실행합니다.

```powershell
dotnet build desktop-source/vrmod/src/GF2VR.Configurator/GF2VR.Configurator.csproj -c Release
dotnet build desktop-source/vrmod/src/GF2VR.Installer/GF2VR.Installer.csproj -c Release
```

PowerShell7의 desktop-source/vrmod/tools/build_desktop_private_runtime.ps1은 -Application Configurator 또는 Installer와 별도 -BuildId로 공식.NET10.0.12 Windows x64 runtime을 포함한 desktop publish를 준비합니다. 첫 실행에는 공식 NuGet 접근이 필요할 수 있습니다. 현재 테스트한 설치본과 스냅샷의 소스 빌드 결과는 별도로 검증합니다.

runtime-contract.json은 INSTALL_ROOT 경로와0으로 채운hash의 비작동 template입니다. 실제 사용에는 검증된 네이티브 DLL과 정확한 설치 계약 생성이 필요합니다. template을 실제 게임 설정 계약으로 복사하지 마세요. 이 소스만으로 전체 네이티브 VR 패키지를 재생성할 수 없습니다.

Microsoft고지는 runtime pack의 고지이며 프로젝트 전체의 라이선스가 아닙니다. 저장소 공개 전환·프로젝트 라이선스·다른 사용자의 private배포 접근·최신 네이티브 배포는 미결정입니다. 재사용 가능한 인증 비밀을 앱/소스/릴리스에 넣지 않습니다.
