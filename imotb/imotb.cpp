#include <windows.h>
#include <stdio.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>

#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "user32.lib")

RECT taskbarRect;
HHOOK hMouseHook;

void AdjustVolume(float level) {
    HRESULT hr;
    IMMDeviceEnumerator *deviceEnumerator = NULL;
    hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        printf("Failed to initialize COM library.\n");
        return;
    }

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER,
                          __uuidof(IMMDeviceEnumerator), (void**)&deviceEnumerator);
    if (FAILED(hr)) {
        printf("Failed to create device enumerator.\n");
        CoUninitialize();
        return;
    }

    IMMDevice *defaultDevice = NULL;
    hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
    if (FAILED(hr)) {
        printf("Failed to get default audio endpoint.\n");
        deviceEnumerator->Release();
        CoUninitialize();
        return;
    }

    IAudioEndpointVolume *endpointVolume = NULL;
    hr = defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (void**)&endpointVolume);
    if (FAILED(hr)) {
        printf("Failed to activate endpoint volume.\n");
        defaultDevice->Release();
        deviceEnumerator->Release();
        CoUninitialize();
        return;
    }

    float currentVolume = 0.0f;
    endpointVolume->GetMasterVolumeLevelScalar(&currentVolume);
    currentVolume += level;
    if (currentVolume < 0.0f) currentVolume = 0.0f;
    if (currentVolume > 1.0f) currentVolume = 1.0f;

    endpointVolume->SetMasterVolumeLevelScalar(currentVolume, NULL);

    endpointVolume->Release();
    defaultDevice->Release();
    deviceEnumerator->Release();
    CoUninitialize();
}

BOOL GetTaskbarRect(HWND taskbar, RECT *rect) {
    return GetWindowRect(taskbar, rect);
}

BOOL IsMouseOverTaskbar(HWND taskbar) {
    POINT pt;
    GetCursorPos(&pt);
    return PtInRect(&taskbarRect, pt);
}

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && wParam == WM_MOUSEWHEEL) {
        MSLLHOOKSTRUCT *mouse = (MSLLHOOKSTRUCT *)lParam;
        POINT pt = mouse->pt;

        // Check if the mouse pointer is over the taskbar area
        if (IsMouseOverTaskbar(NULL)) {
            short delta = GET_WHEEL_DELTA_WPARAM(mouse->mouseData);
            if (delta > 0) {
                 printf("Increasing volume.\n");
                AdjustVolume(0.008f);  // Scroll up, increase volume
            } else if (delta < 0) {
                 printf("Decreasing volume.\n");
               AdjustVolume(-0.008f); // Scroll down, decrease volume
            }
        }
    }
    return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}

int main() {
    HWND taskbar = FindWindowW(L"Shell_TrayWnd", NULL);
    if (taskbar == NULL) {
        printf("Error: Could not find the taskbar.\n");
        return 1;
    }

    if (!GetTaskbarRect(taskbar, &taskbarRect)) {
        printf("Error: Could not get the taskbar rectangle.\n");
        return 1;
    }

    hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, NULL, 0);
    if (hMouseHook == NULL) {
        printf("Error: Could not set mouse hook.\n");
        return 1;
    }

 //   printf("Monitoring mouse wheel for taskbar detection. Press Ctrl+C to exit.\n");

    // Message loop to keep the hook alive
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hMouseHook);
    return 0;
}
