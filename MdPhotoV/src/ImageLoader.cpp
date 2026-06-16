
#include <algorithm>
#include <thread>
#include "AppWindow.h"
#include "ImageLoader.h"


ImageLoader::ImageLoader(AppWindow* owner) : m_owner(owner) {}
ImageLoader::~ImageLoader() = default;

void ImageLoader::LoadImageFromFile(const std::wstring& path, bool rebuildPlaylist) {
    if (m_isLoading) return;
    m_isLoading = true;
    m_currentFilePath = path;

    AsyncLoadData* pData = new AsyncLoadData();
    pData->filePath = path;
    pData->rebuildPlaylist = rebuildPlaylist;
    pData->hr = E_FAIL;

    std::thread worker(&ImageLoader::DecodeWorker, this, pData);
    worker.detach();
}

void ImageLoader::DecodeWorker(AsyncLoadData* pData) {
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    DispatchToDecoder(pData);
    
    // Post message to main thread
    PostMessageW(m_owner->GetHwnd(), WM_USER_IMAGE_LOADED, 0, reinterpret_cast<LPARAM>(pData));
    
    CoUninitialize();
}

void ImageLoader::DispatchToDecoder(AsyncLoadData* pData) {
    std::filesystem::path p(pData->filePath);
    std::wstring ext = p.extension().wstring();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);

    if (ext == L".webp") DecodeWebP(pData);
    else if (ext == L".avif") DecodeAVIF(pData);
    else DecodeWIC(pData);
}

void ImageLoader::OnLoadComplete(AsyncLoadData* pData) {
    m_isLoading = false;
    if (!pData || !SUCCEEDED(pData->hr) || pData->frames.empty()) {
        std::wstring errorMsg = L"Failed to load: " + 
            pData->filePath.substr(pData->filePath.find_last_of(L"\\/") + 1);
        MessageBoxW(m_owner->GetHwnd(), errorMsg.c_str(), L"Error", MB_OK | MB_ICONERROR);
        delete pData;
        return;
    }

	if (pData->rebuildPlaylist && m_owner) {
        // Extract directory path from the full file path
        std::filesystem::path filePath(pData->filePath);
        std::wstring directoryPath = filePath.parent_path().wstring();

		//MessageBoxW(m_owner->GetHwnd(), pData->filePath.c_str(), L"PList updated", MB_OK );		
        // Call the method on AppWindow to rebuild the playlist
        // We assume AppWindow has a public or friend method like RebuildPlaylistFromDir
        m_owner->RebuildPlaylist(pData->filePath);
    }
    
    // Kill old timer
    KillTimer(m_owner->GetHwnd(), IDT_ANIMATION_TIMER);
    for (auto& f : m_frames) SafeRelease(&f.pBitmap);
    m_frames.clear();
    m_currentAnimFrame = 0;

    // Get Renderer safely
    Renderer* renderer = m_owner->GetRenderer();
    
    // Initialize renderer if not already done or if it failed previously
    // Note: Depending on your architecture, you might want to check if it's already initialized
    if (!renderer) {
        MessageBoxW(m_owner->GetHwnd(), L"Renderer not available.", L"Error", MB_OK | MB_ICONERROR);
        delete pData;
        return;
    }

    // If the renderer needs initialization, do it here. 
    // If it's initialized globally, this check handles the failure case.
    if (FAILED(renderer->Initialize(m_owner->GetHwnd()))) {
        MessageBoxW(m_owner->GetHwnd(), L"Failed to initialize graphics.", L"Error", MB_OK | MB_ICONERROR);
        delete pData;
        return;
    }

    // Get the Direct2D context
    ID2D1DeviceContext* pContext = renderer->GetContext();
    if (!pContext) {
        MessageBoxW(m_owner->GetHwnd(), L"Graphics context not available.", L"Error", MB_OK | MB_ICONERROR);
        delete pData;
        return;
    }

    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_NONE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

    D2D1_SIZE_U size = D2D1::SizeU(pData->width, pData->height);
    UINT stride = pData->width * 4;

    m_frames.reserve(pData->frames.size());
    
    // Create bitmaps
    for (const auto& fd : pData->frames) {
        RenderFrame rf;
        rf.delayMs = fd.delayMs;
        
        // Create bitmap safely
        if (SUCCEEDED(pContext->CreateBitmap(size, fd.pixelBuffer.data(), stride, &props, &rf.pBitmap))) {
            m_frames.push_back(rf);
        } else {
            // Handle creation failure for this frame if necessary
            // For now, we just skip it
        }
    }

    if (!m_frames.empty()) {
        // Calculate Zoom Scale
        // Access the first frame's bitmap to get dimensions
        D2D1_SIZE_F bmpSize = m_frames[0].pBitmap->GetSize();
        D2D1_SIZE_F rtSize = pContext->GetSize();
        
        float scaleX = rtSize.width / bmpSize.width;
        float scaleY = rtSize.height / bmpSize.height;
        m_zoomScale = std::min(scaleX, scaleY);
        m_owner->SetZoomScale(m_zoomScale);

        // Check if animated
        m_isAnimated = (m_frames.size() > 1);
        if (m_isAnimated) {
            UINT firstDelay = m_frames[0].delayMs;
            if (firstDelay == 0) firstDelay = 1;
            SetTimer(m_owner->GetHwnd(), IDT_ANIMATION_TIMER, firstDelay, NULL);
        }

        // Update Window Title
        std::wstring fileName = pData->filePath.substr(pData->filePath.find_last_of(L"\\/") + 1);
        SetWindowTextW(m_owner->GetHwnd(), (L"MdPhotoV - " + fileName).c_str());
    }

    delete pData;
    InvalidateRect(m_owner->GetHwnd(), NULL, FALSE);
}

void ImageLoader::NextFrame() {
    if (m_frames.empty()) return;
    m_currentAnimFrame = (m_currentAnimFrame + 1) % m_frames.size();
    
    UINT nextDelay = m_frames[m_currentAnimFrame].delayMs;
    if (nextDelay == 0) nextDelay = 1;
    
    SetTimer(m_owner->GetHwnd(), IDT_ANIMATION_TIMER, nextDelay, NULL);
}

void ImageLoader::Clear() {
    KillTimer(m_owner->GetHwnd(), IDT_ANIMATION_TIMER);
    m_isAnimated = false;
    for (auto& f : m_frames) SafeRelease(&f.pBitmap);
    m_frames.clear();
    m_currentAnimFrame = 0;
}

void ImageLoader::FitToWindow() {
    if (m_frames.empty()) return;

    Renderer* renderer = m_owner->GetRenderer();
    if (!renderer) return;

    ID2D1DeviceContext* pContext = renderer->GetContext();
    if (!pContext) return;

    // Access the first frame's bitmap
    D2D1_SIZE_F bmpSize = m_frames[0].pBitmap->GetSize();
    D2D1_SIZE_F rtSize = pContext->GetSize();
    
    float scaleX = rtSize.width / bmpSize.width;
    float scaleY = rtSize.height / bmpSize.height;
    
    m_zoomScale = std::min(scaleX, scaleY);
    m_owner->SetZoomScale(m_zoomScale);
}