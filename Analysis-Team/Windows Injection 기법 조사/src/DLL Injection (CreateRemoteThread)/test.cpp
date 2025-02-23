//testdll.cpp

#include "main.h"
#pragma comment(lib, "comctl32.lib")

DWORD DLL_EXPORT WINAPI ShowMessageBox(LPVOID lpParam) {
	INITCOMMONCONTROLSEX iccex = {
		sizeof(INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES
	};

	InitCommonControlsEx(&iccex);
	IsGUIThread(TRUE);

	OutputDebugString(TEXT("test.dll: Displaying MessageBox"));

	if (MessageBox(HWND_DESKTOP, TEXT("hi"), TEXT("hi! it's over"), MB_OK | MB_ICONINFORMATION) == 0) {
		TCHAR szBuffer[64];
		wsprintf(szBuffer, TEXT("MessageBox Error: %u"), GetLastError());
		OutputDebugString(szBuffer);
	}
	return 0;
}

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason) 
	{
		case DLL_PROCESS_ATTACH: 
		{
			DWORD dwThreadId;
			DisableThreadLibraryCalls(hInst);
			OutputDebugString(TEXT("test.dll: DLL_PROCESS_ATTACH"));
			CreateThread(NULL, 0, ShowMessageBox, NULL, 0, &dwThreadId);
		}
		break;

		case DLL_PROCESS_DETACH:
			OutputDebugString(TEXT("test.dll: DLL_PROCESS_DETACH"));
			break;
	}
	return TRUE;
}
