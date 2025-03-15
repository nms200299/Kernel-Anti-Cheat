#include "ctx.h"
#include "registry.h"

//
//  Global variables
//
CTX_GLOBAL_DATA Globals;

//
//  Local function prototypes
//
DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
);

NTSTATUS
CtxUnload(
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
);

VOID
CtxContextCleanup(
    _In_ PFLT_CONTEXT Context,
    _In_ FLT_CONTEXT_TYPE ContextType
);

//
//  Filters callback routines
//
FLT_OPERATION_REGISTRATION Callbacks[] = {

    { IRP_MJ_CREATE,
      0,
      CtxPreCreate, CtxPostCreate },

    {
      IRP_MJ_WRITE,
      0,
      CtxPreCreate,NULL },

    { IRP_MJ_READ,
      0,
      CtxPreCreate,NULL },

    { IRP_MJ_SET_INFORMATION,
      0,
      CtxPreCreate,NULL },

    { IRP_MJ_CLEANUP,
      0,
      CtxPreCleanup, NULL },

    { IRP_MJ_CLOSE,
      0,
      CtxPreClose, NULL },

    { IRP_MJ_OPERATION_END }
};

const FLT_CONTEXT_REGISTRATION ContextRegistration[] = {
    { FLT_STREAMHANDLE_CONTEXT,
      0,
      CtxContextCleanup,
      CTX_STREAMHANDLE_CONTEXT_SIZE,
      CTX_STREAMHANDLE_CONTEXT_TAG },

    { FLT_CONTEXT_END }
};

//
// Filters registration data structure
//
FLT_REGISTRATION FilterRegistration = {

    sizeof(FLT_REGISTRATION),                     //  Size
    FLT_REGISTRATION_VERSION,                     //  Version
    0,                                            //  Flags
    ContextRegistration,                          //  Context
    Callbacks,                                    //  Operation callbacks
    CtxUnload,                                    //  Filters unload routine
    NULL, NULL, NULL, NULL, NULL, NULL, NULL      //  Unused routines
};

// 
//  Filter driver initialization and unload routines
//
NTSTATUS
DriverEntry(

    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{

    UNREFERENCED_PARAMETER(RegistryPath);
    NTSTATUS status;
    DbgPrint("[MyDriver]: DriverEntry called.\n");
    ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

    RtlZeroMemory(&Globals, sizeof(Globals));

    // Register with the filter manager
    status = FltRegisterFilter(DriverObject,
        &FilterRegistration,
        &Globals.Filter);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Start filtering I/O
    status = FltStartFiltering(Globals.Filter);
    if (!NT_SUCCESS(status)) {
        FltUnregisterFilter(Globals.Filter);
        return status;

    }

    UNICODE_STRING NtDeviceName;
    UNICODE_STRING DosDevicesLinkName;

    RtlInitUnicodeString(&NtDeviceName, NT_DEVICE_NAME);
    RtlInitUnicodeString(&DosDevicesLinkName, DOS_DEVICES_LINK_NAME);

    status = IoCreateDevice(DriverObject,
        0,
        &NtDeviceName,
        FILE_DEVICE_UNKNOWN,
        0,
        FALSE,
        &g_DeviceObj);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = IoCreateSymbolicLink(&DosDevicesLinkName, &NtDeviceName);

    if (!NT_SUCCESS(status))
    {
        IoDeleteDevice(DriverObject->DeviceObject);
        return status;
    }

    RegisterCallback(DriverObject->DeviceObject);

    if (!NT_SUCCESS(status)) {
        CmUnRegisterCallback(g_Cookie);
        FltUnregisterFilter(Globals.Filter);

    }

    return status;
}

NTSTATUS
CtxUnload(
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
)
{
    UNREFERENCED_PARAMETER(Flags);

    PAGED_CODE();

    FltUnregisterFilter(Globals.Filter);
    CmUnRegisterCallback(g_Cookie);

    Globals.Filter = NULL;
    return STATUS_SUCCESS;
}

VOID
CtxContextCleanup(
    _In_ PFLT_CONTEXT Context,
    _In_ FLT_CONTEXT_TYPE ContextType
)
{
    PCTX_STREAMHANDLE_CONTEXT streamHandleContext;

    PAGED_CODE();

    if (ContextType == FLT_STREAMHANDLE_CONTEXT) {
        streamHandleContext = (PCTX_STREAMHANDLE_CONTEXT)Context;

        if (streamHandleContext->Resource != NULL) {
            ExDeleteResourceLite(streamHandleContext->Resource);
            CtxFreeResource(streamHandleContext->Resource);
        }

        if (streamHandleContext->FileName.Buffer != NULL) {
            CtxFreeUnicodeString(&streamHandleContext->FileName);
        }
    }
}
