#pragma once

typedef struct __gHandle
{
	PFLT_FILTER FilterHandle;
} _gHandle;

typedef struct __gData
{
	ULONG SetDbgLevel;
	UNICODE_STRING DirverPath;	// 1��
	UNICODE_STRING GamePath;	// ����
} _gData;


extern _gHandle gHandle;
extern _gData gData; 