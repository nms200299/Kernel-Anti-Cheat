#include <windows.h>
#include <iostream>

typedef BOOL(WINAPI* DllMainPtr)(HINSTANCE, DWORD, LPVOID);

void InjectDLL(void* dllBuffer) {
    IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)dllBuffer;
    IMAGE_NT_HEADERS* ntHeaders = (IMAGE_NT_HEADERS*)((BYTE*)dllBuffer + dosHeader->e_lfanew);

    // 메모리 할당
    void* allocatedMemory = VirtualAlloc(NULL, ntHeaders->OptionalHeader.SizeOfImage, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (!allocatedMemory) {
        std::cerr << "메모리 할당 실패!" << std::endl;
        return;
    }

    // 헤더 복사
    memcpy(allocatedMemory, dllBuffer, ntHeaders->OptionalHeader.SizeOfHeaders);

    // 섹션 복사
    IMAGE_SECTION_HEADER* sectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
    for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++) {
        void* sectionDest = (BYTE*)allocatedMemory + sectionHeader[i].VirtualAddress;
        void* sectionSrc = (BYTE*)dllBuffer + sectionHeader[i].PointerToRawData;
        memcpy(sectionDest, sectionSrc, sectionHeader[i].SizeOfRawData);
    }

    // 이미지 베이스 재배치
    DWORD_PTR delta = (DWORD_PTR)allocatedMemory - ntHeaders->OptionalHeader.ImageBase;
    if (delta) {
        IMAGE_DATA_DIRECTORY relocDir = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
        if (relocDir.Size) {
            IMAGE_BASE_RELOCATION* reloc = (IMAGE_BASE_RELOCATION*)((BYTE*)allocatedMemory + relocDir.VirtualAddress);
            while (reloc->VirtualAddress) {
                DWORD numEntries = (reloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
                WORD* relocEntries = (WORD*)((BYTE*)reloc + sizeof(IMAGE_BASE_RELOCATION));

                for (DWORD i = 0; i < numEntries; i++) {
                    if (relocEntries[i]) {
                        DWORD_PTR* patchAddress = (DWORD_PTR*)((BYTE*)allocatedMemory + reloc->VirtualAddress + (relocEntries[i] & 0xFFF));
                        *patchAddress += delta;
                    }
                }
                reloc = (IMAGE_BASE_RELOCATION*)((BYTE*)reloc + reloc->SizeOfBlock);
            }
        }
    }

    // IAT 해결 (수정된 부분)
    IMAGE_DATA_DIRECTORY importDir = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (importDir.Size) {
        IMAGE_IMPORT_DESCRIPTOR* importDesc = (IMAGE_IMPORT_DESCRIPTOR*)((BYTE*)allocatedMemory + importDir.VirtualAddress);
        while (importDesc->Name) {
            char* moduleName = (char*)((BYTE*)allocatedMemory + importDesc->Name);
            HMODULE moduleHandle = LoadLibraryA(moduleName);
            if (!moduleHandle) {
                std::cerr << "모듈 로드 실패: " << moduleName << std::endl;
                return;
            }

            IMAGE_THUNK_DATA* thunkData = (IMAGE_THUNK_DATA*)((BYTE*)allocatedMemory + importDesc->FirstThunk);
            while (thunkData->u1.Function) {
                if (!(thunkData->u1.Ordinal & IMAGE_ORDINAL_FLAG)) {
                    IMAGE_IMPORT_BY_NAME* importByName = (IMAGE_IMPORT_BY_NAME*)((BYTE*)allocatedMemory + thunkData->u1.AddressOfData);
                    thunkData->u1.Function = (DWORD_PTR)GetProcAddress(moduleHandle, reinterpret_cast<LPCSTR>(importByName->Name)); // 수정된 부분
                }
                thunkData++;
            }
            importDesc++;
        }
    }

    // DLL Main 호출
    DllMainPtr DllMain = (DllMainPtr)((BYTE*)allocatedMemory + ntHeaders->OptionalHeader.AddressOfEntryPoint);
    DllMain((HINSTANCE)allocatedMemory, DLL_PROCESS_ATTACH, NULL);
}

int main() {
    // DLL 로드 (디스크에 저장하지 않고 메모리에서 직접 로드)
    HANDLE hFile = CreateFileA("test.dll", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "DLL 파일을 열 수 없습니다!" << std::endl;
        return 1;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    void* dllBuffer = VirtualAlloc(NULL, fileSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!dllBuffer) {
        std::cerr << "DLL 버퍼 할당 실패!" << std::endl;
        CloseHandle(hFile);
        return 1;
    }

    DWORD bytesRead;
    ReadFile(hFile, dllBuffer, fileSize, &bytesRead, NULL);
    CloseHandle(hFile);

    // Reflected DLL Injection 실행
    InjectDLL(dllBuffer);

    // 메모리 해제
    VirtualFree(dllBuffer, 0, MEM_RELEASE);
    return 0;
}