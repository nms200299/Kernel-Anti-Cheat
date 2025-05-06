/*
Abstract:
    ����̹� ���� ��Ʈ��

Environment:
    Kernel mode
*/


#include "pch.h"
#include "main.h"

_gHandle gHandle;
_gData gData;

/*************************************************************************
����̹� �ε�
*************************************************************************/
NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    UNREFERENCED_PARAMETER(RegistryPath);
    gData.SetDbgLevel = DBG_LEVEL_DEBUG;
    DBG_PRINT(DBG_LEVEL_NOTI, __func__, "Driver Load");

    // 해시맵 초기화 + 정책 등록
    InitFilterHash();
    AddFilterEntry(L"\\Device\\HarddiskVolume2\\test\\important.txt", FILTER_ACTION_BLOCK);

    NTSTATUS status = FltRegisterFilter(DriverObject, &FilterRegistration, &gHandle.FilterHandle);
    FLT_ASSERT(NT_SUCCESS(status));

    if (NT_SUCCESS(status)) {
        status = FltStartFiltering(gHandle.FilterHandle);

        if (!NT_SUCCESS(status)) {
            DBG_PRINT(DBG_LEVEL_ERROR, __func__, "FltStartFiltering FAILED: 0x%X\n", status);
            FltUnregisterFilter(gHandle.FilterHandle);
        } else {
            DBG_PRINT(DBG_LEVEL_NOTI, __func__, "Filter start SUCCESS\n");
        }
    }

    return status;
}

/*************************************************************************
����̹� ��ε�
*************************************************************************/
NTSTATUS
driverUnload(
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
)
{
    UNREFERENCED_PARAMETER(Flags);
    PAGED_CODE();
    DBG_PRINT(DBG_LEVEL_NOTI, __func__, "Driver Unload");

    FreeAllFilterEntries();  // 해시맵 메모리 해제
    FltUnregisterFilter(gHandle.FilterHandle);

    return STATUS_SUCCESS;
}

