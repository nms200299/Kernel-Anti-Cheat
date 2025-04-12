#include "pch.h"
#include "FilePreOperation.h"
#include "UtilFilter.h"

FLT_PREOP_CALLBACK_STATUS
PreFileOperation(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
)
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);
    PAGED_CODE();

    BOOLEAN IsBlockOpt = FALSE;
    if (Data->Iopb->MajorFunction == IRP_MJ_CREATE) {
        if (Data->Iopb->Parameters.Create.Options & FILE_DELETE_ON_CLOSE) {
            IsBlockOpt = TRUE;
        } else {
            return FLT_PREOP_SUCCESS_NO_CALLBACK;
        }
    } // IRP_MJ_CREATE에 FILE_DELETE_ON_CLOSE 옵션이 없으면 바로 return

    NTSTATUS status;
    PFLT_FILE_NAME_INFORMATION FileInfo = NULL;
    status = GetFileInformation_(Data, &FileInfo);
    if (!NT_SUCCESS(status)) {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    } // 접근하려는 파일 정보 획득

    if (!IsProtectPath(FileInfo)) {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    } // 접근하는 경로가 보호할 경로가 아니면 return


    ULONG AccessPID = FltGetRequestorProcessId(Data);
    // 접근하는 PID 획득

    DBG_PRINT(DBG_LEVEL_DEBUG, __func__, "File Request : PID(%lu)\tIRP(%lu)\tPath(%wZ)\n", (ULONG)(ULONG_PTR)AccessPID, Data->Iopb->MajorFunction, &FileInfo->FinalComponent);

    if (IsAllowPID(AccessPID)) {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    } // 접근하는 PID가 허용된 PID면 return

    if (IsBlockOpt) {
        DBG_PRINT(DBG_LEVEL_WARN, __func__, "File Request Denied : PID(%lu)\tIRP(CREATE/DEL_ON_CLOSE)\tPath(%wZ)\n", (ULONG)(ULONG_PTR)AccessPID, Data->Iopb->MajorFunction, &FileInfo->FinalComponent);
        return FLT_OP_DENIED_(Data);
        // FILE_DELETE_ON_CLOSE 옵션이 보호할 경로에 걸리면 차단
    } 


    switch (Data->Iopb->MajorFunction) {
        case IRP_MJ_WRITE: {
            DBG_PRINT(DBG_LEVEL_WARN, __func__, "File Request Denied : PID(%lu)\tIRP(WRITE)\tPath(%wZ)\n", (ULONG)(ULONG_PTR)AccessPID, Data->Iopb->MajorFunction, &FileInfo->FinalComponent);
            return FLT_OP_DENIED_(Data);
            break;
        } // 파일 쓰기 요청 차단

        case IRP_MJ_SET_INFORMATION: {
            FILE_INFORMATION_CLASS FileInfoClass = Data->Iopb->Parameters.SetFileInformation.FileInformationClass;
            PVOID FileInfoBuffer = Data->Iopb->Parameters.SetFileInformation.InfoBuffer;

            switch (FileInfoClass) {
                case FileDispositionInformation: {
                    PFILE_DISPOSITION_INFORMATION DispositionInfo = (PFILE_DISPOSITION_INFORMATION)FileInfoBuffer;
                    if (DispositionInfo->DeleteFile) {
                        DBG_PRINT(DBG_LEVEL_WARN, __func__, "File Request Denied : PID(%lu)\tIRP(DELETE)\tPath(%wZ)\n", (ULONG)(ULONG_PTR)AccessPID, Data->Iopb->MajorFunction, &FileInfo->FinalComponent);
                        return FLT_OP_DENIED_(Data);
                    }
                    break;
                } // 파일 삭제 요청 차단 (DeleteFile() 등)

                case FileDispositionInformationEx:{
                    PFILE_DISPOSITION_INFORMATION_EX DispositionInfoEx = (PFILE_DISPOSITION_INFORMATION_EX)FileInfoBuffer;
                    if (DispositionInfoEx->Flags != FILE_DISPOSITION_DO_NOT_DELETE) {
                        DBG_PRINT(DBG_LEVEL_WARN, __func__, "File Request Denied : PID(%lu)\tIRP(DELETE_EX)\tPath(%wZ)\n", (ULONG)(ULONG_PTR)AccessPID, Data->Iopb->MajorFunction, &FileInfo->FinalComponent);
                        return FLT_OP_DENIED_(Data);
                    }
                } // 파일 삭제 요청 차단 (NtDeleteFile() 등) 
             
                case FileRenameInformation:
                case FileRenameInformationBypassAccessCheck:
                case FileRenameInformationEx:
                case FileRenameInformationExBypassAccessCheck:{
                    DBG_PRINT(DBG_LEVEL_WARN, __func__, "File Request Denied : PID(%lu)\tIRP(MOVE)\tPath(%wZ)\n", (ULONG)(ULONG_PTR)AccessPID, Data->Iopb->MajorFunction, &FileInfo->FinalComponent);
                    return FLT_OP_DENIED_(Data);
                } // 파일 이동, 이름 변경 요청 차단 (MoveFile() 등) 
            }
        }

        default:{
            return FLT_PREOP_SUCCESS_NO_CALLBACK;
            break;
        } // 위 조건에 해당하지 않은 IRP 메시지는 허용
    }
    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}