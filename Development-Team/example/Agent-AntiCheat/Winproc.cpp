#include "framework.h"
#include "Winproc.h"
#include "Tray.h"

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP)
        {
            POINT pt;
            GetCursorPos(&pt);
            ShowContextMenu(hWnd, pt);
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HWND hEditPath;
    static HBRUSH hBrushSkyBlue;

    switch (message)
    {
    case WM_INITDIALOG:
        // Edit Control 핸들 저장
        hEditPath = GetDlgItem(hDlg, IDC_EDIT_PATH);

        // 하늘색 브러시 생성
        hBrushSkyBlue = CreateSolidBrush(RGB(173, 216, 230));

        return (INT_PTR)TRUE;

    case WM_CTLCOLORDLG:
        return (INT_PTR)hBrushSkyBlue;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_BUTTON_BROWSE:
        {
            OPENFILENAME ofn;
            TCHAR szFile[MAX_PATH] = _T("");

            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hDlg;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrFilter = _T("모든 파일 (*.*)\0*.*\0");
            ofn.nFilterIndex = 1;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

            if (GetOpenFileName(&ofn) == TRUE)
            {
                SetWindowText(hEditPath, szFile);
            }
            break;
        }

        case IDOK:  // 확인 버튼
        case IDCANCEL:  // 취소 버튼
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
