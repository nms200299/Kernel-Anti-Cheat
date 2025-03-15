#include "selfprotect.h"


BOOLEAN IsTxtFile(_In_ PFLT_FILE_NAME_INFORMATION name_info) {
    UNICODE_STRING test_txt_pattern, file_path;
    FltParseFileNameInformation(name_info);
    RtlInitUnicodeString(&test_txt_pattern, L"*.TXT");
    RtlInitUnicodeString(&file_path, name_info->Name.Buffer);
    if (FsRtlIsNameInExpression(&test_txt_pattern, &file_path, TRUE, NULL)) {
        return TRUE;
    }
    return FALSE;
}

BOOLEAN SelfProtectf(_In_ PFLT_FILE_NAME_INFORMATION name_info) {
    UNICODE_STRING test_txt_pattern, file_path;
    FltParseFileNameInformation(name_info);

    RtlInitUnicodeString(&test_txt_pattern, my_dir_name_driver);
    RtlInitUnicodeString(&file_path, name_info->Name.Buffer);
    if (FsRtlIsNameInExpression(&test_txt_pattern, &file_path, TRUE, NULL)) {
        return TRUE;
    }
    return FALSE;
}

NTSTATUS CtxGetFileNameInformation(
    _In_ PFLT_CALLBACK_DATA Cbd,
    _Out_ PFLT_FILE_NAME_INFORMATION* NameInfo
) {
    NTSTATUS status = FltGetFileNameInformation(Cbd, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, NameInfo);
    if (NT_SUCCESS(status)) {
        status = FltParseFileNameInformation(*NameInfo);
        if (!NT_SUCCESS(status)) {
            FltReleaseFileNameInformation(*NameInfo);
        }
    }
    return status;
}

BOOLEAN IsDirectoryChange(PUNICODE_STRING oldPath, PUNICODE_STRING newPath) {
    UNICODE_STRING oldFileName, newFileName;

    // 마지막 '\' 찾기
    USHORT oldIndex = oldPath->Length / sizeof(WCHAR);
    USHORT newIndex = newPath->Length / sizeof(WCHAR);

    while (oldIndex > 0 && oldPath->Buffer[oldIndex - 1] != L'\\') {
        oldIndex--;
    }

    while (newIndex > 0 && newPath->Buffer[newIndex - 1] != L'\\') {
        newIndex--;
    }

    // 파일명 부분만 추출
    oldFileName.Buffer = &oldPath->Buffer[oldIndex];
    oldFileName.Length = oldPath->Length - (oldIndex * sizeof(WCHAR));
    oldFileName.MaximumLength = oldFileName.Length;

    newFileName.Buffer = &newPath->Buffer[newIndex];
    newFileName.Length = newPath->Length - (newIndex * sizeof(WCHAR));
    newFileName.MaximumLength = newFileName.Length;

    // 파일 이름 비교 (대소문자 무시)
    return RtlEqualUnicodeString(&oldFileName, &newFileName, TRUE);
}

VOID TerminateProcess(HANDLE processId)
{
    PEPROCESS process;
    HANDLE processHandle;
    NTSTATUS status;

    // 프로세스 객체 가져오기
    status = PsLookupProcessByProcessId(processId, &process);
    if (!NT_SUCCESS(status)) {
        DbgPrint("[Ctx][ERROR]: Failed to get process object for PID: %lu, Status: 0x%X\n",
            (ULONG)(ULONG_PTR)processId, status);
        return;
    }

    // 프로세스 핸들 열기 (권한: PROCESS_TERMINATE)
    status = ObOpenObjectByPointer(process, OBJ_KERNEL_HANDLE, NULL, PROCESS_TERMINATE,
        *PsProcessType, KernelMode, &processHandle);
    if (!NT_SUCCESS(status)) {
        DbgPrint("[Ctx][ERROR]: Failed to open process handle for PID: %lu, Status: 0x%X\n",
            (ULONG)(ULONG_PTR)processId, status);
        return;
    }

    // 프로세스 종료 시도
    status = ZwTerminateProcess(processHandle, STATUS_ACCESS_DENIED);
    if (NT_SUCCESS(status)) {
        DbgPrint("[Ctx][INFO]: Successfully terminated process PID: %lu\n", (ULONG)(ULONG_PTR)processId);
    }
    else {
        DbgPrint("[Ctx][ERROR]: Failed to terminate process PID: %lu, Status: 0x%X\n",
            (ULONG)(ULONG_PTR)processId, status);
    }

    // 핸들 닫기
    ZwClose(processHandle);
}

