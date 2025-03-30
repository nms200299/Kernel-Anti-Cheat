#ifndef __FILTERPORT_COMMON_H__
#define __FILTERPORT_COMMON_H__

#define MINIFLT_EXAMPLE_PORT_NAME "\\MiniFilterPort_Example"
#define BUFFER_SIZE 4096 // page size

// this structure will be used when filter send message to user
typedef struct _FLT_TO_USER {

	wchar_t path[BUFFER_SIZE / sizeof(wchar_t)];

} FLT_TO_USER, * PFLT_TO_USER;

// this structure will be used when user reply to filter
typedef struct _FLT_TO_USER_REPLY {

	unsigned __int32 block; // if 1, file access will be denied

} FLT_TO_USER_REPLY, * PFLT_TO_USER_REPLY;

// this structure will be used when user send message to user
typedef struct _USER_TO_FLT {

	wchar_t msg[BUFFER_SIZE / sizeof(wchar_t)];

} USER_TO_FLT, * PUSER_TO_FLT;

// this structure will be used when filter reply to user
typedef struct _USER_TO_FLT_REPLY {

	wchar_t msg[BUFFER_SIZE / sizeof(wchar_t)];

} USER_TO_FLT_REPLY, * PUSER_TO_FLT_REPLY;

int DriverPort();
DWORD WINAPI DriverPortThread(LPVOID lpParam);
#endif