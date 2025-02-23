#ifndef MAIN_H
#define MAIN_H

#include <windows.h>
#include <commctrl.h> // InitCommonControlsEx 사용을 위해 필요

// DLL_EXPORT를 위한 매크로 정의
#define DLL_EXPORT __declspec(dllexport)

DWORD DLL_EXPORT WINAPI ShowMessageBox(LPVOID lpParam);

#endif // MAIN_H