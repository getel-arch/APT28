#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "advapi32.lib")

// Structure to hold mutex information
typedef struct {
    HANDLE mutex;
    char mutexName[64];
} SingleInstanceMutex;

// Calculate MD5 hash of a string
void CalculateMD5(const char* input, char* output) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    DWORD dwStatus = 0;
    (void)dwStatus;  // Reserved for future error handling
    BYTE rgbHash[16];
    DWORD cbHash = 16;

    // Acquire cryptographic provider
    if (!CryptAcquireContextA(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        strcpy(output, "error");
        return;
    }

    // Create hash object
    if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        strcpy(output, "error");
        return;
    }

    // Hash the input string
    if (!CryptHashData(hHash, (BYTE*)input, strlen(input), 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        strcpy(output, "error");
        return;
    }

    // Get the hash value
    if (!CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        strcpy(output, "error");
        return;
    }

    // Convert hash to hex string
    for (DWORD i = 0; i < cbHash; i++) {
        sprintf(output + (i * 2), "%02x", rgbHash[i]);
    }
    output[32] = '\0';

    // Clean up
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
}

// Create a single instance mutex based on computer name
SingleInstanceMutex* CreateSingleInstanceMutex(void) {
    SingleInstanceMutex* mutex = (SingleInstanceMutex*)malloc(sizeof(SingleInstanceMutex));
    if (mutex == NULL) {
        return NULL;
    }

    // Get computer name
    CHAR computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
    if (!GetComputerNameA(computerName, &size)) {
        free(mutex);
        return NULL;
    }

    // Calculate MD5 hash of computer name
    char md5Hash[33];
    CalculateMD5(computerName, md5Hash);

    // Create mutex name with prefix
    snprintf(mutex->mutexName, sizeof(mutex->mutexName), "Global\\APT28_%s", md5Hash);

    // Create or open the mutex
    mutex->mutex = CreateMutexA(NULL, TRUE, mutex->mutexName);
    if (mutex->mutex == NULL) {
        free(mutex);
        return NULL;
    }

    // Check if this is the first instance
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        // Another instance is already running
        CloseHandle(mutex->mutex);
        free(mutex);
        return NULL;
    }

    return mutex;
}

// Free the mutex
void FreeSingleInstanceMutex(SingleInstanceMutex* mutex) {
    if (mutex != NULL) {
        if (mutex->mutex != NULL) {
            ReleaseMutex(mutex->mutex);
            CloseHandle(mutex->mutex);
        }
        free(mutex);
    }
}
