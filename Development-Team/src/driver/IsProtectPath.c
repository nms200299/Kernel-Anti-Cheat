#include "pch.h"
#include "IsProtectPath.h"

_gData gData;

// 디스크 자체(물리 디스크, 볼륨 디스크)에 접근하는지 판별하는 함수
BOOLEAN IsHardDiskPath(PFLT_FILE_NAME_INFORMATION FileInfo) {
    if ((FileInfo->FinalComponent.Length == 0) &&
        (FileInfo->ParentDir.Length == 0)){
        return TRUE;
    }
    else {
        return FALSE;
    }
}

// 보호할 경로인지 판별하는 함수 
BOOLEAN IsProtectPath(PFLT_FILE_NAME_INFORMATION FileInfo) {


    //if (FsRtlIsNameInExpression((PUNICODE_STRING)&FileInfo->Name.Buffer, (PUNICODE_STRING)&gData.DirverPath, TRUE, NULL)) {
    //    return TRUE;
    //}

    if (IsHardDiskPath(FileInfo)) {
        return TRUE;
    }


    return FALSE;
}