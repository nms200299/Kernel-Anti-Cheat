#include "pch.h"
#include "IsProtectPath.h"

_gData gData;

// ��ũ ��ü(���� ��ũ, ���� ��ũ)�� �����ϴ��� �Ǻ��ϴ� �Լ�
BOOLEAN IsHardDiskPath(PFLT_FILE_NAME_INFORMATION FileInfo) {
    if ((FileInfo->FinalComponent.Length == 0) &&
        (FileInfo->ParentDir.Length == 0)) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

// ��ȣ�� ������� �Ǻ��ϴ� �Լ� 
BOOLEAN IsProtectPath(PFLT_FILE_NAME_INFORMATION FileInfo) {


    //if (FsRtlIsNameInExpression((PUNICODE_STRING)&FileInfo->Name.Buffer, (PUNICODE_STRING)&gData.DirverPath, TRUE, NULL)) {
    //    return TRUE;
    //}

    if (IsHardDiskPath(FileInfo)) {
        return TRUE;
    }


    return FALSE;
}