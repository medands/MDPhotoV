#ifndef RENDERER_H
#define RENDERER_H

#include <windows.h>
#include <d2d1_1.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include "FrameData.h"
#include <vector>

class Renderer {
public:
    Renderer();
    ~Renderer();

	ID2D1DeviceContext* GetContext() const {
        return m_pD2DContext;
    }
    
    HRESULT Initialize(HWND hWnd);
    void OnResize(HWND hWnd, UINT width, UINT height);
    void OnRender(HWND hWnd, float zoomScale, const std::vector<RenderFrame>& frames, UINT currentFrame);
    void DiscardDeviceResources();

private:
    HRESULT CreateDeviceResources(HWND hWnd);
    HRESULT CreateTargetBitmap();
    void DiscardRenderTarget();

    HWND m_hwnd = NULL;
    ID2D1Factory1* m_pD2DFactory = nullptr;
    ID2D1Device* m_pD2DDevice = nullptr;
    ID2D1DeviceContext* m_pD2DContext = nullptr;
    ID3D11Device* m_pD3DDevice = nullptr;
    ID3D11DeviceContext* m_pD3DImmediateContext = nullptr;
    IDXGIDevice* m_pDXGIDevice = nullptr;
    IDXGISwapChain1* m_pSwapChain = nullptr;
    ID2D1Bitmap1* m_pTargetBitmap = nullptr;
};

#endif