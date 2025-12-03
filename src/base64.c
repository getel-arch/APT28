#include <stdlib.h>
#include <string.h>

// Base64 encoding function
static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char* base64_encode(const unsigned char* data, size_t data_len) {
    size_t encoded_len = ((data_len + 2) / 3) * 4 + 1;
    char* encoded = (char*)malloc(encoded_len);
    if (!encoded) return NULL;
    
    size_t pos = 0;
    for (size_t i = 0; i < data_len; i += 3) {
        unsigned int b = (data[i] << 16) | (i + 1 < data_len ? (data[i + 1] << 8) : 0) | (i + 2 < data_len ? data[i + 2] : 0);
        
        encoded[pos++] = base64_chars[(b >> 18) & 0x3F];
        encoded[pos++] = base64_chars[(b >> 12) & 0x3F];
        encoded[pos++] = i + 1 < data_len ? base64_chars[(b >> 6) & 0x3F] : '=';
        encoded[pos++] = i + 2 < data_len ? base64_chars[b & 0x3F] : '=';
    }
    encoded[pos] = '\0';
    return encoded;
}
