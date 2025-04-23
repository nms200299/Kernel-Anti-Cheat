#include "pch.h"
#include "IsAllowPID.h"

_gData gData;

BOOLEAN IsAllowPID(_In_ ULONG AccessPID) {
    switch (AccessPID)
    {
    case PID_IDLE:
    case PID_SYSTEM: {
        return TRUE;
    } // ����� PID�� TRUE ��ȯ
    default: {
        return FALSE;
    } // �׿� PID�� FALSE ��ȯ
    }
}