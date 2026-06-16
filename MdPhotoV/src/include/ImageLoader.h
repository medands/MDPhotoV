#ifndef IMAGELOADER_H
#define IMAGELOADER_H

#include <windows.h>
#include <vector>
#include <thread>
#include <functional>
#include "FrameData.h"
#include "DecoderWebp.h"
#include "DecoderAvif.h"
#include "DecoderWic.h"

class AppWindow;

class ImageLoader {
public:
    explicit ImageLoader(AppWindow* owner);
    ~ImageLoader();

    void LoadImageFromFile(const std::wstring& path, bool rebuildPlaylist);
    void OnLoadComplete(AsyncLoadData* pData);
    
    const std::vector<RenderFrame>& GetFrames() const { return m_frames; }
    UINT GetCurrentFrameIndex() const { return m_currentAnimFrame; }
    UINT GetFrameCount() const { return static_cast<UINT>(m_frames.size()); }
    bool IsAnimated() const { return m_isAnimated; }
    void NextFrame();
    void Clear();
    void FitToWindow();

private:
    void DecodeWorker(AsyncLoadData* pData);
    void DispatchToDecoder(AsyncLoadData* pData);
    
    //HRESULT DecodeWebP(AsyncLoadData* pData);
    //HRESULT DecodeAVIF(AsyncLoadData* pData);
    //HRESULT DecodeWIC(AsyncLoadData* pData);

    AppWindow* m_owner;
    std::vector<RenderFrame> m_frames;
    UINT m_currentAnimFrame = 0;
    bool m_isAnimated = false;
    bool m_isLoading = false;
    std::wstring m_currentFilePath;
    float m_zoomScale = 1.0f;
};

#endif