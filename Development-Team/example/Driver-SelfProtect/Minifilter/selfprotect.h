#pragma once
#include "miniflt.h"


#define my_dir_name_driver L"*\\WINDOWS\\SYSTEM32\\DRIVERS\\MINIFILTER_CONTEXT64.SYS"
#define my_reg_dir L"*\\WINDOWS\\SYSTEM32\\Config\\"

#ifndef PROCESS_TERMINATE
#define PROCESS_TERMINATE (0x0001) // 수동으로 정의
#endif


BOOLEAN IsTxtFile(_In_ PFLT_FILE_NAME_INFORMATION name_info);

BOOLEAN SelfProtectf(_In_ PFLT_FILE_NAME_INFORMATION name_info);

NTSTATUS CtxGetFileNameInformation(
    _In_ PFLT_CALLBACK_DATA Cbd,
    _Out_ PFLT_FILE_NAME_INFORMATION* NameInfo
);

BOOLEAN IsDirectoryChange(PUNICODE_STRING oldPath, PUNICODE_STRING newPath);

VOID TerminateProcess(HANDLE processId);