#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <windows.h>
#include <shobjidl.h> 
#include <string>

// DirectX 전역 변수
ID3D11Device* g_pd3dDevice = NULL;
ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
IDXGISwapChain* g_pSwapChain = NULL;
ID3D11RenderTargetView* g_mainRenderTargetView = NULL;

// 윈도우 핸들 전역 저장
HWND g_hWnd = NULL;

// 경로 저장 관련
std::string selectedExePath = "";
std::string tempExePath = "";
bool configChanged = false;

const wchar_t* REG_PATH = L"Software\\MyApp\\Settings";
const wchar_t* REG_KEY = L"ExePath";


void SavePathToRegistry(const std::string& path) {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, REG_KEY, 0, REG_SZ, (const BYTE*)std::wstring(path.begin(), path.end()).c_str(), (DWORD)((path.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(hKey);
    }
}

std::string LoadPathFromRegistry() {
    HKEY hKey;
    wchar_t buffer[MAX_PATH];
    DWORD bufferSize = sizeof(buffer);
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExW(hKey, REG_KEY, NULL, NULL, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return std::string(buffer, buffer + wcslen(buffer));
        }
        RegCloseKey(hKey);
    }
    return "";
}

void OpenFileDialog(std::string& path) {
    IFileDialog* pFileDialog;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileDialog)))) {
        COMDLG_FILTERSPEC fileTypes[] = {
            { L"Executable Files", L"*.exe" },
            { L"All Files", L"*.*" }
        };
        pFileDialog->SetFileTypes(ARRAYSIZE(fileTypes), fileTypes);

        if (SUCCEEDED(pFileDialog->Show(g_hWnd))) {
            IShellItem* pItem;
            if (SUCCEEDED(pFileDialog->GetResult(&pItem))) {
                PWSTR pszFilePath;
                if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath))) {
                    char buffer[MAX_PATH];
                    WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, buffer, MAX_PATH, NULL, NULL);
                    path = buffer;
                    CoTaskMemFree(pszFilePath);
                }
                pItem->Release();
            }
        }
        pFileDialog->Release();
    }
}

// 윈도우 프로시저
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    switch (msg) {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED) {
            if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            ID3D11Texture2D* pBackBuffer = NULL;
            g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
            g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
            pBackBuffer->Release();
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}


// DirectX 초기화 함수
bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[1] = { D3D_FEATURE_LEVEL_11_0 };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags,
        featureLevelArray, 1, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
        &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    ID3D11Texture2D* pBackBuffer = NULL;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();

    return true;
}

// DirectX 정리
void CleanupDeviceD3D() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}
void DrawSettingsUI() {
    static bool maximized = false;
    static bool isDragging = false;
    static POINT dragOffset = { 0, 0 };

    // Titlebar area for dragging
    ImVec2 titlebarSize(ImGui::GetWindowWidth() - 120, 30);
    ImGui::SetCursorPos(ImVec2(0, 0));
    ImGui::InvisibleButton("##TitlebarDragZone", titlebarSize, ImGuiButtonFlags_None);

    if (ImGui::IsItemActive()) {
        if (!isDragging) {
            isDragging = true;
            POINT cursor;
            GetCursorPos(&cursor);
            RECT winRect;
            GetWindowRect(g_hWnd, &winRect);
            dragOffset.x = cursor.x - winRect.left;
            dragOffset.y = cursor.y - winRect.top;
        }
    }
    else {
        isDragging = false;
    }

    if (isDragging && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        POINT cursor;
        GetCursorPos(&cursor);
        SetWindowPos(g_hWnd, NULL,
            cursor.x - dragOffset.x,
            cursor.y - dragOffset.y,
            0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }

    // Titlebar buttons
    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth()-80, 0));
    if (ImGui::Button("_")) ShowWindow(g_hWnd, SW_MINIMIZE);
    ImGui::SameLine();
    if (ImGui::Button("[]")) {
        ShowWindow(g_hWnd, maximized ? SW_RESTORE : SW_MAXIMIZE);
        maximized = !maximized;
    }
    ImGui::SameLine();
    if (ImGui::Button("X")) PostQuitMessage(0);

    // Main content
    ImGui::SetCursorPosY(40);
    ImGui::Separator();
    ImGui::Text("Current executable path:");
    ImGui::TextWrapped("%s", tempExePath.empty() ? "(None selected)" : tempExePath.c_str());

    if (ImGui::Button("Browse...")) {
        OpenFileDialog(tempExePath);
        configChanged = true;
    }

    ImGui::Spacing();
    ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x-120);
    ImGui::SetCursorPosY(ImGui::GetContentRegionAvail().y+90);
    if (ImGui::Button("Apply")) {
        selectedExePath = tempExePath;
        SavePathToRegistry(selectedExePath);
        configChanged = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("OK")) {
        selectedExePath = tempExePath;
        SavePathToRegistry(selectedExePath);
        configChanged = false;
        PostQuitMessage(0);
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
        tempExePath = selectedExePath;
        configChanged = false;
        PostQuitMessage(0);
    }
}



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    selectedExePath = LoadPathFromRegistry();
    tempExePath = selectedExePath;

    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L,
                      GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
                      _T("ImGuiWindowClass"), NULL };
    RegisterClassEx(&wc);
    g_hWnd = CreateWindow(wc.lpszClassName, _T("ImGui Settings"),
        WS_POPUP, 100, 100, 600, 200,
        NULL, NULL, wc.hInstance, NULL);

    if (!g_hWnd || !CreateDeviceD3D(g_hWnd)) {
        CleanupDeviceD3D();
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ShowWindow(g_hWnd, SW_SHOWDEFAULT);
    UpdateWindow(g_hWnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //ImGui::StyleColorsDark();
	ImGui::StyleColorsLight();
    //ImGui::StyleColorsClassic();
    io.Fonts->AddFontFromFileTTF("notosanskr-regular.otf", 18.0f, NULL, io.Fonts->GetGlyphRangesKorean());
    //ImFontGlyphRangesBuilder builder;
    //builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
    ImGui_ImplWin32_Init(g_hWnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("Settings", NULL,
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoTitleBar);
        DrawSettingsUI();
        ImGui::End();

        ImGui::Render();
        const float clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clearColor);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(g_hWnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);
    CoUninitialize();

    return 0;
}
