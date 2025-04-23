#include "ctx.h"
#include "selfprotect.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CtxCreateOrReplaceStreamHandleContext)
#pragma alloc_text(PAGE, CtxPreCreate)
#pragma alloc_text(PAGE, CtxPostCreate)
#pragma alloc_text(PAGE, CtxPreCleanup)
#pragma alloc_text(PAGE, CtxPreClose)
#endif

FLT_PREOP_CALLBACK_STATUS
CtxPreCreate(
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
)
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    PAGED_CODE();

    PFLT_FILE_NAME_INFORMATION nameInfo = NULL;
    NTSTATUS status;
    BOOLEAN IsBlock = FALSE;

    status = CtxGetFileNameInformation(Cbd, &nameInfo);
    if (!NT_SUCCESS(status)) {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }
    ULONG processId = FltGetRequestorProcessId(Cbd);
    if (SelfProtectf(nameInfo)) {
        DbgPrint("[Ctx][PRE][SelfProtect]: SelfProtecting file : %wZ From [%lu], IRP : [%lu]\n", &nameInfo->FinalComponent, (ULONG)(ULONG_PTR)processId, Cbd->Iopb->MajorFunction);
        if (processId != (0 || 4)) {
            switch (Cbd->Iopb->MajorFunction) {
            case IRP_MJ_CREATE: {
                ULONG options = Cbd->Iopb->Parameters.Create.Options;

                if (options & FILE_DELETE_ON_CLOSE) {
                    DbgPrint("[CTX][PRE][CREATE][DELETE]: File delete will be blocked : %wZ from process %lu\n", &nameInfo->FinalComponent, (ULONG)(ULONG_PTR)processId);
                    IsBlock = TRUE;
                }
                break;
            }
            case IRP_MJ_WRITE: {
                DbgPrint("[CTX][PRE][WRITE]: File WRITE will be blocked : %wZ from process %lu\n", &nameInfo->FinalComponent, (ULONG)(ULONG_PTR)processId);
                IsBlock = TRUE;
                break;
            }
            case IRP_MJ_SET_INFORMATION: {
                FILE_INFORMATION_CLASS fileInfoClass = Cbd->Iopb->Parameters.SetFileInformation.FileInformationClass;
                PVOID fileInfoBuffer = Cbd->Iopb->Parameters.SetFileInformation.InfoBuffer;
                if (fileInfoClass == FileDispositionInformation || fileInfoClass == FileDispositionInformationEx) {
                    PFILE_DISPOSITION_INFORMATION dispositionInfo = (PFILE_DISPOSITION_INFORMATION)fileInfoBuffer;
                    if (dispositionInfo->DeleteFile) {
                        DbgPrint("[Ctx][PRE][SelfProtect]: Blocking delete attempt on %wZ by process %lu\n",
                            &nameInfo->FinalComponent, (ULONG)(ULONG_PTR)processId);
                            IsBlock = TRUE;
                    }
                }
                if (fileInfoClass == FileDispositionInformationEx) {
                    PFILE_DISPOSITION_INFORMATION_EX dispositionInfoEx = (PFILE_DISPOSITION_INFORMATION_EX)fileInfoBuffer;

                    if (dispositionInfoEx->Flags & FILE_DISPOSITION_DELETE) {
                        DbgPrint("[Ctx][PRE][SelfProtect]: Blocking delete_EX attempt on %wZ by process %lu\n",
                            &nameInfo->FinalComponent, (ULONG)(ULONG_PTR)processId);
                            IsBlock = TRUE;
                    }
                }
                if (fileInfoClass == FileRenameInformation) {
                    DbgPrint("[CTX][PRE][RENAME]: File RENAME will be blocked : %wZ from process %lu\n", &nameInfo->FinalComponent, (ULONG)(ULONG_PTR)processId);
                    IsBlock = TRUE;
                }
                break;
            }
            default:
                break;
            }
        }
    }
    
    if (IsTxtFile(nameInfo)) {
        DbgPrint("[Ctx][PRE]: Tracking file: %wZ\n", &nameInfo->Name);

        if (Cbd->Iopb->MajorFunction != IRP_MJ_CREATE) {
            PCTX_STREAMHANDLE_CONTEXT streamHandleContext = NULL;
            status = FltGetStreamHandleContext(Cbd->Iopb->TargetInstance,
                Cbd->Iopb->TargetFileObject,
                &streamHandleContext);

            if (NT_SUCCESS(status) && streamHandleContext != NULL) {
                switch (Cbd->Iopb->MajorFunction) {
                case IRP_MJ_READ:
                    DbgPrint("[Ctx][PRE][READ]: File Name: %wZ\n", &streamHandleContext->FileName);
                    break;
                case IRP_MJ_WRITE:
                    DbgPrint("[Ctx][PRE][WRITE]: File Name: %wZ\n", &streamHandleContext->FileName);
                    break;
                case IRP_MJ_SET_INFORMATION: {
                    FILE_INFORMATION_CLASS fileInfoClass = Cbd->Iopb->Parameters.SetFileInformation.FileInformationClass;
                    PVOID fileInfoBuffer = Cbd->Iopb->Parameters.SetFileInformation.InfoBuffer;

                    switch (fileInfoClass) {
                    case FileRenameInformation: {
                        PFILE_RENAME_INFORMATION renameInfo = (PFILE_RENAME_INFORMATION)fileInfoBuffer;
                        if (renameInfo->FileNameLength > 0) {
                            UNICODE_STRING newFileName;
                            newFileName.Buffer = renameInfo->FileName;
                            newFileName.Length = (USHORT)renameInfo->FileNameLength;
                            newFileName.MaximumLength = (USHORT)renameInfo->FileNameLength;
                            if (IsDirectoryChange(&streamHandleContext->FileName, &newFileName)) {
                                DbgPrint("[Ctx][PRE][SET_INFO][RENAME]: Changing file's Directory %wZ\n", &streamHandleContext->FileName);
                            }
                            else {
                                DbgPrint("[Ctx][PRE][SET_INFO][RENAME]: Renaming file %wZ\n", &streamHandleContext->FileName);

                            }
                            status = CtxUpdateNameInStreamHandleContext(&newFileName, streamHandleContext);
                            if (NT_SUCCESS(status)) {
                                DbgPrint("[Ctx][PRE][SET_INFO][RENAME]: Updated file name/Directory to %wZ\n", &streamHandleContext->FileName);
                            }
                            else {
                                DbgPrint("[Ctx][PRE][SET_INFO][ERROR]: Failed to update file name in context.\n");
                            }
                        }
                        break;
                    }
                    case FileDispositionInformation: {
                        PFILE_DISPOSITION_INFORMATION dispositionInfo = (PFILE_DISPOSITION_INFORMATION)fileInfoBuffer;
                        if (dispositionInfo->DeleteFile) {
                            DbgPrint("[Ctx][PRE][SET_INFO][DELETE]: File %wZ marked for deletion.\n", &streamHandleContext->FileName);
                        }
                        break;
                    }
                    case FileDispositionInformationEx: {
                        PFILE_DISPOSITION_INFORMATION dispositionInfoex = (PFILE_DISPOSITION_INFORMATION)fileInfoBuffer;
                        if (dispositionInfoex->DeleteFile) {
                            DbgPrint("[Ctx][PRE][SET_INFO][DELETE]: File %wZ marked for deletion.\n", &streamHandleContext->FileName);
                        }
                        break;
                    }
                    default:
                        break;
                    }
                    break;
                }
                default:
                    break;
                }
                FltReleaseContext(streamHandleContext);
            }
            else {
                DbgPrint("[Ctx][PRE][ERROR]: Failed to get stream handle context or context not found.\n");
            }
        }
        else {
            DbgPrint("[Ctx][PRE][CREATE]: IRP_MJ_CREATE CALLED.\n");
            FltReleaseFileNameInformation(nameInfo);
            return FLT_PREOP_SUCCESS_WITH_CALLBACK;
        }
    }
    
    FltReleaseFileNameInformation(nameInfo);

    if (IsBlock)
    {
        TerminateProcess((HANDLE)processId);

        // 삭제를 차단하려면 STATUS_ACCESS_DENIED를 반환해야 함
        Cbd->IoStatus.Status = STATUS_ACCESS_DENIED;
        Cbd->IoStatus.Information = 0;
        return FLT_PREOP_COMPLETE;  // 요청 차단
    }
    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}



FLT_POSTOP_CALLBACK_STATUS
CtxPostCreate(
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Inout_opt_ PVOID CbdContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
)
{
    PCTX_STREAMHANDLE_CONTEXT streamHandleContext = NULL;
    BOOLEAN streamHandleContextReplaced;
    NTSTATUS status;

    PFLT_FILE_NAME_INFORMATION nameInfo = NULL;

    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(CbdContext);

    PAGED_CODE();

    //DbgPrint("[Ctx]: CtxPostCreate -> Enter\n");

   // If the create operation failed, skip
    if (!NT_SUCCESS(Cbd->IoStatus.Status)) {
        goto Cleanup;
    }

    // Create or replace stream handle context
    status = CtxCreateOrReplaceStreamHandleContext(Cbd, TRUE, &streamHandleContext, &streamHandleContextReplaced);

    if (!NT_SUCCESS(status)) {
        DbgPrint("[Ctx][ERROR]: Failed to create/replace stream handle context, Status = 0x%x\n", status);
        goto Cleanup;
    }

    DbgPrint("[Ctx][SUCCESS]: Created/Replaced Stream Handle Context\n");

    status = CtxGetFileNameInformation(Cbd, &nameInfo);

    if (!NT_SUCCESS(status)) {
        /*DbgPrint
            ("[Ctx]: Failed to parse file name information. Status: 0x%X\n", status);*/
        FltReleaseFileNameInformation(nameInfo);
        goto Cleanup;
    }

    // Check if the file name matches ".txt"
    if (IsTxtFile(nameInfo)) {
        DbgPrint
        ("[Ctx][POST]: Tracking file: %wZ\n", &nameInfo->Name);
        status = CtxUpdateNameInStreamHandleContext(&nameInfo->Name, streamHandleContext);
        if (!NT_SUCCESS(status)) {
            DbgPrint("[Ctx][ERROR]: Failed to update file name in context. Status: 0x%x\n", status);
        }
        //ACCESS_MASK access = Cbd->Iopb->Parameters.Create.SecurityContext->DesiredAccess;
        ULONG options = Cbd->Iopb->Parameters.Create.Options;

        if (options & FILE_DELETE_ON_CLOSE) {
            DbgPrint("[CTX][POST][CREATE][DELETE]: File will be deleted on close: %wZ\n", &streamHandleContext->FileName);
        }
        FltReleaseFileNameInformation(nameInfo);
    }
Cleanup:
    if (streamHandleContext != NULL) {
        FltReleaseContext(streamHandleContext);
    }

    // DbgPrint("[Ctx]: CtxPostCreate -> Exit\n");

    return FLT_POSTOP_FINISHED_PROCESSING;
}


FLT_PREOP_CALLBACK_STATUS
CtxPreCleanup(
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
)
{
    PCTX_STREAMHANDLE_CONTEXT streamHandleContext = NULL;
    NTSTATUS status;

    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    PAGED_CODE();

    // DbgPrint("[Ctx]: CtxPreCleanup -> Enter\n");

    // Get stream handle context
    status = FltGetStreamHandleContext(Cbd->Iopb->TargetInstance, Cbd->Iopb->TargetFileObject, &streamHandleContext);
    if (!NT_SUCCESS(status)) {
        //DbgPrint("[Ctx]: Failed to get stream handle context, Status = 0x%x\n", status);
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }


    PFILE_OBJECT fileObject = Cbd->Iopb->TargetFileObject;
    FILE_STANDARD_INFORMATION fileInfo;

    status = FltQueryInformationFile(FltObjects->Instance, fileObject, &fileInfo, sizeof(FILE_STANDARD_INFORMATION), FileStandardInformation, NULL);

    if (NT_SUCCESS(status) && fileInfo.DeletePending) {
        DbgPrint("[CTX][CLEAN][DELETE]: File %wZ is permanently deleted.\n", &streamHandleContext->FileName);
    }


    FltReleaseContext(streamHandleContext);

    DbgPrint("[Ctx][CLEAN]: CtxPreCleanup -> Exit\n");

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


FLT_PREOP_CALLBACK_STATUS
CtxPreClose(
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
)
{
    PCTX_STREAMHANDLE_CONTEXT streamHandleContext = NULL;
    NTSTATUS status;

    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    PAGED_CODE();

    //DbgPrint("[Ctx]: CtxPreClose -> Enter\n");

    // Get stream handle context
    status = FltGetStreamHandleContext(Cbd->Iopb->TargetInstance, Cbd->Iopb->TargetFileObject, &streamHandleContext);
    if (!NT_SUCCESS(status)) {
        //DbgPrint("[Ctx]: Failed to get stream handle context, Status = 0x%x\n", status);
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }



    FltReleaseContext(streamHandleContext);

    DbgPrint("[Ctx][CLOSE]: CtxPreClose -> Exit\n");

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}