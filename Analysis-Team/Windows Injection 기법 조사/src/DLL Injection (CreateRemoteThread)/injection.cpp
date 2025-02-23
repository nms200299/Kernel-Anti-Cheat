#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <TlHelp32.h>
#include <shlwapi.h>
#include <conio.h>
#include <stdio.h>

#pragma comment(lib, "shlwapi.lib")

BOOL Inject(DWORD pID, const char* DLL_NAME);
DWORD GetTargetThreadIDFromProcName(const char * ProcName);

int main(int argc, char* argv[]) {
    DWORD pID = GetTargetThreadIDFromProcName("notepad.exe");

    if (pID == 0) {
        printf("Target process not found!\n");
        return 1;
    }

    char buf[MAX_PATH] = { 0 };
    GetFullPathNameA("test.dll", MAX_PATH, buf, NULL);
    printf("DLL Path: %s\n", buf);

    if (!Inject(pID, buf)) {
        printf("DLL Injection Failed\n");
    } else {
        printf("DLL Successfully Injected\n");
    }

    _getch();
    return 0;
}

BOOL Inject(DWORD pID, const char* DLL_NAME) {
    HANDLE Proc;
    LPVOID RemoteString, LoadLibraryAddr;
    char buf[100] = { 0 };

    if (!pID) {
        sprintf(buf, "Invalid process id: %d\r\n", GetLastError());
        MessageBoxA(NULL, buf, "Loader", MB_OK);
        printf("%s", buf);
        return FALSE;
    }

    Proc = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | 
                       PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, 
                       FALSE, pID);

    if (!Proc) {
        sprintf(buf, "OpenProcess() failed: %d", GetLastError());
        MessageBoxA(NULL, buf, "Loader", MB_OK);
        printf("%s\n", buf);
        return FALSE;
    }

    LoadLibraryAddr = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");

    if (LoadLibraryAddr != NULL) {
        RemoteString = VirtualAllocEx(Proc, NULL, strlen(DLL_NAME) + 1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        
        if (!RemoteString) {
            sprintf(buf, "VirtualAllocEx failed: %d", GetLastError());
            MessageBoxA(NULL, buf, "Loader", MB_OK);
            printf("%s\n", buf);
            CloseHandle(Proc);
            return FALSE;
        }

        if (!WriteProcessMemory(Proc, RemoteString, DLL_NAME, strlen(DLL_NAME) + 1, NULL)) {
            sprintf(buf, "WriteProcessMemory failed: %d", GetLastError());
            MessageBoxA(NULL, buf, "Loader", MB_OK);
            printf("%s\n", buf);
            VirtualFreeEx(Proc, RemoteString, 0, MEM_RELEASE);
            CloseHandle(Proc);
            return FALSE;
        }

        HANDLE hThread = CreateRemoteThread(Proc, NULL, 0, 
                                            (LPTHREAD_START_ROUTINE)LoadLibraryAddr, 
                                            RemoteString, 0, NULL);

        if (!hThread) {
            sprintf(buf, "CreateRemoteThread failed: %d", GetLastError());
            MessageBoxA(NULL, buf, "Loader", MB_OK);
            printf("%s\n", buf);
            VirtualFreeEx(Proc, RemoteString, 0, MEM_RELEASE);
            CloseHandle(Proc);
            return FALSE;
        }

        CloseHandle(hThread);
    } else {
        sprintf(buf, "GetProcAddress() failed: %d", GetLastError());
        MessageBoxA(NULL, buf, "Loader", MB_OK);
        printf("%s\n", buf);
        CloseHandle(Proc);
        return FALSE;
    }

    CloseHandle(Proc);
    return TRUE;
}

DWORD GetTargetThreadIDFromProcName(const char* ProcName) {
    PROCESSENTRY32 pe;
    HANDLE thSnapShot;
    BOOL retval;
    char exeName[MAX_PATH];  // ANSI 문자열 저장용

    thSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    
    if (thSnapShot == INVALID_HANDLE_VALUE) {
        MessageBoxA(NULL, "Error: Unable to create toolhelp snapshot", "Loader", MB_OK);
        printf("Error: Unable to create toolhelp snapshot\n");
        return 0;
    }
    
    pe.dwSize = sizeof(PROCESSENTRY32);

    retval = Process32First(thSnapShot, &pe);
    while (retval) {
        // 유니코드 → ANSI 변환
        WideCharToMultiByte(CP_ACP, 0, pe.szExeFile, -1, exeName, MAX_PATH, NULL, NULL);

        if (_stricmp(exeName, ProcName) == 0) {  // 변환된 ANSI 문자열과 비교
            CloseHandle(thSnapShot);
            return pe.th32ProcessID;
        }
        retval = Process32Next(thSnapShot, &pe);
    }

    CloseHandle(thSnapShot);
    return 0;
}
