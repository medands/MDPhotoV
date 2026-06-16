#include "DecoderWic.h"
#include <wincodec.h>
#include <vector>
#include <algorithm>


static void BlitFrame(const FrameData& frame, std::vector<BYTE>& canvas, UINT canvasWidth, UINT canvasHeight) {
    if (canvasWidth == 0 || canvasHeight == 0) return;
    if (frame.pixelBuffer.empty()) return;
    if (frame.left >= canvasWidth || frame.top >= canvasHeight) return;
    if (frame.width == 0 || frame.height == 0) return;

    // Determine the actual copy area (clamped to canvas bounds)
    UINT copyWidth = std::min(frame.width, canvasWidth - frame.left);
    UINT copyHeight = std::min(frame.height, canvasHeight - frame.top);

    const size_t srcStride = static_cast<size_t>(frame.width) * 4;
    if (srcStride == 0) return;

    for (UINT y = 0; y < copyHeight; ++y) {
        // Destination pointer (BGRA)
        BYTE* dstRow = &canvas[((frame.top + y) * canvasWidth + frame.left) * 4];
        // Source pointer
        const BYTE* srcRow = &frame.pixelBuffer[y * srcStride];

        // Process pixel by pixel to handle transparency
        for (UINT x = 0; x < copyWidth; ++x) {
            BYTE srcAlpha = srcRow[x * 4 + 3]; // Alpha is the 4th byte (BGRA)

            if (srcAlpha == 0) {
                // Fully transparent: Do nothing, keep canvas background
                continue;
            }

            if (srcAlpha == 255) {
                // Fully opaque: Simple copy
                memcpy(&dstRow[x * 4], &srcRow[x * 4], 4);
            } else {
                // Semi-transparent: Alpha blending
                // Formula: Dest = (Src * Alpha + Dest * (255 - Alpha)) / 255
                BYTE srcR = srcRow[x * 4 + 2];
                BYTE srcG = srcRow[x * 4 + 1];
                BYTE srcB = srcRow[x * 4 + 0];

                BYTE dstR = dstRow[x * 4 + 2];
                BYTE dstG = dstRow[x * 4 + 1];
                BYTE dstB = dstRow[x * 4 + 0];

                dstRow[x * 4 + 2] = static_cast<BYTE>((srcR * srcAlpha + dstR * (255 - srcAlpha)) / 255);
                dstRow[x * 4 + 1] = static_cast<BYTE>((srcG * srcAlpha + dstG * (255 - srcAlpha)) / 255);
                dstRow[x * 4 + 0] = static_cast<BYTE>((srcB * srcAlpha + dstB * (255 - srcAlpha)) / 255);
                dstRow[x * 4 + 3] = 255; // Result is opaque after blending
            }
        }
    }
}

HRESULT DecodeWIC(AsyncLoadData* pData) {
    IWICImagingFactory* pLocalWICFactory = nullptr;
    IWICBitmapDecoder* pDecoder = nullptr;

    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pLocalWICFactory));
    if (FAILED(hr)) return hr;

    hr = pLocalWICFactory->CreateDecoderFromFilename(pData->filePath.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder);
    if (FAILED(hr)) { SafeRelease(&pLocalWICFactory); return hr; }

    UINT frameCount = 0;
    pDecoder->GetFrameCount(&frameCount);

    UINT canvasWidth = 0, canvasHeight = 0;
    IWICMetadataQueryReader* pDecoderMeta = nullptr;
    if (SUCCEEDED(pDecoder->GetMetadataQueryReader(&pDecoderMeta))) {
        // Attempt to get global canvas size
        // (Simplified for brevity, fallback to frame 0 if missing)
        pDecoderMeta->Release();
    }

    if (canvasWidth == 0 || canvasHeight == 0) {
        IWICBitmapFrameDecode* pFirstFrame = nullptr;
        if (SUCCEEDED(pDecoder->GetFrame(0, &pFirstFrame))) {
            pFirstFrame->GetSize(&canvasWidth, &canvasHeight);
            pFirstFrame->Release();
        } else {
            SafeRelease(&pDecoder);
            SafeRelease(&pLocalWICFactory);
            return E_FAIL;
        }
    }

    pData->width = canvasWidth;
    pData->height = canvasHeight;

    std::vector<BYTE> workingCanvas(static_cast<size_t>(canvasWidth) * canvasHeight * 4, 0);
    std::vector<BYTE> previousCanvas;

    for (UINT i = 0; i < frameCount; ++i) {
        IWICBitmapFrameDecode* pSource = nullptr;
        if (FAILED(pDecoder->GetFrame(i, &pSource))) continue;

        FrameData fd;
        pSource->GetSize(&fd.width, &fd.height);

		////////////////
		IWICFormatConverter* pConverter = nullptr;
        if (SUCCEEDED(pLocalWICFactory->CreateFormatConverter(&pConverter))) {
            if (SUCCEEDED(pConverter->Initialize(pSource, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeMedianCut))) {
                
                UINT wicStride = 0;
                UINT bufferSize = 0;
                
				wicStride = fd.width * 4; 
                bufferSize = wicStride * fd.height;
                
                fd.pixelBuffer.resize(bufferSize);

                fd.pixelBuffer.resize(bufferSize);
                
                pConverter->CopyPixels(nullptr, wicStride, bufferSize, fd.pixelBuffer.data());
            }
            pConverter->Release();
        }
		///////////////
		
        // Metadata
        IWICMetadataQueryReader* pMeta = nullptr;
        if (SUCCEEDED(pSource->GetMetadataQueryReader(&pMeta))) {
            PROPVARIANT pv;
            PropVariantInit(&pv);
            if (SUCCEEDED(pMeta->GetMetadataByName(L"/imgdesc/Left", &pv))) fd.left = pv.uiVal;
            PropVariantClear(&pv);
            if (SUCCEEDED(pMeta->GetMetadataByName(L"/imgdesc/Top", &pv))) fd.top = pv.uiVal;
            PropVariantClear(&pv);
            if (SUCCEEDED(pMeta->GetMetadataByName(L"/grctlext/Delay", &pv))) fd.delayMs = std::max<UINT>(100u, pv.uiVal * 10);
            PropVariantClear(&pv);
            if (SUCCEEDED(pMeta->GetMetadataByName(L"/grctlext/Disposal", &pv))) fd.disposal = (BYTE)pv.uiVal;
            PropVariantClear(&pv);
            pMeta->Release();
        }

        if (fd.disposal == 3) previousCanvas = workingCanvas;

        BlitFrame(fd, workingCanvas, canvasWidth, canvasHeight);

        FrameData composited;
        composited.width = canvasWidth;
        composited.height = canvasHeight;
        composited.delayMs = fd.delayMs;
        composited.pixelBuffer = workingCanvas;
        pData->frames.push_back(std::move(composited));

        if (fd.disposal == 2) {
            UINT clearWidth = std::min(fd.width, canvasWidth - fd.left);
            UINT clearHeight = std::min(fd.height, canvasHeight - fd.top);
            for (UINT y = 0; y < clearHeight; ++y) {
                BYTE* dst = &workingCanvas[((fd.top + y) * canvasWidth + fd.left) * 4];
                memset(dst, 0, clearWidth * 4);
            }
        } else if (fd.disposal == 3) {
            workingCanvas = previousCanvas;
        }

        pSource->Release();
    }

    SafeRelease(&pDecoder);
    SafeRelease(&pLocalWICFactory);
    pData->hr = S_OK;
    return S_OK;
}
