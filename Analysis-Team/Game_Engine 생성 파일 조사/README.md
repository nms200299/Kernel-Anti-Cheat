# Unreal Engine 4 & Unity 게임 생성 파일 및 레지스트리 정리

## Unreal Engine 4

### 주요 파일
- `game_folder\GameName.exe`: 게임 실행 파일  
- `game_folder\GameName\Content\Paks\GameName-WindowsNoEditor.pak`: 게임 콘텐츠가 압축된 팩 파일  
- `game_folder\Manifest_*.txt`: 패키징된 파일 목록을 담은 텍스트  
- `game_folder\Engine\...`: CrashReportClient, 엔진 관련 실행 파일 포함  

### 설정 파일
- `%LOCALAPPDATA%\GameName\Saved\Config\WindowsNoEditor\GameUserSettings.ini`: 해상도, 전체화면 여부, 그래픽 품질, 프레임 제한 등 사용자 시스템 설정  
- `%LOCALAPPDATA%\GameName\Saved\Config\WindowsNoEditor\Engine.ini`: 렌더링, 오디오, 물리 엔진 등 엔진 설정  
- `%LOCALAPPDATA%\GameName\Saved\Config\WindowsNoEditor\Input.ini`: 키 바인딩 및 입력 설정  
- `%LOCALAPPDATA%\GameName\Saved\Config\WindowsNoEditor\Scalability.ini`: 그래픽 품질 설정 프리셋  

### 로그
- `%LOCALAPPDATA%\GameName\Saved\Logs\Launch.log`: 엔진 초기화, 리소스 로딩, 경고 및 에러 기록  

### 세이브
- `%LOCALAPPDATA%\GameName\Saved\SaveGames\*.sav`: SaveGame 시스템 세이브 파일  

### 레지스트리
- 기본적으로 사용 안 함 (설치 정보는 Windows 프로그램 등록 키 사용 가능)

### 기타
- `%LOCALAPPDATA%\GameName\Saved\Crashes\`: 크래시 덤프 및 리포트  
- `%LOCALAPPDATA%\GameName\Saved\Screenshots\`: 스크린샷 저장  

---

## Unity

### 주요 파일
- `game_folder\GameName.exe`: 게임 실행 파일  
- `game_folder\GameName_Data\`: 리소스 및 엔진 파일 포함  
- `UnityPlayer.dll`: Unity 런타임 DLL  

### 설정 파일
- 기본적으로 없음 (설정은 레지스트리에 저장됨)  
- 일부 게임은 `%AppData%\LocalLow\회사명\게임명\settings.json` 같은 파일로 저장됨  

### 로그
- `%USERPROFILE%\AppData\LocalLow\CompanyName\GameName\Player.log`: 현재 실행 로그  
- `%USERPROFILE%\AppData\LocalLow\CompanyName\GameName\Player-prev.log`: 이전 세션 로그  

### 세이브
- `%USERPROFILE%\AppData\LocalLow\CompanyName\GameName\`: 세이브 파일 또는 설정 JSON 등  

### 레지스트리
- `HKCU\Software\CompanyName\GameName\`  
  - `Screenmanager Resolution Width / Height`: 해상도 설정  
  - `Screenmanager Is Fullscreen mode`: 전체화면 여부  
  - `UnityGraphicsQuality`: 그래픽 품질 레벨 (0~5)  
  - `unity.cloud_userid`, `unity.player_session_count`: 자동 생성된 Unity 세션 값  

### 기타
- `%TEMP%\CompanyName\GameName\Crashes\`: 오류 로그(`error.log`), 덤프(`crash.dmp`) 저장  

---

## Unreal vs Unity

| 항목 | Unreal Engine 4 | Unity 엔진 |
|------|------------------|-------------|
| 설정 저장 방식 | `.ini` 파일 (로컬 앱데이터) | 레지스트리 (PlayerPrefs) 또는 JSON |
| 로그 위치 | `%LOCALAPPDATA%\...\Logs\Launch.log` | `%APPDATA%\LocalLow\...\Player.log` |
| 레지스트리 사용 여부 | 기본적으로 사용 안 함 | 기본 사용 |
| 설치 구조 | 실행 파일 + `.pak` + 엔진 구조 | 실행 파일 + `_Data` 폴더 구조 |
| 세이브 경로 | `%LOCALAPPDATA%\...\SaveGames\` | `%APPDATA%\LocalLow\...` |
| 크래시 처리 | CrashReportClient 사용 | Temp 폴더에 로그/덤프 저장 |

