#include <windows.h>
#include <tchar.h>
#include <iostream>

typedef NTSTATUS(WINAPI* pNtCreateThreadEx)(
    OUT PHANDLE hThread,
    IN ACCESS_MASK DesiredAccess,
    IN LPVOID ObjectAttributes,
    IN HANDLE ProcessHandle,
    IN LPTHREAD_START_ROUTINE StartRoutine,
    IN LPVOID Argument,
    IN ULONG CreateFlags,
    IN SIZE_T ZeroBits,
    IN SIZE_T StackSize,
    IN SIZE_T MaximumStackSize,
    OUT LPVOID AttributeList
);

BOOL SetPrivilege(LPCTSTR lpszPrivilege, BOOL bEnablePrivilege) {
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        std::wcerr << L"OpenProcessToken error: " << GetLastError() << std::endl;
        return FALSE;
    }

    if (!LookupPrivilegeValue(NULL, lpszPrivilege, &luid)) {
        std::wcerr << L"LookupPrivilegeValue error: " << GetLastError() << std::endl;
        CloseHandle(hToken);
        return FALSE;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = bEnablePrivilege ? SE_PRIVILEGE_ENABLED : 0;

    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) {
        std::wcerr << L"AdjustTokenPrivileges error: " << GetLastError() << std::endl;
        CloseHandle(hToken);
        return FALSE;
    }

    CloseHandle(hToken);
    return TRUE;
}

BOOL InjectDll_NtCreateThreadEx(DWORD dwPID, LPCTSTR szDllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);
    if (!hProcess) {
        std::wcerr << L"OpenProcess(" << dwPID << L") failed! Error: " << GetLastError() << std::endl;
        return FALSE;
    }

    SIZE_T dwBufSize = (_tcslen(szDllPath) + 1) * sizeof(TCHAR);
    LPVOID pRemoteBuf = VirtualAllocEx(hProcess, NULL, dwBufSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!pRemoteBuf) {
        std::wcerr << L"VirtualAllocEx failed! Error: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return FALSE;
    }

    if (!WriteProcessMemory(hProcess, pRemoteBuf, (LPVOID)szDllPath, dwBufSize, NULL)) {
        std::wcerr << L"WriteProcessMemory failed! Error: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return FALSE;
    }

    HMODULE hNtDll = GetModuleHandle(L"ntdll.dll");
    pNtCreateThreadEx NtCreateThreadEx = (pNtCreateThreadEx)GetProcAddress(hNtDll, "NtCreateThreadEx");

    if (!NtCreateThreadEx) {
        std::wcerr << L"GetProcAddress(NtCreateThreadEx) failed!" << std::endl;
        CloseHandle(hProcess);
        return FALSE;
    }

    LPTHREAD_START_ROUTINE pThreadProc = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryW");
    if (!pThreadProc) {
        std::wcerr << L"GetProcAddress(LoadLibraryW) failed!" << std::endl;
        CloseHandle(hProcess);
        return FALSE;
    }

    HANDLE hThread = NULL;
    NTSTATUS status = NtCreateThreadEx(&hThread, THREAD_ALL_ACCESS, NULL, hProcess, pThreadProc, pRemoteBuf, FALSE, 0, 0, 0, NULL);

    if (status != 0 || !hThread) {
        std::wcerr << L"NtCreateThreadEx failed! Status: " << status << L", Error: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return FALSE;
    }

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    CloseHandle(hProcess);

    return TRUE;
}

int _tmain(int argc, TCHAR *argv[]) {
    if (argc != 3) {
        std::wcout << L"USAGE: " << argv[0] << L" <pid> <DLL path>" << std::endl;
        return 1;
    }

    if (!SetPrivilege(SE_DEBUG_NAME, TRUE)) {
        std::wcerr << L"Failed to set privilege!" << std::endl;
        return 1;
    }

    if (InjectDll_NtCreateThreadEx((DWORD)_tstol(argv[1]), argv[2])) {
        std::wcout << L"Injection successful: " << argv[2] << std::endl;
    } else {
        std::wcerr << L"Injection failed: " << argv[2] << std::endl;
    }

    return 0;
}
