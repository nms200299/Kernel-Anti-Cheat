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
    } // IRP_MJ_CREATE�� FILE_DELETE_ON_CLOSE �ɼ��� ������ �ٷ� return

    NTSTATUS status;
    PFLT_FILE_NAME_INFORMATION FileInfo = NULL;
    status = GetFileInformation_(Data, &FileInfo);
    if (!NT_SUCCESS(status)) {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    } // �����Ϸ��� ���� ���� ȹ��

    if (!IsProtectPath(FileInfo)) {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    } // �����ϴ� ��ΰ� ��ȣ�� ��ΰ� �ƴϸ� return


    ULONG AccessPID = FltGetRequestorProcessId(Data);
    // �����ϴ� PID ȹ��

    DBG_PRINT(DBG_LEVEL_DEBUG, __func__, "File Request : PID(%lu)\tIRP(%lu)\tPath(%wZ)\n", (ULONG)(ULONG_PTR)AccessPID, Data->Iopb->MajorFunction, &FileInfo->FinalComponent);

    if (IsAllowPID(AccessPID)) {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    } // �����ϴ� PID�� ���� PID�� return

    if (IsBlockOpt) {
        DBG_PRINT(DBG_LEVEL_WARN, __func__, "File Request Denied : PID(%lu)\tIRP(CREATE/DEL_ON_CLOSE)\tPath(%wZ)\n", (ULONG)(ULONG_PTR)AccessPID, Data->Iopb->MajorFunction, &FileInfo->FinalComponent);
        return FLT_OP_DENIED_(Data);
        // FILE_DELETE_ON_CLOSE �ɼ��� ��ȣ�� ��ο� �ɸ��� ����
    } 


    switch (Data->Iopb->MajorFunction) {
        case IRP_MJ_WRITE: {
            DBG_PRINT(DBG_LEVEL_WARN, __func__, "File Request Denied : PID(%lu)\tIRP(WRITE)\tPath(%wZ)\n", (ULONG)(ULONG_PTR)AccessPID, Data->Iopb->MajorFunction, &FileInfo->FinalComponent);
            return FLT_OP_DENIED_(Data);
            break;
        } // ���� ���� ��û ����

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
                } // ���� ���� ��û ���� (DeleteFile() ��)

                case FileDispositionInformationEx:{
                    PFILE_DISPOSITION_INFORMATION_EX DispositionInfoEx = (PFILE_DISPOSITION_INFORMATION_EX)FileInfoBuffer;
                    if (DispositionInfoEx->Flags != FILE_DISPOSITION_DO_NOT_DELETE) {
                        DBG_PRINT(DBG_LEVEL_WARN, __func__, "File Request Denied : PID(%lu)\tIRP(DELETE_EX)\tPath(%wZ)\n", (ULONG)(ULONG_PTR)AccessPID, Data->Iopb->MajorFunction, &FileInfo->FinalComponent);
                        return FLT_OP_DENIED_(Data);
                    }
                } // ���� ���� ��û ���� (NtDeleteFile() ��) 
             
                case FileRenameInformation:
                case FileRenameInformationBypassAccessCheck:
                case FileRenameInformationEx:
                case FileRenameInformationExBypassAccessCheck:{
                    DBG_PRINT(DBG_LEVEL_WARN, __func__, "File Request Denied : PID(%lu)\tIRP(MOVE)\tPath(%wZ)\n", (ULONG)(ULONG_PTR)AccessPID, Data->Iopb->MajorFunction, &FileInfo->FinalComponent);
                    return FLT_OP_DENIED_(Data);
                } // ���� �̵�, �̸� ���� ��û ���� (MoveFile() ��) 
            }
        }

        default:{
            return FLT_PREOP_SUCCESS_NO_CALLBACK;
            break;
        } // �� ���ǿ� �ش����� ���� IRP �޽����� ���
    }
    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}