#pragma once

typedef struct __gHandle
{
	PFLT_FILTER FilterHandle;
} _gHandle;

typedef struct __gData
{
	ULONG SetDbgLevel;
	UNICODE_STRING DirverPath;	// 1°³
	UNICODE_STRING GamePath;	// º¹¼ö
} _gData;


extern _gHandle gHandle;
extern _gData gData;