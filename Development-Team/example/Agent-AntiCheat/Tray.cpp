#include "framework.h"
#include "Tray.h"

void InitTrayIcon(HWND hWnd)
{
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_AGENTANTICHEAT));
    wcscpy_s(nid.szTip, L"Agent_AntiCheat");
    Shell_NotifyIcon(NIM_ADD, &nid);
}

void ShowContextMenu(HWND hWnd, POINT pt)
{
    HMENU hMenu = CreatePopupMenu();
    InsertMenu(hMenu, -1, MF_BYPOSITION, IDM_ABOUT, L"정보(&I)");
    InsertMenu(hMenu, -1, MF_BYPOSITION, IDM_EXIT, L"끝내기(&X)");

    SetForegroundWindow(hWnd); // 메뉴가 제대로 동작하도록

    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, nullptr);
    DestroyMenu(hMenu);
}