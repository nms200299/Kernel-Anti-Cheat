/*++

Copyright (c) 1999 - 2002  Microsoft Corporation

Module Name:

    StreamHandleContext.c

Abstract:

    This module focuses only on the stream handle context operations
    for the kernel mode filter driver.

Environment:

    Kernel mode

--*/

#include "ctx.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CtxCreateOrReplaceStreamHandleContext)
#pragma alloc_text(PAGE, CtxCreateStreamHandleContext)
#pragma alloc_text(PAGE, CtxUpdateNameInStreamHandleContext)
#endif

NTSTATUS
CtxCreateOrReplaceStreamHandleContext(
    _In_ PFLT_CALLBACK_DATA Cbd,
    _In_ BOOLEAN ReplaceIfExists,
    _Outptr_ PCTX_STREAMHANDLE_CONTEXT* StreamHandleContext,
    _Out_opt_ PBOOLEAN ContextReplaced
)
{
    NTSTATUS status;
    PCTX_STREAMHANDLE_CONTEXT streamHandleContext;
    PCTX_STREAMHANDLE_CONTEXT oldStreamHandleContext;

    PAGED_CODE();

    *StreamHandleContext = NULL;
    if (ContextReplaced != NULL) *ContextReplaced = FALSE;

    // Create a stream handle context
    DbgPrint("[Ctx][SUCCESS]: Creating stream handle context (FileObject = %p, Instance = %p)\n",
            Cbd->Iopb->TargetFileObject,
            Cbd->Iopb->TargetInstance);

    status = CtxCreateStreamHandleContext(&streamHandleContext);

    if (!NT_SUCCESS(status)) {
        DbgPrint("[Ctx][ERROR]: Failed to create stream handle context with status 0x%x. (FileObject = %p, Instance = %p)\n",
                status,
                Cbd->Iopb->TargetFileObject,
                Cbd->Iopb->TargetInstance);
        return status;
    }

    // Set the new context
    /*DbgPrint("[Ctx]: Setting stream handle context %p (FileObject = %p, Instance = %p, ReplaceIfExists = %x)\n",
            streamHandleContext,
            Cbd->Iopb->TargetFileObject,
            Cbd->Iopb->TargetInstance,
            ReplaceIfExists);*/

    status = FltSetStreamHandleContext(Cbd->Iopb->TargetInstance,
        Cbd->Iopb->TargetFileObject,
        ReplaceIfExists ? FLT_SET_CONTEXT_REPLACE_IF_EXISTS : FLT_SET_CONTEXT_KEEP_IF_EXISTS,
        streamHandleContext,
        &oldStreamHandleContext);

    if (!NT_SUCCESS(status)) {
        DbgPrint("[Ctx][ERROR]: Failed to set stream handle context with status 0x%x. (FileObject = %p, Instance = %p)\n",
                status,
                Cbd->Iopb->TargetFileObject,
                Cbd->Iopb->TargetInstance);

        FltReleaseContext(streamHandleContext);

        if (status == STATUS_FLT_CONTEXT_ALREADY_DEFINED) {
            DbgPrint("[Ctx][ERROR]: Stream handle context already defined. Retaining old stream handle context %p (FileObject = %p, Instance = %p)\n",
                    oldStreamHandleContext,
                    Cbd->Iopb->TargetFileObject,
                    Cbd->Iopb->TargetInstance);

            streamHandleContext = oldStreamHandleContext;
            status = STATUS_SUCCESS;
        }

        return status;
    }

    if (ReplaceIfExists && oldStreamHandleContext != NULL) {
        FltReleaseContext(oldStreamHandleContext);
        if (ContextReplaced != NULL) *ContextReplaced = TRUE;
    }

    *StreamHandleContext = streamHandleContext;

    return status;
}

NTSTATUS
CtxCreateStreamHandleContext(
    _Outptr_ PCTX_STREAMHANDLE_CONTEXT* StreamHandleContext
)
{
    NTSTATUS status;
    PCTX_STREAMHANDLE_CONTEXT streamHandleContext;

    PAGED_CODE();

    //DbgPrint("[Ctx]: Allocating stream handle context\n");

    status = FltAllocateContext(Globals.Filter,
        FLT_STREAMHANDLE_CONTEXT,
        CTX_STREAMHANDLE_CONTEXT_SIZE,
        PagedPool,
        &streamHandleContext);

    if (!NT_SUCCESS(status)) {
        DbgPrint("[Ctx][ERROR]: Failed to allocate stream handle context with status 0x%x\n",
                status);
        return status;
    }

    RtlZeroMemory(streamHandleContext, CTX_STREAMHANDLE_CONTEXT_SIZE);

    streamHandleContext->Resource = CtxAllocateResource();
    if (streamHandleContext->Resource == NULL) {
        FltReleaseContext(streamHandleContext);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    ExInitializeResourceLite(streamHandleContext->Resource);

    *StreamHandleContext = streamHandleContext;

    return STATUS_SUCCESS;
}

NTSTATUS
CtxUpdateNameInStreamHandleContext(
    _In_ PUNICODE_STRING DirectoryName,
    _Inout_ PCTX_STREAMHANDLE_CONTEXT StreamHandleContext
)
{
    NTSTATUS status;

    PAGED_CODE();

    if (StreamHandleContext->FileName.Buffer != NULL) {
        CtxFreeUnicodeString(&StreamHandleContext->FileName);
    }

    StreamHandleContext->FileName.MaximumLength = DirectoryName->Length;
    status = CtxAllocateUnicodeString(&StreamHandleContext->FileName);
    if (NT_SUCCESS(status)) {
        RtlCopyUnicodeString(&StreamHandleContext->FileName, DirectoryName);
    }

    return status;
}
