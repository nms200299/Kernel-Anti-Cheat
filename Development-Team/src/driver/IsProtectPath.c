#include "pch.h"
#include "IsProtectPath.h"

_gData gData;

// ���� ��ũ ��ü�� �����ϴ��� �Ǻ��ϴ� �Լ�
BOOLEAN IsHardDiskPath() {

}

// ��ȣ�� ������� �Ǻ��ϴ� �Լ� 
BOOLEAN IsProtectPath(PFLT_FILE_NAME_INFORMATION FileInfo) {

    if (FsRtlIsNameInExpression(&FileInfo->Name.Buffer, &gData.DirverPath, TRUE, NULL)) {
        return TRUE;
    }
    return FALSE;
}