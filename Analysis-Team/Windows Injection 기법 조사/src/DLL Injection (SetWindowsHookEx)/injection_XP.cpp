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
    const wchar_t *targetProc = L"notepad.exe";  // 대상 프로세스 이름
    const wchar_t *dllPath = L"C:\\test.dll";      // 인젝션할 DLL 경로

    // 대상 프로세스 PID 찾기
    DWORD pid = GetTargetProcessId(targetProc);
    if (pid == 0) {
        wprintf(L"Target process not found.\n");
        return 1;
    }

    // 필요한 최소 권한만 요청
    DWORD dwDesiredAccess = PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE;
    HANDLE hProcess = OpenProcess(dwDesiredAccess, FALSE, pid);
    if (!hProcess) {
        wprintf(L"Failed to open target process.\n");
        return 1;
    }

    // 대상 프로세스의 메모리에 DLL 경로 문자열 쓰기
    size_t pathSize = (wcslen(dllPath) + 1) * sizeof(wchar_t);
    LPVOID pRemoteBuf = VirtualAllocEx(hProcess, NULL, pathSize, MEM_COMMIT, PAGE_READWRITE);
    if (!pRemoteBuf) {
        wprintf(L"Failed to allocate memory in target process.\n");
        CloseHandle(hProcess);
        return 1;
    }
    
    if (!WriteProcessMemory(hProcess, pRemoteBuf, (LPVOID)dllPath, pathSize, NULL)) {
        wprintf(L"Failed to write to target process memory.\n");
        VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }

    // LoadLibraryW 함수를 이용하여 DLL을 로드할 원격 스레드 생성
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, 
        (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"), 
        pRemoteBuf, 0, NULL);
    if (!hThread) {
        wprintf(L"Failed to create remote thread.\n");
        VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }

    // 원격 스레드가 완료될 때까지 대기
    WaitForSingleObject(hThread, INFINITE);
    
    // 할당한 메모리 해제 및 핸들 정리
    VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);

    wprintf(L"DLL injected successfully.\n");
    return 0;
}
