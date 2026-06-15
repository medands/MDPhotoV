#ifndef FRAMEDATA_H
#define FRAMEDATA_H

#include <windows.h>
#include <vector>
#include <string>
#include <d2d1_1.h>

struct FrameData {
    std::vector<BYTE> pixelBuffer;
    UINT width = 0;
    UINT height = 0;
    UINT left = 0;
    UINT top = 0;
    UINT delayMs = 100;
    BYTE disposal = 0;
};

struct AsyncLoadData {
    std::vector<FrameData> frames;
    UINT width = 0;
    UINT height = 0;
    std::wstring filePath;
    bool rebuildPlaylist = false;
    HRESULT hr = E_FAIL;
};

struct RenderFrame {
    ID2D1Bitmap1* pBitmap = nullptr;
    UINT delayMs = 100;
};

// Utility
template<class T>
inline void SafeRelease(T** ppT) {
    if (*ppT) {
        (*ppT)->Release();
        *ppT = nullptr;
    }
}

#endif