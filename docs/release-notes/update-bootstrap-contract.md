# Update bootstrap contract / 업데이트 부트스트랩 계약

## 한국어

이 문서는 1.2.2 이후 업데이트를 전달하는 최소한의 고정 계약입니다. 패키지 내부 파일과 설치 정책은 새 Installer가 검사하지만, 먼저 실행되는 클라이언트가 아래 계약을 이해할 수 있어야 합니다.

- 저장소는 `deadpixel134/GF2Exilium-VR`, 채널은 `stable`, 태그는 `vMAJOR.MINOR.PATCH`입니다. 배포 버전은 현재 버전보다 높아야 합니다.
- 플레이버마다 ZIP 하나와 descriptor 하나를 제공합니다. 이름은 `GF2VR-Native-SelfContained-{Clean|Dirty}-{version}.zip`, `GF2VR-{Clean|Dirty}-update.json`입니다. ZIP 전체의 `ArchiveSha256`과 공개 자산의 digest·길이를 검증합니다. 여러 ZIP으로 나누는 변경은 이 계약에 포함되지 않습니다.
- Clean/Dirty descriptor는 schemaVersion 1의 다음 11키를 유지합니다: `schemaVersion`, `product`, `repository`, `version`, `channel`, `archiveName`, `archiveSha256`, `packageManifestSha256`, `installerFiles`, `buildFlavor`, `commonCoreSha256`. JSON 키는 여기 적힌 소문자 시작 표기입니다. `product`는 `GF2VR`입니다. `installerFiles` 항목은 `path`, `sha256`, `bytes`입니다.
- ZIP 루트의 helper는 정확히 `GF2VR.Installer.exe`, `GF2VR.Installer.dll`, `GF2VR.Installer.deps.json`, `GF2VR.Installer.runtimeconfig.json` 4파일입니다. 각 길이·SHA를 descriptor에 기록합니다. 이 4파일만 추출한 상태에서 Installer가 실행·검증·설치를 완수해야 합니다. 별도 .NET 런타임 설치나 다섯 번째 DLL/도구가 필요해지는 변경은 금지합니다. 필요한 플랫폼 런타임은 현재와 같이 실행 파일에 포함합니다.
- 다운로드는 적용 확인 뒤에 수행합니다. 실제 설치는 동의를 기록하고 이전 설정 앱의 종료를 확인한 뒤 새 helper가 담당합니다. 게임 실행 중 파일 교체, 버전 역행, 다른 게임 폴더로의 쓰기, 무결성 검사를 생략한 설치는 허용하지 않습니다.
- `packageManifestSha256`은 첫 클라이언트가 전달하는 검증된 claim이며 실제 manifest·runtime-contract·payload 검증은 새 helper 소유입니다. 이들 내부 구조의 확장은 helper 정책으로 처리할 수 있지만 위 envelope와 helper의 자급 조건은 바꾸지 않습니다.

이 계약을 바꾸려면 별도 클라이언트/프로토콜 전환과 구 버전 호환 검증이 먼저 필요합니다. payload 검사를 느슨하게 하거나 파일을 추가한 것만으로 기존 client가 새 bootstrap을 지원한다고 간주하지 않습니다. 계약 회귀 검사와 helper 4파일 단독 실행 검사를 릴리스마다 실행합니다.

## English

This is the minimum stable envelope for updates starting with 1.2.2. The incoming Installer owns package contents and installation policy; the already installed client must continue to understand this envelope.

- Repository: `deadpixel134/GF2Exilium-VR`; channel: `stable`; tag: `vMAJOR.MINOR.PATCH`. The offered version must advance the installed version.
- Each flavor provides one ZIP and one descriptor: `GF2VR-Native-SelfContained-{Clean|Dirty}-{version}.zip` and `GF2VR-{Clean|Dirty}-update.json`. Verify the whole ZIP `ArchiveSha256`, asset digest and length. Splitting an update across several archives is outside this contract.
- Schema 1 Clean/Dirty descriptors retain exactly eleven keys: `schemaVersion`, `product`, `repository`, `version`, `channel`, `archiveName`, `archiveSha256`, `packageManifestSha256`, `installerFiles`, `buildFlavor`, `commonCoreSha256`. JSON uses the lower-camel-case spellings shown here. Product is `GF2VR`; each installerFiles item contains `path`, `sha256`, `bytes`.
- The ZIP-root helper is exactly four files: `GF2VR.Installer.exe`, `GF2VR.Installer.dll`, `GF2VR.Installer.deps.json`, `GF2VR.Installer.runtimeconfig.json`. Bind each length and SHA in the descriptor. These four files alone must start and complete validation/installation. Requiring a separate .NET installation or a fifth DLL/tool breaks the contract and is prohibited. Keep the private platform runtime bundled in the executable.
- Download follows explicit apply consent. The incoming helper records consent, waits for the old Settings process to exit and performs the installation. Running-game replacement, downgrades, writes outside the selected game root and skipped integrity checks remain prohibited.
- The first client forwards the verified packageManifestSha256 claim. The incoming helper validates its actual manifest, runtime contract and payload using its own policy. Internal package evolution must not change this envelope or the four-file self-sufficiency requirement.

A change to this contract requires a separately approved client/protocol migration and old-client compatibility evidence. Adding files or weakening payload validation does not make older clients compatible. Run the envelope regression tests and isolated four-file helper execution for every release.
