# AssaultCube 생성 파일 및 레지스트리 정리

Sandboxie를 통해 분석된 AssaultCube 실행 시 생성되는 주요 파일, 설정, 로그, 레지스트리 경로를 정리한 문서

---

## 주요 파일

- `C:\Program Files (x86)\AssaultCube 1.3.0.2\bin_win32\ac_client.exe`
  - AssaultCube 게임 실행 파일
- `...\profile\clientlog.txt`
  - 설치 디렉토리에 저장되는 실행 로그
- `...\config\autostart\`
  - 자동 실행 관련 설정 디렉토리 (내용 없음 또는 향후 확장 가능성 있음)

---

## 설정 파일

- `C:\Users\%USERNAME%\Documents\My Games\AssaultCube\v1.3\config\`
  - `init.cfg`: 초기 렌더링, 그래픽, 사운드 등 전역 설정
  - `saved.cfg`: 현재 저장된 사용자 설정
  - `servers.cfg`: 사용자 저장 서버 목록
  - `mapmodelattributes.cfg`: 맵 및 모델 속성 설정
  - `saved-old-1.cfg`, `init-old-1.cfg`: 설정 백업
  - `history`: 비어 있음 (0바이트)

---

## 로그

- `clientlog.txt`: 게임 실행 시 생성되는 로그
  - 위치:
    - `Documents\My Games\AssaultCube\v1.3\clientlog.txt`
    - `Program Files\AssaultCube\profile\clientlog.txt`

---

## 고유값 / 세션 정보

- `private\entropy.dat`
  - 세션 관련 고유값 저장 (엔트로피 기반 식별자)

---

## 레지스트리

### 확인된 키 경로

- `HIVE_ROOT\Software\AssaultCube`
  - 실제 Windows 경로 기준:
    ```
    HKEY_USERS\S-1-5-21-...\Software\AssaultCube
    ```
  - Sandboxie가 `RegHive`에 저장한 실제 설정 키 위치
  - Resolution, Fullscreen, UserID 등 설정값 저장 가능성 있음

### 보호 대상 아님

- `HIVE_ROOT\machine\software\classes\CLSID\...`
- `HIVE_ROOT\user\current\software\Microsoft\Windows\CurrentVersion\Explorer\...`
- `HIVE_ROOT\user\current_classes\software\Microsoft\Windows\Shell\...`

> 위 키들은 AssaultCube 실행 중 자동으로 생성된 시스템 캐시/경로로,  
> 게임 설정과는 무관하며 보호 필요 없음

---

## 기타 경로 (참고용)

- `C:\Windows\SbiePst.dat`: Sandboxie 환경 정보
- `%LOCALAPPDATA%\Microsoft\Windows\Explorer\iconcache*.db`: 아이콘 캐시
- `%LOCALAPPDATA%\NVIDIA\GLCache\`: 쉐이더 캐시

---
