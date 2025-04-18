#include "pch.h"
#include "FilePreOperation.h"
#include "UtilFilter.h"

FLT_PREOP_CALLBACK_STATUS
FilePreOperation(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
)
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);
    PAGED_CODE();

    // CREATE 요청인데 DELETE_ON_CLOSE 옵션이 없는 경우는 무시
    if (Data->Iopb->MajorFunction == IRP_MJ_CREATE) {
        if (!(Data->Iopb->Parameters.Create.Options & FILE_DELETE_ON_CLOSE)) {
            return FLT_PREOP_SUCCESS_NO_CALLBACK;
        }
    }

    NTSTATUS status;
    PFLT_FILE_NAME_INFORMATION FileInfo = NULL;
    status = GetFileInformation_(Data, &FileInfo);
    if (!NT_SUCCESS(status)) {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }
    DBG_PRINT(DBG_LEVEL_DEBUG, __func__, "Full path accessed: %wZ\n", &FileInfo->Name);


    // 경로 해시맵에서 정책 조회
    FILTER_ENTRY* entry = LookupFilterEntry(&FileInfo->Name);
    if (!entry) {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    // 접근한 프로세스 PID 확인
    ULONG AccessPID = FltGetRequestorProcessId(Data);

    // 디버깅용 로그 출력
    DBG_PRINT(DBG_LEVEL_DEBUG, __func__,
        "File Request : PID(%lu)\tIRP(%lu)\tPath: %wZ\n",
        (ULONG)(ULONG_PTR)AccessPID, Data->Iopb->MajorFunction, &FileInfo->Name);

    // 정책 분기 처리
    switch (entry->Action) {
    case FILTER_ACTION_ALLOW:
        return FLT_PREOP_SUCCESS_NO_CALLBACK;

    case FILTER_ACTION_LOG:
        DBG_PRINT(DBG_LEVEL_NOTI, __func__,
            "File Access LOG: PID(%lu), Path: %wZ\n",
            (ULONG)(ULONG_PTR)AccessPID, &FileInfo->Name);
        return FLT_PREOP_SUCCESS_NO_CALLBACK;

    case FILTER_ACTION_BLOCK:
    default:
        DBG_PRINT(DBG_LEVEL_WARN, __func__,
            "File Access BLOCKED: PID(%lu), Path: %wZ\n",
            (ULONG)(ULONG_PTR)AccessPID, &FileInfo->Name);
        return FLT_OP_DENIED_(Data);
    }
}
