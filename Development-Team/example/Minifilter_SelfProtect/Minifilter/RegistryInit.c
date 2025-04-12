#include "registry.h"


//
// ������Ʈ�� ���� �Լ� �ݹ��
// 
BOOLEAN
TargetRegistryCheck(
    IN PVOID pRegKeyInfo,
    IN DWORD ObjectInfo
)
{
    BOOLEAN bResult = TRUE;
    PVOID pObject = NULL;
    ULONG tmpBufferSize = 0;
    NTSTATUS status;

    // pObject �Ҵ� ����
    if (pRegKeyInfo == NULL)
    {
        return bResult;
    }

    switch (ObjectInfo)
    {
    case SETVALUEKEY:
        pObject = ((PREG_SET_VALUE_KEY_INFORMATION)pRegKeyInfo)->Object;
        break;

    case DELETEVALUEKEY:
        pObject = ((PREG_DELETE_VALUE_KEY_INFORMATION)pRegKeyInfo)->Object;
        break;

    case CREATEKEY:
        pObject = ((PREG_CREATE_KEY_INFORMATION)pRegKeyInfo)->RootObject;
        break;

    case DELETEKEY:
        pObject = ((PREG_DELETE_KEY_INFORMATION)pRegKeyInfo)->Object;
        break;

    default:
        return bResult;
        //break;
    }

    // pObject�� NULL���� Ȯ��
    if (pObject == NULL)
    {
        return bResult;
    }

    // ũ�� Ȯ���� ���� ù ��° ȣ��
    status = ObQueryNameString(pObject, NULL, 0, &tmpBufferSize);
    if (status != STATUS_INFO_LENGTH_MISMATCH)
    {
        return bResult;
    }

    // ������ ũ��� �޸� �Ҵ�
    POBJECT_NAME_INFORMATION pNameInfo = (POBJECT_NAME_INFORMATION)ExAllocatePoolWithTag(NonPagedPool, tmpBufferSize, 'RegC');
    if (pNameInfo == NULL)
    {
        return bResult;
    }

    // �� ��° ȣ��: ���� ������ ��������
    status = ObQueryNameString(pObject, pNameInfo, tmpBufferSize, &tmpBufferSize);
    if (!NT_SUCCESS(status))
    {
        ExFreePoolWithTag(pNameInfo, 'RegC');
        return bResult;
    }

    // ������Ʈ�� ��� ��
    if (_wcsnicmp(pNameInfo->Name.Buffer, L"\\REGISTRY\\MACHINE\\SOFTWARE\\TEST", wcslen(L"\\REGISTRY\\MACHINE\\SOFTWARE\\TEST")) == 0)
    {
        bResult = FALSE;
        KdPrint(("pRegistryPath => %ws \n", pNameInfo->Name.Buffer));
    }

    // �޸� ����
    ExFreePoolWithTag(pNameInfo, 'RegC');
    return bResult;
}



NTSTATUS RgCallback(
    IN PVOID CallbackContext,
    IN PVOID Argument1,
    IN PVOID Argument2
)
{
    UNREFERENCED_PARAMETER(CallbackContext);
    NTSTATUS status = STATUS_SUCCESS;
    REG_NOTIFY_CLASS NotifyClass;
    NotifyClass = (REG_NOTIFY_CLASS)(ULONG_PTR)Argument1;

    if (Argument2 == NULL)
    {
        DbgPrint("[REG]: Arg2 is NULL.\n");
        return STATUS_SUCCESS;
    }

    switch (NotifyClass)
    {
    case RegNtSetValueKey:
    {
        PREG_SET_VALUE_KEY_INFORMATION pSetValueKeyInfo = (PREG_SET_VALUE_KEY_INFORMATION)Argument2;

        if (TargetRegistryCheck(pSetValueKeyInfo, SETVALUEKEY) == FALSE)
        {
            status = STATUS_ACCESS_DENIED;
        }

        break;
    }

    case RegNtDeleteValueKey:
    {
        PREG_DELETE_VALUE_KEY_INFORMATION pDeleteValueKeyInfo = (PREG_DELETE_VALUE_KEY_INFORMATION)Argument2;

        if (TargetRegistryCheck(pDeleteValueKeyInfo, DELETEVALUEKEY) == FALSE)
        {
            status = STATUS_ACCESS_DENIED;
        }

        break;
    }

    case RegNtPreCreateKeyEx:
    {
        PREG_CREATE_KEY_INFORMATION pCreateKeyInfo = (PREG_CREATE_KEY_INFORMATION)Argument2;

        if (TargetRegistryCheck(pCreateKeyInfo, CREATEKEY) == FALSE)
        {
            status = STATUS_ACCESS_DENIED;
        }

        break;
    }

    case RegNtDeleteKey:
    {
        PREG_DELETE_KEY_INFORMATION pDeleteKeyInfo = (PREG_DELETE_KEY_INFORMATION)Argument2;

        if (TargetRegistryCheck(pDeleteKeyInfo, DELETEKEY) == FALSE)
        {
            status = STATUS_ACCESS_DENIED;
        }
        break;
    }

    default:
        break;
    }
    return status;
}

NTSTATUS RegisterCallback(IN PDEVICE_OBJECT DeviceObject)
{
    DbgPrint("[MyDriver]: RegisterCallback.\n");
    NTSTATUS status;
    UNICODE_STRING usAltitude;

    // DeviceObject ��ȿ�� �˻�
    if (DeviceObject == NULL || DeviceObject->DriverObject == NULL)
    {
        DbgPrint("[MyDriver][ERROR]: DeviceObject or DriverObject is NULL.\n");
        return STATUS_INVALID_PARAMETER;
    }
    RtlInitUnicodeString(&usAltitude, L"322132");

    status = CmRegisterCallbackEx(RgCallback, &usAltitude, DeviceObject->DriverObject, NULL, &g_Cookie, NULL);
    if (!NT_SUCCESS(status))
    {
        g_Cookie.QuadPart = 0; // ���� �� �ʱ�ȭ
        DbgPrint("[MyDriver][ERROR]: RegisterCallback Failed.\n");
        return status;
    }

    return status;
}