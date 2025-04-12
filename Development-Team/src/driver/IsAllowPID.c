#include "pch.h"
#include "IsAllowPID.h"

_gData gData;

BOOLEAN IsAllowPID(_In_ ULONG AccessPID) {
    switch (AccessPID)
    {
        case PID_IDLE:
        case PID_SYSTEM:{
            return TRUE;
        } // 허용할 PID는 TRUE 반환
        default: {
            return FALSE;
        } // 그외 PID는 FALSE 반환
    }
    return FALSE;
}