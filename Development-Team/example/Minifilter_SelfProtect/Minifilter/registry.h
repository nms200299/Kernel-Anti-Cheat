#pragma once
#include "miniflt.h"



LARGE_INTEGER g_Cookie;
#define KEY_PATH_MAX_LEN	255

#define SETVALUEKEY			0x0001
#define DELETEVALUEKEY		0x0002
#define CREATEKEY			0x0003
#define DELETEKEY			0x0004
#define NT_DEVICE_NAME			L"\\Device\\RegFltr"
#define DOS_DEVICES_LINK_NAME	L"\\DosDevices\\RegFltr"

PDEVICE_OBJECT g_DeviceObj;

BOOLEAN
TargetRegistryCheck(
    IN PVOID pRegKeyInfo,
    IN DWORD ObjectInfo
);

NTSTATUS RgCallback(
    IN PVOID CallbackContext,
    IN PVOID Argument1,
    IN PVOID Argument2
);

NTSTATUS RegisterCallback(IN PDEVICE_OBJECT DeviceObject);
