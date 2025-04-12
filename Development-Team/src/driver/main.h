#pragma once
#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, driverUnload)
#endif


EXTERN_C_START
    DRIVER_INITIALIZE DriverEntry;
    NTSTATUS
    DriverEntry(
        _In_ PDRIVER_OBJECT DriverObject,
        _In_ PUNICODE_STRING RegistryPath
    );

    NTSTATUS
    driverUnload(
        _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    );
EXTERN_C_END

extern PFLT_FILTER gFilterHandle;
