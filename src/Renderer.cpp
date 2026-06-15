#include "Renderer.h"
#include <algorithm>

Renderer::Renderer() = default;
Renderer::~Renderer() {
    DiscardDeviceResources();
}

HRESULT Renderer::Initialize(HWND hWnd) {
    m_hwnd = hWnd;
    return CreateDeviceResources(hWnd);
}

HRESULT Renderer::CreateDeviceResources(HWND hWnd) {
    if (m_pD2DContext) return S_OK;

    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
    if (FAILED(hr)) return hr;

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3, D3D_FEATURE_LEVEL_9_2, D3D_FEATURE_LEVEL_9_1
    };
    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    D3D_FEATURE_LEVEL featureLevel;

    hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, creationFlags,
        featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION,
        &m_pD3DDevice, &featureLevel, &m_pD3DImmediateContext);

    if (FAILED(hr)) {
        hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, creationFlags,
            featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION,
            &m_pD3DDevice, &featureLevel, &m_pD3DImmediateContext);
    }
    if (FAILED(hr)) return hr;

    hr = m_pD3DDevice->QueryInterface(IID_PPV_ARGS(&m_pDXGIDevice));
    if (FAILED(hr)) return hr;

    hr = m_pD2DFactory->CreateDevice(m_pDXGIDevice, &m_pD2DDevice);
    if (FAILED(hr)) return hr;

    hr = m_pD2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_pD2DContext);
    if (FAILED(hr)) return hr;

    // Swap Chain
    IDXGIAdapter* pDXGIAdapter = nullptr;
    m_pDXGIDevice->GetAdapter(&pDXGIAdapter);
    IDXGIFactory2* pDXGIFactory = nullptr;
    pDXGIAdapter->GetParent(IID_PPV_ARGS(&pDXGIFactory));
    SafeRelease(&pDXGIAdapter);

    RECT rc;
    GetClientRect(hWnd, &rc);
    UINT width = static_cast<UINT>(rc.right - rc.left);
    UINT height = static_cast<UINT>(rc.bottom - rc.top);
    if (width == 0) width = 1;
    if (height == 0) height = 1;

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    hr = pDXGIFactory->CreateSwapChainForHwnd(m_pD3DDevice, hWnd, &swapChainDesc, nullptr, nullptr, &m_pSwapChain);
    SafeRelease(&pDXGIFactory);
    if (FAILED(hr)) return hr;

    return CreateTargetBitmap();
}

HRESULT Renderer::CreateTargetBitmap() {
    if (!m_pSwapChain || !m_pD2DContext) return E_FAIL;

    IDXGISurface* pBackBuffer = nullptr;
    HRESULT hr = m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (FAILED(hr)) return hr;

    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE));

    hr = m_pD2DContext->CreateBitmapFromDxgiSurface(pBackBuffer, &props, &m_pTargetBitmap);
    SafeRelease(&pBackBuffer);

    if (SUCCEEDED(hr)) {
        m_pD2DContext->SetTarget(m_pTargetBitmap);
    }
    return hr;
}

void Renderer::OnResize(HWND hWnd, UINT width, UINT height) {
	if (hWnd == nullptr) {
        printf("ERROR: Invalid window handle in OnResize!\n");
        return;
    }
    
	
    
    // Check if swap chain exists
    if (m_pSwapChain == nullptr) {
        printf("ERROR: Swap chain not initialized in OnResize!\n");
        return;
    }
    
    if (width == 0 || height == 0) return;
    DiscardRenderTarget();
    HRESULT hr = m_pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (SUCCEEDED(hr)) {
        CreateTargetBitmap();
    } else {
        DiscardDeviceResources();
    }
}

void Renderer::OnRender(HWND hWnd, float zoomScale, const std::vector<RenderFrame>& frames, UINT currentFrame) {
    if (!m_pD2DContext || frames.empty()) {
        // Optional: Draw background
        return;
    }

    //HRESULT hr = 
	m_pD2DContext->BeginDraw();
    //if (FAILED(hr)) return;

    m_pD2DContext->SetTransform(D2D1::Matrix3x2F::Identity());
    m_pD2DContext->Clear(D2D1::ColorF(0.07f, 0.07f, 0.07f));

    const RenderFrame& frame = frames[currentFrame];
    D2D1_SIZE_F bmpSize = frame.pBitmap->GetSize();
    D2D1_SIZE_F rtSize = m_pD2DContext->GetSize();

    float xOffset = (rtSize.width - (bmpSize.width * zoomScale)) / 2.0f;
    float yOffset = (rtSize.height - (bmpSize.height * zoomScale)) / 2.0f;

    D2D1_MATRIX_3X2_F transform = D2D1::Matrix3x2F::Scale(zoomScale, zoomScale) * 
                                   D2D1::Matrix3x2F::Translation(xOffset, yOffset);
    m_pD2DContext->SetTransform(transform);

    m_pD2DContext->DrawImage(frame.pBitmap, nullptr, nullptr, D2D1_INTERPOLATION_MODE_LINEAR, D2D1_COMPOSITE_MODE_SOURCE_OVER);

    HRESULT hr = m_pD2DContext->EndDraw();
    if (hr == static_cast<HRESULT>(D2DERR_RECREATE_TARGET)) {
        DiscardDeviceResources();
    } else if (SUCCEEDED(hr) && m_pSwapChain) {
        m_pSwapChain->Present(1, 0);
    }
}

void Renderer::DiscardRenderTarget() {
    if (m_pD2DContext) m_pD2DContext->SetTarget(nullptr);
    SafeRelease(&m_pTargetBitmap);
}

void Renderer::DiscardDeviceResources() {
    DiscardRenderTarget();
    SafeRelease(&m_pSwapChain);
    SafeRelease(&m_pD2DContext);
    SafeRelease(&m_pD2DDevice);
    SafeRelease(&m_pDXGIDevice);
    SafeRelease(&m_pD3DImmediateContext);
    SafeRelease(&m_pD3DDevice);
    SafeRelease(&m_pD2DFactory);
}