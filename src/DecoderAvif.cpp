#include "DecoderAvif.h"
#include <avif/avif.h>
#include <cstdio>
#include <fstream>
#include <vector>

HRESULT DecodeAVIF(AsyncLoadData* pData) {
    FILE* file = _wfopen(pData->filePath.c_str(), L"rb");
    if (!file) return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size <= 0) { fclose(file); return E_FAIL; }

    std::vector<uint8_t> buffer(size);
    fread(buffer.data(), 1, size, file);
    fclose(file);

    avifDecoder* decoder = avifDecoderCreate();
    if (!decoder) return E_OUTOFMEMORY;

    avifResult result = avifDecoderSetIOMemory(decoder, buffer.data(), buffer.size());
    if (result != AVIF_RESULT_OK) { avifDecoderDestroy(decoder); return E_FAIL; }

    result = avifDecoderParse(decoder);
    if (result != AVIF_RESULT_OK) { avifDecoderDestroy(decoder); return E_FAIL; }

    result = avifDecoderNextImage(decoder);
    if (result != AVIF_RESULT_OK) { avifDecoderDestroy(decoder); return E_FAIL; }

    avifRGBImage rgb;
    avifRGBImageSetDefaults(&rgb, decoder->image);
    rgb.format = AVIF_RGB_FORMAT_BGRA;
    rgb.depth = 8;
    result = avifRGBImageAllocatePixels(&rgb);

    result = avifImageYUVToRGB(decoder->image, &rgb);
    if (result == AVIF_RESULT_OK) {
        pData->width = decoder->image->width;
        pData->height = decoder->image->height;
        FrameData fd;
        fd.width = pData->width;
        fd.height = pData->height;
        fd.delayMs = 0;
        size_t byteCount = rgb.rowBytes * rgb.height;
        fd.pixelBuffer.assign(rgb.pixels, rgb.pixels + byteCount);
        pData->frames.push_back(std::move(fd));
        pData->hr = S_OK;
    } else {
        pData->hr = E_FAIL;
    }

    avifRGBImageFreePixels(&rgb);
    avifDecoderDestroy(decoder);
    return pData->hr;
}