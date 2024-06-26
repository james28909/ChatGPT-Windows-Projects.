#include <windows.h>

// Global variable to store the handle to the window
HWND g_hWnd;

// Window Procedure function declaration
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Step 1: Register the Window Class
    const char *className = "MouseTrackerWindowClass";
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = className;

    if (!RegisterClass(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    // Step 2: Create the Window
    g_hWnd = CreateWindowEx(
        0,
        className,
        "Mouse Tracker",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        400, 300,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!g_hWnd) {
        MessageBox(NULL, "Window Creation Failed!", "Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    // Step 3: Show the Window
    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    // Step 4: Message Loop
    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

// Step 5: Implement the Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_MOUSEMOVE:
            // Check if the mouse is over the taskbar area
            RECT taskbarRect;
            if (SystemParametersInfo(SPI_GETWORKAREA, 0, &taskbarRect, 0)) {
                POINT cursorPos;
                GetCursorPos(&cursorPos);
                if (PtInRect(&taskbarRect, cursorPos)) {
                    MessageBox(hwnd, "Mouse is over the taskbar area!", "Mouse Tracker", MB_ICONINFORMATION | MB_OK);
                }
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}
