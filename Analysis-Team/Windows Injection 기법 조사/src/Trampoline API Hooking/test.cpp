#include <windows.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN64
#pragma pack(push, 1)
typedef struct _JMP_TRAMPOLINE {
    BYTE opcode1;       // push imm32
    DWORD lpTarget1;    // NewWriteFile 주소의 낮은 32비트
    DWORD opcode2;      // MOV [RSP+4], imm32 (상수: 0x042444C7)
    DWORD lpTarget2;    // NewWriteFile 주소의 높은 32비트
    BYTE opcode3;       // RET
} JMP_TRAMPOLINE;
#pragma pack(pop)
#else
#pragma pack(push, 1)
typedef struct _JMP_REL {
    BYTE opcode;        // JMP opcode (0xE9)
    DWORD relAddress;   // NewWriteFile까지의 상대 오프셋
} JMP_REL;
#pragma pack(pop)
#endif

// 전방 선언: 아키텍처에 따라 후킹/후킹 해제 함수 선언
#ifdef _WIN64
DWORD WINAPI Hook64();
DWORD WINAPI Unhook64();
#else
DWORD WINAPI Hook32();
DWORD WINAPI Unhook32();
#endif

#ifdef _WIN64
JMP_TRAMPOLINE orgFP;
#else
JMP_REL orgFP;
#endif

BOOL Hooked = FALSE;

INT WINAPI NewWriteFile(
    _In_ HANDLE hFile,
    _In_ LPCVOID lpBuffer,
    _In_ DWORD nNumberOfBytesToWrite,
    _Out_opt_ LPDWORD lpNumberOfBytesWritten,
    _Inout_opt_ LPOVERLAPPED lpOverlapped)
{
    MessageBoxA(NULL, (LPCSTR)lpBuffer, "Hooking API Call Success", MB_OK);

#ifdef _WIN64
    Unhook64();
#else
    Unhook32();
#endif

    BOOL ret = WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);

#ifdef _WIN64
    Hook64();
#else
    Hook32();
#endif

    return ret;
}

#ifdef _WIN64
DWORD WINAPI Hook64() {
    if (Hooked)
        return 0;

    // 대상 함수 주소 획득
    LPVOID lpOrgFunc = NULL;
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    if (hKernel32 == NULL)
        return -1;
    lpOrgFunc = GetProcAddress(hKernel32, "WriteFile");
    if (lpOrgFunc == NULL)
        return -1;

    // kernelbase.dll 리다이렉션 처리
    if (*(SHORT*)lpOrgFunc == 0x25FF) {
        HMODULE hKernelBase = GetModuleHandleA("kernelbase.dll");
        if (hKernelBase == NULL)
            return -1;
        lpOrgFunc = GetProcAddress(hKernelBase, "WriteFile");
        if (lpOrgFunc == NULL)
            return -1;
    }

    // 대상 함수의 첫 14바이트 영역 보호 해제
    DWORD dwOldProtect;
    if (!VirtualProtect(lpOrgFunc, sizeof(JMP_TRAMPOLINE), PAGE_EXECUTE_READWRITE, &dwOldProtect))
        return -1;

    // 기존 14바이트 백업
    memcpy_s(&orgFP, sizeof(JMP_TRAMPOLINE), lpOrgFunc, sizeof(JMP_TRAMPOLINE));

    // 후킹 코드 구성
    JMP_TRAMPOLINE newFuncObj;
    newFuncObj.opcode1 = 0x68;  // push imm32
    newFuncObj.lpTarget1 = (DWORD)((DWORD64)&NewWriteFile & 0xFFFFFFFF);
    newFuncObj.opcode2 = 0x042444C7;  // MOV [RSP+4], imm32
    newFuncObj.lpTarget2 = (DWORD)((DWORD64)&NewWriteFile >> 32);
    newFuncObj.opcode3 = 0xC3;  // RET

    // 후킹 코드 덮어쓰기
    memcpy_s(lpOrgFunc, sizeof(JMP_TRAMPOLINE), &newFuncObj, sizeof(JMP_TRAMPOLINE));

    // 메모리 보호 복원
    VirtualProtect(lpOrgFunc, sizeof(JMP_TRAMPOLINE), dwOldProtect, &dwOldProtect);

    Hooked = TRUE;
    return 0;
}

DWORD WINAPI Unhook64() {
    if (!Hooked)
        return 0;

    LPVOID lpOrgFunc = NULL;
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    if (hKernel32 == NULL)
        return -1;
    lpOrgFunc = GetProcAddress(hKernel32, "WriteFile");
    if (lpOrgFunc == NULL)
        return -1;

    if (*(SHORT*)lpOrgFunc == 0x25FF) {
        HMODULE hKernelBase = GetModuleHandleA("kernelbase.dll");
        if (hKernelBase == NULL)
            return -1;
        lpOrgFunc = GetProcAddress(hKernelBase, "WriteFile");
        if (lpOrgFunc == NULL)
            return -1;
    }

    DWORD dwOldProtect;
    if (!VirtualProtect(lpOrgFunc, sizeof(JMP_TRAMPOLINE), PAGE_EXECUTE_READWRITE, &dwOldProtect))
        return -1;

    // 백업해둔 원본 14바이트 복원
    memcpy_s(lpOrgFunc, sizeof(JMP_TRAMPOLINE), &orgFP, sizeof(JMP_TRAMPOLINE));

    VirtualProtect(lpOrgFunc, sizeof(JMP_TRAMPOLINE), dwOldProtect, &dwOldProtect);

    Hooked = FALSE;
    return 0;
}
#else
DWORD WINAPI Hook32() {
    if (Hooked)
        return 0;

    LPVOID lpOrgFunc = NULL;
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    if (hKernel32 == NULL)
        return -1;
    lpOrgFunc = GetProcAddress(hKernel32, "WriteFile");
    if (lpOrgFunc == NULL)
        return -1;

    if (*(SHORT*)lpOrgFunc == 0x25FF) {
        HMODULE hKernelBase = GetModuleHandleA("kernelbase.dll");
        if (hKernelBase == NULL)
            return -1;
        lpOrgFunc = GetProcAddress(hKernelBase, "WriteFile");
        if (lpOrgFunc == NULL)
            return -1;
    }

    DWORD dwOldProtect;
    if (!VirtualProtect(lpOrgFunc, sizeof(JMP_REL), PAGE_EXECUTE_READWRITE, &dwOldProtect))
        return -1;

    // 기존 5바이트 백업
    memcpy_s(&orgFP, sizeof(JMP_REL), lpOrgFunc, sizeof(JMP_REL));

    JMP_REL newFuncObj;
    newFuncObj.opcode = 0xE9; // JMP opcode
    newFuncObj.relAddress = (DWORD)((DWORD)&NewWriteFile - ((DWORD)lpOrgFunc + 5));

    memcpy_s(lpOrgFunc, sizeof(JMP_REL), &newFuncObj, sizeof(JMP_REL));
    VirtualProtect(lpOrgFunc, sizeof(JMP_REL), dwOldProtect, &dwOldProtect);

    Hooked = TRUE;
    return 0;
}

DWORD WINAPI Unhook32() {
    if (!Hooked)
        return 0;

    LPVOID lpOrgFunc = NULL;
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    if (hKernel32 == NULL)
        return -1;
    lpOrgFunc = GetProcAddress(hKernel32, "WriteFile");
    if (lpOrgFunc == NULL)
        return -1;

    if (*(SHORT*)lpOrgFunc == 0x25FF) {
        HMODULE hKernelBase = GetModuleHandleA("kernelbase.dll");
        if (hKernelBase == NULL)
            return -1;
        lpOrgFunc = GetProcAddress(hKernelBase, "WriteFile");
        if (lpOrgFunc == NULL)
            return -1;
    }

    DWORD dwOldProtect;
    if (!VirtualProtect(lpOrgFunc, sizeof(JMP_REL), PAGE_EXECUTE_READWRITE, &dwOldProtect))
        return -1;

    // 백업해둔 원본 5바이트 복원
    memcpy_s(lpOrgFunc, sizeof(JMP_REL), &orgFP, sizeof(JMP_REL));
    VirtualProtect(lpOrgFunc, sizeof(JMP_REL), dwOldProtect, &dwOldProtect);

    Hooked = FALSE;
    return 0;
}
#endif

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        MessageBoxA(NULL, "Hook Ready", "Hook Ready", MB_OK);
#ifdef _WIN64
        Hook64();
#else
        Hook32();
#endif
        break;
    case DLL_PROCESS_DETACH:
#ifdef _WIN64
        Unhook64();
#else
        Unhook32();
#endif
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}
