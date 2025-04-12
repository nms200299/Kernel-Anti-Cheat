#include "framework.h"

#include "main.h"

NOTIFYICONDATA nid = { 0 };
HINSTANCE hInst;
HWND hWnd;


int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    hInst = hInstance;

    // 숨겨진 윈도우 생성
    WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.lpszClassName = L"TrayAppClass";
    RegisterClassExW(&wcex);

    hWnd = CreateWindowW(L"TrayAppClass", L"TrayApp", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    InitTrayIcon(hWnd);

    CreateThread(nullptr, 0, DriverPortThread, nullptr, 0, nullptr); // 포트 연결

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    Shell_NotifyIcon(NIM_DELETE, &nid);
    return (int)msg.wParam;
}
