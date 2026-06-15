#include "AppWindow.h"
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <algorithm>
#include <filesystem>
#include "menu.h"
#include "Dialogs.h"

namespace fs = std::filesystem;

static bool g_bAboutDialogOpen = false;

AppWindow::AppWindow() {
    m_renderer = std::make_unique<Renderer>();
    m_loader = std::make_unique<ImageLoader>(this);
    m_playlist = std::make_unique<Playlist>();
}

AppWindow::~AppWindow() {
    // Cleanup happens automatically via unique_ptr
}

void AppWindow::RebuildPlaylist(const std::wstring& Path) {
    if (!m_playlist) {
        m_playlist = std::make_unique<Playlist>();
    }
    
    // Rebuild the playlist from the extracted directory
    m_playlist->BuildPlaylist(Path);
	size_t playlistSize = m_playlist->GetFiles().size();

	// 2. Convert the size to a wide string for MessageBoxW
	std::wstring msg = L"Playlist loaded successfully!\n\nTotal images: " + std::to_wstring(playlistSize);
	
    // If you need to refresh the UI to show playlist status:
    //InvalidateRect(m_hwnd, NULL, FALSE); 
}

float AppWindow::ClampZoom(float zoom) {
    constexpr float kMinZoom = 0.02f;
    constexpr float kMaxZoom = 32.0f;
    if (!(zoom > 0.0f)) return 1.0f;
    return std::clamp(zoom, kMinZoom, kMaxZoom);
}

HRESULT AppWindow::Initialize(HINSTANCE hInstance) {
    m_hInstance = hInstance;

    // Register Window Class
    WNDCLASSEXW wcex = { sizeof(WNDCLASSEXW) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = GlobalWndProc;
    wcex.cbWndExtra = sizeof(LONG_PTR);
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.lpszClassName = L"MdPhotoV_Class";
    wcex.lpszMenuName = L"MdPhotoVMenu";
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(100));
    wcex.hIconSm = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(100), IMAGE_ICON, 16, 16, 0);

    if (!RegisterClassExW(&wcex)) return E_FAIL;

    m_hwnd = CreateWindowExW(
        0, L"MdPhotoV_Class", L"MdPhotoV",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 1024, 768,
        NULL, NULL, hInstance, this);

    if (!m_hwnd) return E_FAIL;

    SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    
    DragAcceptFiles(m_hwnd, TRUE);
    
    // Allow Drag & Drop messages in UI
    ChangeWindowMessageFilterEx(m_hwnd, WM_DROPFILES, MSGFLT_ALLOW, NULL);
    ChangeWindowMessageFilterEx(m_hwnd, WM_COPYDATA, MSGFLT_ALLOW, NULL);
    UINT msg = RegisterWindowMessageW(L"ShellDrop");
    ChangeWindowMessageFilterEx(m_hwnd, msg, MSGFLT_ALLOW, NULL);

    BuildAcceleratorTable();
    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);

    return S_OK;
}

void AppWindow::BuildAcceleratorTable() {
	
    ACCEL accelTable[] = {
    	{ FVIRTKEY , VK_RIGHT, IDM_VIEW_NEXT },
    	{ FVIRTKEY , VK_LEFT, IDM_VIEW_PREV },
        { FVIRTKEY | FCONTROL, 'O', IDM_FILE_OPEN },
        { FVIRTKEY | FCONTROL, 'C', IDM_EDIT_COPY },
        { FVIRTKEY | FCONTROL, VK_DELETE, IDM_EDIT_DELETE },
        { FVIRTKEY | FCONTROL, 'P', IDM_FILE_PRINT },
        { FVIRTKEY | FCONTROL, 'T', IDM_FILE_OPTIONS },
        { FVIRTKEY | FCONTROL, 'Q', IDM_VIEW_ORIGINAL },
        { FVIRTKEY | FCONTROL, VK_NUMPAD1, IDM_VIEW_ORIGINAL },
        { FVIRTKEY | FCONTROL, 'W', IDM_VIEW_FIT },
        { FVIRTKEY | FCONTROL, VK_NUMPAD0, IDM_VIEW_FIT },
        { FVIRTKEY | FCONTROL, VK_ADD, IDM_VIEW_ZOOMIN },
        { FVIRTKEY | FCONTROL, VK_OEM_PLUS, IDM_VIEW_ZOOMIN },
        { FVIRTKEY | FCONTROL, VK_SUBTRACT, IDM_VIEW_ZOOMOUT },
        { FVIRTKEY | FCONTROL, VK_OEM_MINUS, IDM_VIEW_ZOOMOUT },
        { FVIRTKEY, 'M', IDM_HELP_MANUAL },
        { FVIRTKEY, VK_F1, IDM_HELP_MANUAL },
        { FVIRTKEY, VK_F11, IDM_VIEW_FULLSCREEN },
        { FVIRTKEY, VK_F10, IDM_HELP_ABOUT }
    };
    m_hAccel = CreateAcceleratorTableW(accelTable, sizeof(accelTable) / sizeof(ACCEL));
}

LRESULT CALLBACK AppWindow::GlobalWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_NCCREATE) {
        LPCREATESTRUCTW pCreate = reinterpret_cast<LPCREATESTRUCTW>(lParam);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreate->lpCreateParams));
    }
    AppWindow* pThis = reinterpret_cast<AppWindow*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    if (!pThis) return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    return pThis->WndProc(hWnd, uMsg, wParam, lParam);
}

LRESULT AppWindow::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_TIMER:
            if (wParam == IDT_ANIMATION_TIMER && m_loader->IsAnimated()) {
                m_loader->NextFrame();
                InvalidateRect(hWnd, NULL, FALSE);
            }
            return 0;

        case WM_USER_IMAGE_LOADED: {
            AsyncLoadData* pData = reinterpret_cast<AsyncLoadData*>(lParam);
            m_loader->OnLoadComplete(pData);
            return 0;
        }

        case WM_SIZE: {
            if (m_renderer) m_renderer->OnResize(hWnd, LOWORD(lParam), HIWORD(lParam));
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hWnd, &ps);
            if (m_renderer) {
                m_renderer->OnRender(hWnd, m_zoomScale, m_loader->GetFrames(), m_loader->GetCurrentFrameIndex());
            }
            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_DROPFILES: {
            HDROP hDrop = reinterpret_cast<HDROP>(wParam);
            wchar_t szDroppedFile[MAX_PATH];
            if (DragQueryFileW(hDrop, 0, szDroppedFile, MAX_PATH) != 0) {
                LoadImageFromFile(szDroppedFile, true);
            }
            DragFinish(hDrop);
            return 0;
        }

        case WM_KEYDOWN:
            if (wParam == VK_LEFT) Navigate(-1);
            else if (wParam == VK_RIGHT) Navigate(1);
            else if (wParam == VK_ESCAPE && m_isFullscreen) ToggleFullscreen();
            return 0;
        
		case WM_COMMAND:
            switch (LOWORD(wParam)) {
            	case IDM_VIEW_NEXT: Navigate(1); break;
            	case IDM_VIEW_PREV: Navigate(-1); break;
                case IDM_FILE_OPEN: CommandFileOpen(); break;
                case IDM_FILE_OPENDIR: CommandFileOpenDir(); break;
                case IDM_FILE_PRINT: CommandFilePrint(); break;
                case IDM_FILE_CLOSE: CommandFileClose(); break;
                case IDM_FILE_OPTIONS: CommandOptions(); break;
                case IDM_VIEW_ZOOMIN: SetZoomScale(m_zoomScale * 1.25f); InvalidateRect(hWnd, NULL, FALSE); break;
                case IDM_VIEW_ZOOMOUT: SetZoomScale(m_zoomScale / 1.25f); InvalidateRect(hWnd, NULL, FALSE); break;
                case IDM_VIEW_ORIGINAL: SetZoomScale(1.0f); InvalidateRect(hWnd, NULL, FALSE); break;
                case IDM_VIEW_FIT: CommandViewFit(); break;
                case IDM_VIEW_FULLSCREEN: ToggleFullscreen(); break;
                case IDM_EDIT_COPY: CommandEditCopy(); break;
                case IDM_EDIT_RENAME: CommandEditRename(); break;
                case IDM_EDIT_DELETE: CommandEditDelete(); break;
                case IDM_EDIT_OPENWITH: CommandEditOpenWith(); break;
                case IDM_HELP_ABOUT: 
					{
						if (g_bAboutDialogOpen) return 0; 
					    g_bAboutDialogOpen = true;
					    
					    CommandAboutDialog(); 
						g_bAboutDialogOpen = false;
    					return 0;
					}
					break;
                case IDM_HELP_MANUAL: CommandHelpManual(); break;
                case IDM_FILE_EXIT: DestroyWindow(hWnd); break;
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

void AppWindow::LoadImageFromFile(const std::wstring& path, bool rebuildPlaylist) {
    m_loader->LoadImageFromFile(path, rebuildPlaylist);
}

// --- Command Implementations ---

void AppWindow::CommandFileOpen() {
    wchar_t szFile[MAX_PATH] = { 0 };
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"Supported Images\0*.jpg;*.jpeg;*.png;*.bmp;*.avif;*.webp;*.gif;*.tiff\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        LoadImageFromFile(szFile, true);
        //m_playlist->BuildPlaylist(szFile);
    }
}

void AppWindow::CommandFileOpenDir() {
    IFileOpenDialog* pFolderDlg = NULL;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFolderDlg)))) {
        DWORD dwOptions;
        pFolderDlg->GetOptions(&dwOptions);
        pFolderDlg->SetOptions(dwOptions | FOS_PICKFOLDERS);
        if (SUCCEEDED(pFolderDlg->Show(m_hwnd))) {
            IShellItem* pItem = NULL;
            if (SUCCEEDED(pFolderDlg->GetResult(&pItem))) {
                wchar_t* pszPath = NULL;
                if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))) {
                    m_playlist->BuildPlaylist(pszPath);
                    if (!m_playlist->GetFiles().empty()) {
                        LoadImageFromFile(m_playlist->GetFiles().front(), false);
                    }
                }
                CoTaskMemFree(pszPath);
                pItem->Release();
            }
        }
        pFolderDlg->Release();
    }
}

void AppWindow::CommandFilePrint() {
    if (m_playlist->GetFiles().empty()) return;
    ShellExecuteW(m_hwnd, L"print", m_playlist->GetFiles()[m_playlist->GetCurrentIndex()].c_str(), NULL, NULL, SW_SHOWNORMAL);
}

void AppWindow::CommandFileClose() {
    m_loader->Clear();
    m_playlist->Clear();
    m_zoomScale = 1.0f;
    SetWindowTextW(m_hwnd, L"MdPhotoV");
    InvalidateRect(m_hwnd, NULL, TRUE);
}

void AppWindow::CommandViewFit() {
    m_loader->FitToWindow();
    InvalidateRect(m_hwnd, NULL, FALSE);
}

void AppWindow::Navigate(int direction) {
    if (m_playlist->GetFiles().empty()) return;
    size_t newIndex = (direction > 0) 
        ? (m_playlist->GetCurrentIndex() + 1) % m_playlist->GetFiles().size()
        : (m_playlist->GetCurrentIndex() == 0) ? m_playlist->GetFiles().size() - 1 : m_playlist->GetCurrentIndex() - 1;
    
    m_playlist->SetCurrentIndex(newIndex);
    LoadImageFromFile(m_playlist->GetFiles()[newIndex], false);
}

void AppWindow::ToggleFullscreen() {
    DWORD dwStyle = GetWindowLong(m_hwnd, GWL_STYLE);
    if (!m_isFullscreen) {
        MONITORINFO mi = { sizeof(mi) };
        if (GetWindowPlacement(m_hwnd, &m_wpPrev) && GetMonitorInfo(MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTOPRIMARY), &mi)) {
            SetMenu(m_hwnd, NULL);
            SetWindowLong(m_hwnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(m_hwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
                mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top,
                SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            m_isFullscreen = true;
        }
    } 
	else {
        SetMenu(m_hwnd, LoadMenuW(m_hInstance, L"MdPhotoVMenu"));
        SetWindowLong(m_hwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(m_hwnd, &m_wpPrev);
        SetWindowPos(m_hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        m_isFullscreen = false;
        }
    
}

void AppWindow::CommandAboutDialog() {
    ShowAboutDialog(m_hwnd);
}

void AppWindow::CommandHelpManual() {
    ShowHelpWindow(m_hwnd);
}

void AppWindow::CommandOptions() {
    ShowOptionsDialog(m_hwnd);
}

void AppWindow::CommandEditCopy() {
    if (m_playlist->GetFiles().empty()) return;
    std::wstring path = m_playlist->GetFiles()[m_playlist->GetCurrentIndex()];
    if (OpenClipboard(m_hwnd)) {
        EmptyClipboard();
        size_t size = sizeof(DROPFILES) + ((path.length() + 2) * sizeof(wchar_t));
        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, size);
        if (hGlobal) {
            DROPFILES* df = (DROPFILES*)GlobalLock(hGlobal);
            df->pFiles = sizeof(DROPFILES);
            df->fWide = TRUE;
            wchar_t* pData = (wchar_t*)((BYTE*)df + sizeof(DROPFILES));
            std::copy(path.begin(), path.end(), pData);
            GlobalUnlock(hGlobal);
            if (!SetClipboardData(CF_HDROP, hGlobal)) {
                GlobalFree(hGlobal);
            }
        }
        CloseClipboard();
    }
}

void AppWindow::CommandEditRename() {
    MessageBoxW(m_hwnd, L"Rename functionality requires a custom dialog. Hook this to a file rename utility.", L"Rename Route", MB_OK | MB_ICONINFORMATION);
}

void AppWindow::CommandEditDelete() {
    if (m_playlist->GetFiles().empty()) return;
    std::wstring target = m_playlist->GetFiles()[m_playlist->GetCurrentIndex()];
    if (MessageBoxW(m_hwnd, L"Are you sure you want to permanently delete this file?", L"Confirm Delete", MB_YESNO | MB_ICONWARNING) == IDYES) {
        try {
            fs::remove(target);
            m_playlist->RemoveCurrent();
            if (m_playlist->GetFiles().empty()) {
                CommandFileClose();
            } else {
                size_t newIndex = (m_playlist->GetCurrentIndex() >= m_playlist->GetFiles().size()) ? 0 : m_playlist->GetCurrentIndex();
                m_playlist->SetCurrentIndex(newIndex);
                LoadImageFromFile(m_playlist->GetFiles()[newIndex], false);
            }
        } catch (...) {
            MessageBoxW(m_hwnd, L"Access Denied. Could not delete asset.", L"Error", MB_OK | MB_ICONERROR);
        }
    }
}

void AppWindow::CommandEditOpenWith() {
    if (m_playlist->GetFiles().empty()) return;
    std::wstring args = L"shell32.dll,OpenAs_RunDLL " + m_playlist->GetFiles()[m_playlist->GetCurrentIndex()];
    ShellExecuteW(m_hwnd, L"open", L"rundll32.exe", args.c_str(), NULL, SW_SHOWNORMAL);
}

void AppWindow::RunMessageLoop() {
    HACCEL hAccel = m_hAccel;
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (TranslateAcceleratorW(m_hwnd, hAccel, &msg)) {
            continue;
        }
        
        // Handle Escape key in dialogs
			    
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
            HWND hwndActive = GetActiveWindow();
            wchar_t className[64] = { 0 };
            GetClassNameW(hwndActive, className, 64);
            if (wcscmp(className, L"MdPhotoV_About_Class") == 0 || 
                wcscmp(className, L"MdPhotoV_Help_Class") == 0) {
                DestroyWindow(hwndActive);
                continue;
            }
        }
        
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
