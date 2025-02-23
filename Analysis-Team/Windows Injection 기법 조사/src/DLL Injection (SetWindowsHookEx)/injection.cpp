#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <tchar.h>

// 대상 프로세스 찾기 (유니코드 지원)
DWORD GetTargetProcessId(const wchar_t *procName) {
    DWORD pid = 0;
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return 0;

    if (Process32First(hSnapshot, &pe32)) {
        do {
            if (_wcsicmp(pe32.szExeFile, procName) == 0) {  // UNICODE 비교 사용
                pid = pe32.th32ProcessID;
                break;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }
    CloseHandle(hSnapshot);
    return pid;
}

int wmain() {
    const wchar_t *targetProc = L"notepad.exe";  // 유니코드 문자열
    const wchar_t *dllPath = L"C:\\Users\\admin\\Desktop\\test.dll";  // DLL 경로 (유니코드)
    
    // 대상 프로세스 PID 찾기
    DWORD pid = GetTargetProcessId(targetProc);
    if (pid == 0) {
        wprintf(L"Target process not found.\n");
        return 1;
    }

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        wprintf(L"Failed to open target process.\n");
        return 1;
    }

    // 메모리에 DLL 경로 쓰기
    LPVOID pRemoteBuf = VirtualAllocEx(hProcess, NULL, (wcslen(dllPath) + 1) * sizeof(wchar_t), MEM_COMMIT, PAGE_READWRITE);
    WriteProcessMemory(hProcess, pRemoteBuf, (LPVOID)dllPath, (wcslen(dllPath) + 1) * sizeof(wchar_t), NULL);

    // LoadLibraryW 실행 (유니코드 지원)
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, 
        (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"), 
        pRemoteBuf, 0, NULL);
    if (!hThread) {
        wprintf(L"Failed to create remote thread.\n");
        VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }

    WaitForSingleObject(hThread, INFINITE);
    
    // 정리
    VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);

    wprintf(L"DLL injected successfully.\n");
    return 0;
}
