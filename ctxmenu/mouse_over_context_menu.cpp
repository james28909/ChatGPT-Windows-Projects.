#include <iostream>
#include <Windows.h>

// Global variable to store previous mouse wheel delta
int prevScrollDelta = 0;

// Function to check if mouse is over a context menu in Windows Explorer
bool isMouseOverContextMenu() {
    // Get the handle of the window under the mouse cursor
    POINT cursorPos;
    GetCursorPos(&cursorPos);
    HWND hWnd = WindowFromPoint(cursorPos);

    // Check if the window is a context menu in Windows Explorer
    TCHAR className[256];
    if (GetClassName(hWnd, className, sizeof(className)) > 0) {
        // Assuming context menu in Windows Explorer has class name "#32768"
        if (std::wstring(className) == L"#32768") {
            return true;
        }
    }
    return false;
}

// Function to simulate pressing a key
void pressKey(WORD vKey) {
    INPUT input;
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vKey;
    input.ki.wScan = MapVirtualKey(vKey, MAPVK_VK_TO_VSC);
    input.ki.dwFlags = 0; // keydown
    input.ki.time = 0;
    input.ki.dwExtraInfo = 0;
    SendInput(1, &input, sizeof(INPUT));
}

// Function to simulate releasing a key
void releaseKey(WORD vKey) {
    INPUT input;
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vKey;
    input.ki.wScan = MapVirtualKey(vKey, MAPVK_VK_TO_VSC);
    input.ki.dwFlags = KEYEVENTF_KEYUP; // keyup
    input.ki.time = 0;
    input.ki.dwExtraInfo = 0;
    SendInput(1, &input, sizeof(INPUT));
}

// Callback function for processing mouse messages
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    // Check if it's a mouse wheel message and the mouse is over context menu
    if (nCode == HC_ACTION && wParam == WM_MOUSEWHEEL && isMouseOverContextMenu()) {
        MSLLHOOKSTRUCT *pMouseStruct = (MSLLHOOKSTRUCT *)lParam;
        int delta = GET_WHEEL_DELTA_WPARAM(pMouseStruct->mouseData);
        
        if (delta > 0) {
            std::cout << "Scrolling up detected. Sending 'Up' key press." << std::endl;
            pressKey(VK_UP); // Simulate pressing 'Up' arrow key
            releaseKey(VK_UP); // Simulate releasing 'Up' arrow key
        } else if (delta < 0) {
            std::cout << "Scrolling down detected. Sending 'Down' key press." << std::endl;
            pressKey(VK_DOWN); // Simulate pressing 'Down' arrow key
            releaseKey(VK_DOWN); // Simulate releasing 'Down' arrow key
        }
    }

    // Call the next hook in the chain
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

int main() {
    // Install mouse hook to monitor mouse events
    HHOOK mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, NULL, 0);

    if (mouseHook == NULL) {
        std::cerr << "Failed to install mouse hook!" << std::endl;
        return 1;
    }

    // Message loop (optional if hook is used)
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Unhook and exit
    UnhookWindowsHookEx(mouseHook);

    return 0;
}
