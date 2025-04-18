#include "pch.h"
#include "Registration.h"

_gHandle gHandle;

NTSTATUS
InstanceSetupCallback(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
)
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(VolumeDeviceType);
    UNREFERENCED_PARAMETER(VolumeFilesystemType);

    DbgPrint("[AntiCheat] InstanceSetupCallback called\n");
    return STATUS_SUCCESS;
}

FLT_OPERATION_REGISTRATION OperationCallback[] = {
    {IRP_MJ_CREATE,
        0, FilePreOperation, NULL},
    {IRP_MJ_WRITE,
        0, FilePreOperation, NULL},
    {IRP_MJ_SET_INFORMATION,
        0, FilePreOperation, NULL},
    { IRP_MJ_OPERATION_END }
};

NTSTATUS FilterUnloadCallback(
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
)
{
    UNREFERENCED_PARAMETER(Flags);
    PAGED_CODE();
    FltUnregisterFilter(gHandle.FilterHandle);
    gHandle.FilterHandle = NULL;
    return STATUS_SUCCESS;
}


FLT_REGISTRATION FilterRegistration = {
    sizeof(FLT_REGISTRATION),                       //  Size
    FLT_REGISTRATION_VERSION,                       //  Version
    0,                                              //  Flags
    NULL,                                           //  Context
    OperationCallback,                              //  Operation callbacks
    FilterUnloadCallback,                           //  Filters unload routine
    InstanceSetupCallback,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL        //  Unused routines
};