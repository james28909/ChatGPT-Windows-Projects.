#include <Windows.h>
#include <iostream>

SERVICE_STATUS        g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;

// Function prototypes
void WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
void WINAPI ServiceCtrlHandler(DWORD CtrlCode);
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);

int main()
{
    SERVICE_TABLE_ENTRY ServiceTable[] =
    {
        { L"MyService", (LPSERVICE_MAIN_FUNCTION)ServiceMain },
        { NULL, NULL }
    };

    if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
    {
        std::cerr << "StartServiceCtrlDispatcher failed, error = " << GetLastError() << std::endl;
        return -1;
    }

    return 0;
}

void WINAPI ServiceMain(DWORD argc, LPTSTR* argv)
{
    g_StatusHandle = RegisterServiceCtrlHandler(L"MyService", ServiceCtrlHandler);

    if (g_StatusHandle == NULL)
    {
        std::cerr << "RegisterServiceCtrlHandler failed, error = " << GetLastError() << std::endl;
        return;
    }

    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    // Perform service-specific initialization here

    // Example: Starting a worker thread
    HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);
    if (hThread == NULL)
    {
        std::cerr << "Failed to create worker thread, error = " << GetLastError() << std::endl;
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        g_ServiceStatus.dwWin32ExitCode = GetLastError();
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
        return;
    }

    // Service is now running
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    // Wait for the worker thread to complete
    WaitForSingleObject(hThread, INFINITE);

    // Clean up resources
    CloseHandle(hThread);
}

void WINAPI ServiceCtrlHandler(DWORD CtrlCode)
{
    switch (CtrlCode)
    {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

        // Signal the worker thread to stop
        SetEvent(g_ServiceStopEvent);
        return;

    default:
        break;
    }
}

DWORD WINAPI ServiceWorkerThread(LPVOID lpParam)
{
    // Example worker thread function
    while (true)
    {
        // Perform background tasks here

        // Simulate work (remove or replace with actual service logic)
        Sleep(1000); // Sleep for 1 second

        // Check if the service needs to stop
        if (WaitForSingleObject(g_ServiceStopEvent, 0) == WAIT_OBJECT_0)
            break;
    }

    // Perform cleanup tasks if necessary

    return ERROR_SUCCESS;
}
