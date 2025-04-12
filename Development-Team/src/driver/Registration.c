#include "pch.h"
#include "Registration.h"

_gHandle gHandle;

FLT_OPERATION_REGISTRATION OperationCallback[] = {
    {IRP_MJ_CREATE,
        0, PreFileOperation, NULL},
    {IRP_MJ_WRITE,
        0, PreFileOperation, NULL},
    {IRP_MJ_SET_INFORMATION,
        0, PreFileOperation, NULL},
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
    NULL, NULL, NULL, NULL, NULL, NULL, NULL        //  Unused routines
};