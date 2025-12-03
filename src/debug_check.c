#include <stdio.h>
#include <windows.h>
#include <winternl.h>
#include <stdlib.h>

#pragma comment(lib, "ntdll.lib")

// Structure to hold debug detection results
typedef struct {
    BOOL isDebuggedPEB;
    BOOL isDebuggedCheckRemote;
    BOOL isDebuggedIsDebuggerPresent;
    BOOL isDebuggedProcessDebugPort;
    BOOL isDebuggedProcessDebugFlags;
    int debugMethodsDetected;
} DebugDetectionResult;

// Checks the BeingDebugged flag of the PEB
BOOL UsingPEB(void) {
    // Get the address of the PEB
    PEB* peb = (PEB*)__readgsqword(0x60);
    return peb->BeingDebugged;
}

// Checks using the CheckRemoteDebuggerPresent winapi call
BOOL UsingCheckRemoteDebuggerPresent(void) {
    BOOL isDebuggerPresent = FALSE;
    CheckRemoteDebuggerPresent(GetCurrentProcess(), &isDebuggerPresent);
    return isDebuggerPresent;
}

// Checks using the IsDebuggerPresent winapi call
BOOL UsingIsDebuggerPresent(void) {
    return IsDebuggerPresent();
}

// Checks using the NtQueryInformationProcess winapi call and passing the ProcessInformationClass 'ProcessDebugPort'
// which returns nonzero value when the process is being debugged by a ring 3 debugger
BOOL UsingNtQueryInformationProcessDebugPort(void) {
    DWORD debugFlag = 0;
    NtQueryInformationProcess(GetCurrentProcess(), ProcessDebugPort, &debugFlag, sizeof(debugFlag), NULL);
    return debugFlag != 0;
}

// Checks using the NtQueryInformationProcess winapi call with ProcessDebugFlags
BOOL UsingNtQueryInformationProcessDebugFlags(void) {
    DWORD debugFlag = 0;
    NtQueryInformationProcess(GetCurrentProcess(), ProcessDebugFlags, &debugFlag, sizeof(debugFlag), NULL);
    return debugFlag == 0;
}

// Run all debug detection methods and return results
DebugDetectionResult* DetectDebugger(void) {
    DebugDetectionResult* result = (DebugDetectionResult*)malloc(sizeof(DebugDetectionResult));
    if (result == NULL) {
        return NULL;
    }

    // Initialize
    memset(result, 0, sizeof(DebugDetectionResult));

    // Run all detection methods
    result->isDebuggedPEB = UsingPEB();
    result->isDebuggedCheckRemote = UsingCheckRemoteDebuggerPresent();
    result->isDebuggedIsDebuggerPresent = UsingIsDebuggerPresent();
    result->isDebuggedProcessDebugPort = UsingNtQueryInformationProcessDebugPort();
    result->isDebuggedProcessDebugFlags = UsingNtQueryInformationProcessDebugFlags();

    // Count detection methods that detected a debugger
    result->debugMethodsDetected = 0;
    if (result->isDebuggedPEB) result->debugMethodsDetected++;
    if (result->isDebuggedCheckRemote) result->debugMethodsDetected++;
    if (result->isDebuggedIsDebuggerPresent) result->debugMethodsDetected++;
    if (result->isDebuggedProcessDebugPort) result->debugMethodsDetected++;
    if (result->isDebuggedProcessDebugFlags) result->debugMethodsDetected++;

    return result;
}

// Check if debugger is detected by any method
BOOL IsDebuggerDetected(void) {
    DebugDetectionResult* result = DetectDebugger();
    if (result == NULL) {
        return FALSE;
    }

    BOOL detected = (result->debugMethodsDetected > 0);
    free(result);
    return detected;
}

// Check if debugger is detected by multiple methods (more reliable)
BOOL IsDebuggerDetectedMultiple(void) {
    DebugDetectionResult* result = DetectDebugger();
    if (result == NULL) {
        return FALSE;
    }

    // Consider it debugged if at least 2 methods detect it
    BOOL detected = (result->debugMethodsDetected >= 2);
    free(result);
    return detected;
}

// Free debug detection result
void FreeDebugDetectionResult(DebugDetectionResult* result) {
    if (result != NULL) {
        free(result);
    }
}
