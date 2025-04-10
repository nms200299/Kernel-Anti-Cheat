Unreal Engine 4 vs Unity 게임 실행 시 생성 파일 및 레지스트리 정리

Unreal Engine 4

주요 파일

game_folder\GameName.exe: 게임 실행 파일

game_folder\GameName\Content\Paks\GameName-WindowsNoEditor.pak: 게임 콘텐츠가 압축된 팩 파일

game_folder\Manifest_*.txt: 패키징된 파일 목록을 담은 텍스트

game_folder\Engine\...: CrashReportClient, 엔진 관련 실행 파일 포함

설정 파일

%LOCALAPPDATA%\GameName\Saved\Config\WindowsNoEditor\GameUserSettings.ini: 해상도, 전체화면 여부, 그래픽 품질, 프레임 제한 등 사용자 시스템 설정 저장

%LOCALAPPDATA%\GameName\Saved\Config\WindowsNoEditor\Engine.ini: 렌더링, 오디오, 물리 엔진 등 전반적인 엔진 설정

%LOCALAPPDATA%\GameName\Saved\Config\WindowsNoEditor\Input.ini: 사용자 키 바인딩 및 입력 설정 (게임 내 키 설정 변경 시 생성됨)

%LOCALAPPDATA%\GameName\Saved\Config\WindowsNoEditor\Scalability.ini: 그래픽 품질 설정 프리셋 저장 (낮음/중간/높음/에픽 등)

로그

%LOCALAPPDATA%\GameName\Saved\Logs\Launch.log: 실행 로그 (엔진 초기화, 리소스 로딩, 경고 및 에러 메시지 기록)

세이브

%LOCALAPPDATA%\GameName\Saved\SaveGames\*.sav: SaveGame 시스템으로 저장된 게임 세이브 데이터 (객체 직렬화 포맷)

레지스트리

기본적으로 사용 안 함 (설치 정보는 Windows의 프로그램 등록 키에 기록될 수 있음)

기타

%LOCALAPPDATA%\GameName\Saved\Crashes\: 크래시 발생 시 덤프 파일과 XML 리포트 저장

%LOCALAPPDATA%\GameName\Saved\Screenshots\: 스크린샷 저장 경로

Unity

주요 파일

game_folder\GameName.exe: 게임 실행 파일

game_folder\GameName_Data\: 리소스 및 엔진 데이터가 포함된 폴더

UnityPlayer.dll: Unity 엔진 런타임 DLL

설정 파일

기본적으로 없음. 설정은 레지스트리를 통해 저장됨

일부 게임은 %AppData%\LocalLow\회사명\게임명\settings.json 등 사용자 설정 파일을 직접 생성함 (게임마다 다름)

로그

%USERPROFILE%\AppData\LocalLow\CompanyName\GameName\Player.log: 현재 실행 로그

%USERPROFILE%\AppData\LocalLow\CompanyName\GameName\Player-prev.log: 이전 세션 로그 백업

세이브

%USERPROFILE%\AppData\LocalLow\CompanyName\GameName\: 세이브 파일 또는 JSON 설정 저장 (예: SaveData.json, Progress.dat 등)

레지스트리

HKCU\Software\CompanyName\GameName\

Screenmanager Resolution Width / Height: 해상도 설정

Screenmanager Is Fullscreen mode: 전체화면 여부

UnityGraphicsQuality: 품질 설정 레벨 (0~5)

unity.cloud_userid, unity.player_session_count: Unity에서 자동 생성하는 세션 관련 값

기타

%TEMP%\CompanyName\GameName\Crashes\: 게임 충돌 시 덤프 파일(crash.dmp) 및 오류 로그(error.log) 저장

Unreal vs Unity

항목

Unreal Engine 4

Unity 엔진

설정 저장 방식

.ini 파일 (로컬 앱데이터)

레지스트리 (PlayerPrefs) 또는 JSON 파일

로그 위치

%LOCALAPPDATA%\...\Logs\Launch.log

%APPDATA%\LocalLow\...\Player.log

레지스트리 사용 여부

기본적으로 사용 안 함

기본 사용 (설정 값, 세션 정보 저장)

설치 구조

실행 파일 + .pak + 엔진 구조

실행 파일 + _Data 폴더 구조

세이브 경로

%LOCALAPPDATA%\...\SaveGames\

%APPDATA%\LocalLow\...\

크래시 처리

CrashReportClient.exe 사용

Temp 폴더 내 덤프 및 오류 로그 저장

