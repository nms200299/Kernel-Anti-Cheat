/*
Abstract:
    드라이버 메인 엔트리

Environment:
    Kernel mode
*/


#include "pch.h"
#include "main.h"

_gHandle gHandle;
_gData gData;

/*************************************************************************
드라이버 로드
*************************************************************************/
NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    UNREFERENCED_PARAMETER(RegistryPath);
    gData.SetDbgLevel = DBG_LEVEL_DEBUG;
    DBG_PRINT(DBG_LEVEL_NOTI, __func__, "Driver Load");

    NTSTATUS status;
    status = FltRegisterFilter(DriverObject,
                               &FilterRegistration,
                               &gHandle.FilterHandle);

    FLT_ASSERT( NT_SUCCESS( status ) );

    if (NT_SUCCESS( status )) {
        status = FltStartFiltering(gHandle.FilterHandle);
        if (!NT_SUCCESS( status )) {
            FltUnregisterFilter(gHandle.FilterHandle);
        }
    }

    return status;
}



/*************************************************************************
드라이버 언로드
*************************************************************************/
NTSTATUS
driverUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER( Flags );
    PAGED_CODE();
    DBG_PRINT(DBG_LEVEL_NOTI, __func__, "Driver Unload");

    FltUnregisterFilter(gHandle.FilterHandle);

    return STATUS_SUCCESS;
}
