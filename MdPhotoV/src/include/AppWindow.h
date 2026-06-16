#ifndef APPWINDOW_H
#define APPWINDOW_H

#ifndef NTDDI_VERSION
#define NTDDI_VERSION 0x0A000000
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

#include <windows.h>
#include <memory>
#include "Renderer.h"
#include "ImageLoader.h"
#include "Playlist.h"
#include "FrameData.h"

#define WM_USER_IMAGE_LOADED (WM_USER + 1)
#define IDT_ANIMATION_TIMER 1401

class AppWindow {
public:
    AppWindow();
    ~AppWindow();

    HRESULT Initialize(HINSTANCE hInstance);
    void RunMessageLoop();
	
	void RebuildPlaylist(const std::wstring& Path);
	
    // Command Handlers
    void CommandFileOpen();
    void CommandFileOpenDir();
    void CommandFilePrint();
    void CommandFileClose();
    void CommandViewFit();
    void CommandEditCopy();
    void CommandEditRename();
    void CommandEditDelete();
    void CommandEditOpenWith();
    void Navigate(int direction);
    void ToggleFullscreen();
    void CommandAboutDialog();
    void CommandHelpManual();
    void CommandOptions();

    // Accessors
    HWND GetHwnd() const { return m_hwnd; }
    HINSTANCE GetInstance() const { return m_hInstance; }
    float GetZoomScale() const { return m_zoomScale; }
    void SetZoomScale(float scale) { m_zoomScale = ClampZoom(scale); }
    bool IsFullscreen() const { return m_isFullscreen; }
    HACCEL GetAccelTable() const { return m_hAccel; }
    void LoadImageFromFile(const std::wstring& path, bool rebuildPlaylist = true);
	
	Renderer* GetRenderer() {
        if (m_renderer) {
            return m_renderer.get(); // Returns the raw pointer
        }
        return nullptr;
    }

    // Optional: If you need to ensure it's initialized before use
    bool IsRendererReady() const {
        return m_renderer != nullptr;
    }
    
private:
    static LRESULT CALLBACK GlobalWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void BuildAcceleratorTable();
    float ClampZoom(float zoom);

    // Modules
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<ImageLoader> m_loader;
    std::unique_ptr<Playlist> m_playlist;

    HWND m_hwnd = NULL;
    HINSTANCE m_hInstance = NULL;
    HACCEL m_hAccel = NULL;
    WINDOWPLACEMENT m_wpPrev = {};

    float m_zoomScale = 1.0f;
    bool m_isFullscreen = false;
    bool m_isLoading = false;
};

#endif