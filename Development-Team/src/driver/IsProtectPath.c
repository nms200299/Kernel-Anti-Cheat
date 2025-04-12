#include "pch.h"
#include "IsProtectPath.h"

_gData gData;

// 물리 디스크 자체에 접근하는지 판별하는 함수
BOOLEAN IsHardDiskPath() {

}

// 보호할 경로인지 판별하는 함수 
BOOLEAN IsProtectPath(PFLT_FILE_NAME_INFORMATION FileInfo) {

    if (FsRtlIsNameInExpression(&FileInfo->Name.Buffer, &gData.DirverPath, TRUE, NULL)) {
        return TRUE;
    }
    return FALSE;
}