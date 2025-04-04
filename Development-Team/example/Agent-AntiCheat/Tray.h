#pragma once
#include "framework.h"

#define MAX_LOADSTRING 100
#define WM_TRAYICON (WM_USER + 1)

void InitTrayIcon(HWND);
void ShowContextMenu(HWND, POINT);

extern NOTIFYICONDATA nid; 
extern HINSTANCE hInst;

