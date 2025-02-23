#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    MessageBox(NULL, L"Hello from Process Hollowing!", L"Pwned", MB_OK | MB_ICONINFORMATION);
    return 0;
}