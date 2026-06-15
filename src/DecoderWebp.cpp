#include "DecoderWebp.h"
#include <webp/decode.h>
#include <cstdio>
#include <fstream>
#include <vector>

HRESULT DecodeWebP(AsyncLoadData* pData) {
    FILE* file = _wfopen(pData->filePath.c_str(), L"rb");
    if (!file) return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    std::vector<uint8_t> buffer(size);
    fread(buffer.data(), 1, size, file);
    fclose(file);

    int width = 0, height = 0;
    uint8_t* rawPixels = WebPDecodeBGRA(buffer.data(), buffer.size(), &width, &height);

    if (rawPixels) {
        pData->width = width;
        pData->height = height;
        FrameData fd;
        fd.width = width;
        fd.height = height;
        fd.delayMs = 0;
        size_t byteCount = static_cast<size_t>(width) * height * 4;
        fd.pixelBuffer.assign(rawPixels, rawPixels + byteCount);
        pData->frames.push_back(std::move(fd));
        pData->hr = S_OK;
        WebPFree(rawPixels);
    } else {
        pData->hr = E_FAIL;
    }
    return pData->hr;
}