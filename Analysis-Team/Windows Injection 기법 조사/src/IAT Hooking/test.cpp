#include <windows.h>
#include <stdio.h>

typedef ULONG_PTR PTR;
typedef ULONG_PTR* PPTR;

PROC lpApiWriteFile = NULL;
PROC lpIATWriteFile = NULL;

typedef INT(WINAPI* lpWriteFile)(
    _In_ HANDLE hFile,
    _In_ LPCVOID lpBuffer,
    _In_ DWORD nNumberOfBytesToWrite,
    _Out_opt_ LPDWORD lpNumberOfBytesWritten,
    _Inout_opt_ LPOVERLAPPED lpOverlapped);

INT WINAPI MyWriteFile(
    _In_ HANDLE hFile,
    _In_ LPCVOID lpBuffer,
    _In_ DWORD nNumberOfBytesToWrite,
    _Out_opt_ LPDWORD lpNumberOfBytesWritten,
    _Inout_opt_ LPOVERLAPPED lpOverlapped)
{
    MessageBoxA(NULL, (LPCSTR)lpBuffer, "Hooking API Call Success", MB_OK);
    return ((lpWriteFile)lpApiWriteFile)(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}

PTR GetFunctionTable(HMODULE hModule, LPCSTR lpszDll, LPCSTR lpszProc) {
    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((PBYTE)hModule + *(PDWORD)((PBYTE)hModule + 0x3c));
    PIMAGE_DATA_DIRECTORY pImportDirectory = &pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)((PBYTE)hModule + pImportDirectory->VirtualAddress);
    
    DWORD dwCount = 0;
    while (pImportDescriptor->OriginalFirstThunk && pImportDescriptor->FirstThunk) {
        LPCSTR szDllName = (LPCSTR)((PBYTE)hModule + pImportDescriptor->Name);
        // 대소문자 구분 없이 비교 (_stricmp 사용)
        if (!_stricmp(lpszDll, szDllName)) {
            PPTR pOriginalThunk = (PPTR)((PBYTE)hModule + pImportDescriptor->OriginalFirstThunk);
            PTR pIAT = (PTR)((PBYTE)hModule + pImportDescriptor->FirstThunk);
            while (*pOriginalThunk) {
                PIMAGE_IMPORT_BY_NAME pImportByName = (PIMAGE_IMPORT_BY_NAME)((PBYTE)hModule + *pOriginalThunk);
                if (!_stricmp(lpszProc, (LPCSTR)pImportByName->Name)) {
                    return pIAT + dwCount;
                }
                dwCount += sizeof(PTR);
                pOriginalThunk++;
            }
        }
        pImportDescriptor++;
    }
    return 0;
}

PTR PatchAddress(PTR lpOrgProc, PTR lpNewProc) {
    DWORD dwOrgProtect = 0;
    PTR procTemp = *((PPTR)lpOrgProc);
    if (VirtualProtect((LPVOID)lpOrgProc, sizeof(PTR), PAGE_EXECUTE_READWRITE, &dwOrgProtect)) {
        *((PPTR)lpOrgProc) = lpNewProc;
        VirtualProtect((LPVOID)lpOrgProc, sizeof(PTR), dwOrgProtect, &dwOrgProtect);
    }
    return procTemp;
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD ul_reason_for_call,
    LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        HMODULE hTarget = GetModuleHandleA("notepad.exe");
        if (hTarget == NULL) {
            hTarget = GetModuleHandleA(NULL);
        }
        
        // notepad.exe(또는 기본 모듈)의 IAT에서 kernel32.dll 내 WriteFile 함수의 주소를 찾습니다.
        lpIATWriteFile = (PROC)GetFunctionTable(hTarget, "kernel32.dll", "WriteFile");
        if (lpIATWriteFile == 0) {
            MessageBoxA(NULL, "Failed to find WriteFile in IAT", "Error", MB_OK);
            break;
        }
        
        lpApiWriteFile = (PROC)PatchAddress((PTR)lpIATWriteFile, (PTR)MyWriteFile);
        break;
    
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
