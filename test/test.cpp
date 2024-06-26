#include <windows.h>
#include <tchar.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <shellapi.h>

#define TRAY_ICON_ID 1
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001

// Global variables
HHOOK g_hMouseHook;
HWND g_hWndTextView;
NOTIFYICONDATA nid = {};

// Function declarations
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
BOOL IsMouseOverTaskbar();
void ChangeVolume(int direction);
void ShowContextMenu(HWND hWnd);

// Mouse hook procedure
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        if (wParam == WM_MOUSEWHEEL)
        {
            MSLLHOOKSTRUCT *pMouseStruct = (MSLLHOOKSTRUCT *)lParam;
            if (pMouseStruct != NULL && IsMouseOverTaskbar())
            {
                short delta = GET_WHEEL_DELTA_WPARAM(pMouseStruct->mouseData);
                if (delta > 0)
                {
                    SetWindowText(g_hWndTextView, _T("Scrolling up over taskbar"));
                    ChangeVolume(1);  // Increase volume
                }
                else if (delta < 0)
                {
                    SetWindowText(g_hWndTextView, _T("Scrolling down over taskbar"));
                    ChangeVolume(-1); // Decrease volume
                }
            }
        }
    }
    return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
}

// Check if mouse is over the taskbar
BOOL IsMouseOverTaskbar()
{
    RECT taskbarRect;
    HWND hTaskbar = FindWindow(_T("Shell_TrayWnd"), NULL);
    if (hTaskbar && GetWindowRect(hTaskbar, &taskbarRect))
    {
        POINT mousePos;
        GetCursorPos(&mousePos);
        return PtInRect(&taskbarRect, mousePos);
    }
    return FALSE;
}

// Change system volume
void ChangeVolume(int direction)
{
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr))
        return;

    IMMDeviceEnumerator* pEnumerator = NULL;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr))
    {
        CoUninitialize();
        return;
    }

    IMMDevice* pDevice = NULL;
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr))
    {
        pEnumerator->Release();
        CoUninitialize();
        return;
    }

    IAudioEndpointVolume* pEndpointVolume = NULL;
    hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pEndpointVolume);
    if (FAILED(hr))
    {
        pDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return;
    }

    float currentVolume = 0.0f;
    pEndpointVolume->GetMasterVolumeLevelScalar(&currentVolume);

    // Change volume by a small step
    float volumeStep = 0.02f; // Adjust this value to change the step size
    if (direction > 0)
    {
        currentVolume += volumeStep;
        if (currentVolume > 1.0f)
            currentVolume = 1.0f;
    }
    else if (direction < 0)
    {
        currentVolume -= volumeStep;
        if (currentVolume < 0.0f)
            currentVolume = 0.0f;
    }

    pEndpointVolume->SetMasterVolumeLevelScalar(currentVolume, NULL);

    // Cleanup
    pEndpointVolume->Release();
    pDevice->Release();
    pEnumerator->Release();
    CoUninitialize();
}

// Show context menu
void ShowContextMenu(HWND hWnd)
{
    POINT pt;
    GetCursorPos(&pt);
    HMENU hMenu = CreatePopupMenu();
    if (hMenu)
    {
        InsertMenu(hMenu, -1, MF_BYPOSITION, ID_TRAY_EXIT, _T("Exit"));
        SetForegroundWindow(hWnd); // Required to make the menu work correctly
        TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
        DestroyMenu(hMenu);
    }
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        // Create text view (STATIC control)
        g_hWndTextView = CreateWindowEx(0, _T("STATIC"), _T(""),
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            10, 10, 300, 50, hWnd, NULL, GetModuleHandle(NULL), NULL);

        // Initialize and add tray icon
        nid.cbSize = sizeof(NOTIFYICONDATA);
        nid.hWnd = hWnd;
        nid.uID = TRAY_ICON_ID;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_TRAYICON;
        nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        _tcscpy_s(nid.szTip, _T("Volume Control"));
        Shell_NotifyIcon(NIM_ADD, &nid);
        break;

    case WM_TRAYICON:
        switch (lParam)
        {
        case WM_RBUTTONDOWN:
            ShowContextMenu(hWnd);
            break;
        case WM_LBUTTONDOWN:
            // Optionally handle left-click events here
            break;
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_TRAY_EXIT)
        {
            DestroyWindow(hWnd);
        }
        break;

    case WM_DESTROY:
        // Remove the tray icon
        Shell_NotifyIcon(NIM_DELETE, &nid);

        // Unhook the mouse hook when the window is destroyed
        UnhookWindowsHookEx(g_hMouseHook);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Register window class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = _T("MouseScrollDetectionClass");
    wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL, _T("Failed to register window class!"), _T("Error"), MB_ICONERROR);
        return 1;
    }

    // Create window
    HWND hWnd = CreateWindow(_T("MouseScrollDetectionClass"), _T("Mouse Scroll Detection"),
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        400, 200, NULL, NULL, hInstance, NULL);

    if (!hWnd)
    {
        MessageBox(NULL, _T("Failed to create window!"), _T("Error"), MB_ICONERROR);
        return 1;
    }

    // Show window
    //ShowWindow(hWnd, nCmdShow);
    //UpdateWindow(hWnd);

    // Set the mouse hook
    g_hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, hInstance, 0);
    if (!g_hMouseHook)
    {
        MessageBox(NULL, _T("Failed to set mouse hook!"), _T("Error"), MB_ICONERROR);
        return 1;
    }

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
