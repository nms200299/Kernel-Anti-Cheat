#include "pch.h"
#include "UtilFilter.h"

NTSTATUS GetFileInformation_(
    _In_ PFLT_CALLBACK_DATA Data,
    _Out_ PFLT_FILE_NAME_INFORMATION* FileInfo
) {
    NTSTATUS status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, FileInfo);
    if (NT_SUCCESS(status)) {
        status = FltParseFileNameInformation(*FileInfo);
        if (!NT_SUCCESS(status)) {
            FltReleaseFileNameInformation(*FileInfo);
        }
    }
    return status;
}


FLT_PREOP_CALLBACK_STATUS FLT_OP_DENIED_ (
    PFLT_CALLBACK_DATA Data
) {
    Data->IoStatus.Status = STATUS_ACCESS_DENIED;
    Data->IoStatus.Information = 0;
    return FLT_PREOP_COMPLETE;
} // 파일 요청 차단 